#include "MidiFile.h"
#include "plugin.hpp"
#include <osdialog.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <random>

static const char *MIDI_FILTER = "Midi (.mid):mid";
static constexpr int MAX_CHANNEL_SIZE = 16;

static std::string HexStringToByteString(std::string hex)
{
	std::basic_string<uint8_t> bytes;

	for (size_t i = 0; i < hex.length(); i += 2)
	{
		uint16_t byte;
		std::string _byte = hex.substr(i, 2);
		std::istringstream(_byte) >> std::hex >> byte;
		bytes.push_back(static_cast<uint8_t>(byte));
	}

	std::string result(begin(bytes), end(bytes));
	return result;
}

static uint8_t voltPerOctToMidi(float voltage)
{
	return 0;
}

static uint8_t voltVelToMidi(float voltage)
{
	return 0;
}

struct TenseMidiRecorder : Module
{
	enum ParamIds
	{
		TRIGGER_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
		CLOCK_INPUT,
		RECORD_INPUT,
		VOLTAGE_INPUT,
		GATE_INPUT,
		VELOCITY_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
		NUM_OUTPUTS
	};
	enum LightIds
	{
		NUM_LIGHTS
	};

private:
	std::string path;
	std::string directory;
	std::string basename;

	bool shouldIncrementPath;
	bool isRecording;
	bool polyphonyAsDistinctTracks;
	bool isClockConnected;
	bool firstClockReceived;
	bool secondClockReceived;
	bool firstEventReceived;

	float bpm;
	float timeElapsed;
	float duration;

	int incrementIndex;
	int clockMode;

	uint16_t ticksPerQN = 960;

	long totalTime;
	int tickCount;
	long ticksSinceLastEvent;
	double stepCount;

	std::vector<bool> prevGates;

	dsp::ClockDivider clockDivider;
	dsp::SchmittTrigger clockTrigger, trigTrigger;
	dsp::BooleanTrigger recTrigger;

	smf::MidiFile midiFile;

	friend struct TenseMidiRecorderWidget;

public:
	const std::string getPath() const { return path; }

	TenseMidiRecorder()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(TRIGGER_PARAM, 0.f, 1.f, 0.f, "");

		clockDivider.setDivision(32);

		prevGates.resize(MAX_CHANNEL_SIZE, false);

