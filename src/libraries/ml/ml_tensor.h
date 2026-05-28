//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file ml_tensor.h
 * @since 2026
 * @date  May 2026
 * @brief N-dimensional, row-major, reference-counted float32 tensor used as the universal MLLIB data carrier.
 *
 * @ref MLLIB::MlTensor is the interchange type that crosses every
 * MLLIB boundary: feature matrices going into a classifier, batched
 * activations between pre-processing and inference, prediction tensors
 * coming back from ONNX Runtime. The storage is contiguous row-major
 * float32 wrapped in a @c std::shared_ptr buffer so copies, reshapes
 * and slices are all O(1); a non-owning @c view mode lets the same
 * type wrap memory-mapped files, ORT-allocated buffers or external
 * device pointers without an extra allocation.
 *
 * The row-major layout deliberately matches ONNX Runtime, PyTorch and
 * NumPy so handing a tensor to ORT is a pure pointer-and-shape cast,
 * and Eigen interop is exposed through @c Eigen::Map accessors that
 * read/write the same buffer in place. Convenience constructors copy
 * (and, where needed, transpose) from Eigen's column-major
 * @c MatrixXf / @c MatrixXd so existing mne-cpp pipelines can feed
 * data in without restructuring their math code.
 */

#ifndef ML_TENSOR_H
#define ML_TENSOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE MLLIB
//=============================================================================================================

namespace MLLIB{

//=============================================================================================================
/**
 * @brief N-dimensional row-major float32 tensor with shared-buffer storage, Eigen Map accessors and a non-owning view mode.
 *
 * The shape is an arbitrary @c std::vector<int64_t> (matching ONNX
 * Runtime), the buffer is a reference-counted @c std::vector<float>
 * (or external memory in view mode), and copy / reshape / slice all
 * stay O(1) by sharing the underlying storage. Eigen Map accessors
 * give in-place row-major views of 2-D tensors; @c toMatrixXf /
 * @c toMatrixXd produce column-major Eigen copies for code that needs
 * the native Eigen layout.
 */
class MLSHARED_EXPORT MlTensor
{
public:
    //  --- type aliases used in the public API --------------------------------
    using RowMajorMatrixXf      = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    using RowMajorMatrixMap     = Eigen::Map<RowMajorMatrixXf>;
    using ConstRowMajorMatrixMap = Eigen::Map<const RowMajorMatrixXf>;

    //=========================================================================================================
    /**
     * Default constructor – creates an empty 0-element tensor with shape {}.
     */
    MlTensor();

    //=========================================================================================================
    /**
     * Construct from a moved data buffer and an arbitrary shape.
     * The buffer size must equal the product of shape elements.
     *
     * @param[in] data   Flat float32 buffer (row-major order). Moved in.
     * @param[in] shape  Dimension sizes (e.g. {batch, seq, features}).
     */
    MlTensor(std::vector<float>&& data, std::vector<int64_t> shape);

    //=========================================================================================================
    /**
     * Construct by copying from a raw pointer and a shape.
     *
     * @param[in] data   Pointer to contiguous float32 data (row-major).
     * @param[in] shape  Dimension sizes.
     */
    MlTensor(const float* data, std::vector<int64_t> shape);

    //=========================================================================================================
    /**
     * Construct from an Eigen column-major MatrixXf.
     * The data is copied and transposed into row-major layout.
     * Shape is set to {rows, cols}.
     *
     * @param[in] mat   Source matrix (column-major).
     */
    explicit MlTensor(const Eigen::MatrixXf& mat);

    //=========================================================================================================
    /**
     * Construct from an Eigen column-major MatrixXd.
     * The data is cast to float32 and stored in row-major layout.
     * Shape is set to {rows, cols}.
     *
     * @param[in] mat   Source matrix (column-major, double precision).
     */
    explicit MlTensor(const Eigen::MatrixXd& mat);

    //=========================================================================================================
    /**
     * Create a non-owning view over external mutable memory.
     * The caller is responsible for keeping the buffer alive for the
     * lifetime of this tensor (and any copies / reshapes derived from it).
     *
     * @param[in] data   Pointer to external float32 data (row-major).
     * @param[in] shape  Dimension sizes.
     * @return A non-owning MlTensor.
     */
    static MlTensor view(float* data, std::vector<int64_t> shape);

