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
    /** A simple helper struct to manage ranges that should be serialized to/from xml through a juce::ValueTree */
    template <typename FloatType>
    struct SerializableRange
    {
        SerializableRange (const juce::Range<FloatType>& initialRange) : range (initialRange) {}
        SerializableRange (FloatType min, FloatType max)               : range (min, max) {}

        SerializableRange (const juce::String& rangeSerialized) {fromString (rangeSerialized); }
        SerializableRange (const juce::var&    rangeSerialized) {fromString (rangeSerialized.toString()); }

        juce::String toString() {return juce::String (range.getStart()) + "|" + juce::String (range.getEnd()); }
        void fromString (const juce::String& rangeSerialized)
        {
            auto array = juce::StringArray::fromTokens (rangeSerialized, "|", "");

            jassert (array.size() == 2);
            jassert (array[0].containsOnly ("-.0123456789"));
            jassert (array[1].containsOnly ("-.0123456789"));

            range.setStart (array[0].getDoubleValue());
            range.setEnd   (array[1].getDoubleValue());
        }

        operator juce::String()           {return toString(); }
        operator juce::var()              {return toString(); }
        operator juce::Range<FloatType>() {return range; };

        juce::Range<FloatType> range;
    };
}
