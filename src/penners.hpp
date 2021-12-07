//////////////////////////////////////////////////////////////////////
//  Terms of Use: Easing Functions (Equations)
//  Open source under the MIT License and the 3-Clause BSD License.
//
//  MIT License
//  Copyright © 2001 Robert Penner
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//  BSD License
//  Copyright © 2001 Robert Penner
//
//  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//
//  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
//  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
//  Neither the name of the author nor the names of contributors may be used to endorse or promote products derived from this software without specific prior written permission.
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
//  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//////////////////////////////////////////////////////////////////

#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsequence-point"

#include <math.h>

#ifndef PI
#define PI 3.14159265
#endif

struct Ease
{
    enum Mode
    {
        IN,
        OUT,
        BOTH
    };
    static const char *ModeStrings[];

    enum Type
    {
        LINEAR,
        SINE,
        EXPO,
        CIRC,
        CUBIC,
        QUAD,
        QUART,
        QUINT
    };
    static const char *TypeStrings[];

    // Might Delete???
    enum Direction
    {
        FORWARD,
        REVERSE,
        PINGPONG
    };
    static const char *DirectionStrings[];

    using Func = double (*)(Mode mode, double time, double duration, double amplitude, double offset);

    static Func EnumToFunction(Type type)
    {
        switch (type)
        {
        case QUINT:
            return Quint;
        case QUART:
            return Quart;
        case QUAD:
            return Quad;
        case CUBIC:
            return Cubic;
        case CIRC:
            return Circ;
        case EXPO:
            return Expo;
        case SINE:
            return Sine;
        case LINEAR:
        default:
            return Linear;
        }
    }

    static double Linear(Mode mode, double t, double d, double amp, double off)
    {
        return amp * t / d + off;
    }

    static double Sine(Mode mode, double t, double d, double amp, double off)
    {
        switch (mode)
        {
        case Mode::IN:
            return -amp * cos(t / d * (PI / 2)) + amp + off;
        case Mode::OUT:
            return amp * sin(t / d * (PI / 2)) + off;
        case Mode::BOTH:
        default:
            return -amp / 2 * (cos(PI * t / d) - 1) + off;
        }
    }

    static double Expo(Mode mode, double t, double d, double amp, double off)
    {
        switch (mode)
        {
        case Mode::IN:
            return (t == 0) ? off : amp * pow(2, 10 * (t / d - 1)) + off;
        case Mode::OUT:
            return (t == d) ? off + amp : amp * (-pow(2, -10 * t / d) + 1) + off;
        case Mode::BOTH:
        default:
        {
            if (t == 0)
                return off;
            if (t == d)
                return off + amp;
            if ((t /= d / 2) < 1)
                return amp / 2 * pow(2, 10 * (t - 1)) + off;
            return amp / 2 * (-pow(2, -10 * --t) + 2) + off;
        }
        }
    }

    static double Circ(Mode mode, double t, double d, double amp, double off)
    {
        switch (mode)
        {
        case Mode::IN:
            return -amp * (sqrt(1 - (t /= d) * t) - 1) + off;
        case Mode::OUT:
            return amp * sqrt(1 - (t = t / d - 1) * t) + off;
        case Mode::BOTH:
        default:
        {
            if ((t /= d / 2) < 1)
                return -amp / 2 * (sqrt(1 - t * t) - 1) + off;
            return amp / 2 * (sqrt(1 - t * (t -= 2)) + 1) + off;
        }
        }
    }

    static double Cubic(Mode mode, double t, double d, double amp, double off)
    {
        switch (mode)
        {
        case Mode::IN:
            return amp * (t /= d) * t * t + off;
        case Mode::OUT:
            return amp * ((t = t / d - 1) * t * t + 1) + off;
        case Mode::BOTH:
        default:
        {
            if ((t /= d / 2) < 1)
                return amp / 2 * t * t * t + off;
            return amp / 2 * ((t -= 2) * t * t + 2) + off;
        }
        }
    }

    static double Quad(Mode mode, double t, double d, double amp, double off)
    {
        switch (mode)
        {
        case Mode::IN:
            return amp * (t /= d) * t + off;
        case Mode::OUT:
            return -amp * (t /= d) * (t - 2) + off;
        case Mode::BOTH:
        default:
        {
            if ((t /= d / 2) < 1)
                return ((amp / 2) * (t * t)) + off;
            return -amp / 2 * (((t - 2) * (--t)) - 1) + off;
        }
        }
    }

    static double Quart(Mode mode, double t, double d, double amp, double off)
    {
        switch (mode)
        {
        case Mode::IN:
            return amp * (t /= d) * t * t * t + off;
        case Mode::OUT:
            return -amp * ((t = t / d - 1) * t * t * t - 1) + off;
        case Mode::BOTH:
        default:
        {
            if ((t /= d / 2) < 1)
                return amp / 2 * t * t * t * t + off;
            return -amp / 2 * ((t -= 2) * t * t * t - 2) + off;
        }
        }
    }

    static double Quint(Mode mode, double t, double d, double amp, double off)
    {
        switch (mode)
        {
        case Mode::IN:
            return amp * (t /= d) * t * t * t * t + off;
        case Mode::OUT:
            return amp * ((t = t / d - 1) * t * t * t * t + 1) + off;
        case Mode::BOTH:
        default:
        {
            if ((t /= d / 2) < 1)
                return amp / 2 * t * t * t * t * t + off;
            return amp / 2 * ((t -= 2) * t * t * t * t + 2) + off;
        }
        }
    }
};

const char *Ease::ModeStrings[] = {"In", "Out", "Both"};
const char *Ease::TypeStrings[] = {"Linear", "Sine", "Expo", "Circ", "Cubic", "Quad", "Quart", "Quint"};
const char *Ease::DirectionStrings[] = {"Forward", "Reverse", "PingPng"};

static const char *EnumToString(Ease::Mode mode) { return Ease::ModeStrings[mode]; }
static const char *EnumToString(Ease::Type type) { return Ease::TypeStrings[type]; }
static const char *EnumToString(Ease::Direction dir) { return Ease::DirectionStrings[dir]; }

#pragma GCC diagnostic pop