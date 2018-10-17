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

#include "Attributes.h"
#include "Uniforms.h"

namespace ntlab
{
    /**
     * A class to manage a shader dedicated to draw 2D lines. It contains the shader code for the Vertex and Fragment
     * shader, manages all attributes and uniforms and supplies helper functions to set the coordinate system
     * translation uniforms and the line colour.
     */
    class LineShader2D : public juce::OpenGLShaderProgram
    {
    public:

        /** A set of scaling values needed when using the shader-based log scale conversion */
        struct LogScaling {
            LogScaling() : value (1.0f) {};
            LogScaling (float scalingValue) : value (scalingValue) {};
            operator const float() const {return value; }

            static constexpr float base10    = 0.434294481903252f; // 1 / log (10)
            static constexpr float dBPower   = 4.342944819032518f; // 10 / log (10)
            static constexpr float dbVoltage = 8.685889638065035f; // 20 / log (10)

            const float value;
        };

    private:
        /** Holds all Attributes for drawing the 2D line with the Line2DShader */
        class Attributes : public ntlab::OpenGLAttributes {

        public:

            Attributes (juce::OpenGLContext& openGLContext, juce::OpenGLShaderProgram& shaderProgram)
            {
                coord2d.reset (createAttribute (openGLContext, shaderProgram, "aCoord2d"));
            }

            void enable (juce::OpenGLContext& openGLContext) override
            {
                if (coord2d.get() != nullptr)
                {
                    openGLContext.extensions.glVertexAttribPointer (coord2d->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof (juce::Point<float>), 0);
                    openGLContext.extensions.glEnableVertexAttribArray (coord2d->attributeID);
                }
            }

            void disable (juce::OpenGLContext& openGLContext) override
            {
                if (coord2d.get() != nullptr)
                    openGLContext.extensions.glDisableVertexAttribArray (coord2d->attributeID);
            }

            std::unique_ptr<juce::OpenGLShaderProgram::Attribute> coord2d;
        };

        /** Holds all Uniforms for drawing the 2D line with the Line2DShader */
        class Uniforms : public ntlab::OpenGLUniforms {

        public:

            Uniforms (juce::OpenGLContext& openGLContext, juce::OpenGLShaderProgram& shaderProgram)
            {
                scaleX.          reset (createUniform (openGLContext, shaderProgram, "uScaleX"));
                scaleY.          reset (createUniform (openGLContext, shaderProgram, "uScaleY"));
                offsetX.         reset (createUniform (openGLContext, shaderProgram, "uOffsetX"));
                offsetY.         reset (createUniform (openGLContext, shaderProgram, "uOffsetY"));
                lineColour.      reset (createUniform (openGLContext, shaderProgram, "uLineColour"));
                logScalingFactor.reset (createUniform (openGLContext, shaderProgram, "uLogScalingFactor"));
                enableLogScaling.reset (createUniform (openGLContext, shaderProgram, "uEnableLogScaling"));
            }

            std::unique_ptr<juce::OpenGLShaderProgram::Uniform> scaleX, scaleY, offsetX, offsetY, lineColour, logScalingFactor, enableLogScaling;
        };

    public:

        /**
         * Creates a new LineShader2D or returns a nullptr in case of any error. You need to take ownership of the
         * object returned.
         */
        static LineShader2D* create (juce::OpenGLContext &context)
        {
            std::unique_ptr<LineShader2D> newShader (new LineShader2D (context));

            if (   newShader->addVertexShader   (juce::OpenGLHelpers::translateVertexShaderToV3   (vertex))
                && newShader->addFragmentShader (juce::OpenGLHelpers::translateFragmentShaderToV3 (fragment))
                && newShader->link())
            {
                newShader->use();

                newShader->uniforms.reset   (new Uniforms   (context, *newShader));
                newShader->attributes.reset (new Attributes (context, *newShader));

                return newShader.release();
            }

            DBG (newShader->getLastError());
            // Something went wrong during shader compilation. Hopefully the debug string will help you finding out what
            jassertfalse;

            return nullptr;
        }

