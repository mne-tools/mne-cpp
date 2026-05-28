//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mri_types.h
 * @since 2026
 * @date  May 2026
 * @brief Numeric type codes, layout constants, and sentinel values shared by every MRI reader.
 *
 * Centralises the four magic-number families that show up in every
 * MGH/MGZ, COR and (indirectly) NIfTI reader so that the I/O classes
 * themselves stay free of stray integer literals:
 *
 * - @b voxel-type codes (MRI_UCHAR, MRI_INT, MRI_FLOAT, MRI_SHORT)
 * used to decode the @c type field of the MGH header and to
 * dispatch the corresponding QVector storage on the consumer side;
 * - @b frame-selection sentinels (MRI_ALL_FRAMES, MRI_NO_FRAMES)
 * used by the MGH reader API to distinguish "read everything"
 * from "header-only" without overloading the parameter type;
 * - @b format-version (MRI_MGH_VERSION = 1, the only value any
 * FreeSurfer-produced MGH file should ever carry) used as a
 * sanity gate when a candidate file is opened;
 * - @b COR geometry constants (slice size, isotropic 1 mm voxel
 * edge) used to materialise the per-slice coordinate transforms.
 *
 * Format reference:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
 * These constants originate from FreeSurfer's @c mri.h and were
 * ported via MNE C's @c mne_types_mne-c.h by Matti Hamalainen.
 */

#ifndef MRI_TYPES_H
#define MRI_TYPES_H

//=============================================================================================================
// DEFINE NAMESPACE MRILIB
//=============================================================================================================

namespace MRILIB {

//=============================================================================================================
/**
 * @name MGH Format Version
 * @{
 */

/** Current MGH file format version. */
constexpr int MRI_MGH_VERSION   = 1;

/** @} */

//=============================================================================================================
/**
 * @name MGH Voxel Data Types
 *
 * Data type codes for voxels in MGH/MGZ files.
 * From FreeSurfer mri.h and MNE C mne_types_mne-c.h.
 *
 * See: https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
 * "type: data type of the image buffer; can be one of the following:
 *  UCHAR, SHORT, INT, or FLOAT (specified as 0, 4, 1, or 3, respectively)"
 *
 * @{
 */

constexpr int MRI_UCHAR   = 0;     /**< Unsigned char (8-bit). */
constexpr int MRI_INT      = 1;     /**< Signed 32-bit integer. */
constexpr int MRI_LONG     = 2;     /**< Long integer (unused in practice). */
constexpr int MRI_FLOAT    = 3;     /**< 32-bit float. */
constexpr int MRI_SHORT    = 4;     /**< Signed 16-bit short. */
constexpr int MRI_BITMAP   = 5;     /**< Bitmap (unused in practice). */
constexpr int MRI_TENSOR   = 6;     /**< Tensor (unused in practice). */

/** @} */

//=============================================================================================================
/**
 * @name Frame Loading Constants
 * @{
 */

constexpr int MRI_ALL_FRAMES   = -1;    /**< Load all frames. */
constexpr int MRI_NO_FRAMES    = -2;    /**< Do not load data at all. */

/** @} */

//=============================================================================================================
/**
 * @name MGH Header Constants
 *
 * Fixed offsets and sizes in the MGH file header.
 * See: https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
 *
 * @{
 */

/** Fixed byte offset where image data begins (bytes 0-283 are header). */
constexpr int MRI_MGH_DATA_OFFSET = 284;

/** Size of the fixed portion of the header (before RAS info). */
constexpr int MRI_MGH_HEADER_FIXED_SIZE = 30;  // 7*int(4) + 1*short(2)

/** @} */

//=============================================================================================================
/**
 * @name MGH Footer Tag Types
 *
 * Tag identifiers found in the MGH footer after the voxel data.
 * From FreeSurfer tags.h.
 *
 * @{
 */

constexpr int MGH_TAG_OLD_SURF_GEOM     = 20;   /**< Old surface geometry tag. */
constexpr int MGH_TAG_OLD_MGH_XFORM     = 30;   /**< Old MGH transform tag. */
constexpr int MGH_TAG_MGH_XFORM         = 31;   /**< MGH Talairach transform tag (contains path to .xfm file). */

/** @} */

//=============================================================================================================
/**
 * @name COR Slice Constants
 *
 * Fixed dimensions for FreeSurfer COR slice files.
 * COR files contain 256 coronal slices of 256×256 unsigned chars at 1mm isotropic.
 *
 * @{
 */

constexpr int COR_NSLICE       = 256;           /**< Number of COR slices. */
constexpr int COR_WIDTH        = 256;           /**< Width of each COR slice in pixels. */
constexpr int COR_HEIGHT       = 256;           /**< Height of each COR slice in pixels. */
constexpr float COR_PIXEL_SIZE = 1e-3f;         /**< Pixel size for COR slices (1mm in meters). */

/** @} */

} // namespace MRILIB

#endif // MRI_TYPES_H
