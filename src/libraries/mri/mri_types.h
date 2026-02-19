//=============================================================================================================
/**
 * @file     mri_types.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    MRI type definitions and constants.
 *
 *           Constants and type definitions for handling FreeSurfer MRI data
 *           in MGH/MGZ format. Based on the FreeSurfer MGH format specification:
 *           https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
 *
 *           Originally defined in MNE C (mne_types_mne-c.h) by Matti Hamalainen.
 *
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
