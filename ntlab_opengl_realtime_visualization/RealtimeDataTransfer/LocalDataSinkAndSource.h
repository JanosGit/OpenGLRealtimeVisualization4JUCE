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

#include "RealtimeDataSink.h"
#include "VisualizationDataSource.h"

namespace ntlab
{
    /**
     * A simple and lightweight implementation of a directly connected RealtimeDataSink and
     * VisualizationDataSource, suitable for applications where both instances are used in the same
     * binary.
     * To use it, first add all visualizationTargets, then call finishedRegisteringTargets, then register
     * the corresponding collectors. Adding the collectors first will lead to undefined behaviour.
     * After this initialization routine, the setup is ready for realtime data processing.
     */
    class LocalDataSinkAndSource : public RealtimeDataSink, public VisualizationDataSource
    {

    public:

        void registerVisualizationTarget (VisualizationTarget& visualizationTargetToAdd) override
        {
            targetIdentifiers.add (visualizationTargetToAdd.id.toString());
            visualizationTargetToAdd.targetIdx = targetIdentifiers.indexOf (visualizationTargetToAdd.id);
            visualizationTargetToAdd.setDataSource (this);
            targets.add (&visualizationTargetToAdd);
        }

        /** Call this before adding any data collectors */
        void finishedRegisteringTargets()
        {
            collectors.resize (targetIdentifiers.size());
        }

        juce::Result registerDataCollector (DataCollector &dataCollector) override
        {
            int correspondingTargetIdx = targetIdentifiers.indexOf (dataCollector.id);
            if (correspondingTargetIdx == -1)
                return juce::Result::fail ("Failed to find target with identifier " + dataCollector.id);

            dataCollector.sinkIdx = correspondingTargetIdx;
            dataCollector.sink = this;
            collectors.set (correspondingTargetIdx, &dataCollector);

            return juce::Result::ok();
        }

        juce::MemoryBlock& startReading (VisualizationTarget& target) override
        {
            return collectors[target.targetIdx]->startReading();
        }

        void finishedReading (VisualizationTarget& target) override
        {
            collectors[target.targetIdx]->finishedReading();
        }

        void applySettingToCollector (VisualizationTarget& target, const juce::String& setting, const juce::var& value) override
        {
            collectors[target.targetIdx]->applySettingFromTarget (setting, value);
        }

        void applySettingToTarget (DataCollector &dataCollector, const juce::String& setting, juce::var& value) override
        {
            targets[dataCollector.sinkIdx]->applySettingFromCollector (setting, value);
        }
    private:

        juce::StringArray targetIdentifiers;
        juce::Array<VisualizationTarget*> targets;
        juce::Array<DataCollector*> collectors;
    };
}