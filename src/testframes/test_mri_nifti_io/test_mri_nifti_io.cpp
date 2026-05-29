//=============================================================================================================
/**
 * @file     test_mri_nifti_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    Round-trip tests for the NIfTI-1 reader (.nii and .nii.gz).
 *
 *           A tiny synthetic 4x5x6 float volume with an explicit sform is
 *           written to a temp .nii on disk, loaded via @ref MriVolData::read,
 *           and the geometry, datatype, scalar values, and per-slice
 *           transforms are checked. The same buffer is gzip-compressed in
 *           memory and round-tripped via the @c .nii.gz extension to cover
 *           the decompression path.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mri/mri_nifti_io.h>
#include <mri/mri_types.h>
#include <mri/mri_vol_data.h>

#include <fiff/fiff_file.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QByteArray>
#include <QFile>
#include <QtEndian>
#include <QtTest>

//=============================================================================================================
// SYSTEM INCLUDES
//=============================================================================================================

#include <zlib.h>

#include <cstring>

using namespace MRILIB;

namespace {

QByteArray makeSyntheticNifti(int nx, int ny, int nz, float dx, float dy, float dz)
{
    // 348-byte header + 4-byte gap to vox_offset (352), then nx*ny*nz floats in fortran order.
    const int hdrSize = 348;
    const int dataOff = 352;
    QByteArray buf;
    buf.resize(dataOff + nx * ny * nz * 4);
    std::memset(buf.data(), 0, buf.size());

    auto putI32 = [&](int off, qint32 v) { qint32 le = qToLittleEndian<qint32>(v); std::memcpy(buf.data() + off, &le, 4); };
    auto putI16 = [&](int off, qint16 v) { qint16 le = qToLittleEndian<qint16>(v); std::memcpy(buf.data() + off, &le, 2); };
    auto putF32 = [&](int off, float v) {
        quint32 raw;
        std::memcpy(&raw, &v, 4);
        quint32 le = qToLittleEndian<quint32>(raw);
        std::memcpy(buf.data() + off, &le, 4);
    };

    putI32(0, hdrSize);           // sizeof_hdr
    putI16(40, 3);                // dim[0] = ndim
    putI16(42, static_cast<qint16>(nx));
    putI16(44, static_cast<qint16>(ny));
    putI16(46, static_cast<qint16>(nz));
    putI16(48, 1);                // dim[4] = nt
    putI16(70, 16);               // datatype = DT_FLOAT32
    putI16(72, 32);               // bitpix
    putF32(76, 1.0f);             // pixdim[0] = qfac
    putF32(80, dx);
    putF32(84, dy);
    putF32(88, dz);
    putF32(108, static_cast<float>(dataOff));  // vox_offset
    putF32(112, 1.0f);            // scl_slope
    putF32(116, 0.0f);            // scl_inter
    putI16(254, 1);               // sform_code = NIFTI_XFORM_SCANNER_ANAT
    // srow_x = (dx, 0, 0, -dx*nx/2); srow_y = (0, dy, 0, -dy*ny/2); srow_z = (0, 0, dz, -dz*nz/2)
    putF32(280, dx);  putF32(284, 0); putF32(288, 0); putF32(292, -0.5f * dx * nx);
    putF32(296, 0);   putF32(300, dy); putF32(304, 0); putF32(308, -0.5f * dy * ny);
    putF32(312, 0);   putF32(316, 0); putF32(320, dz); putF32(324, -0.5f * dz * nz);
    std::memcpy(buf.data() + 344, "n+1\0", 4);  // magic

    // Voxel payload: value = ix + 10*iy + 100*iz, in fortran order.
    char* data = buf.data() + dataOff;
    for (int iz = 0; iz < nz; ++iz) {
        for (int iy = 0; iy < ny; ++iy) {
            for (int ix = 0; ix < nx; ++ix) {
                const int idx = ix + nx * (iy + ny * iz);
                float v = static_cast<float>(ix + 10 * iy + 100 * iz);
                quint32 raw;
                std::memcpy(&raw, &v, 4);
                quint32 le = qToLittleEndian<quint32>(raw);
                std::memcpy(data + idx * 4, &le, 4);
            }
        }
    }

    return buf;
}

bool writeGz(const QString& path, const QByteArray& raw)
{
    // Compress in memory with a gzip wrapper (MAX_WBITS + 16), mirroring the
    // library's inflateInit2(&strm, MAX_WBITS + 16) decoder. We deliberately
    // avoid the gz* file helpers (gzopen/gzwrite/gzclose): Qt's bundled zlib
    // (ZlibPrivate, used when no system zlib is present, e.g. on Windows CI)
    // omits those file-I/O symbols, while deflate* is always available.
    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));
    if (deflateInit2(&strm, 6, Z_DEFLATED, MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return false;
    }

    QByteArray out;
    out.resize(static_cast<int>(deflateBound(&strm, static_cast<uLong>(raw.size()))));
    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(raw.constData()));
    strm.avail_in = static_cast<uInt>(raw.size());
    strm.next_out = reinterpret_cast<Bytef*>(out.data());
    strm.avail_out = static_cast<uInt>(out.size());

    const int ret = deflate(&strm, Z_FINISH);
    const uLong produced = strm.total_out;
    deflateEnd(&strm);
    if (ret != Z_STREAM_END) {
        return false;
    }
    out.resize(static_cast<int>(produced));

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    const qint64 written = file.write(out);
    file.close();
    return written == out.size();
}

} // namespace

class TestMriNiftiIo : public QObject
{
    Q_OBJECT

private slots:
    void roundTripNii();
    void roundTripNiiGz();
};

void TestMriNiftiIo::roundTripNii()
{
    const int nx = 4, ny = 5, nz = 6;
    const float dx = 1.5f, dy = 1.25f, dz = 2.0f;
    QByteArray buf = makeSyntheticNifti(nx, ny, nz, dx, dy, dz);

    QTemporaryFile tmp(QDir::tempPath() + QStringLiteral("/mne_nifti_XXXXXX.nii"));
    QVERIFY(tmp.open());
    tmp.write(buf);
    tmp.flush();
    const QString path = tmp.fileName();
    tmp.close();

    MriVolData vol;
    QVERIFY(vol.read(path));
    QVERIFY(vol.isValid());
    QCOMPARE(vol.width, nx);
    QCOMPARE(vol.height, ny);
    QCOMPARE(vol.depth, nz);
    QCOMPARE(vol.slices.size(), nz);
    QVERIFY(std::abs(vol.xsize - dx) < 1e-4f);
    QVERIFY(std::abs(vol.ysize - dy) < 1e-4f);
    QVERIFY(std::abs(vol.zsize - dz) < 1e-4f);

    const auto& sliceMid = vol.slices[nz / 2];
    QCOMPARE(sliceMid.pixelFormat, FIFFV_MRI_PIXEL_FLOAT);
    QCOMPARE(sliceMid.pixelsFloat.size(), nx * ny);
    const float expected = static_cast<float>(0 + 10 * 0 + 100 * (nz / 2));
    QVERIFY(std::abs(sliceMid.pixelsFloat[0] - expected) < 1e-4f);

    const float expectedCorner = static_cast<float>((nx - 1) + 10 * (ny - 1) + 100 * (nz / 2));
    QVERIFY(std::abs(sliceMid.pixelsFloat[nx * ny - 1] - expectedCorner) < 1e-4f);
}

void TestMriNiftiIo::roundTripNiiGz()
{
    const int nx = 3, ny = 3, nz = 3;
    QByteArray buf = makeSyntheticNifti(nx, ny, nz, 1.0f, 1.0f, 1.0f);

    QTemporaryFile tmp(QDir::tempPath() + QStringLiteral("/mne_nifti_XXXXXX.nii.gz"));
    QVERIFY(tmp.open());
    const QString path = tmp.fileName();
    tmp.close();
    tmp.setAutoRemove(true);
    QVERIFY2(writeGz(path, buf), "failed to gzip synthetic NIfTI");

    MriVolData vol;
    QVERIFY(vol.read(path));
    QCOMPARE(vol.width, nx);
    QCOMPARE(vol.depth, nz);
    QVERIFY(!vol.slices.isEmpty());
    QCOMPARE(vol.slices.last().pixelsFloat.size(), nx * ny);
}

QTEST_APPLESS_MAIN(TestMriNiftiIo)
#include "test_mri_nifti_io.moc"
