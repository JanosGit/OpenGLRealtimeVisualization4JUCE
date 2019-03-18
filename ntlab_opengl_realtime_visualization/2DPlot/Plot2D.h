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
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include "../Shader/LineShader.h"
#include "../Utilities/Float2String.h"
#include "../Utilities/SharedOpenGLContext.h"

namespace ntlab
{
    /**
     * A general purpose base-class to display two dimensional line plots generated from static data or from realtime
     * data. All data lines need to share a common x value vector with equally spaced values. To update the displayed
     * lines from realtime data set the updateAtFramerate flag to true in the constructor and inherit from this class to
     * override the member functions beginFrame, getBufferForLine and endFrame. In all other cases use setYValues to
     * update the plotted lines.
     *
     * This component uses OpenGL for rendering.
     */
    class Plot2D : public juce::Component, public juce::OpenGLRenderer
    {

    public:

        enum LegendPosition
        {
            topLeft,
            topRight,
            bottomLeft,
            bottomRight
        };

        enum LogScaling
        {
            none,
            dBPower,
            dbVoltage,
            base10,
            baseE
        };

        /**
         * Creates a plot2D instance with 0 data lines and an empty xValue range.
         */
        Plot2D (bool updateAtFramerate);

        /**
         * Creata a Plot2D instance with 0 data lines and the given xValue range. To get the number of x values created
         * call getNumDatapointExpected.
         * @param updateAtFramerate set to true for realtime visualization
         * @param xValueRange       the minimum and maximum x value
         * @param xValueDelta       the spacing between the x values
         * @param xValueScaling     set to none for a linear x scale or to baseE for a logarithmic scaled x axis
         */
        Plot2D (bool updateAtFramerate, juce::Range<float> xValueRange, float xValueDelta, LogScaling xValueScaling = LogScaling::none);

        ~Plot2D();

        /**
         * Sets the number of lines displayed by the plot.
         * @param numLines    The new number of lines displayed by the oscilloscope
         * @param legend      An Array of size numLines containing the legend entries to be displayed for each line
         * @param lineColours An Array of colours used to draw the lines. If it is an empty array, the
         *                       automaticColours lambda will be used
         */
        void setLines (int numLines, juce::StringArray &legend, juce::Array<juce::Colour> lineColours = {});

        /**
         * Sets the background colour of the plot. If changeGridColour is set to true, a grid colour contrasting
         * the background colour is chosen automatically
         */
        void setBackgroundColour (juce::Colour newBackgroundColour, bool changeGridColour = false);

        /** Sets the grid colour of the plot */
        void setGridColour (juce::Colour newGridColour);

        /**
         * Set a line width for the line to be drawn. Will be clipped into the valid range if
         * out of the possible range and print a DBG statement noticing you about this issue.
         * @see getLineWidthRange
         */
        void setLineWidthIfPossibleForGPU (const double desiredLineWidth);

        /**
         * Returns the range of line widths that can be applied by setLineWidthIfPossibleForGPU. If the Component
         * has not been made visible it won't know which GPU to use, so it won't know which line sizes are supported
         * by the GPU. In this case an empty Range will be returned. It will be updated as soon as the Component has
         * become visible, so better wait until then to call this function or call Range::isEmpty to make sure that
         * you got a meaningful result.
         */
        const juce::Range<double> getLineWidthRange();

        /**
         * If updateAtFramerate mode is active, this will be called for every render frame to allow you to prepare
         * your data to be read by getBufferForLine. Make sure to have as many buffers as there are lines, each of
         * them holding number of expected y-values matching the x-value buffer passed before. After all lines
         * have been loaded a call to endFrame will signal that it's now safe to release your resources.
         */
        virtual void beginFrame () {};

        /**
         * Gets called for every line once per frame. It's expected to return a pointer to a buffer containing
         * the number of values passed to beginFrame. If a nullptr is returned, the line won't be drawn.
         */
        virtual const float* getBufferForLine (int lineIdx) {return nullptr; };

        /**
         * This indicates that the buffers prepared for the data to be displayed in the current frame might now be
         * released or modified
         * */
        virtual void endFrame() {};

        /**
         * This will set the x value base for all data lines to be plotted, both for updates at frame rate or y values
         * passed by setYValues. It will create linear spaced x values with a given start and end and delta. It returns
         * the size of the x value vector created. This is the expected number of corresponding y values for each line.
         * If the length of the xValueRange is no integer multiple of xValueDelta the x values created will start at
         * the minimum xValueRange value but not contain the maximum value as the maximum value will be rounded down
         * to fit.
         */
        int setXValues (juce::Range<float> xValueRange, float xValueDelta, LogScaling xValueScaling);

        /** Returns the number of y-values expected for the current x values. */
        int getNumDatapointsExpected();

        /**
         * If updateAtFramerate mode is inactive, this will set the y values for a particular line. Note that the line
         * has to be added by addLine or setLines before trying to set data for a line. The Array passed is expected
         * to hold as many y values as getNumDatapointsExpected returns.
         */
        void setYValues (const juce::Array<float> &yValues, int lineIdx);

