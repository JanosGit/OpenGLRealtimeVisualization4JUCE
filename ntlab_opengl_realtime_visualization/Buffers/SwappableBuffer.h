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


/*
  ==============================================================================

    SwappableBuffer.h
    Created: 14 Jun 2018 7:06:29am
    Author:  Janos Buttgereit

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <algorithm>
#include <mutex>

namespace ntlab
{
    /**
 * A single writer single consumer buffer that contains two memory regions and ensures safe buffer swaps with minimal
 * locking time at the writer side. Perfect for passing samples from the audio thread to the GUI for visualization
 * */
    template <typename T>
    class SwappableBuffer {

        friend class ScopedReadBufferPtr;
    public:

        /** Creates a single channel buffer with the given buffer size */
        SwappableBuffer (int bufferSize, int initialNumChannels = 0, bool initializeWithZeros = false) : size (bufferSize) {
            reallocateChannels (initialNumChannels, initializeWithZeros);
        }

        /**
         * Adds one or more channels to the buffer. This will lock read access and lead to ignoring write access as long
         * as the re-allocation of the underlying buffer takes place. All buffers will be cleared
         */
        void addChannels (int numChannelsToAdd) {
            int newNumChannels = numChannels + numChannelsToAdd;
            reallocateChannels (newNumChannels, true);
        }

        /**
         * Deletes one or more channels to the buffer. This will lock read access and lead to ignoring write access as long
         * as the re-allocation of the underlying buffer takes place. All buffers will be cleared
         */
        void deleteChannels (int numChannelsToDelete) {
            jassert (numChannelsToDelete <= numChannels);
            int newNumChannels = numChannels - numChannelsToDelete;
            reallocateChannels (newNumChannels, true);
        }

        /**
         * Sets the number of channels held by this buffer. This will lock read access and lead to ignoring write access as
         * long as the re-allocation of the underlying buffer takes place. All buffers will be cleared
         */
        void setNumChannels (int numChannelsToAllocate) {
            reallocateChannels (numChannelsToAllocate, true);
        }

        /** Returns the number of channels held by this buffer */
        int getNumChannels() {
            return numChannels;
        }

        /**
         * Copies the data provided to the write buffer region.
         * If no read action is in progress, this will immediately become the read buffer.
         * If a read action is in progress while writing the new buffer, this will become the read buffer
         * as soon as the reader has finished reading.
         * If multiple write calls occur while a read action is in progress, the previously written data will
         * be overwritten even if it has not been consumed by the reader.
         *
         * If numElementsToWrite is smaller than the buffer size passed to the constructor, the remaining buffer
         * space will be filled with zeros. Make sure that numElementsToWrite never gets bigger than the buffer
         * size.
         */
        void writeNewBuffer (const T **dataToWrite, int numElementsToWrite) {

            if (writeBuferLock.try_lock()) {
                readerShouldSwapPtrs = false;

                jassert (numElementsToWrite <= size);

                for (int i = 0; i < numChannels; ++i) {
                    std::memcpy ((*writeChannels)[i], dataToWrite[i], numElementsToWrite * sizeof (T));
                    std::fill ((*writeChannels)[i] + numElementsToWrite, (*writeChannels)[i] + size, T(0));
                }

                // in case the lock is available, directly swap buffer pointers
                if (readBufferLock.try_lock ()) {
                    swapPtrs ();
                    readBufferLock.unlock ();
                }
                    // otherwise let the reader swaps the pointers when he has finished reading
                else {
                    readerShouldSwapPtrs = true;
                }

                writeBufferRemainingCapacity = size;

                writeBuferLock.unlock();
            }

        }

        /**
         * Appends a piece of data to the write buffer. As soon as it is completely filled, a buffer swap will be invoked.
         * If the available data does not fit into the remaining buffer space, all elements that could not be written will
         * be discarded.
         * @return true if the buffer was swapped, false otherwise
         */
        bool appendToWriteBuffer (const T **dataToWrite, int numElementsAvailable) {

            if (writeBuferLock.try_lock()) {
                readerShouldSwapPtrs = false;

                int startIdx = size - writeBufferRemainingCapacity;
                int numElementsToWrite = juce::jmin (numElementsAvailable, writeBufferRemainingCapacity);

                for (int i = 0; i < numChannels; ++i) {
                    std::memcpy ((*writeChannels)[i] + startIdx, dataToWrite[i], numElementsToWrite * sizeof (T));
                }


                writeBufferRemainingCapacity -= numElementsToWrite;

                if (writeBufferRemainingCapacity == 0) {
                    // in case the lock is available, directly swap buffer pointers
                    if (readBufferLock.try_lock ()) {
                        swapPtrs ();
                        readBufferLock.unlock ();
                    }
                        // otherwise let the reader swaps the pointers when he has finished reading
                    else {
                        readerShouldSwapPtrs = true;
                    }

                    writeBufferRemainingCapacity = size;
                    writeBuferLock.unlock();
                    return true;
                }

                writeBuferLock.unlock();
            }

            return false;

        }

        /**
         * An RAII-Object wrapping the pointer needed to access the read buffer region. As long as it stays in scope,
         * this will prevent the corresponding SwappableBuffer object to swap write and read buffers.
         */
        class ScopedReadBufferPtr {
            friend class SwappableBuffer;
        public:

            ~ScopedReadBufferPtr () {
                if (swapableBuffer.readerShouldSwapPtrs) {
                    swapableBuffer.swapPtrs();
                }
                swapableBuffer.readBufferLock.unlock();
            }

            /** Returns the read pointer Array held by this instance */
            const T **get() {return swapableBuffer.readChannels->getRawDataPointer(); };

            /** Access an individual channel in the read buffer held by this instance */
            const T *operator[] (int idx) {return (*swapableBuffer.readChannels)[idx]; };

            /** Access a single sample held by the buffer */
            const T &getSample (int channel, int sample) {return (*swapableBuffer.readChannels)[channel][sample]; };

        private:
            SwappableBuffer &swapableBuffer;
            ScopedReadBufferPtr (SwappableBuffer &s) : swapableBuffer (s) {};
        };

        /**
         * Returns a ScopedReadBufferPtr instance holding the current read buffer pointer that is guranteed to stay valid
         * as long as the object has not been destructed. When no new data was written, repeated calls can result in
         * instances pointing to the same memory location.
         */
        ScopedReadBufferPtr getReadBuffer() {
            readBufferLock.lock();
            return ScopedReadBufferPtr (*this);
        }

        /** The size of each buffer channel*/
        const int size;

    private:

        juce::HeapBlock<T> buffers[2];
        juce::Array<T*> channels[2];
        juce::Array<T*> *writeChannels;
        juce::Array<T*> *readChannels;
        int numChannels;

        int writeBufferRemainingCapacity;

        bool readerShouldSwapPtrs = false;
        std::mutex readBufferLock, writeBuferLock;

        // should only be called it the readBufferLock is locked
        void swapPtrs() {
            std::swap (writeChannels, readChannels);
        }

        // (Re-)allocates the buffers and resets them after changing the number of channels
        void reallocateChannels (int newNumChannels, bool initializeWithZeros) {
            // Lock write and read access
            std::unique_lock<std::mutex> rbl (readBufferLock, std::defer_lock);
            std::unique_lock<std::mutex> wbl (writeBuferLock, std::defer_lock);
            std::lock (rbl, wbl);

            numChannels = newNumChannels;

            buffers[0].allocate (size * numChannels, initializeWithZeros);
            buffers[1].allocate (size * numChannels, initializeWithZeros);

            channels->resize (numChannels);
            for (int i = 0; i < numChannels; ++i) {
                channels[0].set (i, buffers[0].get() + (i * size));
                channels[1].set (i, buffers[1].get() + (i * size));
            }

            writeChannels = &channels[0];
            readChannels = &channels[1];

            writeBufferRemainingCapacity = size;
        }

    };

}

