//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file ml_tensor.cpp
 * @since 2026
 * @date  May 2026
 * @brief Implementation of @ref MLLIB::MlTensor including row-major/Eigen interop and shared-buffer reshape.
 *
 * The translation unit keeps the storage rule in one place: every
 * owning constructor allocates a @c std::shared_ptr<std::vector<float>>
 * sized from the product of the shape, copies (or moves) the caller's
 * buffer in, and points @c m_data at the shared vector. The static
 * @ref MLLIB::MlTensor::view factory bypasses that allocation and
 * leaves @c m_storage null so @ref MLLIB::MlTensor::isView can still
 * distinguish the two cases later.
 *
 * Eigen interop is handled with @c Eigen::Map: row-major maps over the
 * tensor's own buffer for in-place access, and assignment from a
 * column-major @c MatrixXf / @c MatrixXd transposes the layout in a
 * single Eigen expression. @ref MLLIB::MlTensor::reshape simply
 * clones the @c shared_ptr and rewrites the shape vector, which is why
 * any number of reshaped or sliced tensors can share one allocation
 * without lifetime headaches.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_tensor.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstring>
#include <numeric>
#include <stdexcept>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MLLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

int64_t MlTensor::computeSize(const std::vector<int64_t>& shape)
{
    if (shape.empty())
        return 0;
    return std::accumulate(shape.begin(), shape.end(),
                           int64_t(1), std::multiplies<int64_t>());
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MlTensor::MlTensor()
: m_storage(nullptr)
, m_data(nullptr)
, m_shape()
, m_size(0)
{
}

//=============================================================================================================

MlTensor::MlTensor(std::vector<float>&& data, std::vector<int64_t> shape)
: m_shape(std::move(shape))
, m_size(computeSize(m_shape))
{
    if (static_cast<int64_t>(data.size()) != m_size) {
        throw std::invalid_argument("MlTensor: buffer size does not match shape");
    }
    m_storage = std::make_shared<std::vector<float>>(std::move(data));
    m_data = m_storage->data();
}

//=============================================================================================================

MlTensor::MlTensor(const float* data, std::vector<int64_t> shape)
: m_shape(std::move(shape))
, m_size(computeSize(m_shape))
{
    m_storage = std::make_shared<std::vector<float>>(data, data + m_size);
    m_data = m_storage->data();
}

//=============================================================================================================

MlTensor::MlTensor(const MatrixXf& mat)
: m_shape({mat.rows(), mat.cols()})
, m_size(mat.size())
{
    m_storage = std::make_shared<std::vector<float>>(static_cast<size_t>(m_size));
    m_data = m_storage->data();
    // Copy column-major Eigen → row-major contiguous storage (one memop)
    Map<RowMajorMatrixXf>(m_data, mat.rows(), mat.cols()) = mat;
}

//=============================================================================================================

MlTensor::MlTensor(const MatrixXd& mat)
: m_shape({mat.rows(), mat.cols()})
, m_size(mat.size())
{
    m_storage = std::make_shared<std::vector<float>>(static_cast<size_t>(m_size));
    m_data = m_storage->data();
    Map<RowMajorMatrixXf>(m_data, mat.rows(), mat.cols()) = mat.cast<float>();
}

//=============================================================================================================

MlTensor MlTensor::view(float* data, std::vector<int64_t> shape)
{
    MlTensor t;
    t.m_shape = std::move(shape);
    t.m_size  = computeSize(t.m_shape);
    t.m_storage = nullptr;     // non-owning
    t.m_data    = data;
    return t;
}

//=============================================================================================================

MlTensor MlTensor::fromBuffer(const float* data, int rows, int cols)
{
    return MlTensor(data, {static_cast<int64_t>(rows), static_cast<int64_t>(cols)});
}

//=============================================================================================================

int MlTensor::ndim() const
{
    return static_cast<int>(m_shape.size());
}

//=============================================================================================================

int64_t MlTensor::size() const
{
    return m_size;
}

//=============================================================================================================

const std::vector<int64_t>& MlTensor::shape() const
{
    return m_shape;
}

//=============================================================================================================

int64_t MlTensor::shape(int dim) const
{
    if (dim < 0)
        dim += ndim();
    assert(dim >= 0 && dim < ndim());
    return m_shape[static_cast<size_t>(dim)];
}

//=============================================================================================================

int MlTensor::rows() const
{
    assert(ndim() >= 1);
    return static_cast<int>(m_shape[0]);
}

//=============================================================================================================

int MlTensor::cols() const
{
    assert(ndim() >= 2);
    return static_cast<int>(m_shape[1]);
}

//=============================================================================================================

float* MlTensor::data()
{
    return m_data;
}

//=============================================================================================================

const float* MlTensor::data() const
{
    return m_data;
}

//=============================================================================================================

MlTensor::RowMajorMatrixMap MlTensor::matrix()
{
    assert(ndim() == 2);
    return RowMajorMatrixMap(m_data, m_shape[0], m_shape[1]);
}

//=============================================================================================================

MlTensor::ConstRowMajorMatrixMap MlTensor::matrix() const
{
    assert(ndim() == 2);
    return ConstRowMajorMatrixMap(m_data, m_shape[0], m_shape[1]);
}

//=============================================================================================================

MatrixXf MlTensor::toMatrixXf() const
{
    assert(ndim() == 2);
    // Assign row-major map to column-major MatrixXf (Eigen transposes layout)
    return Map<const RowMajorMatrixXf>(m_data, m_shape[0], m_shape[1]);
}

//=============================================================================================================

MatrixXd MlTensor::toMatrixXd() const
{
    assert(ndim() == 2);
    return Map<const RowMajorMatrixXf>(m_data, m_shape[0], m_shape[1]).cast<double>();
}

//=============================================================================================================

MlTensor MlTensor::reshape(std::vector<int64_t> newShape) const
{
    int64_t newSize = computeSize(newShape);
    if (newSize != m_size) {
        throw std::invalid_argument("MlTensor::reshape: new shape size differs from current");
    }

    MlTensor t;
    t.m_storage = m_storage;   // share ownership (or null for views)
    t.m_data    = m_data;
    t.m_shape   = std::move(newShape);
    t.m_size    = newSize;
    return t;
}

//=============================================================================================================

bool MlTensor::isView() const
{
    return m_storage == nullptr && m_data != nullptr;
}

//=============================================================================================================

bool MlTensor::empty() const
{
    return m_size == 0;
}
