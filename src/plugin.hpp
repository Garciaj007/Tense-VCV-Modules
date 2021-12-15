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
#include "components.hpp"

using namespace rack;

extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelTension;
extern Model* modelTenseMidiRecorder;
