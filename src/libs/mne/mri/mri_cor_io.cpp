//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mri_cor_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Implementation of @ref mne::MriCorIO: per-file COR-NNN reader with coronal-orientation slice transform synthesis.
 *
 * Walks the 256 @c COR-001 \u2026 @c COR-256 files in the supplied
 * directory, mmap-friendly reads the 65\u202f536-byte flat slice
 * buffer of each one into a @ref MriSlice with FIFFV_MRI_PIXEL_BYTE
 * encoding, and stamps every slice with its coronal-orientation
 * slice\u2192MRI transform (the (128, -128, 128) mm offset plus the
 * @c x\u2192-x, @c y\u2192z, @c z\u2192y axis permutation). Missing or
 * short-read files are reported as warnings; partial sets are
 * rejected so downstream code can rely on a complete 256-slice
 * coronal stack.
 * Ported from make_cor_set() in MNE C mne_make_cor_set by Matti Hamalainen.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_cor_io.h"

#include <mne/fiff/fiff_coord_trans.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_file.h>
#include <mne/base/byte_stream.h>
#include <mne/base/string_utils.h>
#include <mne/base/log.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdio>
#include <cstring>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool MriCorIO::read(const std::string& dir,
                    std::vector<MriSlice>& slices,
                    bool verbose)
{
    //
    // COR files contain 256 coronal slices of 256×256 unsigned chars (1mm isotropic).
    //
    // The coordinate system is:
    //   - Coronal orientation
    //   - 1mm pixel spacing
    //   - Origin offset: (0.128, -0.128, 0.128) meters
    //   - Rotation: x -> -x, y -> z, z -> y (coronal to MRI surface RAS)
    //
    // Ported from make_cor_set() in mne_make_cor_set/make_cor_set.c
    //

    const std::size_t nPixels = static_cast<std::size_t>(COR_WIDTH) * static_cast<std::size_t>(COR_HEIGHT);
    slices.resize(COR_NSLICE);

    for (int k = 0; k < COR_NSLICE; ++k) {
        char name[16];
        std::snprintf(name, sizeof(name), "/COR-%03d", k + 1);
        const std::string fileName = dir + name;
        const auto data = mne::read_file_bytes(mne::path_from_utf8(fileName));
        if (!data) {
            mne::Log::critical("MriCorIO::read - Could not open COR file {}", fileName);
            return false;
        }

        if (data->size() < nPixels) {
            mne::Log::critical("MriCorIO::read - COR file {} is too small: {} bytes (expected {})", fileName, data->size(), nPixels);
            return false;
        }

        MriSlice& slice = slices[static_cast<std::size_t>(k)];
        slice.fileName = fileName;
        slice.width = COR_WIDTH;
        slice.height = COR_HEIGHT;
        slice.dimx = COR_PIXEL_SIZE;
        slice.dimy = COR_PIXEL_SIZE;
        slice.scale = 1.0f;
        slice.pixelFormat = FIFFV_MRI_PIXEL_BYTE;

        // Copy pixel data
        slice.pixels.resize(nPixels);
        std::memcpy(slice.pixels.data(), data->data(), nPixels);

        // Build the coordinate transformation: slice -> MRI (surface RAS)
        // Ported from the original C code in make_cor_set.c:
        //   move[X] = 0.128; move[Y] = -0.128 + k/1000.0; move[Z] = 0.128;
        //   rot = {{-1,0,0},{0,0,1},{0,1,0}};
        Matrix3f rot;
        rot << -1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f,
            0.0f, 1.0f, 0.0f;

        Eigen::Vector3f move;
        move << 0.128f, -0.128f + static_cast<float>(k) / 1000.0f, 0.128f;

        slice.trans = FiffCoordTrans(FIFFV_COORD_MRI_SLICE, FIFFV_COORD_MRI, rot, move);
    }

    if (verbose) {
        mne::Log::info("Read {} COR slices from {}", COR_NSLICE, dir);
    }

    return true;
}
