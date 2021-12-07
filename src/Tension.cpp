#include "plugin.hpp"

#include "penners.hpp"

#define TENSION_DISPLAY_SIZE 32

//TODO: For Tension 2, Custom Bezier Curves, Input for Trigger!, Bigger Button, Scope & Progress...

struct Tension : Module
{
	enum ParamIds
	{
		TRIGGER_PARAM,
		RESET_PARAM,
		SHAPESLIDER_PARAM,
		RATIOSLIDER_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
		CLOCKINPUT_INPUT,
		RESETINPUT_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		GATEOUTPUT_OUTPUT,
		OUTPUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds
	{
		NUM_LIGHTS
	};

	//* SAVE with RESET *//

	int clockMode = ClockMode::CLOCK;
	int easeType = Ease::LINEAR;
	int easeMode = Ease::BOTH;
	int easeDirection = Ease::FORWARD;
	int voltagePairs = VoltagePairs::NEGATIVE_10_TO_10;

	bool shouldResetHard = false;

	//* DONT SAVE with RESET *//

	RefreshCounter refreshCounter;
	dsp::SchmittTrigger clockTrigger, resetTrigger;

	double timeElapsed = 0.0; // Time that has elapsed
	double duration = 0;	  // The total duration of the osc/animationFunc takes to complete
	double phase = 0.0;		  // The time elapsed relative to the period of one function...
	double amplitude = 0.0;
	double offset = 0.0;
	double freq = 0.0;

	int division = 0;

	bool isClockConnected = false;
	bool firstClockReceived = false;
	bool secondClockReceived = false;
	bool b_buttonState = false;

	float bufferedShapeKnob = 0.f;
	float bufferedRatioKnob = 0.f;
	float bufferedTriggerButton = 0.f;
	float bufferedResetButton = 0.f;

	double evaluate(double x)
	{
		//return Ease::EnumToFunction(Ease::Type(easeType))(Ease::Mode(easeMode), x, duration, amplitude, offset);
		return Ease::EnumToFunction(Ease::Type(easeType))(Ease::Mode(easeMode), x, duration, 2.0, 0.0);
	}

	Tension()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(TRIGGER_PARAM, 0.f, 1.f, 0.f, "");
		configParam(RESET_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SHAPESLIDER_PARAM, 0.f, 1.f, 0.f, "");

		// Ratio Knob [IMPROMPTU MODULAR]
		// configParam<RatioParam>(RATIOSLIDER_PARAM, ((float)NUM_RATIOS - 1.0f) * -1.0f, float(NUM_RATIOS) - 1.0f, 0.0f, "Clock Ratio");

		// Ratio Knob [FROZEN WASTELAND]
		configParam(RATIOSLIDER_PARAM, 0.0, 26.0, 13.0);

		//	TODO V2
		//	paramQuantities[RATIOSLIDER_PARAM]->snapEnabled = true; Enable Snapping Later in VCV Rack v2
		//	configInput(CLOCKINPUT_INPUT, "Clock");
		//	configInput(RESETINPUT_INPUT, "Reset");
		//
		//	configOutput(GATEOUTPUT_OUTPUT, "GATE");
		//	configOutput(OUTPUT_OUTPUT, "CV OUT");

		onReset();

		// leftExpander.producerMessage = producerMessage;
		// leftExpander.consumerMessage = consumerMessage;
	}

	void step(double dt)
	{
		double delta = fmin(freq * dt, 0.5);
		phase += b_buttonState ? -delta : delta;
		phase = clamp(phase,0.0f, 1.0f);
	}

	void onReset() override {}

	void reset(bool hard)
	{
		// sampleRate = (double)(APP->engine->getSampleRate());
		// sampleTime = 1.0 / sampleRate;

		// bufferedRatioKnob = params[RATIOSLIDER_PARAM].getValue();

		if (hard)
		{
			phase = 0.0;
			params[TRIGGER_PARAM].setValue(0.0);
			b_buttonState = false;
		}
	}

