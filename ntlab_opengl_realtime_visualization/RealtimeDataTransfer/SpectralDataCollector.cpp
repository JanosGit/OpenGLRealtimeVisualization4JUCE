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


#include "SpectralDataCollector.h"

namespace ntlab
{

    SpectralDataCollector::SpectralDataCollector (const juce::String identifierExtension) : DataCollector ("SpectralAnalyzer" + identifierExtension) {}

    void SpectralDataCollector::setChannels (int numChannels, juce::StringArray &channelNames)
    {
        this->numChannels = numChannels;
        this->channelNames = channelNames;

        updateGUIChannels();
        recalculateMemory();
    }

    void SpectralDataCollector::setFFTOrder (int newFFTOrder)
    {
        std::lock_guard<std::recursive_mutex> scopedLock (processingLock);
        fftOrder = newFFTOrder;
        numSamplesExpected = 1 << fftOrder;
        fft.reset (new juce::dsp::FFT (fftOrder));
        windowingFunction.reset (new juce::dsp::WindowingFunction<float> (static_cast<size_t> (numSamplesExpected), juce::dsp::WindowingFunction<float>::hamming));
        recalculateMemory();

        updateGUIFFTOrder();
    }

    void SpectralDataCollector::setSampleRate (double newSampleRate, double newStartFrequency)
    {
        // as setSampleRate is always called to initialize processing, allocating memory here for an unspecified fft
        // order should be fine
        if (fftOrder == 0)
            setFFTOrder (11);

        sampleRate = newSampleRate;
        startFrequency = newStartFrequency;

        updateGUIFrequencySpan();
    }

    void SpectralDataCollector::pushChannelsSamples (juce::AudioBuffer<float> &bufferToPush)
    {
        if (bufferToPush.getNumChannels() != numChannels)
            return;

        if (processingLock.try_lock ())
        {
            int numSamplesInPassedBuffer = bufferToPush.getNumSamples();
            int numSamplesToCopy = std::min (numSamplesInPassedBuffer, (numSamplesExpected - numSamplesInSampleBuffer));

            for (int n = 0; n < numChannels; ++n)
            {
                auto writePtr = sampleBuffer.get() + channelOffset[n] + numSamplesInSampleBuffer;
                auto readPtr = bufferToPush.getReadPointer (n);

                // todo: speedup through simd usage?
                for (int s = 0; s < numSamplesToCopy; ++s)
                    writePtr[s] = std::complex<float> (readPtr[s], 0.0f);
            }
            numSamplesInSampleBuffer += numSamplesToCopy;

            if (numSamplesInSampleBuffer >= numSamplesExpected)
                processFFT();

            processingLock.unlock();
        }
    }

    void SpectralDataCollector::updateAllGUIParameters ()
    {
        updateGUIChannels();
        updateGUIFrequencySpan();
        updateGUIFFTOrder();
    }

    void SpectralDataCollector::applySettingFromTarget (const juce::String &setting, const juce::var &value)
    {
        if (setting == settingFFTOrder)
            if (value.isInt())
                setFFTOrder (value);
    }

    void SpectralDataCollector::recalculateMemory ()
    {
        std::lock_guard<std::recursive_mutex> scopedLock (processingLock);
        numSamplesAllChannels = numChannels * numSamplesExpected;
        expectedNumBytesForMemoryBlock = numSamplesAllChannels * sizeof (float);
        resizeMemoryBlock (expectedNumBytesForMemoryBlock);

        sampleBuffer.  allocate (numSamplesAllChannels, true);
        spectralBuffer.allocate (numSamplesAllChannels, true);

        channelOffset.resize (numChannels);
        for (int i = 0; i < numChannels; ++i)
        {
            channelOffset.set (i, i * numSamplesExpected);
        }
    }

    void SpectralDataCollector::processFFT ()
    {
        if (processingLock.try_lock())
        {
            for (int c = 0; c < numChannels; ++c)
                fft->perform (sampleBuffer.get() + channelOffset[c], spectralBuffer.get() + channelOffset[c], false);

            if (currentWriteBlock == nullptr)
            {
                currentWriteBlock = startWriting();

                if (currentWriteBlock != nullptr)
                {
                    juce::FloatVectorOperations::clear (static_cast<float*> (currentWriteBlock->getData()), static_cast<int> (currentWriteBlock->getSize() / sizeof (float)));
                }
            }


            if (currentWriteBlock != nullptr)
            {
                if (currentWriteBlock->getSize() == expectedNumBytesForMemoryBlock)
                {
                    float *writePtr = static_cast<float*> (currentWriteBlock->getData());

                    // todo: speedup through simd usage?
                    for (int n = 0; n < numSamplesAllChannels; ++n)
                        writePtr[n] += std::abs (spectralBuffer[n]);

                    ++numFFTSCalculated;

                    if (numFFTSCalculated == numFFTSToAverage)
                    {
                        juce::FloatVectorOperations::multiply (writePtr, 2.0f / (numSamplesExpected * numFFTSToAverage), numSamplesAllChannels);
                        finishedWriting();
                        currentWriteBlock = nullptr;
                        numFFTSCalculated = 0;
                    }


                }
                else
                {
                    finishedWriting();
                    currentWriteBlock = nullptr;
                }

            }

            numSamplesInSampleBuffer = 0;
            processingLock.unlock();
        }
    }

    void SpectralDataCollector::updateGUIChannels ()
    {
        juce::var ns (numChannels);
        juce::var cn (channelNames);
        sink->applySettingToTarget (*this, settingNumChannels, ns);
        sink->applySettingToTarget (*this, settingChannelNames, cn);
    }

    void SpectralDataCollector::updateGUIFFTOrder()
    {
        juce::var fo (fftOrder);
        sink->applySettingToTarget (*this, settingFFTOrder, fo);
    }

    void SpectralDataCollector::updateGUIFrequencySpan()
    {
        // Have you called updateAllGUIParameters before setting the sample rate?
        jassert (sampleRate > 0.0);

        double endFrequency = sampleRate + startFrequency;
        juce::var sf (startFrequency);
        juce::var ef (endFrequency);
        sink->applySettingToTarget (*this, settingStartFrequency, sf);
        sink->applySettingToTarget (*this, settingEndFrequency, ef);
    }
    const juce::String SpectralDataCollector::settingNumChannels    ("numChannels");
    const juce::String SpectralDataCollector::settingChannelNames   ("channelNames");
    const juce::String SpectralDataCollector::settingStartFrequency ("startFrequency");
    const juce::String SpectralDataCollector::settingEndFrequency   ("endFrequency");
    const juce::String SpectralDataCollector::settingFFTOrder       ("fftOrder");
}