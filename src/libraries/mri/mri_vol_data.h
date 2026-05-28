//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mri_vol_data.h
 * @since 2026
 * @date  May 2026
 * @brief Format-agnostic in-memory representation of a 3D MRI volume plus its slice decomposition.
 *
 * Two cooperating types live in this header:
 *
 * - @ref MriSlice --- a single 2D slice with its own pixel buffer
 * (byte / word / float, picked at load time to mirror the
 * on-disk @c FIFFV_MRI_PIXEL_* encoding) and an explicit
 * slice-to-MRI (surface RAS) @ref FIFFLIB::FiffCoordTrans.
 * This is the unit the rendering pipeline and the COR.fif
 * writer consume, so every loader (MGH, NIfTI, raw COR)
 * ultimately decomposes its 3D buffer into a vector of these.
 * - @ref MriVolData --- the full volume bundle: header geometry
 * (width/height/depth, voxel spacing, direction cosines,
 * RAS centre), optional scan parameters (TR / TE / flip-angle
 * / FoV), and the slice vector above. It owns the
 * @c voxToSurfRAS(), @c voxToTalairach() and inverse
 * transforms derived from the MGH @c Mdc / @c c_ras header
 * fields and any additional transforms attached by the
 * source-file reader (e.g. talairach.xfm).
 *
 * The @c read() convenience method dispatches by file suffix to
 * the matching loader (@ref MriMghIO, @ref MriNiftiIO, COR
 * directory) so application code can stay one-liner clean
 * irrespective of the on-disk format --- the equivalent of
 * @c nibabel.load() on the Python side.
 *
 * Header reference:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
 * Ported from @c mneMRIdataRec / @c mneMRIvolumeRec in MNE C
 * (@c mne_types_mne-c.h) by Matti Hamalainen.
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
 * @brief Single 2D MRI slice (pixels + slice→RAS transform) used as the volume's storage unit.
 *
 * Mirrors the on-disk @c FIFFB_MRI_SLICE record so that round-tripping
 * through COR.fif is lossless: each slice owns its pixel buffer in the
 * source's native @c FIFFV_MRI_PIXEL_BYTE / @c _WORD / @c _FLOAT encoding,
 * its width/height in pixels and millimetres, and an explicit
 * @ref FIFFLIB::FiffCoordTrans mapping slice (column, row) to MRI surface
 * RAS. The slicing pipeline and the COR.fif writer consume slices directly
 * --- every format-specific loader (MGH, NIfTI, raw COR) builds its volume
 * as a vector of these.
 *
 * Ported from @c mriSliceRec in the original MNE C @c mne_make_cor_set.
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
 * @brief Format-agnostic 3D MRI volume: header geometry, voxel buffer (as a vector of @ref MriSlice), scan parameters and provenance.
 *
 * The volume is the single in-memory representation every consumer in
 * mne-cpp operates on, regardless of whether it was loaded from MGH/MGZ,
 * NIfTI-1 or a FreeSurfer COR directory. Storage is decomposed into a
 * vector of @ref MriSlice so the COR.fif writer can serialise without an
 * intermediate copy, while the slice viewer and the BEM coregistration
 * step both index it through the same (column, row, slice) coordinate.
 *
 * The header captures FreeSurfer's MGH conventions: voxel sizes
 * (@c spacingX/Y/Z), direction cosines (@c Mdc, column-major), centre RAS
 * (@c c_ras) and voxel data type (@c MRI_UCHAR / @c MRI_INT / @c MRI_FLOAT
 * / @c MRI_SHORT). Optional scan parameters (TR, flip angle, TE, TI, FoV)
 * and tags (Talairach transform path, provenance) round out the structure.
 * The @c read() entry point dispatches on the file extension so callers do
 * not need to branch on format.
 *
 * Format reference:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
 *
 * Ported from @c mneMRIdataRec in MNE C (@c mne_types_mne-c.h) by Matti
 * Hamalainen.
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
     * Convenience loader: reads an MGH/MGZ file and populates this volume.
     *
     * Wraps MriMghIO::read() so callers don't need to manage a separate
     * additionalTrans vector.  Mirrors MNE-Python's
     * ``nib.load(path)`` one-liner.
     *
     * @param[in] path  Path to the .mgh or .mgz file.
     *
     * @return True on success, false on error.
     */
    bool read(const QString& path);

    //=========================================================================================================
    /**
     * Returns whether this volume contains valid data.
     *
     * @return True if the volume has been loaded successfully.
     */
    bool isValid() const;

    //=========================================================================================================
    /**
     * @return First (x) dimension — fastest-varying axis.
     */
    int dimX() const { return width; }

    //=========================================================================================================
    /**
     * @return Second (y) dimension.
     */
    int dimY() const { return height; }

    //=========================================================================================================
    /**
     * @return Third (z) dimension — slowest-varying axis.
     */
    int dimZ() const { return depth; }

    //=========================================================================================================
    /**
     * @return Volume dimensions as {dimX, dimY, dimZ}.
     */
    QVector<int> dims() const { return {width, height, depth}; }

    //=========================================================================================================
    /**
     * Returns all voxel data as a contiguous float array in x-fastest order.
     *
     * Mirrors MNE-Python's ``img.get_fdata().ravel(order='F')``.  Regardless
     * of the on-disk type (UCHAR, SHORT, INT, FLOAT) the output is always
     * float.  The array length is ``width * height * depth``.
     *
     * @return Flat voxel data; empty vector if the volume has no slices.
     */
    QVector<float> voxelDataAsFloat() const;

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
    Eigen::Vector3f  x_ras;    /**< X-direction cosines (xr, xa, xs). Default: (-1, 0, 0). */
    Eigen::Vector3f  y_ras;    /**< Y-direction cosines (yr, ya, ys). Default: (0, 0, -1). */
    Eigen::Vector3f  z_ras;    /**< Z-direction cosines (zr, za, zs). Default: (0, 1, 0). */
    Eigen::Vector3f  c_ras;    /**< Center RAS coordinates (cr, ca, cs). Default: (0, 0, 0). */

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
