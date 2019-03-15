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


#include "Plot2D.h"

namespace ntlab
{

    Plot2D::Plot2D (bool updateAtFramerate) : openGLContext (SharedOpenGLContext::getInstance()->openGLContext)
    {
        setup (updateAtFramerate);
    }

    Plot2D::Plot2D (bool updateAtFramerate, juce::Range<float> xValueRange, float xValueDelta, LogScaling xValueScaling) : openGLContext (SharedOpenGLContext::getInstance()->openGLContext)
    {
        setup (updateAtFramerate);
        setXValues (xValueRange, xValueDelta, xValueScaling);
    }

    Plot2D::~Plot2D()
    {
        SharedOpenGLContext::getInstance()->removeRenderingTarget (this);
    }

    void Plot2D::setLines (int numLines, juce::StringArray &legend, juce::Array<juce::Colour> lineColours)
    {
        // first delete all buffers
        auto deleteAllGLBuffers = [this] (juce::OpenGLContext &openGLContext)
        {
            for (GLuint l : lineGLBuffers)
                openGLContext.extensions.glDeleteBuffers (1, &l);

            lineGLBuffers.clear();
        };

        SharedOpenGLContext::getInstance()->executeOnGLThread (deleteAllGLBuffers);

        // if no updates at framerate are chosen, all GL Buffers should be filled with 0 for y. To achieve this,
        // the tempRenderDataBuffer is prepared on the render thread right before adding the new static draw buffers
        if (!updatesAtFramerate)
        {
            auto fillTempBufferYWithZeros = [this] (juce::OpenGLContext&)
            {
                for (auto &tb : tempRenderDataBuffer)
                    tb.y = 0;
            };
            SharedOpenGLContext::getInstance()->executeOnGLThread (fillTempBufferYWithZeros);
            //{
            //    std::lock_guard<std::mutex> scopedLock (executeInRenderCallbackLock);
            //    executeInRenderCallback.push_back (fillTempBufferYWithZeros);
            //}
        }

        GLvoid* data =       (updatesAtFramerate) ? NULL           : tempRenderDataBuffer.data();
        GLenum bufferUsage = (updatesAtFramerate) ? GL_STREAM_DRAW : GL_STATIC_DRAW;

        auto addGLBuffer = [this, data, bufferUsage] (juce::OpenGLContext &openGLContext)
        {
            GLuint glBufferLocation;
            openGLContext.extensions.glGenBuffers (1, &glBufferLocation);
            openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, glBufferLocation);
            openGLContext.extensions.glBufferData (GL_ARRAY_BUFFER,
                                                   static_cast<GLsizeiptr> (numDatapointsExpected * sizeof (juce::Point<float>)),
                                                   data,
                                                   bufferUsage);
            lineGLBuffers.add (glBufferLocation);
        };

        SharedOpenGLContext::getInstance()->executeOnGLThreadMultipleTimes (addGLBuffer, numLines);

