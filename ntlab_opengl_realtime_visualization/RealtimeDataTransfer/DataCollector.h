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


namespace ntlab
{
    class RealtimeDataSink;

    /**
     * A base class for all objects collecting some data from the realtime stream, preprocesses this data and then
     * transmits the processed result to a RealtimeDataSink instance that manages the transport of the collected
     * data to the visualization target. To keep it as abstract and extendable as possible, juce::MemoryBlocks are
     * used to exchange the preprocessed data, allowing to exchange any kind of data.
     * @see RealtimeDataSink, VisualizationTarget
     */
    class DataCollector
    {

    public:

        /** The identifier string must be the same on the visualization side to map both instances */
        DataCollector (const juce::String& idenfifier) : id (idenfifier) {};

        virtual ~DataCollector () {};

        /** This can be used to send settings from the target to the DataCollector */
        virtual void applySettingFromTarget (const juce::String& setting, const juce::var& value) {};

        /** Returns immediately and causes the memory blocks managed by this class to resize before the next usage */
        void resizeMemoryBlock (size_t newSizeInBytes)
        {
            expectedBlockSize = newSizeInBytes;
        };

        /** Called to begin writing to a memory block. It won't be swapped until finishedWriting was called */
        juce::MemoryBlock* startWriting()
        {

            if (writeBufferLock.try_lock ())
                return &writeBlock;

            return nullptr;
        };

        /**
         * Always call this after having finished writing to release the block for swapping and invoking the next
         * dataBlockReady call.
         */
        void finishedWriting()
        {
            if (readBufferLock.try_lock())
            {
                writeBlock.swapWith (readBlock);
                writeBufferLock.unlock();
                readBufferLock.unlock();
                dataBlockReady (sinkIdx);
            }
            else {
                readerShouldSwapBlocks = true;
            }
        };

        /**
         * This is called to indicate that the sink can now read a new data block. It doesn't have to use it
         * but can also just poll startReading on a regular basis. The Lambda should be assigned by
         * RealtimeDataSink when adding it to the sink. As this will be called from the realtime thread, it should
         * return as fast as possible
         */
        std::function<void(int)> dataBlockReady = [](int) {};

        /**
         * Called by the sink to start reading from the block. The caller might swap the block supplied with
         * another block
         * */
        juce::MemoryBlock& startReading()
        {
            readBufferLock.lock();
            return readBlock;
        };

        /** Called by the sink to indicate that it has finished reading */
        void finishedReading()
        {
            if (readBlock.getSize() != expectedBlockSize)
                readBlock.setSize (expectedBlockSize, true);
            if (readerShouldSwapBlocks)
            {
                writeBlock.swapWith (readBlock);
                writeBufferLock.unlock();
                readerShouldSwapBlocks = false;
                dataBlockReady (sinkIdx);
            }

            readBufferLock.unlock();
        };

        /** For internal use only, don't change */
        int sinkIdx = -1;

        /** For internal use only, don't change */
        RealtimeDataSink *sink = nullptr;

        const juce::String id;

    private:

        juce::MemoryBlock readBlock;
        juce::MemoryBlock writeBlock;

        bool readerShouldSwapBlocks = false;

        std::mutex readBufferLock, writeBufferLock;

        size_t expectedBlockSize = 0;
    };
}

