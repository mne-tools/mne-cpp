//=============================================================================================================
/**
 * @file     ml_tensor.h
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
 * @brief    MlTensor class declaration — N-dimensional, row-major, zero-copy.
 *
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
 * @brief N-dimensional tensor with contiguous row-major (C-order) float32 storage.
 *
 * Data is held in a reference-counted buffer so that copy, reshape and
 * slice are O(1).  A non-owning "view" mode lets the tensor wrap
 * external memory (e.g. memory-mapped files, CUDA device pointers)
 * without any allocation.
 *
 * Storage layout is row-major to match ONNX Runtime, PyTorch and NumPy
 * conventions.  Eigen interop is provided through Map accessors that
 * expose the data without copying.
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
