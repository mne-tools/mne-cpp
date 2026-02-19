//=============================================================================================================
/**
 * @file     mri_cor_fif_io.h
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
 * @brief    MriCorFifIO class declaration.
 *
 *           Writer for COR.fif FIFF files containing MRI slice data.
 *
 *           COR.fif is the Neuromag/MNE convention for storing MRI volume data
 *           in FIFF format. The file structure is:
 *
 *           FIFFB_MRI
 *             FIFF_BLOCK_ID
 *             FIFFB_MRI_SET
 *               FIFF_COORD_TRANS (HEAD -> MRI identity)
 *               [additional coord transforms: Talairach etc.]
 *               FIFFB_MRI_SLICE (repeated for each slice)
 *                 FIFF_MRI_WIDTH
 *                 FIFF_MRI_WIDTH_M
 *                 FIFF_MRI_HEIGHT
 *                 FIFF_MRI_HEIGHT_M
 *                 FIFF_COORD_TRANS (slice -> MRI)
 *                 FIFF_MRI_PIXEL_ENCODING
 *                 FIFF_MRI_PIXEL_DATA
 *               /FIFFB_MRI_SLICE
 *             /FIFFB_MRI_SET
 *           /FIFFB_MRI
 *
 *           Ported from save_slices() / write_slice() in MNE C write_mri_set.c
 *           by Matti Hamalainen.
 *
 */

#ifndef MRI_COR_FIF_IO_H
#define MRI_COR_FIF_IO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_global.h"
#include "mri_vol_data.h"

#include <fiff/fiff_coord_trans.h>

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
 * Writer for COR.fif FIFF files.
 *
 * Writes MRI slice data (COR or MGH-derived) into the FIFF format used by
 * the MNE C tools and MNE-Python for MRI volume representation.
 *
 * Ported from save_slices() / write_slice() in MNE C write_mri_set.c
 * by Matti Hamalainen.
 *
 * @brief COR.fif FIFF file writer.
 */
class MRISHARED_EXPORT MriCorFifIO
{
public:
    //=========================================================================================================
    /**
     * Writes COR.fif file from slice data.
     *
     * @param[in] fileName         Output FIFF file path.
     * @param[in] slices           Vector of MriSlice structures to write.
     * @param[in] additionalTrans  Additional coordinate transforms (Talairach etc.).
     *
     * @return True on success, false on error.
     */
    static bool write(const QString& fileName,
                      const QVector<MriSlice>& slices,
                      const QVector<FIFFLIB::FiffCoordTrans>& additionalTrans);
};

} // namespace MRILIB

#endif // MRI_COR_FIF_IO_H
