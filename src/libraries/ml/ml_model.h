//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ml_model.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Pure-virtual @ref mne::MlModel interface that every mne_ml inference backend implements.
 *
 * @ref mne::MlModel is the polymorphic seam between mne-cpp
 * application code and the underlying runtime that actually executes a
 * graph (ONNX Runtime today, potentially LibTorch or a native kernel
 * tomorrow). The interface intentionally exposes only four operations
 * - @c predict, @c save, @c load and the @c modelType / @c taskType
 * descriptors - so any backend can be slotted in without leaking
 * runtime-specific types into the public API.
 *
 * Inputs and outputs flow through @ref mne::MlTensor, the shared
 * N-dimensional float32 carrier, which gives ONNX Runtime a zero-copy
 * path into its own tensor representation while keeping Eigen-based
 * pre/post-processing in mne-cpp idiomatic. Implementations are
 * shared through @c std::shared_ptr, so
 * models can be parked on UI threads and invoked from worker threads
 * without ownership questions.
 */

#ifndef ML_MODEL_H
#define ML_MODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_global.h"
#include "ml_types.h"
#include "ml_tensor.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Backend-agnostic inference interface: load, predict, save plus model/task descriptors.
 *
 * @snippet ex_ml/main.cpp ml_onnx_model_usage
 */
class MLSHARED_EXPORT MlModel
{
public:
    //=========================================================================================================
    /**
     * Virtual destructor.
     */
    virtual ~MlModel() = default;

    //=========================================================================================================
    /**
     * Run inference on the given input tensor.
     *
     * @param[in] input   The input data.
     * @return The prediction result.
     */
    virtual MlTensor predict(const MlTensor& input) const = 0;

    //=========================================================================================================
    /**
     * Serialise the model to disk.
     *
     * @param[in] path   File path.
     * @return True if successful.
     */
    virtual bool save(const std::string& path) const = 0;

    //=========================================================================================================
    /**
     * Load a model from disk.
     *
     * @param[in] path   File path.
     * @return True if successful.
     */
    virtual bool load(const std::string& path) = 0;

    //=========================================================================================================
    /**
     * @return Human-readable model type name.
     */
    virtual std::string modelType() const = 0;

    //=========================================================================================================
    /**
     * @return The task type this model is configured for.
     */
    virtual MlTaskType taskType() const = 0;
};

} // namespace mne

#endif // ML_MODEL_H
