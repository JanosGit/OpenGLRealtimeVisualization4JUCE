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
