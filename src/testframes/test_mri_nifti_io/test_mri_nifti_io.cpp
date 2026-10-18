//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mri_nifti_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
#include <QTemporaryDir>

//=============================================================================================================
// SYSTEM INCLUDES
//=============================================================================================================

#include <zlib.h>

#include <algorithm>
#include <cstring>

using namespace MRILIB;

namespace
{

QByteArray makeSyntheticNifti(int nx, int ny, int nz, float dx, float dy, float dz)
{
    // 348-byte header + 4-byte gap to vox_offset (352), then nx*ny*nz floats in fortran order.
    const int hdrSize = 348;
    const int dataOff = 352;
    QByteArray buf;
    buf.resize(dataOff + nx * ny * nz * 4);
    std::memset(buf.data(), 0, buf.size());

    auto putI32 = [&](int off, qint32 v) {
        qint32 le = qToLittleEndian<qint32>(v);
        std::memcpy(buf.data() + off, &le, 4);
    };
    auto putI16 = [&](int off, qint16 v) {
        qint16 le = qToLittleEndian<qint16>(v);
        std::memcpy(buf.data() + off, &le, 2);
    };
    auto putF32 = [&](int off, float v) {
        quint32 raw;
        std::memcpy(&raw, &v, 4);
        quint32 le = qToLittleEndian<quint32>(raw);
        std::memcpy(buf.data() + off, &le, 4);
    };

    putI32(0, hdrSize); // sizeof_hdr
    putI16(40, 3);      // dim[0] = ndim
    putI16(42, static_cast<qint16>(nx));
    putI16(44, static_cast<qint16>(ny));
    putI16(46, static_cast<qint16>(nz));
    putI16(48, 1);    // dim[4] = nt
    putI16(70, 16);   // datatype = DT_FLOAT32
    putI16(72, 32);   // bitpix
    putF32(76, 1.0f); // pixdim[0] = qfac
    putF32(80, dx);
    putF32(84, dy);
    putF32(88, dz);
    putF32(108, static_cast<float>(dataOff)); // vox_offset
    putF32(112, 1.0f);                        // scl_slope
    putF32(116, 0.0f);                        // scl_inter
    putI16(254, 1);                           // sform_code = NIFTI_XFORM_SCANNER_ANAT
    // srow_x = (dx, 0, 0, -dx*nx/2); srow_y = (0, dy, 0, -dy*ny/2); srow_z = (0, 0, dz, -dz*nz/2)
    putF32(280, dx);
    putF32(284, 0);
    putF32(288, 0);
    putF32(292, -0.5f * dx * nx);
    putF32(296, 0);
    putF32(300, dy);
    putF32(304, 0);
    putF32(308, -0.5f * dy * ny);
    putF32(312, 0);
    putF32(316, 0);
    putF32(320, dz);
    putF32(324, -0.5f * dz * nz);
    std::memcpy(buf.data() + 344, "n+1\0", 4); // magic

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
    void readsEveryDatatypeAndForm_data();
    void readsEveryDatatypeAndForm();
    void rejectsBrokenFiles();
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

namespace
{

// Rewrites the synthetic float volume as another datatype and byte order, with a qform instead of the sform
QByteArray variant(qint16 datatype, int bytesPerVoxel, bool bigEndian, bool qform, float slope, float inter)
{
    const int nx = 4, ny = 5, nz = 6, n = nx * ny * nz;
    QByteArray buf = makeSyntheticNifti(nx, ny, nz, 1.5f, 1.25f, 2.0f);
    buf.resize(352 + n * bytesPerVoxel);
    const auto put = [&](int off, const void* le, int size) {
        char* dst = buf.data() + off;
        std::memcpy(dst, le, size);
        if (bigEndian)
            std::reverse(dst, dst + size);
    };
    const auto putI16 = [&](int off, qint16 v) {
        put(off, &v, 2);
    };
    const auto putF32 = [&](int off, float v) {
        put(off, &v, 4);
    };
    const qint32 hdr = 348;
    put(0, &hdr, 4);
    for (const auto& [off, v] : {std::pair{40, 3}, std::pair{42, nx}, std::pair{44, ny}, std::pair{46, nz}, std::pair{48, 1}})
        putI16(off, static_cast<qint16>(v));
    putI16(70, datatype);
    putI16(72, static_cast<qint16>(8 * bytesPerVoxel));
    putF32(76, qform ? -1.0f : 1.0f);
    putF32(80, 1.5f);
    putF32(84, 1.25f);
    putF32(88, 2.0f);
    putF32(108, 352.0f);
    putF32(112, slope);
    putF32(116, inter);
    putI16(252, qform ? 1 : 0);
    putI16(254, qform ? 0 : 1);
    for (int k = 0; k < 12; ++k) {
        float sform;
        std::memcpy(&sform, buf.constData() + 280 + 4 * k, 4);
        putF32(280 + 4 * k, qFromLittleEndian(sform));
    }
    for (const auto& [off, v] : {std::pair{256, 0.1f}, std::pair{260, 0.2f}, std::pair{264, 0.3f}, std::pair{268, 10.0f}, std::pair{272, -20.0f}, std::pair{276, 30.0f}})
        putF32(off, v);
    // Voxel i holds i - 30, so that the signed types see negative values
    for (int i = 0; i < n; ++i) {
        const qint32 value = i - 30;
        const float f = static_cast<float>(value);
        const qint8 i8 = static_cast<qint8>(value);
        const qint16 i16 = static_cast<qint16>(value);
        put(352 + i * bytesPerVoxel, datatype == 16 ? static_cast<const void*>(&f) : bytesPerVoxel == 1 ? static_cast<const void*>(&i8)
                : bytesPerVoxel == 2                                                                    ? static_cast<const void*>(&i16)
                                                                                                        : static_cast<const void*>(&value),
            bytesPerVoxel);
    }
    return buf;
}

QString writeTemp(QTemporaryDir& dir, const QString& name, const QByteArray& bytes)
{
    QFile f(dir.filePath(name));
    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size())
        return {};
    return f.fileName();
}

} // namespace

void TestMriNiftiIo::readsEveryDatatypeAndForm_data()
{
    QTest::addColumn<int>("datatype");
    QTest::addColumn<int>("bytes");
    QTest::addColumn<bool>("bigEndian");
    QTest::addColumn<bool>("qform");
    QTest::addColumn<float>("slope");
    QTest::addColumn<float>("inter");
    QTest::newRow("uint8") << 2 << 1 << false << false << 1.0f << 0.0f;
    QTest::newRow("int8 big-endian") << 256 << 1 << true << false << 1.0f << 0.0f;
    QTest::newRow("int16 big-endian qform") << 4 << 2 << true << true << 1.0f << 0.0f;
    QTest::newRow("uint16") << 512 << 2 << false << true << 1.0f << 0.0f;
    QTest::newRow("int32 scaled") << 8 << 4 << false << false << 2.0f << 1.0f;
    QTest::newRow("float32 big-endian scaled") << 16 << 4 << true << true << 0.5f << -3.0f;
}

void TestMriNiftiIo::readsEveryDatatypeAndForm()
{
    QFETCH(int, datatype);
    QFETCH(int, bytes);
    QFETCH(bool, bigEndian);
    QFETCH(bool, qform);
    QFETCH(float, slope);
    QFETCH(float, inter);
    QTemporaryDir dir;
    MriVolData vol;
    QVERIFY(vol.read(writeTemp(dir, "v.nii", variant(static_cast<qint16>(datatype), bytes, bigEndian, qform, slope, inter))));
    QCOMPARE(vol.width, 4);
    QCOMPARE(vol.depth, 6);

    // The affine is the one nibabel 5.3.3 reports (get_qform / get_sform), in metres
    Eigen::Matrix4f expected;
    if (qform)
        expected << 1.109999975f, -0.64552141f, -0.861889494f, 10.0f, 0.894625695f, 0.999999981f, 0.13094473f, -20.0f,
            -0.46641711f, 0.381840473f, -1.799999994f, 30.0f, 0, 0, 0, 1;
    else
        expected << 1.5f, 0, 0, -3.0f, 0, 1.25f, 0, -3.125f, 0, 0, 2.0f, -6.0f, 0, 0, 0, 1;
    expected.topRows(3) /= 1000.0f;
    QVERIFY2((vol.computeVox2Ras() - expected).cwiseAbs().maxCoeff() < 1e-6f, qPrintable(QString::number((vol.computeVox2Ras() - expected).cwiseAbs().maxCoeff())));

    // Integer data stay as stored, negative values of signed types clamp to zero, and only real values are scaled
    for (const int i : {0, 29, 30, 119}) {
        const MriSlice& slice = vol.slices[i / 20];
        const int p = i % 20;
        const int stored = i - 30;
        switch (datatype) {
            case 2:
                QCOMPARE(int(slice.pixels[p]), stored & 0xff);
                break;
            case 256:
                QCOMPARE(int(slice.pixels[p]), std::max(stored, 0));
                break;
            case 4:
                QCOMPARE(int(slice.pixelsWord[p]), std::max(stored, 0));
                break;
            case 512:
                QCOMPARE(int(slice.pixelsWord[p]), stored & 0xffff);
                break;
            default:
                QCOMPARE(slice.pixelsFloat[p], stored * slope + inter);
        }
    }
}

void TestMriNiftiIo::rejectsBrokenFiles()
{
    QTemporaryDir dir;
    const QByteArray good = variant(16, 4, false, false, 1.0f, 0.0f);
    MriVolData vol;
    QVERIFY(vol.read(writeTemp(dir, "good.nii", good)));
    const auto withI16 = [&good](int off, qint16 v) {
        QByteArray b = good;
        qToLittleEndian(v, b.data() + off);
        return b;
    };
    QByteArray notNifti = good;
    qToLittleEndian<qint32>(347, notNifti.data());
    const QList<std::pair<QString, QByteArray>> broken = {
        {"short.nii", good.left(100)},
        {"magic.nii", notNifti},
        {"flat.nii", withI16(40, 2)},
        {"double.nii", withI16(70, 64)},
        {"truncated.nii", good.left(good.size() - 4)},
        {"empty.nii.gz", QByteArray()},
        {"garbage.nii.gz", QByteArray("not gzip at all")},
    };
    for (const auto& [name, bytes] : broken) {
        QVERIFY2(!MriNiftiIO::read(writeTemp(dir, name, bytes), vol), qPrintable(name));
    }
    QVERIFY(!MriNiftiIO::read(dir.filePath("missing.nii"), vol));

    // A gzip stream cut short ends in an error, not an endless wait for more input
    const QString gz = dir.filePath("cut.nii.gz");
    QVERIFY(writeGz(gz, good));
    QFile cut(gz);
    QVERIFY(cut.open(QIODevice::ReadWrite));
    QVERIFY(cut.resize(cut.size() / 2));
    cut.close();
    QVERIFY(!MriNiftiIO::read(gz, vol));
}

QTEST_APPLESS_MAIN(TestMriNiftiIo)
#include "test_mri_nifti_io.moc"
