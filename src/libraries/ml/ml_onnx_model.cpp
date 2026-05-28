//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file ml_onnx_model.cpp
 * @since 2026
 * @date  May 2026
 * @brief Implementation of @ref MLLIB::MlOnnxModel including session creation, IO-binding cache and zero-copy predict.
 *
 * The translation unit isolates every direct dependency on the ONNX
 * Runtime C++ API behind @c \#ifdef @c MNE_USE_ONNXRUNTIME, so the
 * library object is buildable even when ORT is absent. The single
 * static @c Ort::Env returned by @ref MLLIB::MlOnnxModel::ortEnv is
 * shared by all sessions in the process; per-instance @c Ort::Session
 * and @c Ort::MemoryInfo are held in @c std::unique_ptr so the
 * destructor must live in this TU (the forward-declared Ort types are
 * incomplete at the header).
 *
 * @c load opens the file with @c GraphOptimizationLevel::ORT_ENABLE_ALL,
 * forces a single intra-op thread for deterministic output, disables
 * memory patterns to keep peak RSS small, and caches input/output node
 * names and shapes once so @c predict only pays for the actual
 * @c Session::Run call. @c predict builds a zero-copy @c Ort::Value over
 * the @ref MLLIB::MlTensor buffer, runs the session, and copies the
 * first output back into an owning tensor. @c save is intentionally a
 * no-op: ONNX graphs are produced by their training framework, not by
 * this thin inference wrapper.
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
#include <QFileInfo>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <stdexcept>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MLLIB;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

#ifdef MNE_USE_ONNXRUNTIME
Ort::Env& MlOnnxModel::ortEnv()
{
    static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "mne-cpp");
    return env;
}
#endif

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MlOnnxModel::MlOnnxModel()
{
}

//=============================================================================================================

MlOnnxModel::~MlOnnxModel()
{
    // Explicit destructor needed so unique_ptr can see the complete ORT types.
}

//=============================================================================================================

MlTensor MlOnnxModel::predict(const MlTensor& input) const
{
#ifdef MNE_USE_ONNXRUNTIME
    if (!m_session) {
        throw std::runtime_error("MlOnnxModel::predict – No ONNX model loaded. Call load() first.");
    }

    // Create input Ort::Value — zero-copy from MlTensor's row-major float buffer
    auto inputShape = input.shape();
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
        *m_memoryInfo,
        const_cast<float*>(input.data()),
        static_cast<size_t>(input.size()),
        inputShape.data(),
        inputShape.size());

    // Build C-string name arrays for Run()
    std::vector<const char*> inputNamePtrs;
    inputNamePtrs.reserve(m_inputNames.size());
    for (const auto& n : m_inputNames)
        inputNamePtrs.push_back(n.c_str());

    std::vector<const char*> outputNamePtrs;
    outputNamePtrs.reserve(m_outputNames.size());
    for (const auto& n : m_outputNames)
        outputNamePtrs.push_back(n.c_str());

    // Run inference
    Ort::RunOptions runOpts;
    auto outputTensors = m_session->Run(
        runOpts,
        inputNamePtrs.data(), &inputTensor, inputNamePtrs.size(),
        outputNamePtrs.data(), outputNamePtrs.size());

    if (outputTensors.empty() || !outputTensors[0].IsTensor()) {
        throw std::runtime_error("MlOnnxModel::predict – Model produced no valid output tensor.");
    }

    // Extract output shape and data — copy into an owning MlTensor
    auto outputInfo = outputTensors[0].GetTensorTypeAndShapeInfo();
    std::vector<int64_t> outputShape = outputInfo.GetShape();
    const float* outputData = outputTensors[0].GetTensorData<float>();

    return MlTensor(outputData, std::move(outputShape));
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
    m_modelPath = path;

    if (!QFileInfo::exists(path)) {
        qWarning() << "MlOnnxModel::load – File does not exist:" << path;
        return false;
    }

    try {
        // Session options — enable all graph optimizations, single intra-op thread for determinism
        Ort::SessionOptions sessionOpts;
        sessionOpts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        sessionOpts.SetIntraOpNumThreads(1);
        sessionOpts.DisableMemPattern();  // reduces peak memory for small models

        // Create session from the ONNX file
        std::string modelPathStd = path.toStdString();
        m_session = std::make_unique<Ort::Session>(ortEnv(), modelPathStd.c_str(), sessionOpts);

        // CPU memory info (reused for every predict call)
        m_memoryInfo = std::make_unique<Ort::MemoryInfo>(
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));

        // Cache input names and shapes
        Ort::AllocatorWithDefaultOptions allocator;
        size_t numInputs = m_session->GetInputCount();
        m_inputNames.clear();
        m_inputShapes.clear();
        m_inputNames.reserve(numInputs);
        m_inputShapes.reserve(numInputs);

        for (size_t i = 0; i < numInputs; ++i) {
            auto namePtr = m_session->GetInputNameAllocated(i, allocator);
            m_inputNames.emplace_back(namePtr.get());

            auto typeInfo = m_session->GetInputTypeInfo(i);
            auto shape = typeInfo.GetTensorTypeAndShapeInfo().GetShape();
            // Replace dynamic dimensions (-1) with 1 for logging; actual shape comes from input tensor
            m_inputShapes.push_back(std::move(shape));
        }

        // Cache output names
        size_t numOutputs = m_session->GetOutputCount();
        m_outputNames.clear();
        m_outputNames.reserve(numOutputs);
        for (size_t i = 0; i < numOutputs; ++i) {
            auto namePtr = m_session->GetOutputNameAllocated(i, allocator);
            m_outputNames.emplace_back(namePtr.get());
        }

        qDebug() << "MlOnnxModel::load – Session created for" << path
                 << "(" << numInputs << "inputs," << numOutputs << "outputs)";
        return true;

    } catch (const Ort::Exception& e) {
        qWarning() << "MlOnnxModel::load – ORT error:" << e.what();
        m_session.reset();
        return false;
    }
#else
    m_modelPath = path;
    qWarning() << "MlOnnxModel::load – Path stored but no ONNX Runtime session created (build with -DUSE_ONNXRUNTIME=ON).";
    return false;
#endif
}

//=============================================================================================================

bool MlOnnxModel::isLoaded() const
{
#ifdef MNE_USE_ONNXRUNTIME
    return m_session != nullptr;
#else
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
