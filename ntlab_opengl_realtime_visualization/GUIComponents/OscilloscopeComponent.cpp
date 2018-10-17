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


#include "OscilloscopeComponent.h"
#include "../RealtimeDataTransfer/OscilloscopeDataCollector.h"

namespace ntlab
{
    const juce::Identifier OscilloscopeComponent::parameterGainLinear       ("gainLinear");
    const juce::Identifier OscilloscopeComponent::parameterTimeViewed       ("timeViewed");
    const juce::Identifier OscilloscopeComponent::parameterEnableTriggering ("enableTriggering");

    OscilloscopeComponent::OscilloscopeComponent (const juce::String identifierExtension, juce::UndoManager* undoManager)
    : VisualizationTarget ("Oscilloscope" + identifierExtension, undoManager),
      Plot2D (true)
    {
        valueTree.addListener (this);
        valueTree.setProperty (parameterGainLinear,       1.0,   undoManager);
        valueTree.setProperty (parameterTimeViewed,       0.01,  undoManager);
        valueTree.setProperty (parameterEnableTriggering, false, undoManager);

        setBackgroundColour (juce::Colours::darkturquoise, false);

        automaticLineColours = [] (int numChannels)
        {
            juce::Array<juce::Colour> onlyGreen;
            for (int i = 0; i < numChannels; ++i)
                onlyGreen.add (juce::Colours::azure);

            return onlyGreen;
        };

        setGridProperties (10, 8, juce::Colours::darkgrey);
        enableXAxisTicks (true, "sec");
        enableLegend (true, ntlab::Plot2D::bottomRight, false, 0.0f);
        setLineWidthIfPossibleForGPU (1.5);
    }

    OscilloscopeComponent::~OscilloscopeComponent ()
    {
        valueTree.removeListener (this);
    }

    void OscilloscopeComponent::setTimeViewed (double timeInSeconds)
    {
        valueTree.setProperty (parameterTimeViewed, timeInSeconds, undoManager);
    }

    double OscilloscopeComponent::getTimeViewed()
    {
        return valueTree.getProperty (parameterTimeViewed);
    }

    void OscilloscopeComponent::setGain (double gainLinear)
    {
        valueTree.setProperty (parameterGainLinear, gainLinear, undoManager);
    }

    double OscilloscopeComponent::getGain()
    {
        return valueTree.getProperty (parameterGainLinear);
    }

    void OscilloscopeComponent::enableTriggering (bool shouldBeEnabled)
    {
        valueTree.setProperty (parameterEnableTriggering, shouldBeEnabled, undoManager);
    }

    bool OscilloscopeComponent::getTriggeringState ()
    {
        return valueTree.getProperty (parameterEnableTriggering);
    }

    void OscilloscopeComponent::displaySettingsBar (bool shouldBeDisplayed)
    {
        if ((settingsComponent == nullptr) != shouldBeDisplayed)
            return;

        if (shouldBeDisplayed)
        {
            settingsComponent.reset (new SettingsComponent (valueTree, undoManager));
            addAndMakeVisible (settingsComponent.get());
        }

        else
            settingsComponent.reset (nullptr);
    }

    void OscilloscopeComponent::applySettingFromCollector (const juce::String &setting, juce::var &value)
    {
        if (setting == OscilloscopeDataCollector::settingIsTriggered)
        {
            if (value.isBool())
            {
                valueTree.setProperty (parameterEnableTriggering, value, undoManager);
            }

        }
        else if (setting == OscilloscopeDataCollector::settingChannelNames)
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
        else if (setting == OscilloscopeDataCollector::settingNumChannels)
        {
            if (value.isInt())
            {
                numChannels = value;
                validChannelInformation.set (numChannelsValid);
                updateChannelInformation();
            }

        }
        else if (setting == OscilloscopeDataCollector::settingNumSamples)
        {
            if (value.isInt())
            {
                numSamples = value;
                validChannelInformation.set (numSamplesValid);
                updateChannelInformation();
            }

        }
        else if (setting == OscilloscopeDataCollector::settingTSample)
        {
            if (value.isDouble())
            {
                double tSampleDouble = value;
                tSample = static_cast<float> (tSampleDouble);
                validTimebaseInformation.set (tSampleValid);
                updateTimebaseInformation();
            }
        }
        else if (setting == OscilloscopeDataCollector::settingTimeViewed)
        {
            if (value.isDouble())
            {
                double newTViewed = value;
                validTimebaseInformation.set (tViewedValid);

                tRange = juce::Range<float> (0.0f, static_cast<float> (newTViewed));
                updateTimebaseInformation();

                valueTree.setProperty (parameterTimeViewed, newTViewed, undoManager);
            }
        }
    }

