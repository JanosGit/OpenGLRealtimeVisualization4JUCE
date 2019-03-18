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

#include <juce_opengl/juce_opengl.h>

namespace ntlab
{
    class SharedOpenGLContext : private juce::OpenGLRenderer
    {
    public:
        SharedOpenGLContext();

        ~SharedOpenGLContext();

        void setTopLevelParentComponent (juce::Component& topLevelComponent);

        void detachTopLevelParentComponent();

        bool addRenderingTarget (juce::OpenGLRenderer* newTarget);

        void removeRenderingTarget (juce::OpenGLRenderer* targetToRemove);

        void executeOnGLThread (std::function<void(juce::OpenGLContext&)>&& lambdaToExecute);

        void executeOnGLThreadMultipleTimes (std::function<void(juce::OpenGLContext&)>&& lambdaToExecute, const int repetitions);

        juce::Rectangle<int> getComponentClippingBoundsRelativeToGLRenderingTarget (juce::Component* targetComponent);

        juce::OpenGLContext openGLContext;

        JUCE_DECLARE_SINGLETON (SharedOpenGLContext, false)
    private:
        juce::Component* topLevelComponent = nullptr;

        juce::Array<juce::OpenGLRenderer*> renderingTargets;
        std::mutex renderingTargetsLock;

        std::vector<std::function<void(juce::OpenGLContext&)>> executeInRenderCallback;
        std::mutex executeInRenderCallbackLock;

        // OpenGL related member functions
        void newOpenGLContextCreated() override;
        void renderOpenGL() override;
        void openGLContextClosing() override;
    };
}
