//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fiff_types.h
 * @since 2022
 * @date  February 2026
 * @brief Primitive scalar typedefs and forward-compatible aliases backing the FIFF type system.
 *
 * Defines the fixed-width FIFFLIB primitives (@c fiff_int_t,
 * @c fiff_float_t, @c fiff_byte_t, @c fiff_double_t, the DAU packed
 * half-precision integer aliases, the Julian-day type, etc.) so that
 * on-disk records preserve the exact byte layout documented by the FIFF
 * specification regardless of host platform. It also publishes a handful
 * of Eigen typedefs (@c MatrixDau16, @c MatrixShort) used by the matrix
 * tag readers in @ref FiffStream and a small set of empty-default
 * sentinel objects (@c defaultMatrixXd, @c defaultQStringList, ...) that
 * let callers express ``no value provided'' on optional arguments without
 * relying on @c std::optional, matching the historical MATLAB-style
 * signatures inherited from the MNE matlab toolbox.
 *
 * Note: the leftover ``Old fiff_type declarations - replace them''
 * heading on the previous header line reflects an in-progress cleanup
 * toward strongly typed C++ replacements; new code should prefer the
 * class wrappers (@ref FiffId, @ref FiffChInfo, ...) over the legacy
 * ``Rec'' struct aliases re-exported for backwards compatibility.
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
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QStringList>
#include <QPair>
#include <QVariant>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

const static QStringList defaultQStringList = QStringList();
static Eigen::MatrixXd defaultMatrixXd = Eigen::MatrixXd::Constant(1,1,-1);
const static Eigen::MatrixXd defaultConstMatrixXd(0,0);
const static Eigen::MatrixXi defaultMatrixXi(0,0);
const static Eigen::VectorXi defaultVectorXi;
const static Eigen::RowVectorXi defaultRowVectorXi;
const static QPair<float,float> defaultFloatPair = qMakePair(-1.0f, -1.0f);

typedef Eigen::Matrix<qint16, Eigen::Dynamic, Eigen::Dynamic> MatrixDau16;
typedef Eigen::Matrix<short, Eigen::Dynamic, Eigen::Dynamic> MatrixShort;

//=============================================================================================================
// TYPEDEFS Primitive building blocks:
//=============================================================================================================

typedef unsigned char        fiff_byte_t;
typedef char                 fiff_char_t;
typedef qint16               fiff_short_t;
typedef quint16              fiff_ushort_t;
typedef qint32               fiff_int_t;
typedef quint32              fiff_uint_t;
typedef qint64               fiff_long_t;
typedef quint64              fiff_ulong_t;
typedef float                fiff_float_t;
typedef double               fiff_double_t;
typedef quint16              fiff_dau_pack13_t;
typedef quint16              fiff_dau_pack14_t;
typedef qint16               fiff_dau_pack16_t;
typedef qint32               fiff_julian_t;
typedef char                 fiff_data_t; //unsig char instead of void -> avoid void in C++ cause of its undefined behaviour using delete -> this can happen during lots of casting

//=============================================================================================================
// TYPEDEFS Structured types:
// Note: The old C structs have been replaced by proper C++ classes.
// - fiffTimeRec     -> FiffTime      (fiff_time.h)       - typedefs removed; use FiffTime directly
// - fiffDataRefRec  -> FiffDataRef   (fiff_data_ref.h)   - backward compat typedefs provided
// - fiffTagRec      -> FiffTag       (fiff_tag.h)        - use FiffTag::storageSize() for on-disk header size
// - fiffIdRec       -> FiffId        (fiff_id.h)         - backward compat typedefs below
// - fiffDirEntryRec -> FiffDirEntry  (fiff_dir_entry.h)
// - fiffDigPointRec -> FiffDigPoint  (fiff_dig_point.h)
// - fiffCoordTransRec -> FiffCoordTrans (fiff_coord_trans.h)
// - fiffChPosRec    -> FiffChPos     (fiff_ch_pos.h)
// - fiffSparseMatrixRec -> FiffSparseMatrix (fiff_sparse_matrix.h)
// - fiffDigStringRec, fiff_event_bits, fiff_hpi_coil, fiff_hpi_subsys -> removed (unused)
//=============================================================================================================

//=============================================================================================================
// BACKWARD COMPATIBILITY TYPEDEFS for FiffId (fiff_id.h provides the class)
// The old fiffIdRec struct has been replaced by FiffId.
// These typedefs are kept so that legacy C-compat code compiles without changes.
//=============================================================================================================

// Forward declare FiffId - actual class is in fiff_id.h
class FiffId;

/** @brief Backward-compatible typedef for the old fiffIdRec struct. */
typedef FiffId fiffIdRec;
/** @brief Backward-compatible typedef for the old fiff_id_t. */
typedef FiffId fiff_id_t;
/** @brief Backward-compatible pointer typedef for the old fiffId pointer. */
typedef FiffId* fiffId;

//=============================================================================================================
// BACKWARD COMPATIBILITY TYPEDEFS for FiffDirEntry (fiff_dir_entry.h provides the class)
//=============================================================================================================

// Forward declare FiffDirEntry - actual class is in fiff_dir_entry.h
class FiffDirEntry;

/** @brief Backward-compatible typedef for the old fiffDirEntryRec struct. */
typedef FiffDirEntry fiffDirEntryRec;
/** @brief Backward-compatible typedef for the old fiff_dir_entry_t. */
typedef FiffDirEntry fiff_dir_entry_t;
/** @brief Backward-compatible pointer typedef for the old fiffDirEntry pointer. */
typedef FiffDirEntry* fiffDirEntry;

//=============================================================================================================
// BACKWARD COMPATIBILITY TYPEDEFS for FiffDigPoint (fiff_dig_point.h provides the class)
//=============================================================================================================

// Forward declare FiffDigPoint - actual class is in fiff_dig_point.h
class FiffDigPoint;

/** @brief Backward-compatible typedef for the old fiffDigPointRec struct. */
typedef FiffDigPoint fiffDigPointRec;

#define FIFFM_CHPOS(x) &((x)->chpos)

}//NAMESPACE

#endif // FIFF_TYPES_H