	json_t *dataToJson() override
	{
		json_t *json = json_object();

		json_object_set_new(json, "clockMode", json_integer(clockMode));
		json_object_set_new(json, "easeMode", json_integer(easeMode));
		json_object_set_new(json, "easeType", json_integer(easeType));

		return json;
	}

	void dataFromJson(json_t *json) override
	{
		getFromJsonOrDefault(clockMode, json, "clockMode", json_integer_value, (int)ClockMode::CLOCK);
		getFromJsonOrDefault(easeMode, json, "easeMode", json_integer_value, (int)Ease::LINEAR);
		getFromJsonOrDefault(easeType, json, "easeType", json_integer_value, (int)Ease::BOTH);
	}

	void process(const ProcessArgs &args) override
	{
		timeElapsed += 1.0 / args.sampleRate;

		// Clock Pin
		if (inputs[CLOCKINPUT_INPUT].isConnected())
		{
			if (ClockMode::BPM)
			{
				double bpm = powf(1.0, clamp(inputs[CLOCKINPUT_INPUT].getVoltage(), -10.0f, 10.0f)) * 120.0;
				duration = 60.0 / bpm;
			}
			else
			{
				// CLOCKMODE::CLOCK
				if (clockTrigger.process(inputs[CLOCKINPUT_INPUT].getVoltage()))
				{
					if (firstClockReceived)
					{
						duration = timeElapsed;
						secondClockReceived = true;
					}
					timeElapsed = 0;
					firstClockReceived = true;
				}
				else if (secondClockReceived && timeElapsed > duration)
				{
					duration = timeElapsed;
				}
			}
			isClockConnected = true;
		}
		else
		{
			// TODO	Calculate BPM
			duration = 0.0f;
			firstClockReceived = false;
			secondClockReceived = false;
			isClockConnected = false;
		}

		// Reset Pin
		if (inputs[RESETINPUT_INPUT].isConnected())
		{
			if (resetTrigger.process(inputs[RESETINPUT_INPUT].getVoltage()))
			{
				reset(true);
			}
		}

		// GUI Refresh
		if (refreshCounter.processInputs())
		{
			const auto shape_value = params[SHAPESLIDER_PARAM].getValue();
			if (bufferedShapeKnob != shape_value)
			{
				bufferedShapeKnob = shape_value;
			}

			const auto ratio_value = params[RATIOSLIDER_PARAM].getValue();
			if (bufferedRatioKnob != ratio_value)
			{
				bufferedRatioKnob = ratio_value;
				division = int(clamp(bufferedRatioKnob, 0.0f, 26.0f));

				if (isClockConnected && duration != 0)
				{
					freq = 1.0 / (duration / DIVISIONS[division]);
				}
				else
				{
					// Set BPM Manually if Clock is not connected...
					duration = clamp(60.0f / bufferedRatioKnob, DURATION_MIN_F, DURATION_MAX_F);
					freq = 1.0 / duration;
				}
			}

			// On Button Press, set isRunningToTrue...
			const auto trigger_value = params[TRIGGER_PARAM].getValue();
			if (bufferedTriggerButton != trigger_value)
			{
				bufferedTriggerButton = trigger_value;
				b_buttonState = trigger_value == 0.0;
				reset(shouldResetHard);
				outputs[GATEOUTPUT_OUTPUT].setVoltage(trigger_value);
			}

			const auto reset_value = params[RESET_PARAM].getValue();
			if (bufferedResetButton != reset_value)
			{
				bufferedResetButton = reset_value;
				reset(reset_value == 1.0);
			}
		}

		step(1.0 / args.sampleRate);

		// Light Processing... // Call this to increment Refresh Count 
		if(refreshCounter.processLights()) { }

		outputs[OUTPUT_OUTPUT].setVoltage(evaluate(phase));
	}
};

