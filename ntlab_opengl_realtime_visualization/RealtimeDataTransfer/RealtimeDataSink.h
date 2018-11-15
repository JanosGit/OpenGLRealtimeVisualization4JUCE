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

#include "DataCollector.h"
#include <juce_core/juce_core.h>


namespace ntlab
{
    /**
    * A class used to build a connection between the realtime DSP thread as a data source and a non-realtime target sink
    * used for data visualization. This is a virtual class as there are different options of how the connection could
    * be built, e.g. DSP work and visualization could take place in the same application or visualization could be handled
    * by a remote device connected via network.
    *
    * It is expected to have one instance of it for a certain connection between a data source and a visualisation target
    * which may manage multiple data channels. Each data channel is fed by a DataCollector instance which does some
    * preprocessing and reduction of the realtime data to prepare the data block needed for displaying.
    */
    class RealtimeDataSink
    {

    public:

        virtual ~RealtimeDataSink () {};

        /**
         * Adds a data channel through which a certain dataCollector can send data to a visualisation target. The target
         * is identified by a unique identifier which must match the one passed to the visualisation target constructor.
         * In case of a local connection make sure that the visualisation target was added to the source before adding
         * the dataChannel. It's expected that the class implementing this method assigns the
         * DataCollector::dataBlockReady lambda to be notified when a new data block is available.
         *
         * !!! Make sure that the DataCollectors is not deleted before the end of this instance's lifetime
         */
        virtual juce::Result registerDataCollector (DataCollector& dataCollector) = 0;

        /**
         * Sends some settings value from the DataCollector to the visualization target. This is meant to be a
         * possibility to exchange non-realtime settings values. The value itself can be any kind of data type a
         * var can represent.
         */
        virtual void applySettingToTarget (DataCollector& dataCollector, const juce::String& setting, juce::var& value) = 0;
    };
}

