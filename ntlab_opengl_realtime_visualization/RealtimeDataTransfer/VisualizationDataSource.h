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
#include "juce_data_structures/juce_data_structures.h"

namespace ntlab
{
    class VisualizationDataSource;

    /**
     * A base class for all targets that consume data. Target idx can be set by the VisualizationDataSource
     * to map startReading and finishReading calls.
     *
     * This class contains a valueTree member to store the settings of the Target(Component) as it is expected that
     * some mechanism to store the settings is needed anyway and the value tree seems to be suited to be integrated in
     * existing JUCE apps. However, all parameters managed by the valueTree can also be set by setter functions
     * implemented by classes inheriting from VisualizationTarget.
     */
    class VisualizationTarget
    {

    public:

        /** The identifier string must be the same on the collector side to map both instances */
        VisualizationTarget (const juce::String& identifier, juce::UndoManager* um = nullptr) : id (identifier), valueTree (id), undoManager (um) {};

        virtual ~VisualizationTarget () {};

        /** Should only be called by the source */
        void setDataSource (VisualizationDataSource *newDataSource) {dataSource = newDataSource; };

        /** This can be used to send settings from the DataCollector to the target*/
        virtual void applySettingFromCollector (const juce::String& setting, juce::var& value) {};

        /** Should only be set by the source */
        int targetIdx = -1;

        /**
         * The identifier used to identify this target to connect it with the corresponding data collector instance.
         * Furthermore this will be the type of the ValueTree held by this instance.
         */
        const juce::Identifier id;

        /**
         * A value tree to hold parameters used for the individual subclasses that inherit from VisualizationTarget.
         * This way settings regarding e.g. the visual appearance of the class can be easily integrated into a bigger
         * application context
         */
        juce::ValueTree valueTree;

    protected:
        juce::UndoManager* undoManager;
        VisualizationDataSource* dataSource = nullptr;

    };

    class VisualizationDataSource
    {
    public:

        /** Must be called to connect a target to this source. Usually, this is done once at setup time*/
        virtual void registerVisualizationTarget (VisualizationTarget& visualizationTargetToAdd) = 0;

        virtual ~VisualizationDataSource () {};

        /**
         * Called to start reading the most recent data delivered by the collector. Call finishedReading as soon as
         * the MemoryBlock can be passed back to the collector.
         * */
        virtual juce::MemoryBlock& startReading (VisualizationTarget& target) = 0;

        /** Call this to release the MemoryBlock received by startReading */
        virtual void finishedReading (VisualizationTarget& target) = 0;

        /** Can be used to pass settings from the target to the collector */
        virtual void applySettingToCollector (VisualizationTarget& target, const juce::String& setting, const juce::var& value) = 0;

    };
}