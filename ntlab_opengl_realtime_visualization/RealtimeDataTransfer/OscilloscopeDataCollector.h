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
#include <juce_audio_basics/juce_audio_basics.h>
#include "RealtimeDataSink.h"
#include "DataCollector.h"

namespace ntlab
{
    /**
     * An object that collects samples from a realtime stream and periodically sends them to a corresponding
     * VisualizationTarget. Normally this will be an OscilloscopeComponent. To send data to the VisualizationTarget
     * this instance must be added to a RealtimeDataSink which then will have a connection to a VisualizationDataSource
     * instance which then feeds the OscilloscopeComponent. Take a look at the example code that comes with the module
     * for a more detailed explanation.
     *
     * Note that some parameters can be set from both the collector and the OscilloscopeComponent, such as the time
     * viewed and the triggering depending on your use-case. Setting these parameters from either side will result in
     * the same behaviour.
     *
     * @see RealtimeDataSink, @see VisualisationDataSource, @see VisualizationTarget, @see OscilloscopeComponent
     */
    class OscilloscopeDataCollector : public DataCollector
    {
    public:
        static const juce::String settingTimeViewed;
        static const juce::String settingIsTriggered;
        static const juce::String settingTSample;
        static const juce::String settingNumSamples;
        static const juce::String settingNumChannels;
        static const juce::String settingChannelNames;

        /**
         * Specify an identifier extension to map the DataCollector to the corresponding target.
         * The Identifier will automatically be prepended by "Oscilloscope"
         */
        OscilloscopeDataCollector (const juce::String identifierExtension = "1") : DataCollector ("Oscilloscope" + identifierExtension) {};

        virtual ~OscilloscopeDataCollector () {};

        /**
         * Sets the number of channels displayed by the oscilloscope. Keep in mind that the next call to
         * pushChannelSamples will expect a matching new number of channels so better don't call this while realtime
         * sample processing is running.
         * @param numChannels    The new number of channels displayed by the oscilloscope
         * @param channelNames   An Array of size channelNames containing the names to be displayed for each channel
         */
        void setChannels (int numChannels, juce::StringArray channelNames = juce::StringArray());

        /**
         * Set the timeframe viewed by the Oscilloscope. This impacts the number of samples collected before a GUI update.
         */
        void setTimeViewed (double timeViewedInSeconds);

        /** Sets the sample rate used. The oscilloscope won't display any data until the sample rate was set. */
        void setSampleRate (double newSampleRate);

        /**
         * Enables or disables the triggering. If it is enabled, it will wait for a rising edge on channelToUse
         * before capturing the next frame which will result in a more stable display for most kinds of signals
         */
        void enableTriggering (bool isTriggered, int channelToUse = 0);

        /**
         * Pushes an audio buffer to the sample queue holding as much channels as should
         * be displayed. If an unmatching channel count will be passed, the internal buffer
         * will be filled with zeros.
         */
        void pushChannelsSamples (juce::AudioBuffer<float> &bufferToPush);

        /**
         * Updates all parameters relevant for the Visualization. Call this after (re-)connecting if your collector
         * to sink connection is network based to keep both ends in sync.
         */
        void updateAllGUIParameters();

        void applySettingFromTarget (const juce::String& setting, const juce::var& value) override;

    private:

        // Channels
        int                 numChannels = 0;
        juce::StringArray   channelNames;
        juce::Array<size_t> channelOffset;

        // Memory
        juce::MemoryBlock* currentWriteBlock = nullptr;
        size_t             expectedNumBytesForMemoryBlock = 0;
        int                numSamplesInCurrentBlock = 0;

        // Time
        double tSample = 1.0;
        double tView = 0.01;
        int    numSamplesExpected = 0;

        // Triggering
        bool triggeringEnabled = false;
        bool foundTriggerInCurrentBlock = false;
        int  triggerChannel = 0;

        void fillUnmatchingBlockWithZeros (size_t blockSizeInBytes);

        void prepareForNextSampleBlock();

        void recalculateNumSamples();

        void recalculateMemory();

        void updateGUITimebase();

        void updateGUIChannels();

        void updateGUITriggering();
    };
}
