//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file ml_types.h
 * @since 2026
 * @date  May 2026
 * @brief Strongly typed enumerations describing the ML backend, tensor element type and inference task.
 *
 * The three @c enum @c class definitions in this header are the shared
 * vocabulary used across every MLLIB interface: @ref MLLIB::MlBackend
 * selects which runtime executes a model (currently ONNX Runtime, with
 * a placeholder @c BuiltIn entry reserved for native fallbacks),
 * @ref MLLIB::MlDataType names the scalar element of @ref MLLIB::MlTensor,
 * and @ref MLLIB::MlTaskType labels what a loaded model is meant to do
 * so calling code can route the output tensor without inspecting the
 * graph (classification logits, regression scalars, embedding vectors).
 *
 * Keeping these as scoped enums avoids the usual implicit-int hazards
 * and lets the header stay dependency-free so any other MLLIB header
 * can pull it in without dragging Eigen or ONNX along.
 */

#ifndef ML_TYPES_H
#define ML_TYPES_H

//=============================================================================================================
// DEFINE NAMESPACE MLLIB
//=============================================================================================================

namespace MLLIB{

//=============================================================================================================
/**
 * Available ML back-end engines.
 */
enum class MlBackend { OnnxRuntime, BuiltIn };

//=============================================================================================================
/**
 * Supported data types for tensors.
 */
enum class MlDataType { Float32, Float64, Int64 };

//=============================================================================================================
/**
 * ML task categories.
 */
enum class MlTaskType { Classification, Regression, FeatureExtraction };

} // namespace MLLIB

#endif // ML_TYPES_H
