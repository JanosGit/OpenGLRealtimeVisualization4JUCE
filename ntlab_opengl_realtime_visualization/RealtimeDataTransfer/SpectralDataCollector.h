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
#include <juce_dsp/juce_dsp.h>
#include "RealtimeDataSink.h"
#include "DataCollector.h"

namespace ntlab
{
    /**
     * An object that collects samples from a realtime stream, extracts spectral information from them and periodically
     * sends the spectral data to a corresponding VisualizationTarget. Normally this will be a SpectralAnalyzerComponent.
     * To send data to the VisualizationTarget this instance must be added to a RealtimeDataSink which then will have a
     * connection to a VisualizationDataSource instance which then feeds the OscilloscopeComponent. Take a look at the
     * example code that comes with the module for a more detailled explanation.
     *
     * Currently this implementation uses a hamming window for windowing the data in the time domain and averages over
     * three fft results before updating the display. These might become adjustable values in future.
     *
     * @see RealtimeDataSink, @see VisualisationDataSource, @see VisualizationTarge, @see SpectralAnalyzerComponent
     */
    class SpectralDataCollector : public DataCollector
    {
    public:
        static const juce::String settingNumChannels;
        static const juce::String settingChannelNames;
        static const juce::String settingStartFrequency;
        static const juce::String settingEndFrequency;
        static const juce::String settingFFTOrder;

        /**
         * Specifiy an identifier extension to map the DataCollector to the corresponding target.
         * The Identifier will automatically be prepended by "Oscilloscope"
         */
        SpectralDataCollector (const juce::String identifierExtension);

        virtual ~SpectralDataCollector() {};

        /**
         * Sets the number of channels displayed by the spectral analyzer. Keep in mind that the next call to
         * pushChannelSamples will expect a matching new number of channels so better don't call this while realtime
         * sample processing is running
         * @param numChannels    The new number of channels displayed by the oscilloscope
         * @param channelNames   An Array of size channelNames containing the names to be displayed for each channel
         */
        void setChannels (int numChannels, juce::StringArray &channelNames);

        /**
         * Sets the order of the underlying FFT used for spectral analysis. High FFT orders generate high frequency
         * resolution while introducing more latency. The default value is an order of 11 resulting in an FFT length of
         * 2048 samples
         */
        void setFFTOrder (int newFFTOrder);

        /**
         * Sets the sample rate used. The spectral analyzer won't display any data until the sample rate was set.
         * If the spectral analyzer displays RF data that was mixed down, setting the startFrequency value to a
         * value different from 0 will result in correct scaling of the frequency axis
         */
        void setSampleRate (double newSampleRate, double newStartFrequency = 0.0);

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

        std::unique_ptr<juce::dsp::FFT> fft;
        std::unique_ptr<juce::dsp::WindowingFunction<float>> windowingFunction;
        int fftOrder = 0;
        double sampleRate = 0.0;
        double startFrequency = 0.0;

        int numSamplesExpected = 0;
        static const int numFFTSToAverage = 3;
        int numFFTSCalculated = 0;

        // Channels
        int                 numChannels = 0;
        juce::StringArray   channelNames;
        juce::Array<size_t> channelOffset;

        // Memory
        juce::HeapBlock<std::complex<float>> sampleBuffer;
        juce::HeapBlock<std::complex<float>> spectralBuffer;
        juce::MemoryBlock* currentWriteBlock = nullptr;
        size_t             expectedNumBytesForMemoryBlock = 0;
        int                numSamplesInSampleBuffer = 0;
        int                numSamplesAllChannels = 0;

        std::recursive_mutex processingLock;

        void recalculateMemory();

        void processFFT();

        void updateGUIChannels();

        void updateGUIFFTOrder();

        void updateGUIFrequencySpan();
    };
}