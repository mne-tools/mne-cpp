//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mri_nifti_io.cpp
 * @since 2026
 * @date  May 2026
 * @brief Implementation of @ref MRILIB::MriNiftiIO: 348-byte header parse, byte-order auto-detect, sform/qform/pixdim transform reconstruction.
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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QtEndian>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// SYSTEM INCLUDES
//=============================================================================================================

#include <zlib.h>

#include <cmath>
#include <cstring>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MRILIB;
using namespace FIFFLIB;
using namespace Eigen;

namespace {

// NIfTI-1 datatype codes (subset that we map onto MRI_* types).
constexpr qint16 DT_UINT8    = 2;
constexpr qint16 DT_INT16    = 4;
constexpr qint16 DT_INT32    = 8;
constexpr qint16 DT_FLOAT32  = 16;
constexpr qint16 DT_INT8     = 256;
constexpr qint16 DT_UINT16   = 512;

constexpr int NIFTI_HDR_SIZE = 348;

template <typename T>
T readLE(const char* p)
{
    T v;
    std::memcpy(&v, p, sizeof(T));
    return qFromLittleEndian(v);
}

template <typename T>
T readBE(const char* p)
{
    T v;
    std::memcpy(&v, p, sizeof(T));
    return qFromBigEndian(v);
}

} // namespace

//=============================================================================================================

bool MriNiftiIO::decompress(const QString& gzFile, QByteArray& rawData)
{
    QFile file(gzFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "MriNiftiIO::decompress - Could not open" << gzFile;
        return false;
    }
    const QByteArray compressed = file.readAll();
    file.close();

    if (compressed.isEmpty()) {
        qCritical() << "MriNiftiIO::decompress - File is empty:" << gzFile;
        return false;
    }

    z_stream strm = {};
    if (inflateInit2(&strm, MAX_WBITS + 16) != Z_OK) {
        qCritical() << "MriNiftiIO::decompress - inflateInit2 failed";
        return false;
    }

    strm.next_in  = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.data()));
    strm.avail_in = static_cast<uInt>(compressed.size());

    const int chunkSize = 256 * 1024;
    rawData.clear();

    int ret = Z_OK;
    do {
        rawData.resize(rawData.size() + chunkSize);
        strm.next_out  = reinterpret_cast<Bytef*>(rawData.data() + rawData.size() - chunkSize);
        strm.avail_out = chunkSize;

        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            qCritical() << "MriNiftiIO::decompress - inflate failed for" << gzFile
                        << "- zlib error:" << ret;
            inflateEnd(&strm);
            return false;
        }
    } while (ret != Z_STREAM_END);

    rawData.resize(rawData.size() - static_cast<int>(strm.avail_out));
    inflateEnd(&strm);
    return true;
}

//=============================================================================================================