    void OscilloscopeComponent::resized ()
    {
        if (settingsComponent.get() != nullptr)
            settingsComponent->setBounds (0, 0, getWidth (), 100);
    }

    void OscilloscopeComponent::beginFrame()
    {
        if (dataSource != nullptr)
        {
            lastBuffer = &dataSource->startReading (*this);

            // if the buffer supplied doesn't seem to match just give it back directly
            if (lastBuffer->getSize() != numSamples * numChannels * sizeof (float))
            {
                lastBuffer = nullptr;
                dataSource->finishedReading (*this);
            }
        }
    }

    const float* OscilloscopeComponent::getBufferForLine (int lineIdx)
    {
        if (lastBuffer != nullptr)
            return static_cast<float*> (lastBuffer->getData()) + (numSamples * lineIdx);

        return nullptr;
    }

    void OscilloscopeComponent::endFrame()
    {
        if (lastBuffer != nullptr)
            dataSource->finishedReading (*this);
    }

    void OscilloscopeComponent::valueTreePropertyChanged (juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property)
    {
        if (treeWhosePropertyHasChanged == valueTree)
        {
            auto propertyValue = valueTree.getProperty (property);

            if (property == parameterGainLinear)
            {
                float yLimits = 1.0f / (float)propertyValue;
                juce::Range<float> newYRange (-yLimits, yLimits);
                setYRange (newYRange);
            }
            else if (property == parameterTimeViewed)
            {
                if (dataSource != nullptr)
                    dataSource->applySettingToCollector (*this, OscilloscopeDataCollector::settingTimeViewed, propertyValue);
            }
            else if (property == parameterEnableTriggering)
            {
                if (dataSource != nullptr)
                    dataSource->applySettingToCollector (*this, OscilloscopeDataCollector::settingIsTriggered, propertyValue);
            }
        }
    }

    void OscilloscopeComponent::valueTreeChildAdded (juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenAdded) {}
    void OscilloscopeComponent::valueTreeChildRemoved (juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) {}
    void OscilloscopeComponent::valueTreeChildOrderChanged (juce::ValueTree &parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex) {}
    void OscilloscopeComponent::valueTreeParentChanged (juce::ValueTree &treeWhoseParentHasChanged) {}

    void OscilloscopeComponent::updateChannelInformation ()
    {
        if (validChannelInformation.all())
        {
            setLines (numChannels, channelNames);
        }

    }

    void OscilloscopeComponent::updateTimebaseInformation ()
    {
        if (validTimebaseInformation.all())
        {
            setXValues (tRange, static_cast<float> (tSample), LogScaling::none);
        }
    }

