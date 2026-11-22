//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mri_cor_fif_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Implementation of @ref mne::MriCorFifIO: block-and-tag serialiser that emits a FIFF MRI set from an in-memory @ref MriVolData.
 *
 * Drives a @ref mne::FiffStream through the nested
 * @c FIFFB_MRI / @c FIFFB_MRI_SET / @c FIFFB_MRI_SLICE block
 * hierarchy declared in the header. For each slice it writes
 * the geometry tags (@c FIFF_MRI_WIDTH / @c FIFF_MRI_HEIGHT and
 * their metric counterparts), the slice\u2192MRI coordinate
 * transform, the pixel-encoding selector, and the raw pixel
 * buffer in the source's native @c FIFFV_MRI_PIXEL_BYTE /
 * @c _WORD / @c _FLOAT encoding so reads through the FIFF
 * toolchain reproduce the original sample-level data without
 * quantisation drift.
 * Ported from save_slices() / write_slice() in MNE C write_mri_set.c
 * by Matti Hamalainen.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_cor_fif_io.h"

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/fiff/fiff_file.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/base/log.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool MriCorFifIO::write(const std::string& fileName,
                        const std::vector<MriSlice>& slices,
                        const std::vector<FiffCoordTrans>& additionalTrans)
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

    auto stream = FiffStream::start_file(fileName);

    if (!stream) {
        mne::Log::critical("MriCorFifIO::write - Could not open file for writing: {}", fileName);
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
    for (const MriSlice& slice : slices) {
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
        FiffTag pixelTag;
        pixelTag.kind = FIFF_MRI_PIXEL_DATA;

        int nPixels = slice.width * slice.height;
        switch (slice.pixelFormat) {
            case FIFFV_MRI_PIXEL_BYTE: {
                pixelTag.type = FIFFT_BYTE;
                pixelTag.resize(nPixels);
                memcpy(pixelTag.data(), slice.pixels.data(), nPixels);
                break;
            }
            case FIFFV_MRI_PIXEL_WORD: {
                pixelTag.type = FIFFT_USHORT;
                pixelTag.resize(nPixels * 2);
                memcpy(pixelTag.data(), slice.pixelsWord.data(), nPixels * 2);
                break;
            }
            case FIFFV_MRI_PIXEL_FLOAT: {
                pixelTag.type = FIFFT_FLOAT;
                pixelTag.resize(nPixels * 4);
                memcpy(pixelTag.data(), slice.pixelsFloat.data(), nPixels * 4);
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
