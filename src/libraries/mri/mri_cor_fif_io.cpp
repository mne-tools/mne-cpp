//=============================================================================================================
/**
 * @file     mri_cor_fif_io.cpp
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
 * @brief    MriCorFifIO class definition.
 *
 *           Writer for COR.fif FIFF files.
 *           Ported from save_slices() / write_slice() in MNE C write_mri_set.c
 *           by Matti Hamalainen.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_cor_fif_io.h"

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_file.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MRILIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool MriCorFifIO::write(const QString& fileName,
                        const QVector<MriSlice>& slices,
                        const QVector<FiffCoordTrans>& additionalTrans)
{
    //
    // File structure (ported from save_slices() / write_slice() in write_mri_set.c):
    //
    //   FIFFB_MRI
    //     FIFF_BLOCK_ID
    //     FIFFB_MRI_SET
    //       HEAD -> MRI identity transform
    //       Additional transforms (Talairach etc.)
    //       For each slice:
    //         FIFFB_MRI_SLICE
    //           FIFF_MRI_WIDTH
    //           FIFF_MRI_WIDTH_M
    //           FIFF_MRI_HEIGHT
    //           FIFF_MRI_HEIGHT_M
    //           CoordTrans (slice -> MRI)
    //           FIFF_MRI_PIXEL_ENCODING
    //           FIFF_MRI_PIXEL_DATA
    //         /FIFFB_MRI_SLICE
    //     /FIFFB_MRI_SET
    //   /FIFFB_MRI
    //

    QFile file(fileName);
    FiffStream::SPtr stream = FiffStream::start_file(file);

    if (!stream) {
        qCritical() << "MriCorFifIO::write - Could not open file for writing:" << fileName;
        return false;
    }

    // Start MRI block
    stream->start_block(FIFFB_MRI);
    stream->write_id(FIFF_BLOCK_ID);

    // Start MRI set block
    stream->start_block(FIFFB_MRI_SET);

    // Write identity HEAD -> MRI transform
    // Ported from write_mri_set.c: identity rotation, zero translation
    Matrix3f identityRot = Matrix3f::Identity();
    Vector3f zeroMove = Vector3f::Zero();
    FiffCoordTrans headMriT(
        FIFFV_COORD_HEAD, FIFFV_COORD_MRI, identityRot, zeroMove);
    stream->write_coord_trans(headMriT);

    // Write additional transforms (Talairach etc.)
    for (const FiffCoordTrans& t : additionalTrans) {
        stream->write_coord_trans(t);
    }

    // Write each slice
    for (int k = 0; k < slices.size(); ++k) {
        const MriSlice& slice = slices[k];

        stream->start_block(FIFFB_MRI_SLICE);

        // Width and height in pixels
        fiff_int_t w = slice.width;
        fiff_int_t h = slice.height;
        stream->write_int(FIFF_MRI_WIDTH, &w);

        float wm = static_cast<float>(slice.width) * slice.dimx;
        stream->write_float(FIFF_MRI_WIDTH_M, &wm);

        stream->write_int(FIFF_MRI_HEIGHT, &h);

        float hm = static_cast<float>(slice.height) * slice.dimy;
        stream->write_float(FIFF_MRI_HEIGHT_M, &hm);

        // Coordinate transform
        stream->write_coord_trans(slice.trans);

        // Pixel encoding
        fiff_int_t encoding = slice.pixelFormat;
        stream->write_int(FIFF_MRI_PIXEL_ENCODING, &encoding);

        // Pixel data
        QSharedPointer<FiffTag> pixelTag(new FiffTag());
        pixelTag->kind = FIFF_MRI_PIXEL_DATA;

        int nPixels = slice.width * slice.height;
        switch (slice.pixelFormat) {
            case FIFFV_MRI_PIXEL_BYTE: {
                pixelTag->type = FIFFT_BYTE;
                pixelTag->resize(nPixels);
                memcpy(pixelTag->data(), slice.pixels.constData(), nPixels);
                break;
            }
            case FIFFV_MRI_PIXEL_WORD: {
                pixelTag->type = FIFFT_USHORT;
                pixelTag->resize(nPixels * 2);
                memcpy(pixelTag->data(), slice.pixelsWord.constData(), nPixels * 2);
                break;
            }
            case FIFFV_MRI_PIXEL_FLOAT: {
                pixelTag->type = FIFFT_FLOAT;
                pixelTag->resize(nPixels * 4);
                memcpy(pixelTag->data(), slice.pixelsFloat.constData(), nPixels * 4);
                break;
            }
        }

        stream->write_tag(pixelTag);

        stream->end_block(FIFFB_MRI_SLICE);
    }

    // End blocks
    stream->end_block(FIFFB_MRI_SET);
    stream->end_block(FIFFB_MRI);
    stream->end_file();

    return true;
}
