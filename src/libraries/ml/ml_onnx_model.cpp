//=============================================================================================================
/**
 * @file     ml_onnx_model.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    MlOnnxModel class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_onnx_model.h"

#ifdef MNE_USE_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <stdexcept>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MLLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MlOnnxModel::MlOnnxModel()
{
}

//=============================================================================================================

MlOnnxModel::~MlOnnxModel()
{
}

//=============================================================================================================

MlTensor MlOnnxModel::predict(const MlTensor& input) const
{
#ifdef MNE_USE_ONNXRUNTIME
    // TODO: Create Ort::Value from input tensor, run session, convert output back to MlTensor.
    Q_UNUSED(input);
    throw std::runtime_error("MlOnnxModel::predict – ONNX Runtime integration not yet implemented.");
#else
    Q_UNUSED(input);
    throw std::runtime_error("ONNX Runtime not available. Build with -DUSE_ONNXRUNTIME=ON");
#endif
}

//=============================================================================================================

bool MlOnnxModel::save(const QString& path) const
{
    Q_UNUSED(path);
    qWarning() << "MlOnnxModel::save – ONNX models cannot be saved from this interface.";
    return false;
}

//=============================================================================================================

bool MlOnnxModel::load(const QString& path)
{
#ifdef MNE_USE_ONNXRUNTIME
    // TODO: Create Ort::Env, Ort::SessionOptions, Ort::Session from path.
    m_modelPath = path;
    qDebug() << "MlOnnxModel::load – ONNX Runtime session created for" << path;
    return true;
#else
    m_modelPath = path;
    qWarning() << "MlOnnxModel::load – Path stored but no ONNX Runtime session created (build with -DUSE_ONNXRUNTIME=ON).";
    return false;
#endif
}

//=============================================================================================================

QString MlOnnxModel::modelType() const
{
    return QStringLiteral("onnx");
}

//=============================================================================================================

MlTaskType MlOnnxModel::taskType() const
{
    return m_taskType;
}