struct TensionWidget : ModuleWidget
{
	TensionWidget(Tension *module)
	{
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Tension.svg")));

		addParam(createParamCentered<TToggleButton>(mm2px(Vec(7.67, 12.0)), module, Tension::TRIGGER_PARAM));
		addParam(createParamCentered<TMicroButton>(mm2px(Vec(11.094, 20.5)), module, Tension::RESET_PARAM));
		addParam(createParamCentered<TTinyKnob>(mm2px(Vec(7.67, 37.242)), module, Tension::SHAPESLIDER_PARAM));
		addParam(createParamCentered<TTinyKnob>(mm2px(Vec(7.67, 56.241)), module, Tension::RATIOSLIDER_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.763, 71.419)), module, Tension::CLOCKINPUT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.763, 86.659)), module, Tension::RESETINPUT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.763, 101.659)), module, Tension::GATEOUTPUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.763, 116.899)), module, Tension::OUTPUT_OUTPUT));

		// // mm2px(Vec(12.192, 5.0))
		// addChild(createWidget<Widget>(mm2px(Vec(1.574, 24.703))));
		// // mm2px(Vec(12.192, 5.0))
		// addChild(createWidget<Widget>(mm2px(Vec(1.574, 43.702))));

		{
			BHTensionDisplay *display = new BHTensionDisplay();
			display->module = module;
			display->shapeRect = Rect(mm2px(Vec(1.574, 24.703)), mm2px(Vec(12.192, 5.0)));
			display->ratioRect = Rect(mm2px(Vec(1.574, 43.702)), mm2px(Vec(12.192, 5.0)));
			addChild(display);
		}
	}

	void appendContextMenu(Menu *menu) override
	{
		Tension *module = dynamic_cast<Tension *>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());

		makeAndAddLabel(menu, "Settings");

		// Clock Mode
		menu->addChild(construct<ClockModeMenuItem>(&MenuItem::text, "Clock mode", &MenuItem::rightText, RIGHT_ARROW, &ClockModeMenuItem::module, module));

		// Easing
		menu->addChild(construct<EaseMenuItem>(&MenuItem::text, "Ease", &MenuItem::rightText, RIGHT_ARROW, &EaseMenuItem::module, module));

		// Reset Hard
		menu->addChild(construct<ResetHardMenuItem>(&MenuItem::text, "Reset Hard", &ResetHardMenuItem::module, module));
	}

	//* Menu Items *//

	struct ResetHardMenuItem : MenuItem
	{
		Tension *module;
		void onAction(const event::Action &e) override { module->shouldResetHard = !module->shouldResetHard; }
		void step() override
		{
			rightText = module->shouldResetHard ? CHECKMARK_STRING : "";
			MenuItem::step();
		}
	};

	struct ClockModeMenuItem : MenuItem
	{
		struct ClockModeItem : MenuItem
		{
			Tension *module;
			ClockMode mode;
			void onAction(const event::Action &e) override { module->clockMode = mode; }
			void step() override
			{
				rightText = (module->clockMode == mode) ? CHECKMARK_STRING : "";
				MenuItem::step();
			}
		};

		Tension *module;
		Menu *createChildMenu() override
		{
			Menu *menu = new Menu;
			menu->addChild(construct<ClockModeItem>(&MenuItem::text, EnumToString(ClockMode::CLOCK), &ClockModeItem::module, module, &ClockModeItem::mode, ClockMode::CLOCK));
			menu->addChild(construct<ClockModeItem>(&MenuItem::text, EnumToString(ClockMode::BPM), &ClockModeItem::module, module, &ClockModeItem::mode, ClockMode::BPM));
			return menu;
		}
	};

	struct EaseMenuItem : MenuItem
	{
		struct EaseTypeMenuItem : MenuItem
		{
			struct EaseTypeItem : MenuItem
			{
				Tension *module;
				Ease::Type type;
				void onAction(const event::Action &e) override { module->easeType = type; }
				void step() override
				{
					rightText = (module->easeType == type) ? CHECKMARK_STRING : "";
					MenuItem::step();
				}
			};

			Tension *module;
			Menu *createChildMenu() override
			{
				Menu *menu = new Menu;
				menu->addChild(construct<EaseTypeItem>(&MenuItem::text, EnumToString(Ease::LINEAR), &EaseTypeItem::module, module, &EaseTypeItem::type, Ease::LINEAR));
				menu->addChild(construct<EaseTypeItem>(&MenuItem::text, EnumToString(Ease::SINE), &EaseTypeItem::module, module, &EaseTypeItem::type, Ease::SINE));
				menu->addChild(construct<EaseTypeItem>(&MenuItem::text, EnumToString(Ease::EXPO), &EaseTypeItem::module, module, &EaseTypeItem::type, Ease::EXPO));
				menu->addChild(construct<EaseTypeItem>(&MenuItem::text, EnumToString(Ease::CIRC), &EaseTypeItem::module, module, &EaseTypeItem::type, Ease::CIRC));
				menu->addChild(construct<EaseTypeItem>(&MenuItem::text, EnumToString(Ease::CUBIC), &EaseTypeItem::module, module, &EaseTypeItem::type, Ease::CUBIC));
				menu->addChild(construct<EaseTypeItem>(&MenuItem::text, EnumToString(Ease::QUAD), &EaseTypeItem::module, module, &EaseTypeItem::type, Ease::QUAD));
				menu->addChild(construct<EaseTypeItem>(&MenuItem::text, EnumToString(Ease::QUART), &EaseTypeItem::module, module, &EaseTypeItem::type, Ease::QUART));
				menu->addChild(construct<EaseTypeItem>(&MenuItem::text, EnumToString(Ease::QUINT), &EaseTypeItem::module, module, &EaseTypeItem::type, Ease::QUINT));
				return menu;
			}
		};

		struct EaseModeMenuItem : MenuItem
		{
			struct EaseModeItem : MenuItem
			{
				Tension *module;
				Ease::Mode mode;
				void onAction(const event::Action &e) override { module->easeMode = mode; }
				void step() override
				{
					rightText = (module->easeMode == mode) ? CHECKMARK_STRING : "";
					MenuItem::step();
				}
			};

			Tension *module;
			Menu *createChildMenu() override
			{
				Menu *menu = new Menu;
				menu->addChild(construct<EaseModeItem>(&MenuItem::text, EnumToString(Ease::IN), &EaseModeItem::module, module, &EaseModeItem::mode, Ease::IN));
				menu->addChild(construct<EaseModeItem>(&MenuItem::text, EnumToString(Ease::OUT), &EaseModeItem::module, module, &EaseModeItem::mode, Ease::OUT));
				menu->addChild(construct<EaseModeItem>(&MenuItem::text, EnumToString(Ease::BOTH), &EaseModeItem::module, module, &EaseModeItem::mode, Ease::BOTH));
				return menu;
			}
		};

		struct EaseDirectionMenuItem : MenuItem
		{
			struct EaseDirectionItem : MenuItem
			{
				Tension *module;
				Ease::Direction dir;
				void onAction(const event::Action &e) override { module->easeDirection = dir; }
				void step() override
				{
					rightText = (module->easeDirection == dir) ? CHECKMARK_STRING : "";
					MenuItem::step();
				}
			};

			Tension *module;
			Menu *createChildMenu() override
			{
				Menu *menu = new Menu;
				menu->addChild(construct<EaseDirectionItem>(&MenuItem::text, EnumToString(Ease::FORWARD), &EaseDirectionItem::module, module, &EaseDirectionItem::dir, Ease::FORWARD));
				menu->addChild(construct<EaseDirectionItem>(&MenuItem::text, EnumToString(Ease::REVERSE), &EaseDirectionItem::module, module, &EaseDirectionItem::dir, Ease::REVERSE));
				return menu;
			}
		};

		Tension *module;
		Menu *createChildMenu() override
		{
			Menu *menu = new Menu;
			menu->addChild(construct<EaseTypeMenuItem>(&MenuItem::text, "Shape", &MenuItem::rightText, RIGHT_ARROW, &EaseTypeMenuItem::module, module));
			menu->addChild(construct<EaseModeMenuItem>(&MenuItem::text, "Mode", &MenuItem::rightText, RIGHT_ARROW, &EaseModeMenuItem::module, module));
			menu->addChild(construct<EaseDirectionMenuItem>(&MenuItem::text, "Direction", &MenuItem::rightText, RIGHT_ARROW, &EaseDirectionMenuItem::module, module));
			return menu;
		}
	};

	//* Custom Widgets *//

	struct BHTensionDisplay : TransparentWidget
	{
		Tension *module;
		std::shared_ptr<Font> font;
		std::string fontPath;
		Rect shapeRect, ratioRect;

		BHTensionDisplay()
		{
			fontPath = std::string(asset::plugin(pluginInstance, "res/fonts/MajorMonoDisplay-Regular.ttf"));
		}

		void drawBpmRatio(const DrawArgs &args)
		{
			nvgFontSize(args.vg, 10);
			nvgFontFaceId(args.vg, font->handle);

			Vec textPos = Vec(ratioRect.pos.x - ratioRect.size.x / 8.0, ratioRect.pos.y + ratioRect.size.y - ratioRect.size.y / 4);
			nvgFillColor(args.vg, Colors::LIGHT);
			char text[32];

			snprintf(text, sizeof(text), " %s", DIVISION_NAMES[module->division]);
			nvgText(args.vg, textPos.x, textPos.y, text, NULL);
		}

		void drawShape(const DrawArgs &args)
		{
			nvgStrokeColor(args.vg, Colors::LIGHT);
			nvgStrokeWidth(args.vg, 1.0);

			nvgBeginPath(args.vg);
			Vec startingPoint = Vec(shapeRect.pos.x, shapeRect.pos.y);
			nvgMoveTo(args.vg, startingPoint.x, startingPoint.y + module->evaluate(0.0) * shapeRect.size.y);
			for (int i = 1; i < TENSION_DISPLAY_SIZE; i++)
			{
				nvgLineTo(args.vg,
						  shapeRect.pos.x + (i / TENSION_DISPLAY_SIZE) * shapeRect.size.x,
						  shapeRect.pos.y + module->evaluate(i / TENSION_DISPLAY_SIZE) * shapeRect.size.y);
			}

			nvgStroke(args.vg);
		}

		void drawProgress(const DrawArgs &args)
		{
			nvgStrokeWidth(args.vg, 1.0);
			NVGpaint paint = nvgLinearGradient(args.vg, 0, 0, 0, 151, Colors::ACCENT, nvgTransRGBAf(Colors::ACCENT, 0.2f));
			nvgStrokePaint(args.vg, paint);

			nvgBeginPath(args.vg);

			float progress = module->phase;
			for (int i = 0; i < progress * TENSION_DISPLAY_SIZE; i++)
			{
				nvgMoveTo(args.vg, shapeRect.pos.x + (i / progress * TENSION_DISPLAY_SIZE) * shapeRect.size.x,
						  shapeRect.pos.y + module->evaluate(i / progress * TENSION_DISPLAY_SIZE) * shapeRect.size.y);
				nvgLineTo(args.vg, shapeRect.pos.x + (i / progress * TENSION_DISPLAY_SIZE) * shapeRect.size.x, shapeRect.pos.y + shapeRect.size.y);
			}
			nvgStroke(args.vg);
		}

		void draw(const DrawArgs &args) override
		{
			font = APP->window->loadFont(fontPath);

			if (!module)
				return;

			drawBpmRatio(args);
			drawShape(args);
			drawProgress(args);
		}
	};
};

Model *modelTension = createModel<Tension, TensionWidget>("Tension");