		onReset();
	}

	json_t *dataToJson() override
	{
		json_t *json = json_object();

		json_object_set_new(json, "path", json_string(path.c_str()));
		json_object_set_new(json, "shouldIncrementPath", json_boolean(shouldIncrementPath));
		json_object_set_new(json, "polyphonyAsDistinctTracks", json_boolean(polyphonyAsDistinctTracks));

		return json;
	}

	void dataFromJson(json_t *json) override
	{
		json_t *pathDef = json_object_get(json, "path");
		if (pathDef)
			setPath(json_string_value(pathDef));

		json_t *shouldIncrementPathDef = json_object_get(json, "shouldIncrementPath");
		if (shouldIncrementPathDef)
			shouldIncrementPath = json_boolean_value(shouldIncrementPathDef);

		json_t *polyphonyAsDistinctTracksDef = json_object_get(json, "polyphonyAsDistinctTracks");
		if (polyphonyAsDistinctTracksDef)
			polyphonyAsDistinctTracks = json_boolean_value(polyphonyAsDistinctTracksDef);
	}

	void onReset() override
	{
		path = "";
		directory = "";
		basename = "";
		shouldIncrementPath = true;
		polyphonyAsDistinctTracks = false;
		incrementIndex = 0;
	}

	void process(const ProcessArgs &args) override
	{
		float sampleRate = args.sampleRate;
		timeElapsed += 1.0 / args.sampleRate;

		// Clock Pin
		if (inputs[CLOCK_INPUT].isConnected())
		{
			if (ClockMode::BPM)
			{
				bpm = powf(1.0, clamp(inputs[CLOCK_INPUT].getVoltage(), -10.0f, 10.0f)) * 120.0;
				duration = 60.0 / bpm;
			}
			else
			{
				if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage()))
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
				bpm = duration * bpm;
			}
			isClockConnected = true;
		}
		else
		{
			bpm = 120;
			duration = 60 / bpm;
			firstClockReceived = false;
			secondClockReceived = false;
			isClockConnected = false;
		}

		if (clockDivider.process())
		{
			bool _isRecording = isRecording;

			if (recTrigger.process(params[TRIGGER_PARAM].getValue()))
				_isRecording ^= true;
			if (trigTrigger.process(rescale(inputs[RECORD_INPUT].getVoltage(), 0.1, 2.0, 0.0, 1.0)))
				_isRecording ^= true;

			if (!_isRecording && isRecording)
				stop();
		}

		float tickLength = (sampleRate * 60) / (bpm * ticksPerQN);

		if (isRecording)
		{
			uint8_t numChannels = inputs[GATE_INPUT].getChannels();

			for (int i = 0; i < numChannels; i++)
			{
				int gateIn = inputs[GATE_INPUT].getVoltage(i);
				float noteIn = inputs[VOLTAGE_INPUT].getVoltage(i);
				float velocityIn = inputs[VOLTAGE_INPUT].getVoltage(i);

				if (gateIn && !prevGates[i])
				{
					if (polyphonyAsDistinctTracks)
						midiFile.addNoteOn(i, ticksSinceLastEvent, 0, voltPerOctToMidi(noteIn), voltVelToMidi(velocityIn));
					else
						midiFile.addNoteOn(0, ticksSinceLastEvent, i, voltPerOctToMidi(noteIn), voltVelToMidi(velocityIn));
				}
				else
				{
					if (polyphonyAsDistinctTracks)
						midiFile.addNoteOff(i, ticksSinceLastEvent, 0, voltPerOctToMidi(noteIn), voltVelToMidi(velocityIn));
					else
						midiFile.addNoteOff(0, ticksSinceLastEvent, i, voltPerOctToMidi(noteIn), voltVelToMidi(velocityIn));
				}
			}

			if (firstEventReceived)
			{
				stepCount++;
				if (stepCount >= tickLength)
				{
					stepCount -= tickLength;
					ticksSinceLastEvent++;
					tickCount++;
					if (tickCount >= ticksPerQN)
					{
						totalTime++;
						tickCount = 0;
					}
				}
			}
		}
	}

	void setPath(std::string path)
	{
		if (this->path == path)
			return;

		if (path == "")
		{
			this->path = "";
			directory = "";
			basename = "";
			return;
		}

		directory = string::directory(path);
		basename = string::filenameBase(string::filename(path));
		path = directory + "/" + basename; // midi file extension...
		incrementIndex = 0;
	}

	bool isPathDirectoryValid() const
	{
		if (path == "")
			return false;
		return system::isDirectory(string::directory(path));
	}

	void stop()
	{
		writeToMidiFile();
	}

	void writeToMidiFile()
	{
		if (path == "")
			return;

		std::string incrementedPath = path + string::f(".%03d", incrementIndex) + ".mid";

		midiFile.sortTracks();
		if (midiFile.write(incrementedPath))
		{
			incrementIndex++;
		}
		midiFile.clear();
	}
};

static void selectPath(TenseMidiRecorder *module)
{
	const std::string path = module->getPath();

	std::string dir;
	std::string filename;

	if (path != "")
	{
		dir = string::directory(path);
		filename = string::filename(path);
	}
	else
	{
		dir = asset::user("");
		filename = "Untitled";
	}

	osdialog_filters *filters = osdialog_filters_parse(MIDI_FILTER);
	DEFER({ osdialog_filters_free(filters); });

	char *selectedPath = osdialog_file(OSDIALOG_SAVE, dir.c_str(), filename.c_str(), NULL);
	if (selectedPath)
		module->setPath(selectedPath);
	DEFER({ std::free(selectedPath); });
}

