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


#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() :
        oscilloscopeDataCollector ("MicSignals"),
        spectralDataCollector     ("MicSignals"),
        oscilloscopeComponent     ("MicSignals"),
        spectralAnalyzerComponent ("MicSignals")
{
    ntlab::SharedOpenGLContext::getInstance()->setTopLevelParentComponent (*this);

    // Always first step: Register the target components that visualize your data
    localDataSinkAndSource.registerVisualizationTarget (oscilloscopeComponent);
    localDataSinkAndSource.registerVisualizationTarget (spectralAnalyzerComponent);

    // Always call this after having finished adding targets
    localDataSinkAndSource.finishedRegisteringTargets();

    // Now it's time to register the collectors. They will automatically be connected to the target as they use the same
    // identifier extension ("MicSignals" in this example). The DBG prints make it more clear how the fully expanded name
    // looks like
    localDataSinkAndSource.registerDataCollector (oscilloscopeDataCollector);
    localDataSinkAndSource.registerDataCollector (spectralDataCollector);
    DBG ("Oscilloscope collector ID: " << oscilloscopeDataCollector.id);
    DBG ("Spectral analyzer collector ID: " << spectralDataCollector.id);

    addAndMakeVisible (oscilloscopeComponent);
    // The settings bar is a quick implementation useful to demonstrate the manipulation of the settings in real time.
    // Calling the corresponding setter functions or manipulating the value tree will result in the same behaviour.
    oscilloscopeComponent.displaySettingsBar (true);

    addAndMakeVisible (spectralAnalyzerComponent);
    // using the value tree to set the parameters
    spectralAnalyzerComponent.valueTree.setProperty (ntlab::SpectralAnalyzerComponent::parameterFrequencyLinearLog, true, nullptr);
    spectralAnalyzerComponent.valueTree.setProperty (ntlab::SpectralAnalyzerComponent::parameterHideDC, true, nullptr);

    setSize (800, 800);
    setAudioChannels (2, 0);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
    ntlab::SharedOpenGLContext::getInstance()->detachTopLevelParentComponent();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    int numInChannels = getNumberOfActiveInputChannels();

    juce::StringArray channelNames;
    for (int i = 0; i < numInChannels; ++i) {
        channelNames.add ("Channel " + juce::String (i + 1));
    }

    oscilloscopeDataCollector.setChannels (numInChannels, channelNames);
    oscilloscopeDataCollector.setSampleRate (sampleRate);
    oscilloscopeDataCollector.setTimeViewed (0.03);

    spectralDataCollector.setChannels (numInChannels, channelNames);
    spectralDataCollector.setSampleRate (sampleRate);
}

void MainComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    oscilloscopeDataCollector.pushChannelsSamples (*bufferToFill.buffer);
    spectralDataCollector.    pushChannelsSamples (*bufferToFill.buffer);
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{

}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    int componentHeight = (bounds.getHeight() - 10) / 2;
    oscilloscopeComponent.    setBounds (bounds.removeFromTop    (componentHeight));
    spectralAnalyzerComponent.setBounds (bounds.removeFromBottom (componentHeight));
}

int MainComponent::getNumberOfActiveInputChannels() {
    return deviceManager.getCurrentAudioDevice()->getActiveInputChannels().countNumberOfSetBits();
}