/*
MIT License

Copyright (c) 2018 Janos Buttgereit

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include "LineShader.h"

namespace ntlab
{
    const juce::String LineShader2D::vertex =
#if JUCE_IOS || JUCE_ANDROID
            "precision mediump float;\n"
#endif
              "attribute vec2 aCoord2d;\n"
              "uniform float uScaleX;\n"
              "uniform float uScaleY;\n"
              "uniform float uOffsetX;\n"
              "uniform float uOffsetY;\n"
              "uniform float uLogScalingFactor;\n"
              "uniform bool uEnableLogScaling;\n"
              "\n"
              "void main (void) {\n"
              "  if (uEnableLogScaling) {\n"
              "    float yLogScaled = log (aCoord2d.y) * uLogScalingFactor;"
              "    gl_Position = vec4 ((aCoord2d.x * uScaleX) + uOffsetX, (yLogScaled * uScaleY) + uOffsetY, 0, 1);\n"
              "  }\n"
              "  else { \n"
              "     gl_Position = vec4 ((aCoord2d.x * uScaleX) + uOffsetX, (aCoord2d.y * uScaleY) + uOffsetY, 0, 1);\n"
              "  }\n"
              "}";

    const juce::String LineShader2D::fragment =
#if JUCE_IOS || JUCE_ANDROID
            "precision mediump float;\n"
#endif
              "uniform vec4 uLineColour;\n"
              "\n"
              "void main (void) {\n"
              "  gl_FragColor = uLineColour;\n"
              "}";
}