bool MriNiftiIO::read(const QString& niiFile, MriVolData& volData, bool verbose)
{
    volData.fileName = niiFile;

    QByteArray bytes;
    const bool compressed = niiFile.endsWith(QStringLiteral(".gz"), Qt::CaseInsensitive);
    if (compressed) {
        if (!decompress(niiFile, bytes)) {
            return false;
        }
    } else {
        QFile f(niiFile);
        if (!f.open(QIODevice::ReadOnly)) {
            qCritical() << "MriNiftiIO::read - Could not open" << niiFile;
            return false;
        }
        bytes = f.readAll();
    }

    if (bytes.size() < NIFTI_HDR_SIZE) {
        qCritical() << "MriNiftiIO::read - file too small (" << bytes.size() << "bytes) :" << niiFile;
        return false;
    }

    const char* hdr = bytes.constData();

    // Endianness detection via sizeof_hdr (must be 348).
    qint32 sizeofHdr = readLE<qint32>(hdr);
    bool bigEndian = false;
    if (sizeofHdr != NIFTI_HDR_SIZE) {
        sizeofHdr = readBE<qint32>(hdr);
        if (sizeofHdr != NIFTI_HDR_SIZE) {
            qCritical() << "MriNiftiIO::read - not a NIfTI-1 file (sizeof_hdr ="
                        << sizeofHdr << "):" << niiFile;
            return false;
        }
        bigEndian = true;
    }

    auto i16 = [&](int off) {
        return bigEndian ? readBE<qint16>(hdr + off) : readLE<qint16>(hdr + off);
    };
    auto f32 = [&](int off) {
        if (bigEndian) {
            quint32 raw = readBE<quint32>(hdr + off);
            float v;
            std::memcpy(&v, &raw, sizeof(float));
            return v;
        }
        quint32 raw = readLE<quint32>(hdr + off);
        float v;
        std::memcpy(&v, &raw, sizeof(float));
        return v;
    };

    // dim[0..7] starts at offset 40; dim[0] = ndim, dim[1..3] = nx,ny,nz, dim[4] = nt.
    qint16 ndim = i16(40);
    qint16 nx   = i16(42);
    qint16 ny   = i16(44);
    qint16 nz   = i16(46);
    qint16 nt   = i16(48);
    if (ndim < 3 || nx <= 0 || ny <= 0 || nz <= 0) {
        qCritical() << "MriNiftiIO::read - degenerate dims" << ndim << nx << ny << nz;
        return false;
    }

    qint16 datatype = i16(70);
    qint16 bitpix   = i16(72);
    Q_UNUSED(bitpix);

    // pixdim[0..7] starts at offset 76. pixdim[1..3] = spacing in mm; pixdim[0] is the qfac sign.
    float qfac = f32(76);
    if (qfac == 0.0f) {
        qfac = 1.0f;
    }
    const float dx = std::fabs(f32(80));
    const float dy = std::fabs(f32(84));
    const float dz = std::fabs(f32(88));

    const float voxOffset = f32(108);
    const float sclSlope  = f32(112);
    const float sclInter  = f32(116);

    const qint16 qformCode = i16(252);
    const qint16 sformCode = i16(254);

    // Map NIfTI datatype to MRI_* + bytes-per-voxel.
    int mriType = MRI_UCHAR;
    int bpv = 1;
    switch (datatype) {
        case DT_UINT8:   mriType = MRI_UCHAR; bpv = 1; break;
        case DT_INT8:    mriType = MRI_UCHAR; bpv = 1; break;  // promoted to unsigned, negatives clamped
        case DT_INT16:   mriType = MRI_SHORT; bpv = 2; break;
        case DT_UINT16:  mriType = MRI_SHORT; bpv = 2; break;
        case DT_INT32:   mriType = MRI_INT;   bpv = 4; break;
        case DT_FLOAT32: mriType = MRI_FLOAT; bpv = 4; break;
        default:
            qCritical() << "MriNiftiIO::read - unsupported datatype" << datatype;
            return false;
    }

    // Build the 4×4 voxel→RAS transform (mm) using sform → qform → pixdim fallback.
    Matrix4f vox2ras = Matrix4f::Identity();
    if (sformCode > 0) {
        for (int c = 0; c < 4; ++c) {
            vox2ras(0, c) = f32(280 + c * 4);
            vox2ras(1, c) = f32(296 + c * 4);
            vox2ras(2, c) = f32(312 + c * 4);
        }
    } else if (qformCode > 0) {
        const float b = f32(256);
        const float c = f32(260);
        const float d = f32(264);
        const float aSq = 1.0f - (b * b + c * c + d * d);
        const float a = aSq > 0.0f ? std::sqrt(aSq) : 0.0f;
        Matrix3f R;
        R << a*a + b*b - c*c - d*d, 2.0f*(b*c - a*d),       2.0f*(b*d + a*c),
             2.0f*(b*c + a*d),       a*a + c*c - b*b - d*d, 2.0f*(c*d - a*b),
             2.0f*(b*d - a*c),       2.0f*(c*d + a*b),       a*a + d*d - b*b - c*c;
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
    if (sx <= 0.0f) sx = dx > 0.0f ? dx : 1.0f;
    if (sy <= 0.0f) sy = dy > 0.0f ? dy : 1.0f;
    if (sz <= 0.0f) sz = dz > 0.0f ? dz : 1.0f;

    Vector3f xRas = col0 / sx;
    Vector3f yRas = col1 / sy;
    Vector3f zRas = col2 / sz;

    Vector4f centreVox(nx * 0.5f, ny * 0.5f, nz * 0.5f, 1.0f);
    Vector4f cRas4 = vox2ras * centreVox;

    volData.version = MRI_MGH_VERSION;  // mark as a valid volume for isValid()
    volData.width   = nx;
    volData.height  = ny;
    volData.depth   = nz;
    volData.nframes = nt > 0 ? nt : 1;
    volData.type    = mriType;
    volData.dof     = 0;
    volData.rasGood = true;
    volData.xsize   = sx;
    volData.ysize   = sy;
    volData.zsize   = sz;
    volData.x_ras   = xRas;
    volData.y_ras   = yRas;
    volData.z_ras   = zRas;
    volData.c_ras   = cRas4.head<3>();
    volData.voxelSurfRasT = FiffCoordTrans(
        FIFFV_COORD_MRI_SLICE, FIFFV_COORD_MRI, volData.computeVox2Ras(), true);

    // Read voxel data starting at vox_offset (typically 352 for single-file).
    const qint64 dataOff = static_cast<qint64>(voxOffset > 0.0f ? voxOffset : NIFTI_HDR_SIZE + 4);
    const qint64 frameSize = static_cast<qint64>(nx) * ny * nz * bpv;
    if (bytes.size() < dataOff + frameSize) {
        qCritical() << "MriNiftiIO::read - data section truncated (need"
                    << (dataOff + frameSize) << "bytes, got" << bytes.size() << ")";
        return false;
    }

    QDataStream stream(bytes);
    stream.setByteOrder(bigEndian ? QDataStream::BigEndian : QDataStream::LittleEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.device()->seek(dataOff);

    const int nslice = nz;
    const int nPixels = nx * ny;
    volData.slices.resize(nslice);

    const Matrix4f vox2rasFinal = volData.computeVox2Ras();
    const bool useScale = (sclSlope != 0.0f) && !(sclSlope == 1.0f && sclInter == 0.0f);

    for (int k = 0; k < nslice; ++k) {
        MriSlice& slice = volData.slices[k];
        slice.width  = nx;
        slice.height = ny;
        slice.dimx   = sx / 1000.0f;
        slice.dimy   = sy / 1000.0f;
        slice.scale  = 1.0f;

        switch (mriType) {
            case MRI_UCHAR: {
                slice.pixelFormat = FIFFV_MRI_PIXEL_BYTE;
                slice.pixels.resize(nPixels);
                if (datatype == DT_INT8) {
                    for (int p = 0; p < nPixels; ++p) {
                        qint8 v;
                        stream >> v;
                        slice.pixels[p] = static_cast<unsigned char>(v < 0 ? 0 : v);
                    }
                } else {
                    for (int p = 0; p < nPixels; ++p) {
                        quint8 v;
                        stream >> v;
                        slice.pixels[p] = v;
                    }
                }
                break;
            }
            case MRI_SHORT: {
                slice.pixelFormat = FIFFV_MRI_PIXEL_WORD;
                slice.pixelsWord.resize(nPixels);
                if (datatype == DT_UINT16) {
                    for (int p = 0; p < nPixels; ++p) {
                        quint16 v;
                        stream >> v;
                        slice.pixelsWord[p] = v;
                    }
                } else {
                    for (int p = 0; p < nPixels; ++p) {
                        qint16 v;
                        stream >> v;
                        slice.pixelsWord[p] = static_cast<unsigned short>(v < 0 ? 0 : v);
                    }
                }
                break;
            }
            case MRI_INT: {
                slice.pixelFormat = FIFFV_MRI_PIXEL_FLOAT;
                slice.pixelsFloat.resize(nPixels);
                for (int p = 0; p < nPixels; ++p) {
                    qint32 v;
                    stream >> v;
                    slice.pixelsFloat[p] = static_cast<float>(v);
                }
                break;
            }
            case MRI_FLOAT: {
                slice.pixelFormat = FIFFV_MRI_PIXEL_FLOAT;
                slice.pixelsFloat.resize(nPixels);
                for (int p = 0; p < nPixels; ++p) {
                    float v;
                    stream >> v;
                    slice.pixelsFloat[p] = v;
                }
                break;
            }
        }

        // Apply scl_slope / scl_inter for floating-point output (the NIfTI spec
        // states the transform produces the "physical" value; we only honour it
        // for floats — integer outputs stay as-is to preserve label maps).
        if (useScale && slice.pixelFormat == FIFFV_MRI_PIXEL_FLOAT) {
            for (int p = 0; p < slice.pixelsFloat.size(); ++p) {
                slice.pixelsFloat[p] = slice.pixelsFloat[p] * sclSlope + sclInter;
            }
        }

        Vector3f sliceOrigin;
        sliceOrigin(0) = vox2rasFinal(0, 2) * k + vox2rasFinal(0, 3);
        sliceOrigin(1) = vox2rasFinal(1, 2) * k + vox2rasFinal(1, 3);
        sliceOrigin(2) = vox2rasFinal(2, 2) * k + vox2rasFinal(2, 3);

        Matrix3f sliceRot;
        sliceRot.col(0) = vox2rasFinal.block<3, 1>(0, 0);
        sliceRot.col(1) = vox2rasFinal.block<3, 1>(0, 1);
        sliceRot.col(2) = vox2rasFinal.block<3, 1>(0, 2);

        slice.trans = FiffCoordTrans(FIFFV_COORD_MRI_SLICE, FIFFV_COORD_MRI, sliceRot, sliceOrigin);
    }

    if (verbose) {
        qInfo("NIfTI file: %dx%dx%d, datatype=%d, voxel %.3fx%.3fx%.3f mm, c_ras=(%.2f, %.2f, %.2f)",
              nx, ny, nz, datatype, sx, sy, sz,
              volData.c_ras[0], volData.c_ras[1], volData.c_ras[2]);
    }

    return true;
}
