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



class PerformanceAnalyzer
{
private:
    struct TicksAndCounter
    {
        uint64_t numTicks = 0;
        int numMeasurements = 0;
    };

public:

    class MeasurementSection
    {
        friend class PerformanceAnalyzer;
    public:

        void sectionStart()
        {
            numTicksLastStart = juce::Time::getHighResolutionTicks();
        }

        void sectionEnd()
        {
            auto timeDiff = juce::Time::getHighResolutionTicks() - numTicksLastStart;
            sectionCounter->numTicks += timeDiff;
            sectionCounter->numMeasurements++;
        }

    private:
        MeasurementSection (juce::String& sn) : sectionName (sn) {};

        juce::String sectionName;
        TicksAndCounter sectionCounters[2];
        // this pointer is used for fast write access
        TicksAndCounter* sectionCounter = sectionCounters;
        int8_t counterToWriteTo = 0;


        int64_t numTicksLastStart;

        TicksAndCounter& swapCounter()
        {
            counterToWriteTo = (!counterToWriteTo);
            sectionCounter = &sectionCounters[counterToWriteTo];
            return sectionCounters[!counterToWriteTo];
        }
    };

    /**
     * Creates a new measurement section object that is managed by this owned and managed by this PerformanceAnalyzer
     * instance. Make sure that you don't do any call to this object after the corresponding PerformanceAnalyzer went
     * out of scope. The optional sectionIdx value will be filled with an integer that can be passed to getResultFor
     * to query the results for this special measurementSection.
     */
    MeasurementSection* createMeasurementSection (juce::String& sectionName, int* sectionIdx = nullptr)
    {
        if (sectionIdx != nullptr)
            *sectionIdx = measurementSections.size();

        return measurementSections.add (new MeasurementSection (sectionName));
    }

    /**
     * Fills the arrays passed with measurement results for all sections managed by this analyzer instance. Note that
     * all section counters are reset after this call to begin new averaging periods
     */
    void getResultsForAll (juce::StringArray& sectionNames, juce::Array<int>& numMeasurements, juce::Array<double>& averageTimePerSection)
    {
        sectionNames.         clearQuick();
        numMeasurements.      clearQuick();
        averageTimePerSection.clearQuick();

        auto highResTicksPerSecond = juce::Time::getHighResolutionTicksPerSecond();

        for (auto* m : measurementSections)
        {
            auto results = m->swapCounter();
            sectionNames.add (m->sectionName);
            numMeasurements.add (results.numMeasurements);

            double averageNumHighResTicksPerMeasurement = static_cast<double> (results.numTicks) / results.numTicks;
            averageTimePerSection.add (averageNumHighResTicksPerMeasurement / highResTicksPerSecond);
        }
    }

    /**
     * Returns the average time spent for this particular section. Optionally fills numMeasurements with the number of
     * measurements taken. Note that the section counter is reset after this call to begin a new averaging period
     */
    double getResultFor (int sectionIdx, int* numMeasurements = nullptr)
    {
        auto results = measurementSections[sectionIdx]->swapCounter();

        double averageNumHighResTicksPerMeasurement = static_cast<double> (results.numTicks) / results.numTicks;
        if (numMeasurements != nullptr)
            *numMeasurements = results.numMeasurements;

        return averageNumHighResTicksPerMeasurement / juce::Time::getHighResolutionTicksPerSecond();
    }
private:
    juce::OwnedArray<MeasurementSection> measurementSections;
};

