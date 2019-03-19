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


#include "SpectralAnalyzerComponent.h"
#include "../RealtimeDataTransfer/SpectralDataCollector.h"
#include "../Utilities/SerializableRange.h"

namespace ntlab
{
    const juce::Identifier SpectralAnalyzerComponent::parameterFFTOrder                ("fftOrder");
    const juce::Identifier SpectralAnalyzerComponent::parameterMagnitudeRange          ("magnitudeRange");
    const juce::Identifier SpectralAnalyzerComponent::parameterHideNegativeFrequencies ("hideNegativeFrequencies");
    const juce::Identifier SpectralAnalyzerComponent::parameterHideDC                  ("hideDC");
    const juce::Identifier SpectralAnalyzerComponent::parameterMagnitudeLinearDB       ("magnitudeLinearDB");
    const juce::Identifier SpectralAnalyzerComponent::parameterFrequencyLinearLog      ("frequencyLinearLog");

    SpectralAnalyzerComponent::SpectralAnalyzerComponent (const juce::String identifierExtension, juce::UndoManager *undoManager)
    : VisualizationTarget ("SpectralAnalyzer" + identifierExtension, undoManager),
      Plot2D (true)
    {
        // create value tree properties
        valueTree.addListener (this);
        valueTree.setProperty (parameterFFTOrder,                11,                              undoManager);
        valueTree.setProperty (parameterMagnitudeRange, SerializableRange<float> (-60.0f, 10.0f), undoManager);
        valueTree.setProperty (parameterHideNegativeFrequencies, true,                            undoManager);
        valueTree.setProperty (parameterMagnitudeLinearDB,       true,                            undoManager);
        valueTree.setProperty (parameterFrequencyLinearLog,      true,                            undoManager);

        setBackgroundColour (juce::Colours::darkturquoise, false);

        automaticLineColours = [] (int numChannels)
        {
            juce::Array<juce::Colour> onlyGreen;
            for (int i = 0; i < numChannels; ++i)
                onlyGreen.add (juce::Colours::azure);

            return onlyGreen;
        };

        setGridProperties (10, 7, juce::Colours::darkgrey);
        enableXAxisTicks (true, "Hz", false);
        enableLegend (true, ntlab::Plot2D::bottomRight, false, 0.0f);
        setLineWidthIfPossibleForGPU (1.5);
    }

    SpectralAnalyzerComponent::~SpectralAnalyzerComponent ()
    {
        valueTree.removeListener (this);
    }

    void SpectralAnalyzerComponent::setFFTOrder (int newOrder)
    {
        jassert (newOrder > 3);
        valueTree.setProperty (parameterFFTOrder, newOrder, undoManager);
    }

    void SpectralAnalyzerComponent::hideNegativeFrequencies (bool shouldHideNegativeFrequencies, bool shouldAlsoHideDC)
    {
        valueTree.setProperty (parameterHideNegativeFrequencies, shouldHideNegativeFrequencies, undoManager);
        if (shouldHideNegativeFrequencies)
            valueTree.setProperty (parameterHideDC, shouldAlsoHideDC, undoManager);
    }

    void SpectralAnalyzerComponent::setMagnitudeScaling (bool shouldBeLog)
    {
        valueTree.setProperty (parameterMagnitudeLinearDB, shouldBeLog, undoManager);
    }

    void SpectralAnalyzerComponent::setFrequencyAxisScaling (bool shouldBeLog)
    {
        valueTree.setProperty (parameterFrequencyLinearLog, shouldBeLog, undoManager);
    }

    void SpectralAnalyzerComponent::applySettingFromCollector (const juce::String &setting, juce::var &value)
    {
        if (setting == SpectralDataCollector::settingChannelNames)
        {
            if (value.isArray())
            {
                auto newChannelNames = value.getArray();
                channelNames.clearQuick();

                for (auto& channelName : *newChannelNames)
                    channelNames.add (channelName);

                validChannelInformation.set (channelNamesValid);
                updateChannelInformation();
            }
        }
        else if (setting == SpectralDataCollector::settingNumChannels)
        {
            if (value.isInt())
            {
                numChannels = value;
                validChannelInformation.set (numChannelsValid);
                updateChannelInformation();
            }

        }
        else if (setting == SpectralDataCollector::settingFFTOrder)
        {
            if (value.isInt())
            {
                valueTree.setProperty (parameterFFTOrder, value, undoManager);
            }

        }
        else if (setting == SpectralDataCollector::settingStartFrequency)
        {
            if (value.isDouble())
            {
                frequencyRange.setStart (value);
                updateFrequencyRangeInformation();
            }
        }
        else if (setting == SpectralDataCollector::settingEndFrequency)
        {
            if (value.isDouble())
            {
                frequencyRange.setEnd (value);
                updateFrequencyRangeInformation();
            }
        }
    }

