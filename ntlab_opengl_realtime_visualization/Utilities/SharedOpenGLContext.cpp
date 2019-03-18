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

#include "SharedOpenGLContext.h"

namespace ntlab
{
    JUCE_IMPLEMENT_SINGLETON (SharedOpenGLContext)

    SharedOpenGLContext::SharedOpenGLContext ()
    {
        openGLContext.setRenderer (this);
    }

    SharedOpenGLContext::~SharedOpenGLContext ()
    {
        // make sure all your OpenGLRenderer targets have been removed before the shared context is closing
        jassert (renderingTargets.size() == 0);
    }

    void SharedOpenGLContext::setTopLevelParentComponent (juce::Component& topLevelComponent)
    {
        this->topLevelComponent = &topLevelComponent;
        openGLContext.attachTo (topLevelComponent);
    }

    void SharedOpenGLContext::detachTopLevelParentComponent ()
    {
        openGLContext.detach();
    }

    bool SharedOpenGLContext::addRenderingTarget (juce::OpenGLRenderer* newTarget)
    {
        // Make sure the target you add inherits from both Component and OpenGLRenderer public
        jassert (dynamic_cast<juce::Component*> (newTarget) != nullptr);

        executeOnGLThread ([newTarget](juce::OpenGLContext&) {newTarget->newOpenGLContextCreated(); });

        std::lock_guard<std::mutex> scopedLock (renderingTargetsLock);
        renderingTargets.add (newTarget);

        return true;
    }

    void SharedOpenGLContext::removeRenderingTarget (juce::OpenGLRenderer* targetToRemove)
    {
        // if you hit this assertion you are trying to remove a target that is not managed by the SharedOpenGLContext
        jassert (renderingTargets.contains (targetToRemove));

        executeOnGLThread ([targetToRemove](juce::OpenGLContext&) {targetToRemove->openGLContextClosing(); });

        std::lock_guard<std::mutex> scopedLock (renderingTargetsLock);
        renderingTargets.removeFirstMatchingValue (targetToRemove);
    }

    void SharedOpenGLContext::executeOnGLThread (std::function<void (juce::OpenGLContext&)>&& lambdaToExecute)
    {
        std::lock_guard<std::mutex> scopedLock (executeInRenderCallbackLock);
        executeInRenderCallback.emplace_back (lambdaToExecute);
    }

    void SharedOpenGLContext::executeOnGLThreadMultipleTimes (std::function<void (juce::OpenGLContext&)>&& lambdaToExecute, const int repetitions)
    {
        std::lock_guard<std::mutex> scopedLock (executeInRenderCallbackLock);
        for (int i = 0; i < repetitions; ++i)
            executeInRenderCallback.push_back (lambdaToExecute);
    }

    juce::Rectangle<int> SharedOpenGLContext::getComponentClippingBoundsRelativeToGLRenderingTarget (juce::Component* targetComponent)
    {
        // You haven't set the top level parent component that is used for GL rendering, how should getting the bounds
        // relative to this component work at this point?
        jassert (topLevelComponent != nullptr);

        auto globalTopLeftOfGLRenderingParent = topLevelComponent->localPointToGlobal (juce::Point<int> (0, 0));

        auto targetComponentParent = targetComponent->getParentComponent();

        // your component has no GL rendering parent yet. Don't call this before having added it to a component that is
        // rendered by the SharedOpenGLContext
        jassert (targetComponentParent != nullptr);

        auto globalBoundsTarget = targetComponentParent->localAreaToGlobal (targetComponent->getBoundsInParent());

        auto targetBoundsRelativeToGLRenderingParent = globalBoundsTarget - globalTopLeftOfGLRenderingParent;

        auto renderingScale = openGLContext.getRenderingScale();

        return juce::Rectangle<int> (juce::roundToInt (renderingScale * targetBoundsRelativeToGLRenderingParent.getX()),
                                     juce::roundToInt (renderingScale * (topLevelComponent->getHeight() - targetComponent->getHeight() - targetBoundsRelativeToGLRenderingParent.getY())),
                                     juce::roundToInt (renderingScale * targetBoundsRelativeToGLRenderingParent.getWidth()),
                                     juce::roundToInt (renderingScale * targetBoundsRelativeToGLRenderingParent.getHeight()));
    }

    void SharedOpenGLContext::newOpenGLContextCreated ()
    {

    }

    void SharedOpenGLContext::renderOpenGL ()
    {
        if (topLevelComponent == nullptr)
            return;

        {
            std::lock_guard<std::mutex> scopedLock (executeInRenderCallbackLock);
            // Execute all pending jobs that should be executed on the OpenGL thread
            for (auto &glThreadJob : executeInRenderCallback)
                glThreadJob (openGLContext);

            executeInRenderCallback.clear();
        }

        {
            std::lock_guard<std::mutex> scopedLock (renderingTargetsLock);
            for (auto* target : renderingTargets)
            {
                auto* component = dynamic_cast<juce::Component*> (target);

                if (component->isVisible())
                    target->renderOpenGL();
            }
        }

    }

    void SharedOpenGLContext::openGLContextClosing ()
    {

    }
}