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

/*******************************************************************************
 The block below describes the properties of this module, and is read by
 the Projucer to automatically generate project code that uses it.
 For details about the syntax and how to create or use a module, see the
 JUCE Module Format.txt file.


 BEGIN_JUCE_MODULE_DECLARATION

  ID:               ntlab_opengl_realtime_visualization
  vendor:           Janos Buttgereit
  version:          1.0.0
  name:             Open GL Realtime Visualization
  description:      Classes to analyze realtime sample streams and visualize them efficiently
  website:          www.github.com/janosgit
  license:          MIT

  dependencies:     juce_dsp

 END_JUCE_MODULE_DECLARATION

*******************************************************************************/

#pragma once

#include "RealtimeDataTransfer/DataCollector.h"
#include "RealtimeDataTransfer/LocalDataSinkAndSource.h"
#include "RealtimeDataTransfer/OscilloscopeDataCollector.h"
#include "RealtimeDataTransfer/RealtimeDataSink.h"
#include "RealtimeDataTransfer/SpectralDataCollector.h"
#include "RealtimeDataTransfer/VisualizationDataSource.h"

#include "Buffers/SwappableBuffer.h"

#include "Utilities/Float2String.h"

// These parts of the module won't be needed by the sender, which might be a GUI-less application maybe not even
// running on a system with any GUI
#if JUCE_MODULE_AVAILABLE_juce_opengl

#include "Utilities/SharedOpenGLContext.h"
#include "Utilities/SerializableRange.h"

#include "2DPlot/Plot2D.h"

#include "GUIComponents/OscilloscopeComponent.h"
#include "GUIComponents/SpectralAnalyzerComponent.h"

#include "Shader/Attributes.h"
#include "Shader/Uniforms.h"
#include "Shader/LineShader.h"

#endif

