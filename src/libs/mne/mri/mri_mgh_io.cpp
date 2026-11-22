//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mri_mgh_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February 2026
 * @brief    Implementation of @ref mne::MriMghIO: byte-exact MGH/MGZ parser and zlib-streamed MGZ decoder.
 *
 * Implements the big-endian header parse, the column-major voxel
 * buffer reader (templated over the four MGH voxel types ---
 * UCHAR / SHORT / INT / FLOAT --- so quantisation is preserved
 * instead of being eagerly promoted to float), and the optional
 * footer-tag walker that recovers TR / TE / flipAngle / FoV plus
 * the @c talairach.xfm path. MGZ inputs are routed through a
 * single zlib @c inflate() call with @c MAX_WBITS+16 (gzip
 * wrapping enabled) so the on-disk and in-memory paths share the
 * same parser. Output is materialised as the slice-of-slices
 * @ref MriVolData representation consumed by every downstream
 * rendering and export path.
 * Ported from make_mgh_cor_set() in MNE C mne_make_cor_set by Matti Hamalainen.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_mgh_io.h"

#include <mne/base/byte_stream.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/fiff/fiff_coord_trans_set.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_file.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <cstdio>
#include <filesystem>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;
using mne::Log;

//=============================================================================================================
// DEFINE LOCAL FUNCTIONS
//=============================================================================================================

