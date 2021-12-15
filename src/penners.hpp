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
        QUINT,
        BACK,
        ELASTIC,
        BOUNCE,
        COUNT
    };
    static const char *TypeStrings[];
    static const char *TypeIdStrings[];

    // mode, Where mode = Ease::Mode, x, Where x = 0...1
    using Func = double (*)(Mode mode, double x);

    static Func EnumToFunction(Type type)
    {
        switch (type)
        {
        case BOUNCE:
            return Bounce;
        case ELASTIC:
            return Elastic;
        case BACK:
            return Back;
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

    static double Linear(Mode mode, double x)
    {
        return x;
    }

    static double Sine(Mode mode, double x)
    {
        switch (mode)
        {
        case Mode::IN:
            return 1.0 - cos((x * PI) / 2.0);
        case Mode::OUT:
            return sin((x * PI) / 2);
        case Mode::BOTH:
        default:
            return -(cos(PI * x) - 1) / 2;
        }
    }

    static double Expo(Mode mode, double x)
    {
        switch (mode)
        {
        case Mode::IN:
            return x == 0 ? 0 : pow(2, 10 * x - 10);
        case Mode::OUT:
            return x == 1 ? 1 : 1 - pow(2, -10 * x);
        case Mode::BOTH:
        default:
            return x == 0
                       ? 0
                   : x == 1
                       ? 1
                   : x < 0.5 ? pow(2, 20 * x - 10) / 2
                             : (2 - pow(2, -20 * x + 10)) / 2;
        }
    }

    static double Circ(Mode mode, double x)
    {
        switch (mode)
        {
        case Mode::IN:
            return 1 - sqrt(1 - pow(x, 2));
        case Mode::OUT:
            return sqrt(1 - pow(x - 1, 2));
        case Mode::BOTH:
        default:
            return x < 0.5
                       ? (1 - sqrt(1 - pow(2 * x, 2))) / 2
                       : (sqrt(1 - pow(-2 * x + 2, 2)) + 1) / 2;
        }
    }

    static double Cubic(Mode mode, double x)
    {
        switch (mode)
        {
        case Mode::IN:
            return x * x * x;
        case Mode::OUT:
            return 1 - pow(1 - x, 3);
        case Mode::BOTH:
        default:
            return x < 0.5 ? 4 * x * x * x : 1 - pow(-2 * x + 2, 3) / 2;
        }
    }

    static double Quad(Mode mode, double x)
    {
        switch (mode)
        {
        case Mode::IN:
            return x * x;
        case Mode::OUT:
            return 1 - (1 - x) * (1 - x);
        case Mode::BOTH:
        default:
            return x < 0.5 ? 2 * x * x : 1 - pow(-2 * x + 2, 2) / 2;
        }
    }

    static double Quart(Mode mode, double x)
    {
        switch (mode)
        {
        case Mode::IN:
            return x * x * x * x;
        case Mode::OUT:
            return 1 - pow(1 - x, 4);
        case Mode::BOTH:
        default:
            return x < 0.5 ? 8 * x * x * x * x : 1 - pow(-2 * x + 2, 4) / 2;
        }
    }

    static double Quint(Mode mode, double x)
    {
        switch (mode)
        {
        case Mode::IN:
            return x * x * x * x * x;
        case Mode::OUT:
            return 1 - pow(1 - x, 5);
        case Mode::BOTH:
        default:
            return x < 0.5 ? 16 * x * x * x * x * x : 1 - pow(-2 * x + 2, 5) / 2;
        }
    }

    static constexpr double c1 = 1.70158;
    static constexpr double c2 = c1 * 1.525;
    static constexpr double c3 = c1 + 1;
    static double Back(Mode mode, double x)
    {
        switch (mode)
        {
        case Mode::IN:
            return c3 * x * x * x - c1 * x * x;
        case Mode::OUT:
            return 1 + c3 * pow(x - 1, 3) + c1 * pow(x - 1, 2);
        case Mode::BOTH:
        default:
            return x < 0.5
                       ? (pow(2 * x, 2) * ((c2 + 1) * 2 * x - c2)) / 2
                       : (pow(2 * x - 2, 2) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;
        }
    }

    static constexpr double c4 = (2 * PI) / 3;
    static double Elastic(Mode mode, double x)
    {
        switch (mode)
        {
        case Mode::IN:
            return x == 0
                       ? 0
                   : x == 1
                       ? 1
                       : -pow(2, 10 * x - 10) * sin((x * 10 - 10.75) * c4);
        case Mode::OUT:
            return x == 0
                       ? 0
                   : x == 1
                       ? 1
                       : pow(2, -10 * x) * sin((x * 10 - 0.75) * c4) + 1;
        case Mode::BOTH:
        default:
        {
            const double c2 = c1 * 1.525;
            return x < 0.5
                       ? (pow(2 * x, 2) * ((c2 + 1) * 2 * x - c2)) / 2
                       : (pow(2 * x - 2, 2) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;
        }
        }
    }

    static constexpr double n1 = 7.5625;
    static constexpr double d1 = 2.75;
    static double Bounce(Mode mode, double x)
    {
        switch (mode)
        {
        case Mode::IN:
            return 1 - Bounce(Mode::OUT, 1 - x);
        case Mode::OUT:
        {
            if (x < 1 / d1)
            {
                return n1 * x * x;
            }
            else if (x < 2 / d1)
            {
                return n1 * (x -= 1.5 / d1) * x + 0.75;
            }
            else if (x < 2.5 / d1)
            {
                return n1 * (x -= 2.25 / d1) * x + 0.9375;
            }
            else
            {
                return n1 * (x -= 2.625 / d1) * x + 0.984375;
            }
        }
        case Mode::BOTH:
        default:
            return x < 0.5
                       ? (1 - Bounce(Mode::OUT, 1 - 2 * x)) / 2
                       : (1 + Bounce(Mode::OUT, 2 * x - 1)) / 2;
        }
    }
};

static const char *EnumToString(Ease::Mode mode) { return Ease::ModeStrings[mode]; }
static const char *EnumToString(Ease::Type type) { return Ease::TypeStrings[type]; }

const char *Ease::TypeStrings[] = {
    "Linear", "Sine", "Expo", "Circ", "Cubic", "Quad", "Quart", "Quint", "Back", "Elastic", "Bounce"};
const char *Ease::TypeIdStrings[] = {
    "Line", "Sine", "Expo", "Circ", "Cube", "Quad", "Qurt", "Qunt", "Back", "Elas", "Bunc"};
const char *Ease::ModeStrings[] = {"In", "Out", "Both"};

#pragma GCC diagnostic pop