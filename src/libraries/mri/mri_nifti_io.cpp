//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mri_nifti_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of @ref mne::MriNiftiIO: 348-byte header parse, byte-order auto-detect, sform/qform/pixdim transform reconstruction.
 *
 * Implements the three pieces the header documents: (1) a
 * little-endian-by-default 348-byte header parse that flips to
 * big-endian when @c sizeof_hdr fails its magic check and applies
 * the swap uniformly to every multi-byte field; (2) the voxel
 * reader, which respects @c datatype (UINT8 / INT16 / INT32 /
 * FLOAT32, the four types we actually encounter in practice)
 * and promotes to the matching @ref MriSlice pixel buffer so
 * round-tripping through @ref MriCorFifIO stays lossless;
 * (3) the sform \u2192 qform \u2192 pixdim transform fallback chain
 * (the same ordering nibabel and FSL use) that produces a
 * single canonical voxel\u2192RAS affine regardless of how the
 * source file was authored. The @c .nii.gz path shares the
 * MGZ zlib decoder so both compressed formats route through
 * the same code.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_nifti_io.h"

#include "mri_types.h"
#include "mri_vol_data.h"

#include <fiff/fiff_file.h>

#include <base/byte_stream.h>

#include <base/endian.h>
#include <base/log.h>
#include <base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;
using mne::Log;

namespace
{

// NIfTI-1 datatype codes (subset that we map onto MRI_* types).
constexpr std::int16_t DT_UINT8 = 2;
constexpr std::int16_t DT_INT16 = 4;
constexpr std::int16_t DT_INT32 = 8;
constexpr std::int16_t DT_FLOAT32 = 16;
constexpr std::int16_t DT_INT8 = 256;
constexpr std::int16_t DT_UINT16 = 512;

constexpr std::size_t NIFTI_HDR_SIZE = 348;

// Value of type T at @p p, stored with the file's byte order
template<mne::Swappable T>
T readValue(const std::byte* p, bool bigEndian)
{
    T v;
    std::memcpy(&v, p, sizeof(T));
    return bigEndian != (std::endian::native == std::endian::big) ? mne::byte_swap(v) : v;
}

// Decodes @p count values of type T from @p p and converts each one with @p convert into @p out
template<mne::Swappable T, typename Out, typename Convert>
void readValues(const std::byte*& p, bool bigEndian, std::vector<Out>& out, std::size_t count, Convert convert)
{
    out.resize(count);
    for (std::size_t i = 0; i < count; ++i, p += sizeof(T))
        out[i] = convert(readValue<T>(p, bigEndian));
}

} // namespace

//=============================================================================================================

bool MriNiftiIO::decompress(const std::string& gzFile, std::vector<std::byte>& rawData)
{
    auto bytes = mne::read_file_bytes(mne::path_from_utf8(gzFile), mne::Compression::Gzip);
    if (!bytes) {
        Log::critical("MriNiftiIO::decompress - {}", bytes.error().what());
        return false;
    }
    rawData = std::move(*bytes);
    return true;
}

//=============================================================================================================