    //=========================================================================================================
    /**
     * Copy raw data into a new owning tensor (legacy 2-D helper).
     *
     * @param[in] data   Pointer to row-major float data.
     * @param[in] rows   Number of rows.
     * @param[in] cols   Number of columns.
     * @return Newly constructed tensor with shape {rows, cols}.
     */
    static MlTensor fromBuffer(const float* data, int rows, int cols);

    //  --- shape access -------------------------------------------------------

    //=========================================================================================================
    /**
     * @return Number of dimensions.
     */
    int ndim() const;

    //=========================================================================================================
    /**
     * @return Total number of elements (product of all dimensions).
     */
    int64_t size() const;

    //=========================================================================================================
    /**
     * @return Const reference to the full shape vector.
     */
    const std::vector<int64_t>& shape() const;

    //=========================================================================================================
    /**
     * @param[in] dim   Dimension index (negative indices count from the end).
     * @return Size of the requested dimension.
     */
    int64_t shape(int dim) const;

    //=========================================================================================================
    /**
     * 2-D convenience — equivalent to shape(0). Asserts ndim >= 1.
     * @return Number of rows.
     */
    int rows() const;

    //=========================================================================================================
    /**
     * 2-D convenience — equivalent to shape(1). Asserts ndim >= 2.
     * @return Number of columns.
     */
    int cols() const;

    //  --- raw data access (zero-copy) ----------------------------------------

    //=========================================================================================================
    /**
     * @return Mutable pointer to the contiguous float32 buffer.
     */
    float* data();

    //=========================================================================================================
    /**
     * @return Const pointer to the contiguous float32 buffer.
     */
    const float* data() const;

    //  --- Eigen Map accessors (zero-copy, 2-D) -------------------------------

    //=========================================================================================================
    /**
     * Mutable row-major Eigen::Map for 2-D tensors.
     * Asserts ndim == 2.
     *
     * @return Row-major mutable Map.
     */
    RowMajorMatrixMap matrix();

    //=========================================================================================================
    /**
     * Const row-major Eigen::Map for 2-D tensors.
     * Asserts ndim == 2.
     *
     * @return Row-major const Map.
     */
    ConstRowMajorMatrixMap matrix() const;

    //  --- Eigen copy helpers (produce column-major copies) -------------------

    //=========================================================================================================
    /**
     * @return Column-major float copy of the 2-D data. Asserts ndim == 2.
     */
    Eigen::MatrixXf toMatrixXf() const;

    //=========================================================================================================
    /**
     * @return Column-major double copy of the 2-D data. Asserts ndim == 2.
     */
    Eigen::MatrixXd toMatrixXd() const;

    //  --- reshape / query ----------------------------------------------------

    //=========================================================================================================
    /**
     * Return a tensor that shares the same storage but has a different shape.
     * The total element count must be unchanged.  If this tensor owns its
     * data the result shares ownership (reference-counted).  If this tensor
     * is a view, the result is also a view into the same external buffer.
     *
     * @param[in] newShape   New shape (product must equal size()).
     * @return Reshaped tensor (zero-copy).
     */
    MlTensor reshape(std::vector<int64_t> newShape) const;

    //=========================================================================================================
    /**
     * @return True if this tensor does not own its data buffer.
     */
    bool isView() const;

    //=========================================================================================================
    /**
     * @return True if the tensor contains zero elements.
     */
    bool empty() const;

private:
    static int64_t computeSize(const std::vector<int64_t>& shape);

    std::shared_ptr<std::vector<float>> m_storage;  /**< Ref-counted owned buffer (null for views). */
    float*              m_data  = nullptr;           /**< Always points to valid data (owned or external). */
    std::vector<int64_t> m_shape;                    /**< Dimension sizes. */
    int64_t             m_size  = 0;                 /**< Cached total element count. */
};

} // namespace MLLIB

#endif // ML_TENSOR_H
