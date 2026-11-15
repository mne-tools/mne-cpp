//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ml_onnx_model.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    ONNX Runtime backed @ref mne::MlModel implementation for loading and evaluating @c .onnx graphs.
 *
 * @ref mne::MlOnnxModel is the default inference backend in mne_ml:
 * load any model exported to the ONNX format - from PyTorch,
 * scikit-learn (via @c skl2onnx), TensorFlow, or hand-crafted - and
 * predict on @ref mne::MlTensor inputs without any framework runtime
 * other than ONNX Runtime itself. The class owns the @c Ort::Session,
 * a reusable CPU @c Ort::MemoryInfo, and the cached input/output node
 * names and shapes so per-call overhead stays minimal.
 *
 * Each @c predict wraps the input tensor's row-major float buffer in
 * an @c Ort::Value zero-copy, dispatches @c Session::Run, and copies
 * the first output tensor into a fresh @ref mne::MlTensor; multi-IO
 * graphs are supported by the cached name arrays but the public API
 * intentionally exposes a single-in / single-out shape until a use
 * case requires more. When mne-cpp is built without
 * @c USE_ONNXRUNTIME the methods compile to stubs that throw
 * @c std::runtime_error or log and return, so dependent code can be
 * gated at runtime rather than via @c \#ifdef chains.
 */

#ifndef ML_ONNX_MODEL_H
#define ML_ONNX_MODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_global.h"
#include "ml_model.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// FORWARD DECLARE ORT TYPES
//=============================================================================================================

#ifdef MNE_USE_ONNXRUNTIME
namespace Ort
{
class Env;
class Session;
class MemoryInfo;
class RunOptions;
}
#endif

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief @ref MlModel backend that runs @c .onnx graphs through ONNX Runtime with a cached CPU session.
 *
 * When mne-cpp is configured without @c USE_ONNXRUNTIME every method
 * either throws @c std::runtime_error (@c predict) or returns false /
 * logs a warning (@c load, @c save), so the same client code compiles
 * and links on minimal builds and on WebAssembly without conditional
 * call sites.
 *
 * @snippet ex_ml/main.cpp ml_onnx_model_usage
 */
class MLSHARED_EXPORT MlOnnxModel : public MlModel
{
public:
    //=========================================================================================================
    /**
     * Default constructor.
     */
    MlOnnxModel();

    //=========================================================================================================
    /**
     * Destructor.
     */
    ~MlOnnxModel() override;

    //=========================================================================================================
    MlTensor predict(const MlTensor& input) const override;
    bool save(const std::string& path) const override;
    bool load(const std::string& path) override;
    std::string modelType() const override;
    MlTaskType taskType() const override;

    //=========================================================================================================
    /**
     * @return True if an ONNX Runtime session has been loaded and is ready for inference.
     */
    bool isLoaded() const;

    //=========================================================================================================
    /**
     * @param[in] key  Custom metadata key stored in the model file.
     * @return The value stored under @p key, or an empty string if the model has none or is not loaded.
     */
    std::string metadata(const std::string& key) const;

private:
#ifdef MNE_USE_ONNXRUNTIME
    static Ort::Env& ortEnv();
#endif

    std::string m_modelPath;                            /**< Path to ONNX model file. */
    MlTaskType m_taskType = MlTaskType::Classification; /**< Task type.               */

#ifdef MNE_USE_ONNXRUNTIME
    std::unique_ptr<Ort::Session> m_session;         /**< ORT inference session.              */
    std::unique_ptr<Ort::MemoryInfo> m_memoryInfo;   /**< CPU memory info (reused).            */
    std::vector<std::string> m_inputNames;           /**< Cached input node names.             */
    std::vector<std::string> m_outputNames;          /**< Cached output node names.            */
    std::vector<std::vector<int64_t>> m_inputShapes; /**< Cached input node shapes.            */
#endif
};

} // namespace mne

#endif // ML_ONNX_MODEL_H