    void SpectralAnalyzerComponent::resized () {}

    void SpectralAnalyzerComponent::beginFrame ()
    {
        if (dataSource != nullptr)
        {
            lastBuffer = &dataSource->startReading (*this);

            // if the buffer supplied doesn't seem to match just give it back directly
            if (lastBuffer->getSize() != numFFTBins * numChannels * sizeof (float))
            {
                lastBuffer = nullptr;
                dataSource->finishedReading (*this);
            }
        }
    }

    const float* SpectralAnalyzerComponent::getBufferForLine (int lineIdx)
    {
        if (lastBuffer != nullptr)
            return static_cast<float*> (lastBuffer->getData()) + (numFFTBins * lineIdx);

        return nullptr;
    }

    void SpectralAnalyzerComponent::endFrame ()
    {
        if (lastBuffer != nullptr)
            dataSource->finishedReading (*this);
    }

    void SpectralAnalyzerComponent::valueTreePropertyChanged (juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property)
    {
        if (treeWhosePropertyHasChanged == valueTree)
        {
            if (property == parameterFFTOrder)
            {
                int fftOrder = valueTree.getProperty (property);
                numFFTBins = 1 << fftOrder;

                validChannelInformation.set (numFFTBinsValid);
                updateChannelInformation();
            }
            else if ((property == parameterMagnitudeLinearDB) || (property == parameterMagnitudeRange))
            {
                bool magnitudeShouldBeLog = valueTree.getProperty (parameterMagnitudeLinearDB);
                SerializableRange<float> magnitudeRange (valueTree.getProperty (parameterMagnitudeRange));

                if (magnitudeShouldBeLog)
                {
                    setYRange (magnitudeRange, Plot2D::LogScaling::dBPower);
                    enableYAxisTicks (true, "dB", true);
                }
                else
                {
                    setYRange (magnitudeRange, Plot2D::LogScaling::none);
                    enableYAxisTicks (true, "", true);
                }
            }
            else if ((property == parameterFrequencyLinearLog) || (property == parameterHideNegativeFrequencies) || (property == parameterHideDC))
            {
                updateFrequencyRangeInformation();
            }
        }
    }

    void SpectralAnalyzerComponent::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) {}
    void SpectralAnalyzerComponent::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) {}
    void SpectralAnalyzerComponent::valueTreeChildOrderChanged (juce::ValueTree&, int, int) {}
    void SpectralAnalyzerComponent::valueTreeParentChanged (juce::ValueTree&) {}

    void SpectralAnalyzerComponent::updateChannelInformation ()
    {
        if (validChannelInformation.all())
            setLines (numChannels, channelNames);
    }

    void SpectralAnalyzerComponent::updateFrequencyRangeInformation ()
    {
        if (!frequencyRange.isEmpty())
        {
            float frequencySpacing = frequencyRange.getLength() / numFFTBins;

            auto frequencyRangeToUse = frequencyRange;

            if (valueTree.getProperty (parameterHideNegativeFrequencies))
            {
                if (valueTree.getProperty (parameterHideDC))
                    frequencyRangeToUse = juce::Range<float> (frequencySpacing, frequencyRange.getEnd() / 2);
                else
                    frequencyRangeToUse = frequencyRange.withEnd (frequencyRange.getEnd() / 2);
            }

            LogScaling scalingToUse = none;

            if (valueTree.getProperty (parameterFrequencyLinearLog))
                scalingToUse = baseE;

            setXValues (frequencyRangeToUse, frequencySpacing, scalingToUse);
        }
    }
}