        lineNames = legend;
        if (lineColours.size() == 0)
            this->lineColours = automaticLineColours (numLines);
        else
            this->lineColours = lineColours;
        this->numLines = numLines;
    }

    void Plot2D::setBackgroundColour (juce::Colour newBackgroundColour, bool changeGridColour)
    {
        backgroundColour = newBackgroundColour;
        if (changeGridColour)
            gridLineColour = backgroundColour.contrasting (0.5);
    }

    void Plot2D::setGridColour (juce::Colour newGridColour)
    {
        gridLineColour = newGridColour;
    }

    void Plot2D::setLineWidthIfPossibleForGPU (const double desiredLineWidth)
    {
        SharedOpenGLContext::getInstance()->executeOnGLThread ([this, desiredLineWidth] (juce::OpenGLContext &openGLContext)
        {
            auto lineWidthRange = getLineWidthRange();
            double usedLineWidth = lineWidthRange.clipValue (desiredLineWidth);
            if (!lineWidthRange.contains (desiredLineWidth))
                DBG ("Desired line width " << desiredLineWidth << " is not possible for GPU, applied width of " << usedLineWidth << " from possible range " << lineWidthRange.getStart() << " to " << lineWidthRange.getEnd());
            glLineWidth (static_cast<GLfloat> (usedLineWidth));
        });
    }

    const juce::Range<double> Plot2D::getLineWidthRange ()
    {
        return lineWidthRange;
    }

    int Plot2D::getNumDatapointsExpected ()
    {
        return numDatapointsExpected;
    }

    void Plot2D::setYValues (float *yValues, int lineIdx)
    {
        /**
         * Don't use this function when updates at framerate are enabled.
         * In this case override beginFrame, getBufferForLine, endFrame
         * to let the GL thread automatically fetch new data
         */
        jassert (updatesAtFramerate == false);

        GLuint lineBuffer = lineGLBuffers[lineIdx];

        std::vector<juce::Point<float>> tempRenderDataBufferCopy (tempRenderDataBuffer);

        for (auto &p : tempRenderDataBufferCopy) {
            p.y = *yValues;
            ++yValues;
        }

        auto updateLineBuffer = [tempRenderDataBufferCopy = std::move (tempRenderDataBufferCopy), lineBuffer] (juce::OpenGLContext& openGLContext)
        {
            openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, lineBuffer);
            openGLContext.extensions.glBufferSubData (GL_ARRAY_BUFFER,
                                                      0,
                                                      static_cast<GLsizeiptr> (tempRenderDataBufferCopy.size() * sizeof (juce::Point<float>)),
                                                      tempRenderDataBufferCopy.data());
        };

        SharedOpenGLContext::getInstance()->executeOnGLThread (updateLineBuffer);

        openGLContext.triggerRepaint();
    }

    void Plot2D::setYValues (const juce::Array<float> &yValues, int lineIdx)
    {
        /**
         * Don't use this function when updates at framerate are enabled.
         * In this case override beginFrame, getBufferForLine, endFrame
         * to let the GL thread automatically fetch new data
         */
        jassert (updatesAtFramerate == false);

        // Make sure you pass a buffer that matches the number of x values set
        jassert (numDatapointsExpected == yValues.size());

        std::vector<juce::Point<float>> tempRenderDataBufferCopy (tempRenderDataBuffer);

        for (int i = 0; i < numDatapointsExpected; ++i)
            tempRenderDataBufferCopy[i].y = yValues[i];

        auto updateLineBuffer = [this, tempRenderDataBufferCopy = std::move (tempRenderDataBufferCopy), lineIdx] (juce::OpenGLContext& openGLContext)
        {
            openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, lineGLBuffers[lineIdx]);
            openGLContext.extensions.glBufferSubData (GL_ARRAY_BUFFER,
                                                      0,
                                                      static_cast<GLsizeiptr> (tempRenderDataBufferCopy.size() * sizeof (juce::Point<float>)),
                                                      tempRenderDataBufferCopy.data());
        };

        SharedOpenGLContext::getInstance()->executeOnGLThread (updateLineBuffer);

        openGLContext.triggerRepaint();
    }

    int Plot2D::setXValues (juce::Range<float> xValueRange, float xValueDelta, LogScaling xValueScaling)
    {
        int newNumDatapointsExpected = std::floor (xValueRange.getLength() / xValueDelta);
        tempRenderDataBuffer.resize (newNumDatapointsExpected);

        if (newNumDatapointsExpected > numDatapointsExpected)
            resizeLineGLBuffers();

        numDatapointsExpected = newNumDatapointsExpected;

        switch (xValueScaling)
        {
            case LogScaling::none :
            {
                float x = 0;
                float xValueDeltaNormalized = 1 / static_cast<float> (numDatapointsExpected);
                for (auto &p : tempRenderDataBuffer) {
                    p.x = x;
                    x += xValueDeltaNormalized;
                }
            }
                break;
            case LogScaling::baseE :
            {
                float x = xValueRange.getStart() + 1.0;
                std::vector<float> linearXValues (numDatapointsExpected);
                for (auto &lxv : linearXValues)
                {
                    lxv = x;
                    x += xValueDelta;
                }

                float minLogValue = std::log (xValueRange.getStart() + 1.0f);
                float maxLogValue = std::log (xValueRange.getEnd()   + 1.0f);
                float logValueRange = maxLogValue - minLogValue;

                for (int i = 0; i < numDatapointsExpected; ++i)
                {
                    tempRenderDataBuffer[i].x = (std::log (linearXValues[i]) - minLogValue) / logValueRange;
                }
            }
                break;
            case LogScaling::base10 :
            {
                // unsupported
                jassertfalse;
            }
                break;
            case LogScaling::dbVoltage :
            {
                // unsupported
                jassertfalse;
            }
                break;
            case LogScaling::dBPower :
            {
                // unsupported
                jassertfalse;
            }
                break;
        }

        xLogScaling = xValueScaling;
        setXRange (xValueRange);

        return numDatapointsExpected;
    }

    void Plot2D::setYRange (juce::Range<float> newYValueRange, LogScaling logScaling)
    {
        // base e is unsupported for Y log scaling
        jassert (logScaling != LogScaling::baseE);

        yValueRange = newYValueRange;
        yLogScaling = logScaling;
        juce::MessageManager::callAsync ([this](){repaint(); });
    }

    void Plot2D::setXRange (juce::Range<float> newXValueRange)
    {
        xValueRange = newXValueRange;
        juce::MessageManager::callAsync ([this](){repaint(); });
    }

    void Plot2D::newOpenGLContextCreated ()
    {
        lineShader.reset (LineShader2D::create (openGLContext));
        openGLContext.extensions.glGenBuffers (1, &gridLineGLBuffer);
    }

    void Plot2D::setGridProperties (int newNumXGridLines, int newNumYGridLines, bool applyGridColourContrastingBackground)
    {
        if (applyGridColourContrastingBackground)
            setGridProperties (newNumXGridLines, newNumYGridLines, backgroundColour.contrasting (0.5));
        else
            setGridProperties (newNumXGridLines, newNumYGridLines, gridLineColour);
    }

    void Plot2D::setGridProperties (int newNumXGridLines, int newNumYGridLines, juce::Colour newGridLineColour)
    {
        jassert (newNumXGridLines >= 0);
        jassert (newNumYGridLines >= 0);

        gridLineColour = newGridLineColour;

        if ((newNumXGridLines == numXGridLines) && (newNumYGridLines == numYGridLines))
            return;

        std::vector<juce::Point<float>> lineBuffer(2 * (newNumXGridLines + newNumYGridLines));

        // create x lines
        int i = 0;
        float currentX = 0.0f;
        float xSpacing = 1.0f / newNumXGridLines;
        for (; i < newNumXGridLines; ++i)
        {
            lineBuffer[2 * i].x = currentX;
            lineBuffer[2 * i].y = 0.0f;
            lineBuffer[2 * i + 1].x = currentX;
            lineBuffer[2 * i + 1].y = 1.0f;
            currentX += xSpacing;
        }

        // create y lines
        float currentY = 0.0f;
        float ySpacing = 1.0f / newNumYGridLines;
        for(; i < newNumXGridLines + newNumYGridLines; ++i)
        {
            lineBuffer[2 * i].x = 0.0f;
            lineBuffer[2 * i].y = currentY;
            lineBuffer[2 * i + 1].x = 1.0f;
            lineBuffer[2 * i + 1].y = currentY;
            currentY += ySpacing;
        }


        auto fillGridLineGLBuffer = [this, lineBuffer = std::move (lineBuffer), newNumXGridLines, newNumYGridLines] (juce::OpenGLContext &openGLContext)
        {
            // update of the member variables should happen here to avoid draw calls with a wrong number of lines before
            // the buffer was updated
            numXGridLines = newNumXGridLines;
            numYGridLines = newNumYGridLines;

            openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, gridLineGLBuffer);
            openGLContext.extensions.glBufferData (GL_ARRAY_BUFFER,
                                                   static_cast<GLsizeiptr> (2 * (numXGridLines + numYGridLines) * sizeof (juce::Point<float>)),
                                                   lineBuffer.data(),
                                                   GL_STATIC_DRAW);

            shouldRenderGrid = true;
        };

        SharedOpenGLContext::getInstance()->executeOnGLThread (fillGridLineGLBuffer);
    }

    int Plot2D::getNumXGridLines ()
    {
        return numXGridLines;
    }

    int Plot2D::getNumYGridLines ()
    {
        return numYGridLines;
    }

    void Plot2D::openGLContextClosing () {
        // delete all buffers
        openGLContext.extensions.glDeleteBuffers (1, &gridLineGLBuffer);

        for (GLuint l : lineGLBuffers)
            openGLContext.extensions.glDeleteBuffers (1, &l);

        lineGLBuffers.clearQuick();
    }

    void Plot2D::getLineWidthRangePossibleForGPU ()
    {
        // query the device for supported line widths
        GLfloat glLineWidthRange[2];
        glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, glLineWidthRange);

        lineWidthRange.setStart (static_cast<double> (glLineWidthRange[0]));
        lineWidthRange.setEnd   (static_cast<double> (glLineWidthRange[1]));
    }

    void Plot2D::resizeLineGLBuffers ()
    {
        GLvoid* data =       (updatesAtFramerate) ? NULL           : tempRenderDataBuffer.data();
        GLenum bufferUsage = (updatesAtFramerate) ? GL_STREAM_DRAW : GL_STATIC_DRAW;

        auto resizeAllLineGLBuffers = [this, data, bufferUsage] (juce::OpenGLContext &openGLContext)
        {
            for (auto glBufferLocation : lineGLBuffers)
            {
                openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, glBufferLocation);
                openGLContext.extensions.glBufferData (GL_ARRAY_BUFFER,
                                                       static_cast<GLsizeiptr> (numDatapointsExpected * sizeof (juce::Point<float>)),
                                                       data,
                                                       bufferUsage);
            }
        };

        SharedOpenGLContext::getInstance()->executeOnGLThread (resizeAllLineGLBuffers);
        //executeInRenderCallback.push_back (resizeAllLineGLBuffers);
    }

    void Plot2D::setup (bool updateAtFramerate)
    {
        setOpaque (true);

        SharedOpenGLContext::getInstance()->addRenderingTarget (this);

        lineWidthRange = juce::Range<double>::emptyRange (1.0);

        SharedOpenGLContext::getInstance()->executeOnGLThread ([this] (juce::OpenGLContext&) { getLineWidthRangePossibleForGPU(); });

        updatesAtFramerate = updateAtFramerate;
        if (updateAtFramerate)
        {
            openGLContext.setContinuousRepainting (true);
        }

        xValueRange = { 0.0f, 1.0f};
        yValueRange = {-1.0f, 1.0f};
    }

    void Plot2D::renderOpenGL ()
    {
        // This is the region relative to the GL rendering parent component where our rendering should take place
        auto clip = SharedOpenGLContext::getInstance()->getComponentClippingBoundsRelativeToGLRenderingTarget (this);
        glViewport (clip.getX(), clip.getY(), clip.getWidth(), clip.getHeight());

        // enabling the scissor test leads to clearing just the part of the screen where drawing should take place
        juce::OpenGLHelpers::enableScissorTest (clip);
        juce::OpenGLHelpers::clear (backgroundColour);
        glDisable (GL_SCISSOR_TEST);

        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        lineShader->use();

        if (shouldRenderGrid)
        {
            //lineShader->setCoordinateSystemMatchingTo2DDrawing();
            lineShader->setCustomScalingAndTranslation (2.0f, -2.0f, -1.0f, 1.0f);

            openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, gridLineGLBuffer);
            lineShader->setLineColour (gridLineColour);
            lineShader->enableAttributes (openGLContext);
            glDrawArrays (GL_LINES, 0, static_cast<GLuint> (2 * (numXGridLines + numYGridLines)));
            lineShader->disableAttributes (openGLContext);
        }

        switch (yLogScaling)
        {
            case base10:
                lineShader->setCoordinateSystemFittingRange (juce::Range<float> (0, 1), yValueRange, true, LineShader2D::LogScaling::base10);
                break;
            case dBPower:
                lineShader->setCoordinateSystemFittingRange (juce::Range<float> (0, 1), yValueRange, true, LineShader2D::LogScaling::dBPower);
                break;
            case dbVoltage:
                lineShader->setCoordinateSystemFittingRange (juce::Range<float> (0, 1), yValueRange, true, LineShader2D::LogScaling::dbVoltage);
                break;
            default:
                lineShader->setCoordinateSystemFittingRange (juce::Range<float> (0, 1), yValueRange);
                break;
        }

        if (updatesAtFramerate)
        {
            beginFrame();

            for (int i = 0; i < numLines; ++i)
            {
                lineShader->setLineColour (lineColours[i]);
                float* y = const_cast<float*> (getBufferForLine (i));
                if (y != nullptr)
                {

                    for (auto &p : tempRenderDataBuffer) {
                        p.y = *y;
                        ++y;
                    }

                    openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, lineGLBuffers[i]);
                    openGLContext.extensions.glBufferSubData (GL_ARRAY_BUFFER,
                                                              0,
                                                              static_cast<GLsizeiptr> (numDatapointsExpected * sizeof (juce::Point<float>)),
                                                              tempRenderDataBuffer.data());

                    lineShader->enableAttributes (openGLContext);
                    // seems that the "count" value passes the number of primitives, not vertices??
                    glDrawArrays (GL_LINE_STRIP, 0, static_cast<GLuint> (numDatapointsExpected - 1));
                    lineShader->disableAttributes (openGLContext);

                }
            }

            endFrame();
        }
        else
        {
            for (int i = 0; i < numLines; ++i)
            {
                openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, lineGLBuffers[i]);
                lineShader->setLineColour (lineColours[i]);
                lineShader->enableAttributes (openGLContext);
                glDrawArrays (GL_LINE_STRIP, 0, static_cast<GLuint> (numDatapointsExpected - 1));
                lineShader->disableAttributes (openGLContext);
            }
        }
        
        // Reset the element buffers so child Components draw correctly
        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, 0);
        openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void Plot2D::enableLegend (bool shouldBeEnabled, LegendPosition legendPosition, bool withBorder, float backgroundTransparency)
    {
        if (!shouldBeEnabled)
        {
            legendState = -1;
            return;
        }

        legendState = legendPosition;
        drawLegendBorder = withBorder;
        legendBackgroundTransparency = backgroundTransparency;
    }

    void Plot2D::enableXAxisTicks (bool shouldBeEnabled, const juce::String unitPostfix, bool equalPrefixForEachTick)
    {
        drawXTicks = shouldBeEnabled;
        xTickPostfix = unitPostfix;
        equalPrefixForEachXTick = equalPrefixForEachTick;
    }

    void Plot2D::enableYAxisTicks (bool shouldBeEnabled, const juce::String unitPostfix, bool equalPrefixForEachTick)
    {
        drawYTicks = shouldBeEnabled;
        yTickPostfix = unitPostfix;
        equalPrefixForEachYTick = equalPrefixForEachTick;
    }

    void Plot2D::paint (juce::Graphics &g)
    {
        if (tempRenderDataBuffer.size() > 0)
        {
            g.setColour (gridLineColour);
            if (drawXTicks && (numXGridLines > 0))
            {

                int   xTickYPos = getHeight() - tickTextHeight;
                float xTickXPos = 1.0f;
                float xTickPosOffset = getWidth() / static_cast<float> (numXGridLines);

                juce::Rectangle<int> textArea (juce::roundToInt (xTickXPos), xTickYPos, juce::roundToInt (xTickPosOffset), tickTextHeight);
                auto prefix = ntlab::Float2String::getBestSIPrefixForValue (xValueRange.getEnd(), 2);

                switch (xLogScaling)
                {
                    case LogScaling::baseE :
                    {
                        float a = (xValueRange.getStart() + 1.0f);
                        float b = (xValueRange.getEnd() + 1.0f) / a;

                        for (int i = 0; i < numXGridLines; ++i)
                        {
                            textArea.setX (juce::roundToInt (xTickXPos));
                            float xFrac = static_cast<float> (i) / numXGridLines;

                            float nextXValue = a * std::pow (b, xFrac);
                            if (equalPrefixForEachXTick == false)
                                prefix = ntlab::Float2String::getBestSIPrefixForValue (nextXValue, 3);

                            g.drawText (ntlab::Float2String::withSIPrefix (nextXValue, 4, prefix) + xTickPostfix, textArea, juce::Justification::left, true);
                            xTickXPos += xTickPosOffset;

                        }
                    }
                        break;
                    case LogScaling::none :
                    {
                        for (int i = 0; i < numXGridLines; ++i)
                        {
                            textArea.setX (juce::roundToInt (xTickXPos));
                            float nextXValue = xValueRange.getEnd() * static_cast<float> (i) / numXGridLines;
                            if (equalPrefixForEachXTick)
                                g.drawText (ntlab::Float2String::withSIPrefix (nextXValue, 4, prefix) + xTickPostfix, textArea, juce::Justification::left, true);
                            else
                                g.drawText (ntlab::Float2String::withSIPrefix (nextXValue, 4) + xTickPostfix, textArea, juce::Justification::left, true);
                            xTickXPos += xTickPosOffset;
                        }

                    }
                        break;
                    default:
                        break; // just to fix compiler warnings - should never be reached
                }
            }

            if (drawYTicks && (numYGridLines > 0))
            {
                float yTickYPos = 1.0f;
                int   yTickXPos = 1;

                float yTick = yValueRange.getEnd();
                float yTickOffset = yValueRange.getLength() / numYGridLines;
                float yTickPosOffset = getHeight() / static_cast<float> (numYGridLines);

                juce::Rectangle<int> textArea (yTickXPos, juce::roundToInt (yTickYPos), getWidth(), tickTextHeight);
                ntlab::Float2String::SIPrefix prefix = ntlab::Float2String::getBestSIPrefixForValue (yValueRange.getEnd(), 3);

                for (int i = 0; i < numYGridLines; ++i)
                {
                    textArea.setY (juce::roundToInt (yTickYPos));
                    if (equalPrefixForEachYTick)
                        g.drawText (ntlab::Float2String::withSIPrefix (yTick, 3, prefix) + yTickPostfix, textArea, juce::Justification::left, true);
                    else
                        g.drawText (ntlab::Float2String::withSIPrefix (yTick, 3) + yTickPostfix, textArea, juce::Justification::left, true);
                    yTickYPos += yTickPosOffset;
                    yTick -= yTickOffset;
                }
            }
        }


        if (legendState == -1)
            return;

        const int legendBoxBorderMargin = 20;
        const int lineHeight = 15;
        int legendBoxWidth = 0;
        int legendBoxHeight = numLines * lineHeight + 10;

        for (auto& n : lineNames)
        {
            legendBoxWidth = std::max (g.getCurrentFont().getStringWidth (n), legendBoxWidth);
        }
        legendBoxWidth += 15;


        juce::Rectangle<int> localBounds = getLocalBounds();
        juce::Rectangle<int> legendBounds;

        switch (legendState)
        {
            case topLeft:
                localBounds.removeFromTop  (legendBoxBorderMargin);
                localBounds.removeFromLeft (legendBoxBorderMargin);
                legendBounds = localBounds.removeFromLeft (legendBoxWidth).removeFromTop (legendBoxHeight);
                break;

            case topRight:
                localBounds.removeFromTop   (legendBoxBorderMargin);
                localBounds.removeFromRight (legendBoxBorderMargin);
                legendBounds = localBounds.removeFromRight (legendBoxWidth).removeFromTop (legendBoxHeight);
                break;

            case bottomLeft:
                localBounds.removeFromBottom (legendBoxBorderMargin);
                localBounds.removeFromLeft   (legendBoxBorderMargin);
                legendBounds = localBounds.removeFromLeft (legendBoxWidth).removeFromBottom (legendBoxHeight);
                break;

            case bottomRight:
                localBounds.removeFromBottom (legendBoxBorderMargin);
                localBounds.removeFromRight  (legendBoxBorderMargin);
                legendBounds = localBounds.removeFromRight (legendBoxWidth).removeFromBottom (legendBoxHeight);
                break;

            default:
                return;
        }

        g.setColour (gridLineColour.withAlpha (1.0f - legendBackgroundTransparency));
        g.fillRect (legendBounds);

        if (drawLegendBorder)
        {
            g.setColour (gridLineColour);
            g.drawRect (legendBounds);
        }


        legendBounds.removeFromTop (5);
        legendBounds.removeFromLeft (5);
        g.setFont (lineHeight);

        for (int i = 0; i < numLines; ++i)
        {
            g.setColour (lineColours[i]);
            g.drawText (lineNames[i], legendBounds, juce::Justification::topLeft, false);
            legendBounds.removeFromTop (lineHeight);
        }
    }

    void Plot2D::resized()
    {

    }

    juce::Range<float> Plot2D::linearRangeToLogRange (juce::Range<float> linearRange, LogScaling scalingMode)
    {
        switch (scalingMode)
        {
            case base10:
                return juce::Range<float> (std::log10 (linearRange.getStart()), std::log10 (linearRange.getEnd()));
            case dBPower:
                return juce::Range<float> (std::log10 (linearRange.getStart()) * 10, std::log10 (linearRange.getEnd()) * 10);
            case dbVoltage:
                return juce::Range<float> (std::log10 (linearRange.getStart()) * 20, std::log10 (linearRange.getEnd()) * 20);
            default:
                return linearRange;
        }
    }
}