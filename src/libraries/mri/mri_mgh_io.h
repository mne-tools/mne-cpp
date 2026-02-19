//=============================================================================================================
/**
 * @file     mri_mgh_io.h
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
 * @brief    MriMghIO class declaration.
 *
 *           Reader for FreeSurfer MGH/MGZ (gzip-compressed MGH) volume files.
 *
 *           File format reference:
 *           https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
 *
 *           MGH Header Layout (all big-endian, total 284 bytes):
 *
 *           Offset  Size  Field
 *           ------  ----  -----
 *              0      4   version (int32) — must be 1
 *              4      4   width (int32) — first image dimension
 *              8      4   height (int32) — second image dimension
 *             12      4   depth (int32) — third image dimension
 *             16      4   nframes (int32) — number of frames
 *             20      4   type (int32) — voxel data type (0=uchar, 1=int, 3=float, 4=short)
 *             24      4   dof (int32) — degrees of freedom
 *             28      2   goodRASflag (int16) — if > 0 the following fields are valid
 *             30     12   spacingX, spacingY, spacingZ (3×float32) — voxel sizes in mm
 *             42     36   direction cosines: xr,xa,xs, yr,ya,ys, zr,za,zs (9×float32)
 *             78     12   c_ras: cr, ca, cs (3×float32) — center RAS coordinates
 *             90    194   unused (padding to byte 284)
 *
 *           Image Data (starting at byte 284):
 *             width × height × depth × nframes × sizeof(type) bytes
 *             Stored in column-major (Fortran) order: x varies fastest.
 *
 *           Footer (after image data, optional):
 *             Scan parameters: TR (float32), flipAngle (float32), TE (float32),
 *             TI (float32), FoV (float32)
 *             Tags: type(int32) + length(int32/int64) + data
 *               TAG_OLD_SURF_GEOM (20): old surface geometry, length is int32
 *               TAG_OLD_MGH_XFORM (30): old transform, length is int32
 *               TAG_MGH_XFORM (31): path to talairach.xfm file, length is int64
 *
 *           Ported from make_mgh_cor_set() in MNE C mne_make_cor_set by Matti Hamalainen.
 *
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
 * Reader for FreeSurfer MGH/MGZ volume files.
 *
 * Reads the header, voxel data, and footer tags from MGH/MGZ files into
 * an MriVolData structure. Automatically decompresses .mgz files using gunzip.
 *
 * Based on the FreeSurfer MGH format specification:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FsTutorial/MghFormat
 *
 * Ported from make_mgh_cor_set() in MNE C mne_make_cor_set by Matti Hamalainen.
 *
 * @brief FreeSurfer MGH/MGZ file reader.
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
     * For .mgz files, automatic gunzip decompression is performed via QProcess.
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
};

} // namespace MRILIB

#endif // MRI_MGH_IO_H