        /**
         * Sets scaling and offset for the coordinate system so that it matches the standard 2D System
         * with (0, 0) top left and (1, 1) bottom right
         */
        void setCoordinateSystemMatchingTo2DDrawing()
        {
            uniforms->scaleX->          set (2.0f);
            uniforms->scaleY->          set (-2.0f);
            uniforms->offsetX->         set (-1.0f);
            uniforms->offsetY->         set (1.0f);
            uniforms->enableLogScaling->set (false);
        }

        /**
         * Sets scaling and offset for the coordinate system so that (0, 0) is at center left, (0, 1) is top left
         * (0, -1) is bottom left and (1, 0) is center right. Perfect for applications like an oscilloscope.
         */
        void setCoordinateSystemYOriginCentered()
        {
            uniforms->scaleX->          set (2.0f);
            uniforms->scaleY->          set (1.0f);
            uniforms->offsetX->         set (-1.0f);
            uniforms->offsetY->         set (0.0f);
            uniforms->enableLogScaling->set (false);
        }

        /**
         * Sets scaling and offset for the coordinate system so that (0, 0) is at bottom left, (1, 1) is top right.
         * Perfect for applications like a spectral analyser
         */
        void setCoordinateSystemYOriginBottomLeft()
        {
            uniforms->scaleX->set           (2.0f);
            uniforms->scaleY->set           (2.0f);
            uniforms->offsetX->set          (-1.0f);
            uniforms->offsetY->set          (-1.0f);
            uniforms->enableLogScaling->set (false);
        }

        /**
         * Scales the coordinate system in such a way that (xRange.start, yRange.start) is at bottom left and
         * (xRange.end, yRange.end) is at top right.
         */
        void setCoordinateSystemFittingRange (juce::Range<float> xRange, juce::Range<float> yRange, bool enableLogScaling = false, const LogScaling logScalingValue = LogScaling::dBPower)
        {
            float scaleX = 2.0f / xRange.getLength();
            float scaleY = 2.0f / yRange.getLength();
            uniforms->scaleX->set (scaleX);
            uniforms->scaleY->set (scaleY);

            float offsetX = -xRange.getStart() * scaleX - 1;
            float offsetY = -yRange.getStart() * scaleY - 1;

            uniforms->offsetX->set (offsetX);
            uniforms->offsetY->set (offsetY);

            uniforms->enableLogScaling->set (enableLogScaling);

            if (enableLogScaling)
                uniforms->logScalingFactor->set (logScalingValue);
        }

        /** Allows a custom coordinate scaling/translation to be applied */
        void setCustomScalingAndTranslation (float scaleX, float scaleY, float offsetX, float offsetY)
        {
            uniforms->scaleX ->set (scaleX);
            uniforms->scaleY ->set (scaleY);
            uniforms->offsetX->set (offsetX);
            uniforms->offsetY->set (offsetY);

            uniforms->enableLogScaling->set (false);
        }

        /** Sets the next line drawn to a specific colour */
        void setLineColour (const juce::Colour& lineColour)
        {
            uniforms->lineColour->set (lineColour.getFloatRed(),
                                       lineColour.getFloatGreen(),
                                       lineColour.getFloatBlue(),
                                       lineColour.getFloatAlpha());
        }

        /** Needs to be called before every call to GLDrawArrays if the LineShader is used to draw them */
        void enableAttributes (juce::OpenGLContext &context)
        {
            attributes->enable (context);
        }

        /** Needs to be called after every call to GLDrawArrays*/
        void disableAttributes (juce::OpenGLContext &context)
        {
            attributes->disable (context);
        }

    private:

        std::unique_ptr<Uniforms>   uniforms;
        std::unique_ptr<Attributes> attributes;

        LineShader2D (juce::OpenGLContext &context) : juce::OpenGLShaderProgram (context) {};

        static const juce::String vertex;
        static const juce::String fragment;
    };
}