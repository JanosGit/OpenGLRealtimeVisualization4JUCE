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

#include "../RealtimeDataTransfer/VisualizationDataSource.h"
#include "../2DPlot/Plot2D.h"

namespace ntlab
{
    /**
     * The Component designed to visualize frequency-domain data collected by a SpectralDataCollector instance.
     * It exports the parameters fFTOrder, hideNegativeFrequencies, hideDC, magnitudeLinearDB, frequencyLinearLog
     * to the VisualizationTarget valueTree member. It inherits ntlab::Plot2D and therefore uses OpenGL for rendering.
     */
    class SpectralAnalyzerComponent : public ntlab::VisualizationTarget, public ntlab::Plot2D, private juce::ValueTree::Listener
    {
    public:

        /** A positive integer controlling the order of the underlying FFT. Default value: 11, resulting in 2048 bins. */
        static const juce::Identifier parameterFFTOrder;

        /** A 2-Element integer Array containing the minimal and maximal magnitude visualized. */
        static const juce::Identifier parameterMagnitudeRange;

        /** A boolean selecting if a symmetric spectrum with negative frequences is desired. */
        static const juce::Identifier parameterHideNegativeFrequencies;

        /** A boolean to select if in case of hidden negative frequencies also the DC part should be hidden. */
        static const juce::Identifier parameterHideDC;

        /** A boolean to select if the magnitude should be displayed linear (=false) or logarithmic (=true) */
        static const juce::Identifier parameterMagnitudeLinearDB;

        /**
         * A boolean to select if the frequencies should be displayed linear (=false) or logarithmic (=true). Note
         * that logarithmic scaling only works if the negative frequencies are hidden
         */
        static const juce::Identifier parameterFrequencyLinearLog;

        /**
         * Specifiy an identifier extension to map the SpectralAnalyzerComponent to the corresponding source.
         * The Identifier will automatically be prepended by "SpectralAnalyzer". The optional undo manager can
         * be passed to enable undo functionality for the parameters held by the ValueTree.
         */
        SpectralAnalyzerComponent (const juce::String identifierExtension, juce::UndoManager* undoManager = nullptr);

        ~SpectralAnalyzerComponent();

        /** Sets the order of the underlying FFT. Should be > 3 */
        void setFFTOrder (int newOrder);

        /**
         * If enabled, the negative frequencies are hidden, as they are redundant for most use-cases.
         * In case negative frequencies are hidden, the DC part can also be hidden as it is irrelevant for
         * a lot of use cases where a DC-free signal is expected
         */
        void hideNegativeFrequencies (bool shouldHideNegativeFrequencies, bool shouldAlsoHideDC = false);

        /** If enabled the magnitude axis of the spectrum is scaled in dB Values */
        void setMagnitudeScaling (bool shouldBeLog);

        /**
         * If enabled the frequency axis is logarithmic. This is especially useful for audio applications as this
         * reflects the human hearing perception better. Note that a logarithmic frequency scaling can only be
         * applied to a spectrum where the negative frequencies are hidden.
         */
        void setFrequencyAxisScaling (bool shouldBeLog);

#ifndef DOXYGEN
        void applySettingFromCollector (const juce::String& setting, juce::var& value) override;
        void resized() override;
#endif

    private:

        // bitfield index values for all settings that have been set
        enum ValidSettings
        {
            numChannelsValid  = 0,
            numFFTBinsValid   = 1,
            channelNamesValid = 2,
        };
        std::bitset<3> validChannelInformation;

        int numChannels = 0;
        int numFFTBins = 0;
        juce::StringArray channelNames;
        juce::Range<float> frequencyRange;

        juce::MemoryBlock* lastBuffer = nullptr;

        // Plot2D Member functions
        void beginFrame() override;
        const float* getBufferForLine (int lineIdx) override;
        void endFrame() override;

        // ValueTree::Listener functions
        void valueTreePropertyChanged (juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property) override;
        void valueTreeChildAdded (juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenAdded) override;
        void valueTreeChildRemoved (juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
        void valueTreeChildOrderChanged (juce::ValueTree &parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex) override;
        void valueTreeParentChanged (juce::ValueTree &treeWhoseParentHasChanged) override;

        void updateChannelInformation();
        void updateFrequencyRangeInformation();
    };
}