namespace
{

// Reads @p count big-endian values of type T and converts each one with @p convert into @p out
template<typename T, typename Out, typename Convert>
bool readBigEndianInto(mne::ByteStream& stream, std::vector<Out>& out, std::size_t count, Convert convert)
{
    std::vector<T> values(count);
    if (!stream.read_big_endian(values.data(), count))
        return false;
    out.resize(count);
    std::transform(values.begin(), values.end(), out.begin(), convert);
    return true;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool MriMghIO::read(const std::string& mgzFile,
                    MriVolData& volData,
                    std::vector<FiffCoordTrans>& additionalTrans,
                    const std::string& subjectMriDir,
                    bool verbose)
{
    volData.fileName = mgzFile;

    // Step 1: Get raw (decompressed) bytes
    const bool isCompressed = mne::to_lower(mgzFile).ends_with(".mgz");
    const auto fileData = mne::read_file_bytes(mne::path_from_utf8(mgzFile), isCompressed ? mne::Compression::Gzip : mne::Compression::None);
    if (!fileData) {
        Log::critical("MriMghIO::read - {}", fileData.error().what());
        return false;
    }

    if (fileData->size() < MRI_MGH_DATA_OFFSET) {
        Log::critical("MriMghIO::read - File {} is too small to be a valid MGH file ({} bytes)", mgzFile, fileData->size());
        return false;
    }

    // Step 2: Parse header
    if (!parseHeader(*fileData, volData, verbose)) {
        return false;
    }

    // Step 3: Build the voxel -> surface RAS transform
    Matrix4f vox2ras = volData.computeVox2Ras();
    volData.voxelSurfRasT = FiffCoordTrans(
        FIFFV_COORD_MRI_SLICE, FIFFV_COORD_MRI, vox2ras, true);

    if (verbose) {
        Log::info("Voxel -> FsSurface RAS transform:");
        for (int r = 0; r < 4; ++r) {
            Log::info("  {:10.6f} {:10.6f} {:10.6f} {:10.6f}", vox2ras(r, 0), vox2ras(r, 1), vox2ras(r, 2), vox2ras(r, 3));
        }
    }

    // Surface RAS -> scanner RAS: a shift by the volume centre (MNE-C mne_mri_add_transforms).
    additionalTrans.push_back(FiffCoordTrans(FIFFV_COORD_MRI, FIFFV_MNE_COORD_RAS, Matrix3f::Identity(), Vector3f(volData.c_ras / 1000.0f)));

    // Step 4: Read voxel data
    if (!readVoxelData(*fileData, volData)) {
        return false;
    }

    // Step 5: Parse footer (optional)
    parseFooter(*fileData, volData, additionalTrans, subjectMriDir, verbose);

    if (verbose) {
        Log::info("Read {} slices from {} ({}x{} pixels)", volData.slices.size(), mgzFile, volData.width, volData.height);
    }

    return true;
}

//=============================================================================================================

bool MriMghIO::parseHeader(const std::vector<std::byte>& data, MriVolData& volData, bool verbose)
{
    //
    // MGH header layout (all big-endian):
    //   Bytes 0-3:    version (int32)
    //   Bytes 4-7:    width (int32)
    //   Bytes 8-11:   height (int32)
    //   Bytes 12-15:  depth (int32)
    //   Bytes 16-19:  nframes (int32)
    //   Bytes 20-23:  type (int32)
    //   Bytes 24-27:  dof (int32)
    //   Bytes 28-29:  goodRASflag (int16)
    //   Bytes 30-41:  spacingX/Y/Z (3×float32) — only if goodRASflag > 0
    //   Bytes 42-77:  Mdc (9×float32) — direction cosines, only if goodRASflag > 0
    //   Bytes 78-89:  c_ras (3×float32) — center RAS, only if goodRASflag > 0
    //   Bytes 90-283: unused (padding)
    //

    mne::MemoryByteStream stream({data.begin(), data.begin() + MRI_MGH_DATA_OFFSET});

    std::int32_t header[7] = {};
    stream.read_big_endian(header, 7);
    const auto [version, width, height, depth, nframes, type, dof] = header;

    if (version != MRI_MGH_VERSION) {
        Log::critical("MriMghIO::parseHeader - Unknown MGH version: {}", version);
        return false;
    }

    volData.version = version;
    volData.width = width;
    volData.height = height;
    volData.depth = depth;
    volData.nframes = nframes;
    volData.type = type;
    volData.dof = dof;

    if (verbose) {
        Log::info("MGH file: {}x{}x{}, {} frame(s), type={}", width, height, depth, nframes, type);
    }

    std::int16_t goodRASflag = 0;
    stream.read_big_endian(goodRASflag);
    volData.rasGood = (goodRASflag > 0);

    if (goodRASflag > 0) {
        // Voxel sizes, then the direction cosines (x, y, z columns) and the centre RAS
        float geometry[15] = {};
        stream.read_big_endian(geometry, 15);
        volData.xsize = geometry[0];
        volData.ysize = geometry[1];
        volData.zsize = geometry[2];
        volData.x_ras = Vector3f(geometry[3], geometry[4], geometry[5]);
        volData.y_ras = Vector3f(geometry[6], geometry[7], geometry[8]);
        volData.z_ras = Vector3f(geometry[9], geometry[10], geometry[11]);
        volData.c_ras = Vector3f(geometry[12], geometry[13], geometry[14]);
    }
    // Else: default values from MriVolData constructor are used

    if (verbose) {
        Log::info("Voxel sizes: {:.4f} x {:.4f} x {:.4f} mm", volData.xsize, volData.ysize, volData.zsize);
        Log::info("goodRAS: {}", int(goodRASflag));
        Log::info("c_ras: {:.4f} {:.4f} {:.4f}", volData.c_ras[0], volData.c_ras[1], volData.c_ras[2]);
    }

    return true;
}

//=============================================================================================================

bool MriMghIO::readVoxelData(const std::vector<std::byte>& data, MriVolData& volData)
{
    //
    // Read voxel data starting at byte 284 (MRI_MGH_DATA_OFFSET).
    // Data layout in MGH: [width][height][depth][frames] in Fortran order (x fastest).
    // Only the first frame is read.
    //

    const int bpv = bytesPerVoxel(volData.type);
    if (bpv == 0) {
        Log::critical("MriMghIO::readVoxelData - Unsupported MGH data type: {}", volData.type);
        return false;
    }

    const std::size_t nPixels = static_cast<std::size_t>(volData.width) * static_cast<std::size_t>(volData.height);
    const std::size_t nslice = static_cast<std::size_t>(volData.depth);
    const std::size_t frameSize = nPixels * nslice * static_cast<std::size_t>(bpv);
    if (data.size() < MRI_MGH_DATA_OFFSET + frameSize) {
        Log::critical("MriMghIO::readVoxelData - File too small for expected data size");
        return false;
    }

    mne::MemoryByteStream stream({data.begin() + MRI_MGH_DATA_OFFSET, data.begin() + std::ptrdiff_t(MRI_MGH_DATA_OFFSET + frameSize)});
    volData.slices.resize(nslice);

    // Build the vox2ras transform for per-slice transforms
    Matrix4f vox2ras = volData.computeVox2Ras();

    for (std::size_t k = 0; k < nslice; ++k) {
        MriSlice& slice = volData.slices[k];
        slice.width = volData.width;
        slice.height = volData.height;
        slice.dimx = volData.xsize / 1000.0f; // mm -> meters
        slice.dimy = volData.ysize / 1000.0f;
        slice.scale = 1.0f;

        switch (volData.type) {
            case MRI_UCHAR:
                slice.pixelFormat = FIFFV_MRI_PIXEL_BYTE;
                readBigEndianInto<std::uint8_t>(stream, slice.pixels, nPixels, [](std::uint8_t v) { return v; });
                break;
            case MRI_SHORT:
                slice.pixelFormat = FIFFV_MRI_PIXEL_WORD;
                readBigEndianInto<std::int16_t>(stream, slice.pixelsWord, nPixels, [](std::int16_t v) { return static_cast<unsigned short>(v < 0 ? 0 : v); });
                break;
            case MRI_INT:
                // Convert INT to FLOAT
                slice.pixelFormat = FIFFV_MRI_PIXEL_FLOAT;
                readBigEndianInto<std::int32_t>(stream, slice.pixelsFloat, nPixels, [](std::int32_t v) { return static_cast<float>(v); });
                break;
            case MRI_FLOAT:
                slice.pixelFormat = FIFFV_MRI_PIXEL_FLOAT;
                readBigEndianInto<float>(stream, slice.pixelsFloat, nPixels, [](float v) { return v; });
                break;
        }

        //
        // Build per-slice coordinate transform (slice -> MRI surface RAS).
        // For each slice k:
        //   sliceOrigin = vox2ras * [0, 0, k, 1]^T
        //   sliceRot    = vox2ras rotation columns (x, y, z pixel axes)
        //
        const float kf = static_cast<float>(k);
        Vector3f sliceOrigin;
        sliceOrigin(0) = vox2ras(0, 2) * kf + vox2ras(0, 3);
        sliceOrigin(1) = vox2ras(1, 2) * kf + vox2ras(1, 3);
        sliceOrigin(2) = vox2ras(2, 2) * kf + vox2ras(2, 3);

        Matrix3f sliceRot;
        sliceRot.col(0) = vox2ras.block<3, 1>(0, 0); // x-pixel direction
        sliceRot.col(1) = vox2ras.block<3, 1>(0, 1); // y-pixel direction
        sliceRot.col(2) = vox2ras.block<3, 1>(0, 2); // z (normal) direction

        slice.trans = FiffCoordTrans(FIFFV_COORD_MRI_SLICE, FIFFV_COORD_MRI, sliceRot, sliceOrigin);
    }

    return true;
}

//=============================================================================================================

bool MriMghIO::parseFooter(const std::vector<std::byte>& data,
                           MriVolData& volData,
                           std::vector<FiffCoordTrans>& additionalTrans,
                           const std::string& subjectMriDir,
                           bool verbose)
{
    //
    // The footer starts after the voxel data.
    // It contains (in order):
    //   1. Scan parameters: TR(f32), flipAngle(f32), TE(f32), TI(f32), FoV(f32)
    //   2. Tags: tagType(i32) + tagLen(i32 or i64) + tagData
    //

    int bpv = bytesPerVoxel(volData.type);
    if (bpv == 0) {
        bpv = 1; // Fallback for footer calculation
    }

    const std::size_t frameSize = static_cast<std::size_t>(volData.width) * static_cast<std::size_t>(volData.height) * static_cast<std::size_t>(volData.depth) * static_cast<std::size_t>(bpv);
    const std::size_t footerPos = MRI_MGH_DATA_OFFSET + frameSize;

    // Scan parameters take 5 x float32; without them there is no footer
    constexpr std::size_t kScanParamBytes = 5 * sizeof(float);
    if (data.size() < footerPos + kScanParamBytes) {
        return true;
    }

    mne::MemoryByteStream stream({data.begin() + std::ptrdiff_t(footerPos), data.end()});
    float scan[5] = {};
    stream.read_big_endian(scan, 5);
    volData.TR = scan[0];
    volData.flipAngle = scan[1];
    volData.TE = scan[2];
    volData.TI = scan[3];
    volData.FoV = scan[4];

    // Parse tags
    while (!stream.at_end()) {
        std::int32_t tagType = 0;
        if (!stream.read_big_endian(tagType))
            break;

        std::int64_t tagLen = 0;
        // TAG_OLD_SURF_GEOM (20) and TAG_OLD_MGH_XFORM (30) have a 4-byte length, newer tags an 8-byte one
        if (tagType == MGH_TAG_OLD_SURF_GEOM || tagType == MGH_TAG_OLD_MGH_XFORM) {
            std::int32_t len32 = 0;
            if (!stream.read_big_endian(len32))
                break;
            tagLen = len32;
        } else if (!stream.read_big_endian(tagLen)) {
            break;
        }

        if (tagLen <= 0 || static_cast<std::uint64_t>(tagLen) > data.size())
            break;

        std::string tagData(static_cast<std::size_t>(tagLen), '\0');
        if (stream.read(tagData.data(), tagData.size()) != tagData.size())
            break;

        if (tagType == MGH_TAG_MGH_XFORM) {
            // TAG_MGH_XFORM: path to talairach.xfm; FreeSurfer includes the terminating NUL in the length
            std::string xfmPath(mne::trimmed(std::string_view(tagData).substr(0, tagData.find('\0'))));
            volData.talairachXfmPath = xfmPath;

            if (verbose) {
                Log::info("Found Talairach transform reference: {}", xfmPath);
            }

            // Resolve relative paths using subject MRI directory
            if (!mne::path_from_utf8(xfmPath).is_absolute() && !subjectMriDir.empty()) {
                xfmPath = mne::utf8_from_path(mne::path_from_utf8(subjectMriDir) / "transforms" / mne::path_from_utf8(xfmPath));
            }

            FiffCoordTransSet talairach;
            if (talairach.addTalairach(mne::path_from_utf8(xfmPath))) {
                additionalTrans.push_back(talairach.RAS_MNI_tal_t);
                additionalTrans.push_back(talairach.MNI_tal_tal_gtz_t);
                additionalTrans.push_back(talairach.MNI_tal_tal_ltz_t);
                if (verbose) {
                    Log::info("Read Talairach transform from {}", xfmPath);
                }
            } else if (verbose) {
                Log::warning("Talairach transform not readable: {}", xfmPath);
            }
        }
    }

    return true;
}

//=============================================================================================================

int MriMghIO::bytesPerVoxel(int type)
{
    switch (type) {
        case MRI_UCHAR:
            return 1;
        case MRI_SHORT:
            return 2;
        case MRI_INT:
            return 4;
        case MRI_FLOAT:
            return 4;
        default:
            return 0;
    }
}
