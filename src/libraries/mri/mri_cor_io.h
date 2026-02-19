//=============================================================================================================
/**
 * @file     mri_cor_io.h
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
 * @brief    MriCorIO class declaration.
 *
 *           Reader for FreeSurfer COR slice files.
 *
 *           COR files are a legacy FreeSurfer volume format where each coronal
 *           slice is stored as a separate file named COR-001 through COR-256.
 *           Each file contains 256×256 unsigned chars at 1mm isotropic resolution.
 *
 *           The coordinate system uses coronal orientation:
 *             - Origin offset: (0.128, -0.128, 0.128) meters
 *             - Rotation:  x → -x,  y → z,  z → y  (coronal to MRI surface RAS)
 *
 *           Ported from make_cor_set() in MNE C mne_make_cor_set by Matti Hamalainen.
 *
 */

#ifndef MRI_COR_IO_H
#define MRI_COR_IO_H

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
 * Reader for FreeSurfer COR slice files.
 *
 * Reads 256 COR-NNN files from a directory and builds per-slice MriSlice
 * structures with coronal orientation coordinate transforms.
 *
 * Ported from make_cor_set() in MNE C mne_make_cor_set by Matti Hamalainen.
 *
 * @brief FreeSurfer COR slice file reader.
 */
class MRISHARED_EXPORT MriCorIO
{
public:
    //=========================================================================================================
    /**
     * Reads COR slice files from a directory.
     *
     * Expects files named COR-001 through COR-256, each containing
     * 256×256 unsigned char pixel data.
     *
     * @param[in]  dir      Path to the directory containing COR-NNN files.
     * @param[out] slices   Vector of MriSlice structures to populate.
     * @param[in]  verbose  If true, print progress information.
     *
     * @return True on success, false on error.
     */
    static bool read(const QString& dir,
                     QVector<MriSlice>& slices,
                     bool verbose = false);
};

} // namespace MRILIB

#endif // MRI_COR_IO_H
