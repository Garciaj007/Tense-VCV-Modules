#include "plugin.hpp"

#include "penners.hpp"

#define TENSION_DISPLAY_SIZE 32

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
		TRIGGER_INPUT,
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
	int voltagePairs = VoltagePairs::NEGATIVE_10_TO_10;

	bool shouldResetHard = false;

	//* DONT SAVE with RESET *//

	RefreshCounter refreshCounter;
	dsp::SchmittTrigger clockTrigger, inputTrigger;

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
	bool b_buttonState = true;

	float bufferedShapeKnob = 0.f;
	float bufferedRatioKnob = 0.f;
	float bufferedTriggerButton = 0.f;
	float bufferedResetButton = 0.f;

	double evaluate(double x)
	{
		//return Ease::EnumToFunction(Ease::Type(easeType))(Ease::Mode(easeMode), x, duration, amplitude, offset);
		return Ease::EnumToFunction(Ease::Type(easeType))(Ease::Mode(easeMode), x);
	}

	Tension()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(TRIGGER_PARAM, 0.f, 1.f, 0.f, "");
		configParam(RESET_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SHAPESLIDER_PARAM, 0.0, Ease::Type::COUNT, 0.f, "");

		// Ratio Knob [IMPROMPTU MODULAR]
		// configParam<RatioParam>(RATIOSLIDER_PARAM, ((float)NUM_RATIOS - 1.0f) * -1.0f, float(NUM_RATIOS) - 1.0f, 0.0f, "Clock Ratio");

		// Ratio Knob [FROZEN WASTELAND]
		configParam(RATIOSLIDER_PARAM, 0.0, DIVISION_COUNT - 1, (DIVISION_COUNT - 1) / 2);

		//	TODO V2
		//	paramQuantities[RATIOSLIDER_PARAM]->snapEnabled = true; Enable Snapping Later in VCV Rack v2
		//	configInput(CLOCKINPUT_INPUT, "Clock");
		//	configInput(TRIGGER_INPUT, "Trigger");
		//
		//	configOutput(GATEOUTPUT_OUTPUT, "GATE");
		//	configOutput(OUTPUT_OUTPUT, "CV OUT");

		onReset();
	}

	// Direction Must Always be FORWARD!!!!
	void step(double dt)
	{
		double delta = fmin(freq * dt, 0.5);
		phase += delta;
		phase = clamp(phase);
	}

	void onReset() override {}

	void reset(bool hard)
	{
		// Reset The Phase...
		phase = 1.0 - phase;

		if (hard)
		{
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
		auto* jsonDef = json_object_get(json, "clockMode");
		if(jsonDef) clockMode = json_integer_value(jsonDef);
		jsonDef = json_object_get(json, "easeMode");
		if(jsonDef) easeMode = json_integer_value(jsonDef);
		jsonDef = json_object_get(json, "easeType");
		if(jsonDef) easeType = json_integer_value(jsonDef);
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

		// GUI Refresh
		if (refreshCounter.processInputs())
		{
			const auto shape_value = params[SHAPESLIDER_PARAM].getValue();
			if (bufferedShapeKnob != shape_value)
			{
				bufferedShapeKnob = shape_value;
				easeType = int(clamp(bufferedShapeKnob, 0.0f, (float)Ease::Type::COUNT - 1));
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
			// Input Takes Priority...
			const auto _input = inputs[TRIGGER_INPUT].getVoltage() + params[TRIGGER_PARAM].getValue();
			if (bufferedTriggerButton != _input)
			{
				bufferedTriggerButton = _input;
				b_buttonState = _input != 0.0;
				reset(shouldResetHard);
				outputs[GATEOUTPUT_OUTPUT].setVoltage(b_buttonState * 10.0);
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
		if (refreshCounter.processLights())
		{
		}

		double tension = b_buttonState ? 1 - evaluate(phase) : evaluate(phase);

		//outputs[GATEOUTPUT_OUTPUT].setVoltage(phase);
		outputs[OUTPUT_OUTPUT].setVoltage(tension * 10.0); // Sets Voltage 0 V ... 10 V
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
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.763, 86.659)), module, Tension::TRIGGER_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.763, 101.659)), module, Tension::GATEOUTPUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.763, 116.899)), module, Tension::OUTPUT_OUTPUT));

		BHTensionDisplay *display = new BHTensionDisplay();
		display->module = module;
		display->shapeRect = Rect(mm2px(Vec(1.574, 24.703)), mm2px(Vec(12.192, 5.0)));
		display->ratioRect = Rect(mm2px(Vec(1.574, 43.702)), mm2px(Vec(12.192, 5.0)));
		addChild(display);
	}

	void appendContextMenu(Menu *menu) override
	{
		Tension *module = dynamic_cast<Tension *>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator);

		// Clock Mode
		menu->addChild(construct<ClockModeMenuItem>(&MenuItem::text, "Clock mode", &MenuItem::rightText, RIGHT_ARROW, &ClockModeMenuItem::module, module));

		menu->addChild(new MenuSeparator());

		// Easing
		menu->addChild(construct<EaseTypeMenuItem>(&MenuItem::text, "Shape", &MenuItem::rightText, RIGHT_ARROW, &EaseTypeMenuItem::module, module));
		menu->addChild(construct<EaseModeMenuItem>(&MenuItem::text, "Mode", &MenuItem::rightText, RIGHT_ARROW, &EaseModeMenuItem::module, module));

		menu->addChild(new MenuSeparator());

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
			menu->addChild(construct<EaseTypeItem>(&MenuItem::text, EnumToString(Ease::BACK), &EaseTypeItem::module, module, &EaseTypeItem::type, Ease::BACK));
			menu->addChild(construct<EaseTypeItem>(&MenuItem::text, EnumToString(Ease::ELASTIC), &EaseTypeItem::module, module, &EaseTypeItem::type, Ease::ELASTIC));
			menu->addChild(construct<EaseTypeItem>(&MenuItem::text, EnumToString(Ease::BOUNCE), &EaseTypeItem::module, module, &EaseTypeItem::type, Ease::BOUNCE));
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

		void drawBpmRatioText(const DrawArgs &args)
		{
			nvgFontSize(args.vg, 10);
			nvgFontFaceId(args.vg, font->handle);
			nvgFillColor(args.vg, Colors::LIGHT);

			char text[32];
			Vec textPos = Vec(ratioRect.pos.x - ratioRect.size.x / 8.0, ratioRect.pos.y + ratioRect.size.y - ratioRect.size.y / 4);
			snprintf(text, sizeof(text), " %s", DIVISION_NAMES[module->division]);
			nvgText(args.vg, textPos.x, textPos.y, text, NULL);
		}

		void drawTypeText(const DrawArgs &args)
		{
			nvgFontSize(args.vg, 10);
			nvgFontFaceId(args.vg, font->handle);
			nvgFillColor(args.vg, Colors::LIGHT);

			char text[32];
			Vec textPos = Vec(shapeRect.pos.x - shapeRect.size.x / 8.0, shapeRect.pos.y + shapeRect.size.y - shapeRect.size.y / 4);
			snprintf(text, sizeof(text), " %s", Ease::TypeIdStrings[module->easeType]);
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

			drawBpmRatioText(args);
			drawTypeText(args);

			// TODO...
			// drawShape(args);
			// drawProgress(args);
		}
	};
};

Model *modelTension = createModel<Tension, TensionWidget>("Tension");
