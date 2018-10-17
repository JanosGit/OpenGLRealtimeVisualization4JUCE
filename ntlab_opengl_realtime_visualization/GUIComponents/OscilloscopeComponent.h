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

#include "../RealtimeDataTransfer/VisualizationDataSource.h"
#include "../2DPlot/Plot2D.h"

namespace ntlab
{
    /**
     * The Component designed to visualize time-domain data collected by an OscilloscopeDataCollector instance.
     * It exports the parameters "gainLinear", "timeViewed" and "enableTriggering" to the VisualizationTarget
     * valueTree member as an alternative way to set these parameters by using the setter member functions and
     * save/restore its state. It inherits ntlab::Plot2D and therefore uses OpenGL for rendering.
     */
    class OscilloscopeComponent : public ntlab::VisualizationTarget, public ntlab::Plot2D, private juce::ValueTree::Listener
    {
    public:

        /** A double value specifying the gain applied to the signal for scaling it to the view */
        static const juce::Identifier parameterGainLinear;

        /** A double value specifying the timeframe viewed by the oscilloscope in seconds */
        static const juce::Identifier parameterTimeViewed;

        /** A boolean value specifying if the oscilloscope should be triggered to the rising edge of the first channel */
        static const juce::Identifier parameterEnableTriggering;

        /**
         * Specifiy an identifier extension to map the OscilloscopeComponent to the corresponding source.
         * The Identifier will automatically be prepended by "Oscilloscope". The optional undo manager can
         * be passed to enable undo functionality for the parameters held by the ValueTree.
         */
        OscilloscopeComponent (const juce::String identifierExtension, juce::UndoManager* undoManager = nullptr);

        ~OscilloscopeComponent();

        /**
         * Set the time frame viewed by the Oscilloscope. This impacts the number of samples collected before a GUI
         * update. Calling this is equal to updating the parameterTimeViewed property of the value tree.
         */
        void setTimeViewed (double timeInSeconds);

        /**
         * Returns the time frame viewed by the oscilloscope in seconds. Calling this is equal to reading the
         * parameterTimeViewed property of the value tree.
         */
        double getTimeViewed();

        /**
         * Sets the gain applied to the input signal to scale it up or down. Calling this is equal to updating the
         * parameterGainLinear property of the value tree.
         */
        void setGain (double gainLinear);

        /**
         * Returns the linear gain value applied tho the input signal for scaling. Calling this is equal to reading the
         * parameterGainLinear property of the value tree.
         */
        double getGain();

        /**
         * Enables or disables triggering. Calling this is equal to updating the parameterEnableTriggering property
         * of the value tree.
         */
        void enableTriggering (bool shouldBeEnabled = true);

        /**
         * Returns true if triggering is enabled. Calling this is equal to reading the parameterEnableTriggering
         * property of the value tree.
         */
        bool getTriggeringState();

        /**
         * This overlays a simple semi-transparent settings bar above the scope, allowing to adjust time viewed, gain
         * and triggering. You might however want to implement controls that suit your GUI design better. Use the
         * valueTree member or the getter and setter functions mentioned above to control those settings.
         */
        void displaySettingsBar (bool shouldBeDisplayed);

        void applySettingFromCollector (const juce::String& setting, juce::var& value) override;
        void resized() override;

    private:

        class SettingsComponent : public juce::Component, private juce::ValueTree::Listener
        {
        public:
            SettingsComponent (juce::ValueTree& valueTree, juce::UndoManager* um);
            ~SettingsComponent();
            void paint (juce::Graphics& g) override;
            void resized() override;

        private:
            juce::Slider timebaseSlider, gainSlider;
            juce::ToggleButton enableTriggeringButton;
            bool timebaseSliderChangedByGUI = false;
            bool gainSliderChangedByGUI = false;
            bool enableTriggeringButtonChangedByGUI = false;
            juce::ValueTree oscilloscopeValueTree;
            juce::UndoManager* undoManager;
            void valueTreePropertyChanged (juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property) override;
            void valueTreeChildAdded (juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenAdded) override;
            void valueTreeChildRemoved (juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
            void valueTreeChildOrderChanged (juce::ValueTree &parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex) override;
            void valueTreeParentChanged (juce::ValueTree &treeWhoseParentHasChanged) override;
        };

        // bitfield index values for all settings that have been set
        enum ValidSettings
        {
            numChannelsValid = 0,
            numSamplesValid = 1,
            channelNamesValid = 2,
            tSampleValid = 0,
            tViewedValid = 1
        };
        std::bitset<3> validChannelInformation;
        std::bitset<2> validTimebaseInformation;

        // channel information
        int numChannels = 0;
        int numSamples = 0;
        juce::StringArray channelNames;

        // timebase information
        float tSample;
        juce::Range<float> tRange;

        juce::MemoryBlock* lastBuffer = nullptr;

        std::unique_ptr<SettingsComponent> settingsComponent;

        // Plot2D Member functions
        void beginFrame() override;
        const float* getBufferForLine (int lineIdx) override;
        void endFrame() override;

        // ValueTree::Listener functions
        void valueTreePropertyChanged (juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property) override;
        void valueTreeChildAdded (juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenAdded) override;
        void valueTreeChildRemoved (juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
        void valueTreeChildOrderChanged (juce::ValueTree &parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex) override;
        void valueTreeParentChanged (juce::ValueTree &treeWhoseParentHasChanged) override;

        void updateChannelInformation();
        void updateTimebaseInformation();
    };
}

