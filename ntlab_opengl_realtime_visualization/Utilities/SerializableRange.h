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