        /**
         * If updateAtFramerate mode is inactive, this will set the y values for a particular line. Note that the line
         * has to be added by addLine or setLines before trying to set data for a line. The pointer passed must point
         * to a memory region holding as many y values as getNumDatapointsExpected returns.
         */
        void setYValues (float* yValues, int lineIdx);

        /**
         * Changes the range of y values displayed independent of the range of values passed to setYValues. An optional
         * logarithmic scaling can be applied to the data if it suits the use case. Note that this computation is
         * performed on the GPU, making it especially less heavy for the CPU when rendering realtime data. If
         * logarithmic scaling is choosen, the range supplied will set the log-value range displayed.
         * @see linearRangeToLogRange
         */
        void setYRange (juce::Range<float> newYValueRange, LogScaling logScaling = LogScaling::none);

        /** Changes the range of x values displayed independent of the range of values passed to setXValues. */
        void setXRange (juce::Range<float> newXValueRange);

        /**
         * Sets the number of vertical and horizontal grid lines. If applyGridColourContrastingBackground is true, it
         * will chose a colour that has a good contrast to to the background colour or leave the colour unchanged if set
         * to false.
         */
        void setGridProperties (int newNumXGridLines, int newNumYGridLines, bool applyGridColourContrastingBackground = true);

        /** Sets the number and colour of vertical and horizontal grid lines */
        void setGridProperties (int newNumXGridLines, int newNumYGridLines, juce::Colour newGridLineColour);

        /** Returns the current number of X grid lines displayed */
        int getNumXGridLines();

        /** Returns the current number of Y grid lines displayed */
        int getNumYGridLines();

        /** Enables or disables the legend and optionally sets some legend parameters */
        void enableLegend (bool shouldBeEnabled, LegendPosition legendPosition = topLeft, bool withBorder = true, float backgroundTransparency = 0.5f);

        /**
         * Enables value ticks at each grid line on the x axis. If a postfix string is supplied it will be appended to
         * each tick. If equalPrefixForEachTick is true, the prefix will be chosen according to the maximum tick value,
         * otherwise the best fitting prefix is chosen for each value independently.
         */
        void enableXAxisTicks (bool shouldBeEnabled, const juce::String unitPostfix = "", bool equalPrefixForEachTick = true);

        /**
         * Enables value ticks at each grid line on the y axis. If a postfix string is supplied it will be appended to
         * each tick. If equalPrefixForEachTick is true, the prefix will be chosen according to the maximum tick value,
         * otherwise the best fitting prefix is chosen for each value independently.
         */
        void enableYAxisTicks (bool shouldBeEnabled, const juce::String unitPostfix = "", bool equalPrefixForEachTick = true);

        void paint (juce::Graphics &g) override;
        void resized() override;

        /**
         * A lambda that's invoked to supply the colours of each data line if no explicit line colours are handed to
         * setLines.
         */
        std::function<juce::Array<juce::Colour>(int)> automaticLineColours = [] (int numChannels)
        {
            juce::Array<juce::Colour> onlyBlack;
            for (int i = 0; i < numChannels; ++i)
                onlyBlack.add (juce::Colours::black);

            return onlyBlack;
        };

        /** Helper function: Returns the corresponding range of log scaled values to a given linear scaled range */
        static juce::Range<float> linearRangeToLogRange (juce::Range<float> linearRange, LogScaling scalingMode);

        // OpenGLRenderer related member functions
        void newOpenGLContextCreated() override;
        void renderOpenGL() override;
        void openGLContextClosing() override;

    private:

        // OpenGL related member variables
        juce::OpenGLContext&          openGLContext;
        std::unique_ptr<LineShader2D> lineShader;
        GLuint                        gridLineGLBuffer;
        bool                          shouldRenderGrid = false;

        // Arrays containing information for each line
        juce::Array<GLuint>          lineGLBuffers; // the buffer location to use on the GPU side
        juce::StringArray            lineNames;
        juce::Array<juce::Colour>    lineColours;
        int numDatapointsExpected = 0;
        int numLines = 0;

        // Needed for non-continous drawing
        bool updatesAtFramerate;

        // A preallocated temporary buffer that holds the plot data prepared for copying them to the GPU memory
        std::vector<juce::Point<float>> tempRenderDataBuffer;

        // Some variables needed to compute the visual aperance
        juce::Range<double> lineWidthRange;
        juce::Range<float> xValueRange, yValueRange;
        LogScaling xLogScaling = LogScaling::none;
        LogScaling yLogScaling = LogScaling::none;
        juce::Colour backgroundColour = juce::Colours::white;
        juce::Colour gridLineColour = juce::Colours::darkgrey;
        int numXGridLines = 0;
        int numYGridLines = 0;
        bool drawXTicks = false;
        bool drawYTicks = false;
        bool equalPrefixForEachXTick = true;
        bool equalPrefixForEachYTick = true;
        juce::String xTickPostfix, yTickPostfix;
        static const int tickTextHeight = 20;
        int legendState = -1;
        bool drawLegendBorder = true;
        float legendBackgroundTransparency = 0.5f;

        // Some member functions needed to compute the visual aperance
        void getLineWidthRangePossibleForGPU();
        void resizeLineGLBuffers();

        // Called once in each constructor
        void setup (bool updateAtFramerate);
    };


}