    OscilloscopeComponent::SettingsComponent::SettingsComponent (juce::ValueTree& valueTree, juce::UndoManager* um) : oscilloscopeValueTree (valueTree), undoManager (um)
    {
        addAndMakeVisible (timebaseSlider);
        addAndMakeVisible (gainSlider);
        addAndMakeVisible (enableTriggeringButton);

        timebaseSlider.setRange (0.001, 0.1);
        gainSlider.    setRange (0.0,  30.0);

        timebaseSlider.setSliderStyle (juce::Slider::SliderStyle::Rotary);
        gainSlider.    setSliderStyle (juce::Slider::SliderStyle::Rotary);

        timebaseSlider.setTextBoxStyle (juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 90, 15);
        gainSlider.    setTextBoxStyle (juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 90, 15);

        timebaseSlider.setNumDecimalPlacesToDisplay (3);
        gainSlider.    setNumDecimalPlacesToDisplay (3);

        timebaseSlider.setTextValueSuffix (" sec");
        gainSlider.    setTextValueSuffix (" dB");

        timebaseSlider.onValueChange = [this] ()
        {
            timebaseSliderChangedByGUI = true;
            oscilloscopeValueTree.setProperty (parameterTimeViewed, timebaseSlider.getValue(), undoManager);
            timebaseSliderChangedByGUI = false;
        };

        gainSlider.onValueChange = [this] ()
        {
            gainSliderChangedByGUI = true;
            double gainValueLinear = juce::Decibels::decibelsToGain (gainSlider.getValue());
            oscilloscopeValueTree.setProperty (parameterGainLinear, gainValueLinear, undoManager);
            gainSliderChangedByGUI = false;
        };

        enableTriggeringButton.onStateChange = [this] ()
        {
            enableTriggeringButtonChangedByGUI = true;
            oscilloscopeValueTree.setProperty (parameterEnableTriggering, enableTriggeringButton.getToggleState(), undoManager);
            enableTriggeringButtonChangedByGUI = false;
        };

        timebaseSlider.setValue (oscilloscopeValueTree.getProperty (parameterTimeViewed), juce::NotificationType::dontSendNotification);
        gainSlider.    setValue (oscilloscopeValueTree.getProperty (parameterGainLinear), juce::NotificationType::dontSendNotification);
        enableTriggeringButton.setToggleState (oscilloscopeValueTree.getProperty (parameterEnableTriggering), juce::NotificationType::dontSendNotification);
        oscilloscopeValueTree.addListener (this);
    }

    OscilloscopeComponent::SettingsComponent::~SettingsComponent ()
    {
        oscilloscopeValueTree.removeListener (this);
    }

    void OscilloscopeComponent::SettingsComponent::paint (juce::Graphics &g)
    {
        auto textBounds = getLocalBounds().removeFromTop (25);
        g.fillAll (juce::Colours::grey.withAlpha (0.5f));

        g.setColour (juce::Colours::white);
        g.drawText (TRANS ("Time"),    textBounds.removeFromLeft (100), juce::Justification::centred, false);
        g.drawText (TRANS ("Gain"),    textBounds.removeFromLeft (100), juce::Justification::centred, false);
        g.drawText (TRANS ("Trigger"), textBounds.removeFromRight (80), juce::Justification::centred, false);
    }

    void OscilloscopeComponent::SettingsComponent::resized()
    {
        auto controlElementsBounds = getLocalBounds();
        controlElementsBounds.removeFromTop (25);
        timebaseSlider.        setBounds (controlElementsBounds.removeFromLeft  (100));
        gainSlider.            setBounds (controlElementsBounds.removeFromLeft  (100));
        enableTriggeringButton.setBounds (controlElementsBounds.removeFromRight (50));
    }

    void OscilloscopeComponent::SettingsComponent::valueTreePropertyChanged (juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property)
    {
        if (treeWhosePropertyHasChanged == oscilloscopeValueTree)
        {
            if (property == parameterTimeViewed)
            {
                if (!timebaseSliderChangedByGUI)
                    juce::MessageManager::callAsync ([this]() {timebaseSlider.setValue (oscilloscopeValueTree.getProperty (parameterTimeViewed), juce::NotificationType::dontSendNotification);} );
            }
            else if (property == parameterGainLinear)
            {
                if (!gainSliderChangedByGUI)
                    juce::MessageManager::callAsync ([this]() {gainSlider.setValue (juce::Decibels::gainToDecibels ((double)oscilloscopeValueTree.getProperty (parameterGainLinear)), juce::NotificationType::dontSendNotification);} );
            }
            else if (property == parameterEnableTriggering)
            {
                if (!enableTriggeringButtonChangedByGUI)
                    juce::MessageManager::callAsync ([this]() {enableTriggeringButton.setToggleState (oscilloscopeValueTree.getProperty (parameterEnableTriggering), juce::NotificationType::dontSendNotification);} );
            }
        }
    }

    void OscilloscopeComponent::SettingsComponent::valueTreeChildAdded (juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenAdded) {}
    void OscilloscopeComponent::SettingsComponent::valueTreeChildRemoved (juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) {}
    void OscilloscopeComponent::SettingsComponent::valueTreeChildOrderChanged (juce::ValueTree &parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex) {}
    void OscilloscopeComponent::SettingsComponent::valueTreeParentChanged (juce::ValueTree &treeWhoseParentHasChanged) {}
}

