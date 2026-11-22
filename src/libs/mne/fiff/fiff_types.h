//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_types.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     August 2012
 * @brief    Primitive scalar typedefs and forward-compatible aliases backing the FIFF type system.
 *
 * Defines the fixed-width mne_fiff primitives (@c fiff_int_t,
 * @c fiff_float_t, @c fiff_byte_t, @c fiff_double_t, the DAU packed
 * half-precision integer aliases, the Julian-day type, etc.) so that
 * on-disk records preserve the exact byte layout documented by the FIFF
 * specification regardless of host platform. It also publishes a handful
 * of Eigen typedefs (@c MatrixDau16, @c MatrixShort) used by the matrix
 * tag readers in @ref mne::FiffStream and a small set of empty-default
 * sentinel objects (@c defaultMatrixXd, ...) that
 * let callers express "no value provided" on optional arguments without
 * relying on @c std::optional, matching the historical MATLAB-style
 * signatures inherited from the MNE matlab toolbox.
 *
 * Note: the leftover "Old fiff_type declarations - replace them"
 * heading on the previous header line reflects an in-progress cleanup
 * toward strongly typed C++ replacements; new code should prefer the
 * class wrappers (@ref mne::FiffId, @ref mne::FiffChInfo, ...) over the legacy
 * "Rec" struct aliases re-exported for backwards compatibility.
 */

#ifndef FIFF_TYPES_H
#define FIFF_TYPES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_constants.h"
#include "fiff_time.h"
#include "fiff_data_ref.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

static Eigen::MatrixXd defaultMatrixXd = Eigen::MatrixXd::Constant(1, 1, -1);
const static Eigen::MatrixXd defaultConstMatrixXd(0, 0);
const static Eigen::MatrixXi defaultMatrixXi(0, 0);
const static Eigen::VectorXi defaultVectorXi;
const static Eigen::RowVectorXi defaultRowVectorXi;

typedef Eigen::Matrix<std::int16_t, Eigen::Dynamic, Eigen::Dynamic> MatrixDau16;
typedef Eigen::Matrix<short, Eigen::Dynamic, Eigen::Dynamic> MatrixShort;

//=============================================================================================================
// TYPEDEFS Primitive building blocks:
//=============================================================================================================

typedef unsigned char fiff_byte_t;
typedef char fiff_char_t;
typedef std::int16_t fiff_short_t;
typedef std::uint16_t fiff_ushort_t;
typedef std::int32_t fiff_int_t;
typedef std::uint32_t fiff_uint_t;
typedef std::int64_t fiff_long_t;
typedef std::uint64_t fiff_ulong_t;
typedef float fiff_float_t;
typedef double fiff_double_t;
typedef std::uint16_t fiff_dau_pack13_t;
typedef std::uint16_t fiff_dau_pack14_t;
typedef std::int16_t fiff_dau_pack16_t;
typedef std::int32_t fiff_julian_t;
typedef char fiff_data_t; //unsig char instead of void -> avoid void in C++ cause of its undefined behaviour using delete -> this can happen during lots of casting

} //NAMESPACE

#endif // FIFF_TYPES_H
