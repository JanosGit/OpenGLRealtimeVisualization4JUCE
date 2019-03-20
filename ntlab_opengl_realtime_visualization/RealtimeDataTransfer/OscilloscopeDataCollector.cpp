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


#include "OscilloscopeDataCollector.h"

namespace ntlab
{
    void OscilloscopeDataCollector::setChannels (int numChannels, juce::StringArray channelNames)
    {
        this->numChannels = numChannels;
        this->channelNames = channelNames;

        updateGUIChannels();
        recalculateMemory();
    }

    void OscilloscopeDataCollector::setTimeViewed (double timeViewedInSeconds)
    {
        jassert (timeViewedInSeconds > 0.0);
        tView = timeViewedInSeconds;
        recalculateNumSamples();
    }

    void OscilloscopeDataCollector::setSampleRate (double newSampleRate)
    {
        jassert (newSampleRate > 0);
        tSample = 1.0 / newSampleRate;
        recalculateNumSamples();
    }

    void OscilloscopeDataCollector::enableTriggering (bool isTriggered, int channelToUse)
    {
        triggeringEnabled = isTriggered;
        triggerChannel = channelToUse;
        updateGUITriggering();
    }

    void OscilloscopeDataCollector::pushChannelsSamples (juce::AudioBuffer<float> &bufferToPush)
    {
        if (currentWriteBlock == nullptr)
            currentWriteBlock = startWriting();

        if (currentWriteBlock != nullptr)
        {
            size_t blockSizeInBytes = currentWriteBlock->getSize();
            if (bufferToPush.getNumChannels() != numChannels)
            {
                fillUnmatchingBlockWithZeros (blockSizeInBytes);
                return;
            }

            if (blockSizeInBytes  != expectedNumBytesForMemoryBlock)
            {
                fillUnmatchingBlockWithZeros (blockSizeInBytes);
                return;
            }

            int numSamplesInBuffer = bufferToPush.getNumSamples();

            if (triggeringEnabled && !foundTriggerInCurrentBlock)
            {
                const float *tc = bufferToPush.getReadPointer (triggerChannel);
                for (int i = 1; i < numSamplesInBuffer; ++i)
                {
                    if ((tc[i - 1] <= 0.0f) && (tc[i] > 0.0f))
                    {
                        foundTriggerInCurrentBlock = true;
                        int numSamplesAvailable = numSamplesInBuffer - i;
                        int numSamplesToCopy = std::min (numSamplesAvailable, (numSamplesExpected - numSamplesInCurrentBlock));

                        for (int n = 0; n < numChannels; ++n)
                        {
                            float *writePtr = static_cast<float*> (currentWriteBlock->getData()) + channelOffset[n];
                            juce::FloatVectorOperations::copy (writePtr, bufferToPush.getReadPointer (n) + i, numSamplesToCopy);
                        }

                        numSamplesInCurrentBlock = numSamplesToCopy;
                        break;
                    }
                }
            }
            else
            {
                int numSamplesToCopy = std::min (numSamplesInBuffer, (numSamplesExpected - numSamplesInCurrentBlock));
                for (int n = 0; n < numChannels; ++n)
                {
                    float *writePtr = static_cast<float*> (currentWriteBlock->getData()) + channelOffset[n] + numSamplesInCurrentBlock;
                    juce::FloatVectorOperations::copy (writePtr, bufferToPush.getReadPointer (n), numSamplesToCopy);
                }
                numSamplesInCurrentBlock += numSamplesToCopy;
            }

            if (numSamplesInCurrentBlock == numSamplesExpected)
                prepareForNextSampleBlock();

        }
    }

    void OscilloscopeDataCollector::applySettingFromTarget (const juce::String& setting, const juce::var& value)
    {
        if (setting == settingTimeViewed)
        {
            if (value.isDouble())
            {
                tView = value;
                recalculateNumSamples();
            }
        }
        else if (setting == settingIsTriggered)
        {
            if (value.isBool())
                enableTriggering (value);
        }
    }

    void OscilloscopeDataCollector::fillUnmatchingBlockWithZeros (size_t blockSizeInBytes)
    {
        juce::FloatVectorOperations::clear (static_cast<float*> (currentWriteBlock->getData()), static_cast<int> (blockSizeInBytes / sizeof (float)));
        prepareForNextSampleBlock();
    }

    void OscilloscopeDataCollector::prepareForNextSampleBlock()
    {
        finishedWriting();
        currentWriteBlock = nullptr;
        foundTriggerInCurrentBlock = false;
        numSamplesInCurrentBlock = 0;
    }

    void OscilloscopeDataCollector::recalculateNumSamples()
    {
        // Always set the samplerate before setting the time viewed!
        jassert (tSample != 1);
        numSamplesExpected = juce::roundToInt (tView / tSample);
        numSamplesInCurrentBlock = 0;
        updateGUITimebase();
        recalculateMemory();
    }

    void OscilloscopeDataCollector::recalculateMemory()
    {
        expectedNumBytesForMemoryBlock = numChannels * numSamplesExpected * sizeof (float);
        resizeMemoryBlock (expectedNumBytesForMemoryBlock);

        channelOffset.resize (numChannels);
        for (int i = 0; i < numChannels; ++i)
        {
            channelOffset.set (i, i * numSamplesExpected);
        }
    }

    void OscilloscopeDataCollector::updateAllGUIParameters()
    {
        updateGUITimebase();
        updateGUIChannels();
        updateGUITriggering();
    }

    void OscilloscopeDataCollector::updateGUITimebase()
    {
        juce::var ts (tSample);
        juce::var tv (tView);
        juce::var nse (numSamplesExpected);
        sink->applySettingToTarget (*this, settingTSample, ts);
        sink->applySettingToTarget (*this, settingTimeViewed, tv);
        sink->applySettingToTarget (*this, settingNumSamples, nse);
    }

    void OscilloscopeDataCollector::updateGUIChannels()
    {
        juce::var ns (numChannels);
        juce::var cn (channelNames);
        sink->applySettingToTarget (*this, settingNumChannels, ns);
        sink->applySettingToTarget (*this, settingChannelNames, cn);
    }

    void OscilloscopeDataCollector::updateGUITriggering()
    {
        juce::var te (triggeringEnabled);
        sink->applySettingToTarget (*this, settingIsTriggered, te);
    }

    const juce::String OscilloscopeDataCollector::settingTimeViewed   ("timeViewed");
    const juce::String OscilloscopeDataCollector::settingIsTriggered  ("isTriggered");
    const juce::String OscilloscopeDataCollector::settingTSample      ("tSample");
    const juce::String OscilloscopeDataCollector::settingNumSamples   ("numSamples");
    const juce::String OscilloscopeDataCollector::settingNumChannels  ("numChannels");
    const juce::String OscilloscopeDataCollector::settingChannelNames ("channelNames");
}

