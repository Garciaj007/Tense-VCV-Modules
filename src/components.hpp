//////////////////////////////////////////////////////////////////////////
//  Beyond Help Module Collection
//  for VCV Rack By Juriel Garcia Sanchez
//
//  Based on code from the Fundamental by Andrew Belt,
//  And on code from Impromptu Modular by Marc Boulé,
//  And on code from Frozen Wasteland by Eric Sterling
//
//  See ./LICENSE.md for all licenses
//  See ./res/fonts/ for font licenses
////////////////////////////////////////////////////////////////////////

#pragma once

#include <rack.hpp>

using namespace rack;

extern Plugin *pluginInstance;

/* Constants */

static const int NUM_RATIOS = 35;
static const float RATIO_VALUES[NUM_RATIOS] = {1, 1.5, 2, 2.5, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 19, 23, 24, 29, 31, 32, 37, 41, 43, 47, 48, 53, 59, 61, 64, 96};

static const int BPM_MAX = 300;
static const int BPM_MIN = 30;

static constexpr float DURATION_MIN_F = 60.0f / BPM_MIN;
static constexpr float DURATION_MAX_F = 60.0f / BPM_MAX;

static const float DIVISIONS[] = {1 / 64.0, 1 / 32.0, 1 / 16.0, 1 / 13.0, 1 / 11.0, 1 / 8.0, 1 / 7.0, 1 / 6.0, 1 / 5.0, 1 / 4.0, 1 / 3.0, 1 / 2.0, 1 / 1.5, 1, 1.5, 2, 3, 4, 5, 6, 7, 8, 11, 13, 16, 32, 64};
static const char *DIVISION_NAMES[] = {"/64", "/32", "/16", "/13", "/11", "/8", "/7", "/6", "/5", "/4", "/3", "/2", "/1.5", "x1", "x1.5", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x11", "x13", "x16", "x32", "x64"};
static constexpr int DIVISION_COUNT = sizeof(DIVISIONS) / sizeof(DIVISIONS[0]);

inline double clamp(double x, double a = 0., double b = 1.)
{
    return std::fmax(std::fmin(x, b), a);
}
struct Colors
{
    static const NVGcolor BG;
    static const NVGcolor FG;
    static const NVGcolor PRIMARY;
    static const NVGcolor ACCENT;
    static const NVGcolor LIGHT;
};

enum VoltagePairs
{
    NEGATIVE_10_TO_10,
    NEGATIVE_5_TO_5,
    NEGATIVE_3_TO_3,
    NEGATIVE_1_TO_1,
    POSITIVE_0_TO_10,
    POSITIVE_0_TO_5,
    POSITIVE_0_TO_3,
    POSITIVE_0_TO_1,
};
static const char *VoltagePairStrings[] = {"-10 - 10", "-5 - 5", "-3 - 3", "-1 - 1", "0 - 10", "0 - 5", "0 - 3", "0 - 1"};
static const char *EnumToString(VoltagePairs pair) { return VoltagePairStrings[(int)pair]; }

enum ClockMode
{
    CLOCK,
    BPM
};
static const char *ClockModeStrings[] = {"Clock", "BPM"};
static const char *EnumToString(ClockMode mode) { return ClockModeStrings[(int)mode]; }

/** Helpers **/

/// Spinlock
struct RefreshCounter
{
    // Note: because of stagger, and asyncronous dataFromJson, should not assume this processInputs() will return true on first run
    // of module::process()
    static const unsigned int displayRefreshStepSkips = 256;
    static const unsigned int userInputsStepSkipMask = 0xF; // sub interval of displayRefreshStepSkips, since inputs should be more responsive than lights
    // above value should make it such that inputs are sampled > 1kHz so as to not miss 1ms triggers

    unsigned int refreshCounter = (random::u32() % displayRefreshStepSkips); // stagger start values to avoid processing peaks when many Geo and Impromptu modules in the patch

    bool processInputs()
    {
        return ((refreshCounter & userInputsStepSkipMask) == 0);
    }
    bool processLights()
    { // this must be called even if module has no lights, since counter is decremented here
        refreshCounter++;
        bool process = refreshCounter >= displayRefreshStepSkips;
        if (process)
        {
            refreshCounter = 0;
        }
        return process;
    }
};

struct RatioParam : ParamQuantity
{
    float getDisplayValue() override
    {
        int knobVal = (int)std::round(getValue());
        knobVal = clamp(knobVal, (NUM_RATIOS - 1) * -1, NUM_RATIOS - 1);
        if (knobVal < 0)
        {
            return -RATIO_VALUES[-knobVal];
        }
        return RATIO_VALUES[knobVal];
    }
    void setDisplayValue(float displayValue) override
    {
        bool div = displayValue < 0;
        float setVal;
        if (div)
        {
            displayValue = -displayValue;
        }
        // code below is not pretty, but easiest since irregular spacing of ratio values
        if (displayValue > 80.0f)
        {
            setVal = 34.0f;
        }
        else if (displayValue >= 62.5f)
        {
            setVal = 33.0f;
        }
        else if (displayValue >= 60.0f)
        {
            setVal = 32.0f;
        }
        else if (displayValue >= 56.0f)
        {
            setVal = 31.0f;
        }
        else if (displayValue >= 50.5f)
        {
            setVal = 30.0f;
        }
        else if (displayValue >= 47.5f)
        {
            setVal = 29.0f;
        }
        else if (displayValue >= 45.0f)
        {
            setVal = 28.0f;
        }
        else if (displayValue >= 42.0f)
        {
            setVal = 27.0f;
        }
        else if (displayValue >= 39.0f)
        {
            setVal = 26.0f;
        }
        else if (displayValue >= 34.5f)
        {
            setVal = 25.0f;
        }
        else if (displayValue >= 31.5f)
        {
            setVal = 24.0f;
        }
        else if (displayValue >= 30.0f)
        {
            setVal = 23.0f;
        }
        else if (displayValue >= 26.5f)
        {
            setVal = 22.0f;
        }
        else if (displayValue >= 23.5f)
        {
            setVal = 21.0f;
        }
        else if (displayValue >= 21.0f)
        {
            setVal = 20.0f;
        }
        else if (displayValue >= 18.0f)
        {
            setVal = 19.0f;
        }
        else if (displayValue >= 16.5f)
        {
            setVal = 18.0f;
        }
        else if (displayValue >= 2.75f)
        {
            // 3 to 16 map into 4 to 17
            setVal = 1.0 + std::round(displayValue);
        }
        else if (displayValue >= 2.25f)
        {
            setVal = 3.0f;
        }
        else if (displayValue >= 1.75f)
        {
            setVal = 2.0f;
        }
        else if (displayValue >= 1.25f)
        {
            setVal = 1.0f;
        }
        else
        {
            setVal = 0.0f;
        }
        if (setVal != 0.0f && div)
        {
            setVal *= -1.0f;
        }
        setValue(setVal);
    }
    std::string getUnit() override
    {
        if (getValue() >= 0.0f)
            return std::string("x");
        return std::string(" (÷)");
    }
};

template <typename T, typename Ret>
static bool getFromJson(T &value, const json_t *json, const char *key, Ret (*func)(const json_t *))
{
    json_t *json_object = json_object_get(json, key);
    if (json_object)
    {
        value = func(json_object);
    }
    return false;
}

template <typename T, typename Ret>
static void getFromJsonOrDefault(T &value, const json_t *json, const char *key, Ret (*func)(const json_t *), T default_value)
{
    if (!getFromJson(value, json, key, func))
        value = default_value;
}

/** Components **/

struct TToggleButton : app::SvgSwitch
{
    TToggleButton()
    {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/TToggleButton_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/TToggleButton_1.svg")));
    }
};

struct TMicroButton : app::SvgSwitch
{
    TMicroButton()
    {
        momentary = true;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/TMicroButton_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/TMicroButton_1.svg")));
    }
};

struct TTinyKnob : app::SvgKnob
{
    TTinyKnob()
    {
        minAngle = -0.75 * M_PI;
        maxAngle = 0.75 * M_PI;

        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/components/TTinyKnob.svg")));
    }
};