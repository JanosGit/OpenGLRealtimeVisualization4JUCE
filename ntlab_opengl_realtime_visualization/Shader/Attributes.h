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


#pragma once

#include <juce_core/juce_core.h>
#include <juce_opengl/juce_opengl.h>

namespace ntlab
{
    /** A base class to manage Open GL shader attributes */
    class OpenGLAttributes
    {
    public:
        virtual ~OpenGLAttributes () {};

        virtual void enable  (juce::OpenGLContext& openGLContext) = 0;
        virtual void disable (juce::OpenGLContext& openGLContext) = 0;

    protected:

        /**
         * Returns a pointer to a juce::OpenGLShaderProgram::Attribute that refers to the attribute with the given
         * attributeName in a given shader. In case of success you need to take over lifetime management of the object
         * returned. In case the attribute with the name cannot be found in the shader code a nullptr will be returned.
         * Note that this can be the case if the attribute is actually present in the shader but its usage can be
         * completely optimized away by the GL compiler.
         */
        static juce::OpenGLShaderProgram::Attribute *createAttribute (juce::OpenGLContext &openGLContext,
                                                                      juce::OpenGLShaderProgram &shader,
                                                                      const juce::String &attributeName) {

            if (openGLContext.extensions.glGetAttribLocation (shader.getProgramID(), attributeName.toRawUTF8()) < 0)
                return nullptr;

            return new juce::OpenGLShaderProgram::Attribute (shader, attributeName.toRawUTF8());
        }
    };
}