struct TenseMidiRecorderWidget : ModuleWidget
{
	TenseMidiRecorderWidget(TenseMidiRecorder *module)
	{
		setModule(module);

		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TenseMidiRecorder.svg")));

		addParam(createParamCentered<TToggleButton>(mm2px(Vec(7.67, 12.0)), module, TenseMidiRecorder::TRIGGER_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 56.179)), module, TenseMidiRecorder::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 71.419)), module, TenseMidiRecorder::RECORD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 86.659)), module, TenseMidiRecorder::VOLTAGE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 101.659)), module, TenseMidiRecorder::GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 116.899)), module, TenseMidiRecorder::VELOCITY_INPUT));

		// mm2px(Vec(12.192, 5.0))
		addChild(createWidget<Widget>(mm2px(Vec(1.574, 22.15))));
		// mm2px(Vec(12.192, 5.0))
		addChild(createWidget<Widget>(mm2px(Vec(1.574, 31.054))));
		// mm2px(Vec(12.192, 5.0))
		addChild(createWidget<Widget>(mm2px(Vec(1.574, 40.075))));
	}

	void appendContextMenu(Menu *menu) override
	{
		TenseMidiRecorder *module = dynamic_cast<TenseMidiRecorder *>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator);

		menu->addChild(createMenuLabel("Ouput"));

		std::string path = string::ellipsizePrefix(module->path, 30);
		menu->addChild(construct<PathItem>(&MenuItem::text, path != "" ? path : "Select...", &TMRItem::module, module));
		menu->addChild(construct<ShouldIncrementPathItem>(&MenuItem::text, "Append (000, 001, 002...)", &TMRItem::module, module));

		menu->addChild(new MenuSeparator);

		menu->addChild(construct<PolyphonyAsDistinctTracks>(&MenuItem::text, "Polyphony as distinct tracks", &TMRItem::module, module));

		// TODO Some More Settings :D
	}

	struct TMRItem : MenuItem
	{
		TenseMidiRecorder *module;
	};

	struct ClockModeMenuItem : TMRItem
	{
		struct ClockModeItem : TMRItem
		{
			ClockMode mode;
			void onAction(const event::Action &e) override { module->clockMode = mode; }
			void step() override
			{
				rightText = (module->clockMode == mode) ? CHECKMARK_STRING : "";
				MenuItem::step();
			}
		};

		Menu *createChildMenu() override
		{
			Menu *menu = new Menu;
			menu->addChild(construct<ClockModeItem>(&MenuItem::text, EnumToString(ClockMode::CLOCK), &ClockModeItem::module, module, &ClockModeItem::mode, ClockMode::CLOCK));
			menu->addChild(construct<ClockModeItem>(&MenuItem::text, EnumToString(ClockMode::BPM), &ClockModeItem::module, module, &ClockModeItem::mode, ClockMode::BPM));
			return menu;
		}
	};

	struct PathItem : TMRItem
	{
		void onAction(const event::Action &e) override { selectPath(module); }
	};

	struct ShouldIncrementPathItem : TMRItem
	{
		void onAction(const event::Action &e) override { module->shouldIncrementPath ^= true; }
		void step() override
		{
			rightText = module->shouldIncrementPath ? CHECKMARK_STRING : "";
			MenuItem::step();
		}
	};

	struct PolyphonyAsDistinctTracks : TMRItem
	{
		void onAction(const event::Action &e) override { module->polyphonyAsDistinctTracks ^= true; }
		void step() override
		{
			rightText = module->polyphonyAsDistinctTracks ? CHECKMARK_STRING : "";
			MenuItem::step();
		}
	};
};

Model *modelTenseMidiRecorder = createModel<TenseMidiRecorder, TenseMidiRecorderWidget>("TenseMidiRecorder");