//=============================================================================================================
/**
 * @file     mri_vol_data.h
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
 * @brief    MriVolData class declaration.
 *
 *           Holds loaded MRI volume data from FreeSurfer MGH/MGZ files,
 *           including header geometry, voxel-to-RAS transforms, and voxel data.
 *
 *           Based on the FreeSurfer MGH format specification:
 *           https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
 *
 *           Ported from mneMRIdataRec / mneMRIvolumeRec in MNE C (mne_types_mne-c.h)
 *           by Matti Hamalainen.
 *
 */

#ifndef MRI_VOL_DATA_H
#define MRI_VOL_DATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_global.h"
#include "mri_types.h"

#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QVector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MRILIB
//=============================================================================================================

namespace MRILIB {

//=============================================================================================================
/**
 * Holds data for a single MRI slice within a volume.
 *
 * Used for COR-format representation: each slice is a 2D image with
 * associated pixel data and a coordinate transform from slice coordinates
 * to MRI (surface RAS) coordinates.
 *
 * Ported from mriSliceRec in the original MNE C mne_make_cor_set.
 *
 * @brief Single MRI slice data.
 */
struct MRISHARED_EXPORT MriSlice
{
    QString                     fileName;       /**< Source file name. */
    QVector<unsigned char>      pixels;         /**< Pixel data (unsigned char), for FIFFV_MRI_PIXEL_BYTE. */
    QVector<unsigned short>     pixelsWord;     /**< Pixel data (unsigned short), for FIFFV_MRI_PIXEL_WORD. */
    QVector<float>              pixelsFloat;    /**< Pixel data (float), for FIFFV_MRI_PIXEL_FLOAT. */
    int                         pixelFormat;    /**< Pixel format: FIFFV_MRI_PIXEL_BYTE(1), WORD(2), FLOAT(4). */
    int                         width;          /**< Width of the image in pixels. */
    int                         height;         /**< Height of the image in pixels. */
    float                       dimx;           /**< Pixel size in x direction (meters). */
    float                       dimy;           /**< Pixel size in y direction (meters). */
    float                       scale;          /**< Scaling factor for pixel data. */
    FIFFLIB::FiffCoordTrans     trans;          /**< Coordinate transform: slice -> MRI (surface RAS). */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    MriSlice()
    : pixelFormat(1)    // FIFFV_MRI_PIXEL_BYTE
    , width(0)
    , height(0)
    , dimx(COR_PIXEL_SIZE)
    , dimy(COR_PIXEL_SIZE)
    , scale(1.0f)
    {}
};

//=============================================================================================================
/**
 * Holds a complete MRI volume loaded from a FreeSurfer MGH/MGZ file.
 *
 * This class encapsulates all header geometry, coordinate transforms,
 * scan parameters, and voxel data that describe a 3D MRI volume.
 *
 * Based on the FreeSurfer MGH format specification:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
 *
 * Header structure (all big-endian):
 *   - version (int32): current value is 1
 *   - width (int32): first dimension (fastest-varying)
 *   - height (int32): second dimension
 *   - depth (int32): third dimension (slowest)
 *   - nframes (int32): number of scalar components per voxel
 *   - type (int32): voxel data type (MRI_UCHAR=0, MRI_INT=1, MRI_FLOAT=3, MRI_SHORT=4)
 *   - dof (int32): degrees of freedom
 *   - goodRASflag (int16): if true, direction cosines follow
 *   - spacingX/Y/Z (float32): voxel sizes in mm
 *   - Mdc (9×float32): direction cosine matrix (column-major)
 *   - c_ras (3×float32): center RAS coordinates
 *   - (padding to byte 284)
 *
 * Image data starts at byte 284.
 *
 * Footer (after data): optional scan parameters (TR, FlipAngle, TE, TI, FoV)
 * and tags (Talairach transform path, provenance info).
 *
 * Ported from mneMRIdataRec in MNE C (mne_types_mne-c.h) by Matti Hamalainen.
 *
 * @brief MRI volume data from FreeSurfer MGH/MGZ file.
 */
class MRISHARED_EXPORT MriVolData
{
public:
    //=========================================================================================================
    /**
     * Default constructor.
     */
    MriVolData();

    //=========================================================================================================
    /**
     * Returns whether this volume contains valid data.
     *
     * @return True if the volume has been loaded successfully.
     */
    bool isValid() const;

    //=========================================================================================================
    /**
     * Builds the voxel-to-surface-RAS (MRI) 4×4 transform matrix.
     *
     * Following FreeSurfer convention:
     *   M = Mdc * diag(xsize, ysize, zsize)
     *   P0 = c_ras - M * (dim/2)
     *   vox2ras = | M   P0 |  (in mm, converted to meters for FIFF)
     *             | 0    1 |
     *
     * @return The 4×4 voxel-to-surface-RAS transform in meters.
     */
    Eigen::Matrix4f computeVox2Ras() const;

    //=========================================================================================================
    // MGH Header Fields
    //=========================================================================================================

    QString     fileName;           /**< Name of the source file. */
    int         version;            /**< MGH format version (should be 1). */
    int         width;              /**< First dimension of the image buffer (fastest). */
    int         height;             /**< Second dimension. */
    int         depth;              /**< Third dimension (slowest). */
    int         nframes;            /**< Number of frames (scalar components per voxel). */
    int         type;               /**< Voxel data type (MRI_UCHAR, MRI_INT, MRI_FLOAT, MRI_SHORT). */
    int         dof;                /**< Degrees of freedom. */
    bool        rasGood;            /**< Whether the direction cosines in the header are valid. */
    float       xsize;              /**< Voxel spacing in X direction (mm). */
    float       ysize;              /**< Voxel spacing in Y direction (mm). */
    float       zsize;              /**< Voxel spacing in Z direction (mm). */
    float       x_ras[3];           /**< X-direction cosines (xr, xa, xs). Default: (-1, 0, 0). */
    float       y_ras[3];           /**< Y-direction cosines (yr, ya, ys). Default: (0, 0, -1). */
    float       z_ras[3];           /**< Z-direction cosines (zr, za, zs). Default: (0, 1, 0). */
    float       c_ras[3];           /**< Center RAS coordinates (cr, ca, cs). Default: (0, 0, 0). */

    //=========================================================================================================
    // Coordinate Transforms
    //=========================================================================================================

    FIFFLIB::FiffCoordTrans  voxelSurfRasT;      /**< Voxel -> surface RAS (MRI) transform. */

    //=========================================================================================================
    // Optional Footer Data (scan parameters)
    //=========================================================================================================

    float       TR;                 /**< Repetition time (ms). */
    float       flipAngle;          /**< Flip angle (radians). */
    float       TE;                 /**< Echo time (ms). */
    float       TI;                 /**< Inversion time (ms). */
    float       FoV;                /**< Field of view (unreliable per FreeSurfer docs). */

    //=========================================================================================================
    // Talairach Transform
    //=========================================================================================================

    QString     talairachXfmPath;   /**< Path to the Talairach .xfm file (from MGH footer tags). */

    //=========================================================================================================
    // Slice Data
    //=========================================================================================================

    QVector<MriSlice> slices;       /**< Per-slice data (for COR-equivalent representation). */
};

} // namespace MRILIB

#endif // MRI_VOL_DATA_H
