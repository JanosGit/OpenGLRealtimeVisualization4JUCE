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
#include <cmath>
#include <sstream>
#include <iomanip>


namespace ntlab
{
    /**
     * A quickly implemented class to manage some more sophisticated conversions from float numbers to strings,
     * including finding/adding SI prefixes to those numbers. Not optimized for speed in any way.
     */
    class Float2String
    {

    public:
        enum SIPrefix
        {
            femto = -5,
            pico  = -4,
            nano  = -3,
            micro = -2,
            milli = -1,
            none  = 0,
            kilo  = 1,
            mega  = 2,
            giga  = 3,
            tera  = 4,
            peta  = 5
        };

        /** Converts a floating point number to a String represantation with a fixed number of digits */
        template <typename FloatType>
        static juce::String withFixedLength (FloatType floatNumber, int overallNumberOfDigits)
        {
            static_assert (std::is_floating_point<FloatType>::value, "Only floating point types are allowed for Float2String");

            if (isnan (floatNumber))
                return "NaN";

            if (isinf (floatNumber))
                return "Inf";

            // if you are hitting this assert, the number is too big to be displayed by this number of digits and will
            // be displayed wrong
            jassert (floatNumber < std::pow (10, overallNumberOfDigits));

            // prevents too small number to be displayed in exponential writing
            if (std::abs (floatNumber) < std::pow (10, -overallNumberOfDigits))
                floatNumber = FloatType (0);

            // todo: I'm sure this can be done better
            std::ostringstream os;
            os << std::setprecision (overallNumberOfDigits) << floatNumber;
            auto numberAsStdString = os.str();

            juce::String returnString;
            returnString.preallocateBytes (overallNumberOfDigits + 2);

            int numPlacesAppended = 0;
            bool hasDecimalDot = false;
            for (char& c : numberAsStdString)
            {
                returnString += c;
                if (c == '.')
                    hasDecimalDot = true;
                else
                    ++numPlacesAppended;
                if (c == '-')
                    ++overallNumberOfDigits;
                if (numPlacesAppended == overallNumberOfDigits)
                    break;
            }

            if (int numPlacesMissing = overallNumberOfDigits - numPlacesAppended)
            {
                if (!hasDecimalDot)
                    returnString += '.';

                returnString += juce::String::repeatedString ("0", numPlacesMissing);
            }

            return returnString;
        }

        /** Returns the best fitting SIPrefix to display the number passed */
        template <typename FloatType>
        static SIPrefix getBestSIPrefixForValue (FloatType floatNumber, int maxNumberOfDigitsBeforeDecimalPoint = 1)
        {
            // scaling the number before conversion shifts it into a range resulting in the desired number of digits
            // before the decimal point
            floatNumber *= std::pow (10, 3 - maxNumberOfDigitsBeforeDecimalPoint);

            // to figure out the range of the base 10 exponent si prefix we get the base 2 exponent and scale it by a
            // conversion to the corresponding prefix idx
            int exponentBase2;
            std::frexp (floatNumber, &exponentBase2);
            int prefixIdx = static_cast<int> (std::floor (exponentBase2 * base2ExponentToPrefixIdxConversionFactor));

            // limit the range so that it fits the possible si prefixes implemented
            return static_cast<SIPrefix > (juce::jlimit (-5, 5, prefixIdx));
        }

        /**
         * Converts a floating point number into a version with an si prefix appended. The best fitting prefix is choosen
         * automatically, depending on the range of the input number and the number is scaled accordingly.
         * As si prefixes have a spacing of 10^3 make sure that overallNumberOfDigits is at least 3 as otherwise the
         * values could be incorrect
         */
        template <typename FloatType>
        static juce::String withSIPrefix (FloatType floatNumber, int overallNumberOfDigits)
        {
            SIPrefix bestPrefix = getBestSIPrefixForValue (floatNumber);
            return withSIPrefix (floatNumber, overallNumberOfDigits, bestPrefix);
        }

        /**
         * Converts a floating point number into a version with the desired si prefix appended. The number is scaled
         * according to the chosen prefix. Make sure that overallNumberOfDigits is big enough to display big numbers.
         */
        template <typename FloatType>
        static juce::String withSIPrefix (FloatType floatNumber, int overallNumberOfDigits, SIPrefix desiredPrefix)
        {
            if (isnan (floatNumber))
                return "NaN";

            if (isinf (floatNumber))
                return "Inf";

            int prefixIdx = desiredPrefix;
            int exponentBase10 = prefixIdx * 3;

            prefixIdx += siPrefixArrayOffset;
            floatNumber /= std::powf (10, exponentBase10);

            return withFixedLength (floatNumber, overallNumberOfDigits) + siPrefixes[prefixIdx];
        }

    private:
        static constexpr double base2ExponentToPrefixIdxConversionFactor = 0.100343331887994; // log10 (2) / 3
        static constexpr char siPrefixes[11][3] = {"f", "p", "n", "μ", "m", "", "k", "M", "G", "T", "P"};
        static const int siPrefixArrayOffset = 5;
    };
}

