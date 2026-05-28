//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mri_mgh_io.h
 * @since February 2026
 * @brief FreeSurfer MGH / MGZ volume reader: byte-level decoder for the 284-byte fixed header, voxel buffer, and optional tag footer.
 *
 * MGH is FreeSurfer's native volume container --- a fixed 284-byte
 * big-endian header followed by a column-major voxel buffer and an
 * optional tag footer carrying scan parameters and the path to
 * @c talairach.xfm. MGZ is the same layout wrapped in a single
 * gzip stream; this reader handles both transparently by detecting
 * the @c .mgz suffix and routing the file through zlib's
 * @c MAX_WBITS+16 inflate mode before parsing.
 *
 * Output is materialised into an @ref MriVolData (slice-of-slices
 * representation) using the same per-slice pixel formats the
 * COR.fif writer expects, so the volume can be re-serialised
 * round-trip without a second conversion pass.
 *
 * File format reference:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
 *
 * MGH Header Layout (all big-endian, total 284 bytes):
 *
 * Offset  Size  Field
 * ------  ----  -----
 * 0      4   version (int32) — must be 1
 * 4      4   width (int32) — first image dimension
 * 8      4   height (int32) — second image dimension
 * 12      4   depth (int32) — third image dimension
 * 16      4   nframes (int32) — number of frames
 * 20      4   type (int32) — voxel data type (0=uchar, 1=int, 3=float, 4=short)
 * 24      4   dof (int32) — degrees of freedom
 * 28      2   goodRASflag (int16) — if > 0 the following fields are valid
 * 30     12   spacingX, spacingY, spacingZ (3×float32) — voxel sizes in mm
 * 42     36   direction cosines: xr,xa,xs, yr,ya,ys, zr,za,zs (9×float32)
 * 78     12   c_ras: cr, ca, cs (3×float32) — center RAS coordinates
 * 90    194   unused (padding to byte 284)
 *
 * Image Data (starting at byte 284):
 * width × height × depth × nframes × sizeof(type) bytes
 * Stored in column-major (Fortran) order: x varies fastest.
 *
 * Footer (after image data, optional):
 * Scan parameters: TR (float32), flipAngle (float32), TE (float32),
 * TI (float32), FoV (float32)
 * Tags: type(int32) + length(int32/int64) + data
 * TAG_OLD_SURF_GEOM (20): old surface geometry, length is int32
 * TAG_OLD_MGH_XFORM (30): old transform, length is int32
 * TAG_MGH_XFORM (31): path to talairach.xfm file, length is int64
 *
 * Ported from make_mgh_cor_set() in MNE C mne_make_cor_set by Matti Hamalainen.
 */

#ifndef MRI_MGH_IO_H
#define MRI_MGH_IO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_global.h"
#include "mri_vol_data.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE MRILIB
//=============================================================================================================

namespace MRILIB {

//=============================================================================================================
/**
 * @brief Stateless decoder for FreeSurfer MGH and MGZ volume containers.
 *
 * Parses the 284-byte big-endian header, decodes the column-major voxel
 * buffer for every supported MRI type (uchar, int, float, short) and reads
 * the optional tag footer (scan parameters, @c talairach.xfm path) into an
 * @ref MriVolData. @c .mgz inputs are inflated in-memory through zlib's
 * @c MAX_WBITS+16 gunzip mode before parsing, so callers never need to know
 * whether they are looking at the compressed or uncompressed variant.
 *
 * Format reference:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
 *
 * Ported from @c make_mgh_cor_set() in MNE C @c mne_make_cor_set by Matti
 * Hamalainen.
 */
class MRISHARED_EXPORT MriMghIO
{
public:
    //=========================================================================================================
    /**
     * Reads a FreeSurfer MGH or MGZ file.
     *
     * Parses the header geometry (dimensions, voxel sizes, direction cosines,
     * center RAS), reads voxel data into per-slice MriSlice structures, and
     * extracts footer tags including the Talairach .xfm path.
     *
     * For .mgz files, automatic gzip decompression is performed via zlib.
     *
     * @param[in]  mgzFile         Path to the .mgz or .mgh file.
     * @param[out] volData          MriVolData structure to populate.
     * @param[out] additionalTrans  Additional coordinate transforms found in footer (e.g., Talairach).
     * @param[in]  subjectMriDir   Path to subject's mri/ directory (for resolving relative .xfm paths).
     * @param[in]  verbose         If true, print progress information.
     *
     * @return True on success, false on error.
     */
    static bool read(const QString& mgzFile,
                     MriVolData& volData,
                     QVector<FIFFLIB::FiffCoordTrans>& additionalTrans,
                     const QString& subjectMriDir = QString(),
                     bool verbose = false);

private:
    //=========================================================================================================
    /**
     * Decompresses a .mgz file to raw MGH bytes.
     *
     * @param[in]  mgzFile  Path to the .mgz file.
     * @param[out] rawData  Decompressed byte array.
     *
     * @return True on success.
     */
    static bool decompress(const QString& mgzFile, QByteArray& rawData);

    //=========================================================================================================
    /**
     * Parses the MGH header from raw bytes.
     *
     * @param[in]  data     Raw MGH file data.
     * @param[out] volData  MriVolData to populate with header fields.
     * @param[in]  verbose  Print header info.
     *
     * @return True on success.
     */
    static bool parseHeader(const QByteArray& data, MriVolData& volData, bool verbose);

    //=========================================================================================================
    /**
     * Reads voxel data and builds per-slice MriSlice structures.
     *
     * @param[in]  data     Raw MGH file data.
     * @param[out] volData  MriVolData to populate with slice data.
     *
     * @return True on success.
     */
    static bool readVoxelData(const QByteArray& data, MriVolData& volData);

    //=========================================================================================================
    /**
     * Parses the MGH footer for scan parameters and tags (Talairach .xfm path).
     *
     * @param[in]  data             Raw MGH file data.
     * @param[out] volData          MriVolData to populate with footer data.
     * @param[out] additionalTrans  Coordinate transforms found in tags.
     * @param[in]  subjectMriDir   Path to subject's mri/ directory.
     * @param[in]  verbose          Print info.
     *
     * @return True on success (footer is optional, so missing footer is not an error).
     */
    static bool parseFooter(const QByteArray& data,
                            MriVolData& volData,
                            QVector<FIFFLIB::FiffCoordTrans>& additionalTrans,
                            const QString& subjectMriDir,
                            bool verbose);

    //=========================================================================================================
    /**
     * Returns the number of bytes per voxel for the given MGH data type.
     *
     * @param[in] type  MGH voxel data type (MRI_UCHAR, MRI_SHORT, MRI_INT, MRI_FLOAT).
     *
     * @return Bytes per voxel, or 0 if the type is unsupported.
     */
    static int bytesPerVoxel(int type);
};

} // namespace MRILIB

#endif // MRI_MGH_IO_H