bool MriNiftiIO::read(const std::string& niiFile, MriVolData& volData, bool verbose)
{
    volData.fileName = niiFile;

    const bool compressed = mne::to_lower(niiFile).ends_with(".gz");
    const auto content = mne::read_file_bytes(mne::path_from_utf8(niiFile), compressed ? mne::Compression::Gzip : mne::Compression::None);
    if (!content) {
        Log::critical("MriNiftiIO::read - {}", content.error().what());
        return false;
    }
    const std::vector<std::byte>& bytes = *content;

    if (bytes.size() < NIFTI_HDR_SIZE) {
        Log::critical("MriNiftiIO::read - file too small ({} bytes) : {}", bytes.size(), niiFile);
        return false;
    }

    const std::byte* hdr = bytes.data();

    // Endianness detection via sizeof_hdr (must be 348).
    bool bigEndian = false;
    std::int32_t sizeofHdr = readValue<std::int32_t>(hdr, bigEndian);
    if (sizeofHdr != std::int32_t(NIFTI_HDR_SIZE)) {
        bigEndian = true;
        sizeofHdr = readValue<std::int32_t>(hdr, bigEndian);
        if (sizeofHdr != std::int32_t(NIFTI_HDR_SIZE)) {
            Log::critical("MriNiftiIO::read - not a NIfTI-1 file (sizeof_hdr = {}): {}", sizeofHdr, niiFile);
            return false;
        }
    }

    auto i16 = [&](std::size_t off) {
        return readValue<std::int16_t>(hdr + off, bigEndian);
    };
    auto f32 = [&](std::size_t off) {
        return readValue<float>(hdr + off, bigEndian);
    };

    // dim[0..7] starts at offset 40; dim[0] = ndim, dim[1..3] = nx,ny,nz, dim[4] = nt.
    const std::int16_t ndim = i16(40);
    const std::int16_t nx = i16(42);
    const std::int16_t ny = i16(44);
    const std::int16_t nz = i16(46);
    const std::int16_t nt = i16(48);
    if (ndim < 3 || nx <= 0 || ny <= 0 || nz <= 0) {
        Log::critical("MriNiftiIO::read - degenerate dims {} {} {} {}", ndim, nx, ny, nz);
        return false;
    }

    const std::int16_t datatype = i16(70);

    // pixdim[0..7] starts at offset 76. pixdim[1..3] = spacing in mm; pixdim[0] is the qfac sign.
    float qfac = f32(76);
    if (qfac == 0.0f) {
        qfac = 1.0f;
    }
    const float dx = std::fabs(f32(80));
    const float dy = std::fabs(f32(84));
    const float dz = std::fabs(f32(88));

    const float voxOffset = f32(108);
    const float sclSlope = f32(112);
    const float sclInter = f32(116);

    const std::int16_t qformCode = i16(252);
    const std::int16_t sformCode = i16(254);

    // Map NIfTI datatype to MRI_* + bytes-per-voxel.
    int mriType = MRI_UCHAR;
    int bpv = 1;
    switch (datatype) {
        case DT_UINT8:
            mriType = MRI_UCHAR;
            bpv = 1;
            break;
        case DT_INT8:
            mriType = MRI_UCHAR;
            bpv = 1;
            break; // promoted to unsigned, negatives clamped
        case DT_INT16:
            mriType = MRI_SHORT;
            bpv = 2;
            break;
        case DT_UINT16:
            mriType = MRI_SHORT;
            bpv = 2;
            break;
        case DT_INT32:
            mriType = MRI_INT;
            bpv = 4;
            break;
        case DT_FLOAT32:
            mriType = MRI_FLOAT;
            bpv = 4;
            break;
        default:
            Log::critical("MriNiftiIO::read - unsupported datatype {}", datatype);
            return false;
    }

    // Build the 4×4 voxel→RAS transform (mm) using sform → qform → pixdim fallback.
    Matrix4f vox2ras = Matrix4f::Identity();
    if (sformCode > 0) {
        for (int c = 0; c < 4; ++c) {
            const std::size_t col = static_cast<std::size_t>(c) * 4;
            vox2ras(0, c) = f32(280 + col);
            vox2ras(1, c) = f32(296 + col);
            vox2ras(2, c) = f32(312 + col);
        }
    } else if (qformCode > 0) {
        const float b = f32(256);
        const float c = f32(260);
        const float d = f32(264);
        const float aSq = 1.0f - (b * b + c * c + d * d);
        const float a = aSq > 0.0f ? std::sqrt(aSq) : 0.0f;
        Matrix3f R;
        R << a * a + b * b - c * c - d * d, 2.0f * (b * c - a * d), 2.0f * (b * d + a * c),
            2.0f * (b * c + a * d), a * a + c * c - b * b - d * d, 2.0f * (c * d - a * b),
            2.0f * (b * d - a * c), 2.0f * (c * d + a * b), a * a + d * d - b * b - c * c;
        Matrix3f S = Matrix3f::Zero();
        S(0, 0) = dx;
        S(1, 1) = dy;
        S(2, 2) = dz * qfac;
        Matrix3f M = R * S;
        vox2ras.block<3, 3>(0, 0) = M;
        vox2ras(0, 3) = f32(268);
        vox2ras(1, 3) = f32(272);
        vox2ras(2, 3) = f32(276);
    } else {
        // Method 1: pixdim diagonal, origin at volume centre.
        vox2ras(0, 0) = dx;
        vox2ras(1, 1) = dy;
        vox2ras(2, 2) = dz;
        vox2ras(0, 3) = -0.5f * dx * nx;
        vox2ras(1, 3) = -0.5f * dy * ny;
        vox2ras(2, 3) = -0.5f * dz * nz;
    }

    // Decompose vox2ras → (Mdc, spacing, c_ras) so MriVolData::computeVox2Ras round-trips.
    Vector3f col0 = vox2ras.block<3, 1>(0, 0);
    Vector3f col1 = vox2ras.block<3, 1>(0, 1);
    Vector3f col2 = vox2ras.block<3, 1>(0, 2);
    float sx = col0.norm();
    float sy = col1.norm();
    float sz = col2.norm();
    if (sx <= 0.0f)
        sx = dx > 0.0f ? dx : 1.0f;
    if (sy <= 0.0f)
        sy = dy > 0.0f ? dy : 1.0f;
    if (sz <= 0.0f)
        sz = dz > 0.0f ? dz : 1.0f;

    Vector3f xRas = col0 / sx;
    Vector3f yRas = col1 / sy;
    Vector3f zRas = col2 / sz;

    Vector4f centreVox(nx * 0.5f, ny * 0.5f, nz * 0.5f, 1.0f);
    Vector4f cRas4 = vox2ras * centreVox;

    volData.version = MRI_MGH_VERSION; // mark as a valid volume for isValid()
    volData.width = nx;
    volData.height = ny;
    volData.depth = nz;
    volData.nframes = nt > 0 ? nt : 1;
    volData.type = mriType;
    volData.dof = 0;
    volData.rasGood = true;
    volData.xsize = sx;
    volData.ysize = sy;
    volData.zsize = sz;
    volData.x_ras = xRas;
    volData.y_ras = yRas;
    volData.z_ras = zRas;
    volData.c_ras = cRas4.head<3>();
    volData.voxelSurfRasT = FiffCoordTrans(
        FIFFV_COORD_MRI_SLICE, FIFFV_COORD_MRI, volData.computeVox2Ras(), true);

    // Read voxel data starting at vox_offset (typically 352 for single-file).
    const std::size_t dataOff = static_cast<std::size_t>(voxOffset > 0.0f ? voxOffset : float(NIFTI_HDR_SIZE + 4));
    const std::size_t nPixels = static_cast<std::size_t>(nx) * static_cast<std::size_t>(ny);
    const std::size_t nslice = static_cast<std::size_t>(nz);
    const std::size_t frameSize = nPixels * nslice * static_cast<std::size_t>(bpv);
    if (bytes.size() < dataOff + frameSize) {
        Log::critical("MriNiftiIO::read - data section truncated (need {} bytes, got {})", dataOff + frameSize, bytes.size());
        return false;
    }

    const std::byte* voxels = bytes.data() + dataOff;
    volData.slices.resize(nslice);

    const Matrix4f vox2rasFinal = volData.computeVox2Ras();
    const bool useScale = (sclSlope != 0.0f) && !(sclSlope == 1.0f && sclInter == 0.0f);

    for (std::size_t k = 0; k < nslice; ++k) {
        MriSlice& slice = volData.slices[k];
        slice.width = nx;
        slice.height = ny;
        slice.dimx = sx / 1000.0f;
        slice.dimy = sy / 1000.0f;
        slice.scale = 1.0f;

        switch (mriType) {
            case MRI_UCHAR:
                slice.pixelFormat = FIFFV_MRI_PIXEL_BYTE;
                if (datatype == DT_INT8) {
                    readValues<std::int8_t>(voxels, bigEndian, slice.pixels, nPixels, [](std::int8_t v) { return static_cast<unsigned char>(v < 0 ? 0 : v); });
                } else {
                    readValues<std::uint8_t>(voxels, bigEndian, slice.pixels, nPixels, [](std::uint8_t v) { return v; });
                }
                break;
            case MRI_SHORT:
                slice.pixelFormat = FIFFV_MRI_PIXEL_WORD;
                if (datatype == DT_UINT16) {
                    readValues<std::uint16_t>(voxels, bigEndian, slice.pixelsWord, nPixels, [](std::uint16_t v) { return v; });
                } else {
                    readValues<std::int16_t>(voxels, bigEndian, slice.pixelsWord, nPixels, [](std::int16_t v) { return static_cast<unsigned short>(v < 0 ? 0 : v); });
                }
                break;
            case MRI_INT:
                slice.pixelFormat = FIFFV_MRI_PIXEL_FLOAT;
                readValues<std::int32_t>(voxels, bigEndian, slice.pixelsFloat, nPixels, [](std::int32_t v) { return static_cast<float>(v); });
                break;
            case MRI_FLOAT:
                slice.pixelFormat = FIFFV_MRI_PIXEL_FLOAT;
                readValues<float>(voxels, bigEndian, slice.pixelsFloat, nPixels, [](float v) { return v; });
                break;
        }

        // Apply scl_slope / scl_inter for floating-point output (the NIfTI spec
        // states the transform produces the "physical" value; we only honour it
        // for floats — integer outputs stay as-is to preserve label maps).
        if (useScale && slice.pixelFormat == FIFFV_MRI_PIXEL_FLOAT) {
            for (float& v : slice.pixelsFloat) {
                v = v * sclSlope + sclInter;
            }
        }

        const float kf = static_cast<float>(k);
        Vector3f sliceOrigin;
        sliceOrigin(0) = vox2rasFinal(0, 2) * kf + vox2rasFinal(0, 3);
        sliceOrigin(1) = vox2rasFinal(1, 2) * kf + vox2rasFinal(1, 3);
        sliceOrigin(2) = vox2rasFinal(2, 2) * kf + vox2rasFinal(2, 3);

        Matrix3f sliceRot;
        sliceRot.col(0) = vox2rasFinal.block<3, 1>(0, 0);
        sliceRot.col(1) = vox2rasFinal.block<3, 1>(0, 1);
        sliceRot.col(2) = vox2rasFinal.block<3, 1>(0, 2);

        slice.trans = FiffCoordTrans(FIFFV_COORD_MRI_SLICE, FIFFV_COORD_MRI, sliceRot, sliceOrigin);
    }

    if (verbose) {
        Log::info("NIfTI file: {}x{}x{}, datatype={}, voxel {:.3f}x{:.3f}x{:.3f} mm, c_ras=({:.2f}, {:.2f}, {:.2f})",
                  nx, ny, nz, datatype, sx, sy, sz, volData.c_ras[0], volData.c_ras[1], volData.c_ras[2]);
    }

    return true;
}
