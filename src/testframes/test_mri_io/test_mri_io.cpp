//=============================================================================================================
/**
 * @file     test_mri_io.cpp
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
 * @brief    Unit tests for the mne_mri library.
 *
 *           Tests cover:
 *             - MriVolData: default construction, isValid(), computeVox2Ras()
 *             - MriMghIO: reading .mgz files, header parsing, slice data, transforms
 *             - MriCorFifIO: write-then-read round-trip of COR.fif
 *             - MriTypes: constant values
 *
 *           Uses FreeSurfer sample data from MNE-sample-data when available.
 *           Unit tests that don't require sample data always run.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>

#include <mri/mri_types.h>
#include <mri/mri_vol_data.h>
#include <mri/mri_mgh_io.h>
#include <mri/mri_cor_io.h>
#include <mri/mri_cor_fif_io.h>

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_file.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryDir>
#include <QDataStream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MRILIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestMriIO
 *
 * @brief Unit tests for the mne_mri library (MriVolData, MriMghIO, MriCorFifIO).
 */
class TestMriIO : public QObject
{
    Q_OBJECT

public:
    TestMriIO();

private slots:
    void initTestCase();

    // MriTypes constants
    void testConstants();

    // MriVolData unit tests
    void testVolDataDefaultConstruction();
    void testVolDataIsValid();
    void testVolDataComputeVox2Ras_identity();
    void testVolDataComputeVox2Ras_sampleGeometry();

    // MriMghIO tests (require sample data)
    void testMghReadT1();
    void testMghReadBrain();
    void testMghHeaderGeometry();
    void testMghSliceTransforms();
    void testMghInvalidFile();

    // MriMghIO synthetic unit tests (no sample data needed)
    void testMghReadSyntheticUchar();
    void testMghReadSyntheticShort();
    void testMghReadSyntheticInt();
    void testMghReadSyntheticFloat();
    void testMghDefaultDirectionCosines();
    void testMghFooterScanParams();
    void testMghFooterTalairachTag();
    void testMghUnsupportedDataType();
    void testMghCorruptedMgz();
    void testMghEmptyFile();

    // MriCorFifIO round-trip tests
    void testCorFifWriteRead();
    void testCorFifWriteReadSynthetic();

    void cleanupTestCase();

private:
    /** Find the MNE sample data subjects directory. */
    QString findSubjectsDir();

    /** Create a synthetic MGH byte array for testing. */
    static QByteArray createSyntheticMgh(int width, int height, int depth, int type,
                                         bool setRas, bool addScanParams = false,
                                         float tr = 0.0f, float flipAngle = 0.0f,
                                         float te = 0.0f, float ti = 0.0f, float fov = 0.0f);

    /** Write a QByteArray to a file. */
    bool writeToFile(const QString& path, const QByteArray& data);

    bool m_bDataAvailable;      /**< Whether sample data is found. */
    QString m_sSubjectsDir;     /**< Path to subjects dir. */
    QString m_sMriDir;          /**< Path to sample/mri/. */
    MriVolData m_volT1;         /**< Loaded T1 volume (populated in initTestCase). */
    QVector<FiffCoordTrans> m_transT1;  /**< Additional transforms from T1. */
    QTemporaryDir m_tempDir;    /**< Temporary directory for output files. */
};

//=============================================================================================================

TestMriIO::TestMriIO()
: m_bDataAvailable(false)
{
}

//=============================================================================================================

QByteArray TestMriIO::createSyntheticMgh(int width, int height, int depth, int type,
                                         bool setRas, bool addScanParams,
                                         float tr, float flipAngle,
                                         float te, float ti, float fov)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    // Header (7 × int32 + 1 × int16 = 30 bytes)
    stream << qint32(MRI_MGH_VERSION)
           << qint32(width) << qint32(height) << qint32(depth)
           << qint32(1)       // nframes
           << qint32(type)
           << qint32(0);      // dof

    if (setRas) {
        stream << qint16(1);  // goodRASflag
        stream << 1.0f << 1.0f << 1.0f;            // xsize, ysize, zsize
        stream << -1.0f << 0.0f << 0.0f;           // x_ras
        stream <<  0.0f << 0.0f << -1.0f;          // y_ras
        stream <<  0.0f << 1.0f <<  0.0f;          // z_ras
        stream <<  0.0f << 0.0f <<  0.0f;          // c_ras
    } else {
        stream << qint16(0);  // goodRASflag
    }

    // Pad header to 284 bytes
    int bytesWritten = data.size();
    for (int i = bytesWritten; i < MRI_MGH_DATA_OFFSET; ++i) {
        stream << quint8(0);
    }

    // Voxel data
    int nVoxels = width * height * depth;
    for (int i = 0; i < nVoxels; ++i) {
        switch (type) {
            case MRI_UCHAR: stream << quint8(i % 256); break;
            case MRI_SHORT: stream << qint16(i % 1000); break;
            case MRI_INT:   stream << qint32(i); break;
            case MRI_FLOAT: stream << float(i * 0.5f); break;
            default:        stream << quint8(0); break;
        }
    }

    // Footer: scan parameters
    if (addScanParams) {
        stream << tr << flipAngle << te << ti << fov;
    }

    return data;
}

//=============================================================================================================

bool TestMriIO::writeToFile(const QString& path, const QByteArray& data)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(data);
    f.close();
    return true;
}

//=============================================================================================================

QString TestMriIO::findSubjectsDir()
{
    // Try environment variable first
    QString envDir = qEnvironmentVariable("SUBJECTS_DIR");
    if (!envDir.isEmpty() && QDir(envDir + "/sample/mri").exists()) {
        return envDir;
    }

    // Try standard MNE-sample-data location
    QString home = QDir::homePath();
    QString stdPath = home + "/mne_data/MNE-sample-data/subjects";
    if (QDir(stdPath + "/sample/mri").exists()) {
        return stdPath;
    }

    return QString();
}

//=============================================================================================================

void TestMriIO::initTestCase()
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);

    QVERIFY(m_tempDir.isValid());

    m_sSubjectsDir = findSubjectsDir();
    if (!m_sSubjectsDir.isEmpty()) {
        m_sMriDir = m_sSubjectsDir + "/sample/mri";
        m_bDataAvailable = true;
        qDebug() << "Sample data found at" << m_sMriDir;

        // Pre-load T1 volume for multiple tests
        QString t1Path = m_sMriDir + "/T1.mgz";
        if (QFile::exists(t1Path)) {
            bool ok = MriMghIO::read(t1Path, m_volT1, m_transT1, m_sMriDir, false);
            QVERIFY2(ok, "Failed to pre-load T1.mgz in initTestCase");
        }
    } else {
        qDebug() << "MNE sample data not found — skipping data-dependent tests";
    }
}

//=============================================================================================================
// MriTypes constant tests
//=============================================================================================================

void TestMriIO::testConstants()
{
    // Verify MGH format constants match FreeSurfer specification
    QCOMPARE(MRI_MGH_VERSION, 1);
    QCOMPARE(MRI_MGH_DATA_OFFSET, 284);
    QCOMPARE(MRI_UCHAR, 0);
    QCOMPARE(MRI_INT, 1);
    QCOMPARE(MRI_FLOAT, 3);
    QCOMPARE(MRI_SHORT, 4);
    QCOMPARE(MGH_TAG_MGH_XFORM, 31);

    // COR constants
    QCOMPARE(COR_NSLICE, 256);
    QCOMPARE(COR_WIDTH, 256);
    QCOMPARE(COR_HEIGHT, 256);
    QVERIFY(qFuzzyCompare(COR_PIXEL_SIZE, 1e-3f));
}

//=============================================================================================================
// MriVolData unit tests
//=============================================================================================================

void TestMriIO::testVolDataDefaultConstruction()
{
    MriVolData vol;

    // Default construction should leave fields at zero/default
    QCOMPARE(vol.version, 0);
    QCOMPARE(vol.width, 0);
    QCOMPARE(vol.height, 0);
    QCOMPARE(vol.depth, 0);
    QCOMPARE(vol.nframes, 0);
    QCOMPARE(vol.type, MRI_UCHAR);
    QCOMPARE(vol.dof, 0);
    QCOMPARE(vol.rasGood, false);
    QVERIFY(qFuzzyCompare(vol.xsize, 1.0f));
    QVERIFY(qFuzzyCompare(vol.ysize, 1.0f));
    QVERIFY(qFuzzyCompare(vol.zsize, 1.0f));

    // Default direction cosines (FreeSurfer defaults)
    QVERIFY(qFuzzyCompare(vol.x_ras[0], -1.0f));
    QVERIFY(qFuzzyCompare(1.0f + vol.x_ras[1], 1.0f)); // ~0
    QVERIFY(qFuzzyCompare(1.0f + vol.x_ras[2], 1.0f));
    QVERIFY(qFuzzyCompare(1.0f + vol.y_ras[0], 1.0f));
    QVERIFY(qFuzzyCompare(1.0f + vol.y_ras[1], 1.0f));
    QVERIFY(qFuzzyCompare(vol.y_ras[2], -1.0f));
    QVERIFY(qFuzzyCompare(1.0f + vol.z_ras[0], 1.0f));
    QVERIFY(qFuzzyCompare(vol.z_ras[1], 1.0f));
    QVERIFY(qFuzzyCompare(1.0f + vol.z_ras[2], 1.0f));

    // Center RAS defaults to zero
    QVERIFY(qFuzzyCompare(1.0f + vol.c_ras[0], 1.0f));
    QVERIFY(qFuzzyCompare(1.0f + vol.c_ras[1], 1.0f));
    QVERIFY(qFuzzyCompare(1.0f + vol.c_ras[2], 1.0f));

    // Should have no slices
    QVERIFY(vol.slices.isEmpty());
    QVERIFY(vol.talairachXfmPath.isEmpty());
}

//=============================================================================================================

void TestMriIO::testVolDataIsValid()
{
    MriVolData vol;
    // Default-constructed should be invalid
    QVERIFY(!vol.isValid());

    // Set version and dimensions — should become valid
    vol.version = MRI_MGH_VERSION;
    vol.width = 256;
    vol.height = 256;
    vol.depth = 256;
    QVERIFY(vol.isValid());

    // Wrong version — invalid
    vol.version = 99;
    QVERIFY(!vol.isValid());

    // Zero dimension — invalid
    vol.version = MRI_MGH_VERSION;
    vol.width = 0;
    QVERIFY(!vol.isValid());
}

//=============================================================================================================

void TestMriIO::testVolDataComputeVox2Ras_identity()
{
    // With default direction cosines and 1mm voxels, center at origin,
    // and 256^3 volume, the vox2ras should produce known values.
    MriVolData vol;
    vol.version = MRI_MGH_VERSION;
    vol.width = 256;
    vol.height = 256;
    vol.depth = 256;
    vol.xsize = 1.0f;
    vol.ysize = 1.0f;
    vol.zsize = 1.0f;
    // Default direction cosines: x=(-1,0,0), y=(0,0,-1), z=(0,1,0)
    // center at (0,0,0)

    Matrix4f vox2ras = vol.computeVox2Ras();

    // Check that it's a proper 4x4 with [0,0,0,1] bottom row
    QVERIFY(qFuzzyCompare(vox2ras(3, 0), 0.0f));
    QVERIFY(qFuzzyCompare(vox2ras(3, 1), 0.0f));
    QVERIFY(qFuzzyCompare(vox2ras(3, 2), 0.0f));
    QVERIFY(qFuzzyCompare(vox2ras(3, 3), 1.0f));

    // The diagonal should be (-0.001, 0, 0) for x, etc. (1mm / 1000)
    float eps = 1e-6f;
    QVERIFY(std::abs(vox2ras(0, 0) - (-0.001f)) < eps);  // x_ras[0]*xsize/1000
    QVERIFY(std::abs(vox2ras(1, 2) - (0.001f)) < eps);   // z_ras[1]*zsize/1000

    // P0 = c_ras - M * center = 0 - M * 128
    // P0_x = 0 - (-0.001)*128 = 0.128
    QVERIFY(std::abs(vox2ras(0, 3) - 0.128f) < eps);
}

//=============================================================================================================

void TestMriIO::testVolDataComputeVox2Ras_sampleGeometry()
{
    if (!m_bDataAvailable) {
        QSKIP("Sample data not available");
    }

    // The T1.mgz from MNE sample data has known geometry
    QVERIFY(m_volT1.isValid());

    Matrix4f vox2ras = m_volT1.computeVox2Ras();

    // Known values from previous successful test runs:
    // Voxel sizes: 1.0 x 1.0 x 1.0 mm
    // c_ras: -5.2736 9.0391 -27.2880
    // vox2ras(0,3) = 0.122726  (P0_x in meters)
    float eps = 1e-4f;
    QVERIFY(std::abs(vox2ras(0, 3) - 0.122726f) < eps);
    QVERIFY(std::abs(vox2ras(1, 3) - (-0.118961f)) < eps);
    QVERIFY(std::abs(vox2ras(2, 3) - 0.100712f) < eps);

    // Diagonal should be close to ±0.001 (1mm voxels in meters)
    QVERIFY(std::abs(std::abs(vox2ras(0, 0)) - 0.001f) < eps);
}

//=============================================================================================================
// MriMghIO tests
//=============================================================================================================

void TestMriIO::testMghReadT1()
{
    if (!m_bDataAvailable) {
        QSKIP("Sample data not available");
    }

    // T1 was pre-loaded in initTestCase
    QVERIFY(m_volT1.isValid());
    QCOMPARE(m_volT1.version, MRI_MGH_VERSION);
    QCOMPARE(m_volT1.width, 256);
    QCOMPARE(m_volT1.height, 256);
    QCOMPARE(m_volT1.depth, 256);
    QCOMPARE(m_volT1.nframes, 1);
    QCOMPARE(m_volT1.type, MRI_UCHAR);
    QVERIFY(m_volT1.rasGood);
    QVERIFY(qFuzzyCompare(m_volT1.xsize, 1.0f));
    QVERIFY(qFuzzyCompare(m_volT1.ysize, 1.0f));
    QVERIFY(qFuzzyCompare(m_volT1.zsize, 1.0f));

    // Should have 256 slices (one per depth)
    QCOMPARE(m_volT1.slices.size(), 256);

    // Verify scan parameters were parsed from footer
    QVERIFY(m_volT1.TR > 0.0f);           // T1 MPRAGE has non-zero TR
    QVERIFY(m_volT1.flipAngle > 0.0f);    // Non-zero flip angle
}

//=============================================================================================================

void TestMriIO::testMghReadBrain()
{
    if (!m_bDataAvailable) {
        QSKIP("Sample data not available");
    }

    QString brainPath = m_sMriDir + "/brain.mgz";
    if (!QFile::exists(brainPath)) {
        QSKIP("brain.mgz not found");
    }

    MriVolData vol;
    QVector<FiffCoordTrans> trans;
    bool ok = MriMghIO::read(brainPath, vol, trans, m_sMriDir, false);
    QVERIFY(ok);
    QVERIFY(vol.isValid());
    QCOMPARE(vol.width, 256);
    QCOMPARE(vol.height, 256);
    QCOMPARE(vol.depth, 256);
    QCOMPARE(vol.slices.size(), 256);

    // brain.mgz should have the same geometry as T1 (same volume, just skull-stripped)
    float eps = 1e-4f;
    QVERIFY(std::abs(vol.c_ras[0] - m_volT1.c_ras[0]) < eps);
    QVERIFY(std::abs(vol.c_ras[1] - m_volT1.c_ras[1]) < eps);
    QVERIFY(std::abs(vol.c_ras[2] - m_volT1.c_ras[2]) < eps);
}

//=============================================================================================================

void TestMriIO::testMghHeaderGeometry()
{
    if (!m_bDataAvailable) {
        QSKIP("Sample data not available");
    }

    // Verify direction cosines are valid unit vectors
    float lenX = std::sqrt(m_volT1.x_ras[0] * m_volT1.x_ras[0]
                         + m_volT1.x_ras[1] * m_volT1.x_ras[1]
                         + m_volT1.x_ras[2] * m_volT1.x_ras[2]);
    float lenY = std::sqrt(m_volT1.y_ras[0] * m_volT1.y_ras[0]
                         + m_volT1.y_ras[1] * m_volT1.y_ras[1]
                         + m_volT1.y_ras[2] * m_volT1.y_ras[2]);
    float lenZ = std::sqrt(m_volT1.z_ras[0] * m_volT1.z_ras[0]
                         + m_volT1.z_ras[1] * m_volT1.z_ras[1]
                         + m_volT1.z_ras[2] * m_volT1.z_ras[2]);

    float eps = 1e-5f;
    QVERIFY(std::abs(lenX - 1.0f) < eps);
    QVERIFY(std::abs(lenY - 1.0f) < eps);
    QVERIFY(std::abs(lenZ - 1.0f) < eps);
}

//=============================================================================================================

void TestMriIO::testMghSliceTransforms()
{
    if (!m_bDataAvailable) {
        QSKIP("Sample data not available");
    }

    QCOMPARE(m_volT1.slices.size(), 256);

    // Each slice should have valid dimensions and pixel data
    for (int k = 0; k < m_volT1.slices.size(); ++k) {
        const MriSlice& slice = m_volT1.slices[k];
        QCOMPARE(slice.width, 256);
        QCOMPARE(slice.height, 256);
        QCOMPARE(slice.pixels.size(), 256 * 256);
        QCOMPARE(slice.pixelFormat, FIFFV_MRI_PIXEL_BYTE);

        // Pixel dimensions should be in meters (1mm = 0.001m)
        float eps = 1e-6f;
        QVERIFY(std::abs(slice.dimx - 0.001f) < eps);
        QVERIFY(std::abs(slice.dimy - 0.001f) < eps);

        // Each slice transform should have correct coordinate frames
        QCOMPARE(slice.trans.from, FIFFV_COORD_MRI_SLICE);
        QCOMPARE(slice.trans.to, FIFFV_COORD_MRI);
    }

    // First and last slice should have different origin translations
    // The slice origin = vox2ras * [0, 0, k, 1]^T, so translation differs
    // by vox2ras.col(2) * 255 across the volume
    Vector3f t0(m_volT1.slices[0].trans.trans(0, 3),
                m_volT1.slices[0].trans.trans(1, 3),
                m_volT1.slices[0].trans.trans(2, 3));
    Vector3f t255(m_volT1.slices[255].trans.trans(0, 3),
                  m_volT1.slices[255].trans.trans(1, 3),
                  m_volT1.slices[255].trans.trans(2, 3));
    float dist = (t255 - t0).norm();
    QVERIFY(dist > 0.01f);  // Should be ~0.255 m (255 × 1mm)
}

//=============================================================================================================

void TestMriIO::testMghInvalidFile()
{
    MriVolData vol;
    QVector<FiffCoordTrans> trans;

    // Non-existent file
    bool ok = MriMghIO::read("/nonexistent/file.mgz", vol, trans);
    QVERIFY(!ok);

    // Create a file that is too small
    QString tinyFile = m_tempDir.path() + "/tiny.mgh";
    {
        QFile f(tinyFile);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("TOO_SMALL");
        f.close();
    }
    ok = MriMghIO::read(tinyFile, vol, trans);
    QVERIFY(!ok);

    // Create a file with wrong version
    QString badVersion = m_tempDir.path() + "/badversion.mgh";
    {
        QFile f(badVersion);
        QVERIFY(f.open(QIODevice::WriteOnly));
        QDataStream ds(&f);
        ds.setByteOrder(QDataStream::BigEndian);
        qint32 wrongVersion = 99;
        ds << wrongVersion;
        // Pad to at least 284 bytes
        QByteArray padding(280, '\0');
        f.write(padding);
        f.close();
    }
    ok = MriMghIO::read(badVersion, vol, trans);
    QVERIFY(!ok);
}

//=============================================================================================================

void TestMriIO::testCorFifWriteRead()
{
    if (!m_bDataAvailable) {
        QSKIP("Sample data not available");
    }

    // Write COR.fif from T1 data, then read it back and verify structure
    QString corFifPath = m_tempDir.path() + "/COR_test.fif";

    bool ok = MriCorFifIO::write(corFifPath, m_volT1.slices, m_transT1);
    QVERIFY2(ok, "MriCorFifIO::write failed");
    QVERIFY(QFile::exists(corFifPath));

    // Verify file size is reasonable (256 slices × 256×256 bytes + overhead ≈ 16MB)
    QFileInfo fi(corFifPath);
    QVERIFY(fi.size() > 10 * 1024 * 1024);  // > 10 MB
    QVERIFY(fi.size() < 30 * 1024 * 1024);  // < 30 MB

    // Read back and verify FIFF structure
    QFile file(corFifPath);
    FiffStream::SPtr stream(new FiffStream(&file));
    QVERIFY(stream->open());

    // Find and verify blocks
    // The file should have FIFFB_MRI block
    FiffTag::SPtr tag;
    bool foundMriBlock = false;
    bool foundMriSet = false;
    int sliceCount = 0;

    // Scan through the file tags
    stream->device()->seek(0);
    while (!stream->device()->atEnd()) {
        stream->read_tag(tag);
        if (!tag) break;

        if (tag->kind == FIFF_BLOCK_START) {
            fiff_int_t blockKind = *tag->toInt();
            if (blockKind == FIFFB_MRI) foundMriBlock = true;
            if (blockKind == FIFFB_MRI_SET) foundMriSet = true;
            if (blockKind == FIFFB_MRI_SLICE) sliceCount++;
        }
    }

    QVERIFY2(foundMriBlock, "No FIFFB_MRI block found");
    QVERIFY2(foundMriSet, "No FIFFB_MRI_SET block found");
    QCOMPARE(sliceCount, 256);

    stream->close();
}

//=============================================================================================================
// Synthetic MGH unit tests (no sample data needed)
//=============================================================================================================

void TestMriIO::testMghReadSyntheticUchar()
{
    // 4×4×2 UCHAR volume with RAS info
    QByteArray mghData = createSyntheticMgh(4, 4, 2, MRI_UCHAR, true);
    QString path = m_tempDir.path() + "/synth_uchar.mgh";
    QVERIFY(writeToFile(path, mghData));

    MriVolData vol;
    QVector<FiffCoordTrans> trans;
    bool ok = MriMghIO::read(path, vol, trans);
    QVERIFY(ok);
    QVERIFY(vol.isValid());
    QCOMPARE(vol.width, 4);
    QCOMPARE(vol.height, 4);
    QCOMPARE(vol.depth, 2);
    QCOMPARE(vol.type, MRI_UCHAR);
    QVERIFY(vol.rasGood);
    QCOMPARE(vol.slices.size(), 2);
    QCOMPARE(vol.slices[0].pixelFormat, FIFFV_MRI_PIXEL_BYTE);
    QCOMPARE(vol.slices[0].pixels.size(), 16);

    // Verify first pixel values (cycling through 0..255)
    QCOMPARE(static_cast<int>(vol.slices[0].pixels[0]), 0);
    QCOMPARE(static_cast<int>(vol.slices[0].pixels[1]), 1);
}

//=============================================================================================================

void TestMriIO::testMghReadSyntheticShort()
{
    QByteArray mghData = createSyntheticMgh(4, 4, 2, MRI_SHORT, true);
    QString path = m_tempDir.path() + "/synth_short.mgh";
    QVERIFY(writeToFile(path, mghData));

    MriVolData vol;
    QVector<FiffCoordTrans> trans;
    bool ok = MriMghIO::read(path, vol, trans);
    QVERIFY(ok);
    QVERIFY(vol.isValid());
    QCOMPARE(vol.type, MRI_SHORT);
    QCOMPARE(vol.slices.size(), 2);
    QCOMPARE(vol.slices[0].pixelFormat, FIFFV_MRI_PIXEL_WORD);
    QCOMPARE(vol.slices[0].pixelsWord.size(), 16);

    // Verify first pixel values
    QCOMPARE(static_cast<int>(vol.slices[0].pixelsWord[0]), 0);
    QCOMPARE(static_cast<int>(vol.slices[0].pixelsWord[1]), 1);
}

//=============================================================================================================

void TestMriIO::testMghReadSyntheticInt()
{
    QByteArray mghData = createSyntheticMgh(4, 4, 2, MRI_INT, true);
    QString path = m_tempDir.path() + "/synth_int.mgh";
    QVERIFY(writeToFile(path, mghData));

    MriVolData vol;
    QVector<FiffCoordTrans> trans;
    bool ok = MriMghIO::read(path, vol, trans);
    QVERIFY(ok);
    QVERIFY(vol.isValid());
    QCOMPARE(vol.type, MRI_INT);
    QCOMPARE(vol.slices.size(), 2);
    // INT is converted to FLOAT in the reader
    QCOMPARE(vol.slices[0].pixelFormat, FIFFV_MRI_PIXEL_FLOAT);
    QCOMPARE(vol.slices[0].pixelsFloat.size(), 16);

    // Verify conversion: voxel i -> float(i)
    float eps = 1e-5f;
    QVERIFY(std::abs(vol.slices[0].pixelsFloat[0] - 0.0f) < eps);
    QVERIFY(std::abs(vol.slices[0].pixelsFloat[1] - 1.0f) < eps);
}

//=============================================================================================================

void TestMriIO::testMghReadSyntheticFloat()
{
    QByteArray mghData = createSyntheticMgh(4, 4, 2, MRI_FLOAT, true);
    QString path = m_tempDir.path() + "/synth_float.mgh";
    QVERIFY(writeToFile(path, mghData));

    MriVolData vol;
    QVector<FiffCoordTrans> trans;
    bool ok = MriMghIO::read(path, vol, trans);
    QVERIFY(ok);
    QVERIFY(vol.isValid());
    QCOMPARE(vol.type, MRI_FLOAT);
    QCOMPARE(vol.slices.size(), 2);
    QCOMPARE(vol.slices[0].pixelFormat, FIFFV_MRI_PIXEL_FLOAT);
    QCOMPARE(vol.slices[0].pixelsFloat.size(), 16);

    // Verify values: voxel i -> i * 0.5f
    float eps = 1e-5f;
    QVERIFY(std::abs(vol.slices[0].pixelsFloat[0] - 0.0f) < eps);
    QVERIFY(std::abs(vol.slices[0].pixelsFloat[1] - 0.5f) < eps);
    QVERIFY(std::abs(vol.slices[0].pixelsFloat[2] - 1.0f) < eps);
}

//=============================================================================================================

void TestMriIO::testMghDefaultDirectionCosines()
{
    // Create MGH with goodRASflag = 0 → defaults should be used
    QByteArray mghData = createSyntheticMgh(4, 4, 2, MRI_UCHAR, false);
    QString path = m_tempDir.path() + "/synth_noras.mgh";
    QVERIFY(writeToFile(path, mghData));

    MriVolData vol;
    QVector<FiffCoordTrans> trans;
    bool ok = MriMghIO::read(path, vol, trans);
    QVERIFY(ok);
    QVERIFY(vol.isValid());
    QVERIFY(!vol.rasGood);

    // Default direction cosines: x=(-1,0,0), y=(0,0,-1), z=(0,1,0)
    float eps = 1e-6f;
    QVERIFY(std::abs(vol.x_ras[0] - (-1.0f)) < eps);
    QVERIFY(std::abs(vol.x_ras[1]) < eps);
    QVERIFY(std::abs(vol.x_ras[2]) < eps);
    QVERIFY(std::abs(vol.y_ras[0]) < eps);
    QVERIFY(std::abs(vol.y_ras[1]) < eps);
    QVERIFY(std::abs(vol.y_ras[2] - (-1.0f)) < eps);
    QVERIFY(std::abs(vol.z_ras[0]) < eps);
    QVERIFY(std::abs(vol.z_ras[1] - 1.0f) < eps);
    QVERIFY(std::abs(vol.z_ras[2]) < eps);

    // Default voxel sizes should be 1mm
    QVERIFY(qFuzzyCompare(vol.xsize, 1.0f));
    QVERIFY(qFuzzyCompare(vol.ysize, 1.0f));
    QVERIFY(qFuzzyCompare(vol.zsize, 1.0f));
}

//=============================================================================================================

void TestMriIO::testMghFooterScanParams()
{
    // Create MGH with footer containing scan parameters
    float testTR = 2300.0f;
    float testFlip = 0.1396f;  // ~8 degrees in radians
    float testTE = 2.98f;
    float testTI = 900.0f;
    float testFoV = 256.0f;

    QByteArray mghData = createSyntheticMgh(2, 2, 2, MRI_UCHAR, true,
                                            true, testTR, testFlip, testTE, testTI, testFoV);
    QString path = m_tempDir.path() + "/synth_footer.mgh";
    QVERIFY(writeToFile(path, mghData));

    MriVolData vol;
    QVector<FiffCoordTrans> trans;
    bool ok = MriMghIO::read(path, vol, trans);
    QVERIFY(ok);
    QVERIFY(vol.isValid());

    // Verify scan parameters were read from footer
    float eps = 1e-2f;
    QVERIFY2(std::abs(vol.TR - testTR) < eps,
             qPrintable(QString("TR: expected %1, got %2").arg(testTR).arg(vol.TR)));
    QVERIFY(std::abs(vol.flipAngle - testFlip) < eps);
    QVERIFY(std::abs(vol.TE - testTE) < eps);
    QVERIFY(std::abs(vol.TI - testTI) < eps);
    QVERIFY(std::abs(vol.FoV - testFoV) < eps);
}

//=============================================================================================================

void TestMriIO::testMghFooterTalairachTag()
{
    // Create an MGH with a TAG_MGH_XFORM in the footer pointing to a dummy xfm
    QByteArray mghData = createSyntheticMgh(2, 2, 2, MRI_UCHAR, true, true, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

    // Append TAG_MGH_XFORM tag
    QByteArray xfmPath = "talairach.xfm";
    QDataStream tagStream(&mghData, QIODevice::Append);
    tagStream.setByteOrder(QDataStream::BigEndian);
    tagStream << qint32(MGH_TAG_MGH_XFORM);  // tag type
    tagStream << qint64(xfmPath.size());       // tag length (int64 for new tags)
    tagStream.writeRawData(xfmPath.constData(), xfmPath.size());

    QString path = m_tempDir.path() + "/synth_xfm.mgh";
    QVERIFY(writeToFile(path, mghData));

    // Create a dummy .xfm file in a transforms/ subdirectory
    QString xfmDir = m_tempDir.path() + "/transforms";
    QDir().mkpath(xfmDir);
    QString xfmFilePath = xfmDir + "/talairach.xfm";
    {
        QFile f(xfmFilePath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        f.write("MNI Transform File\n");
        f.write("% id\n");
        f.write("Transform_Type = Linear;\n");
        f.write("Linear_Transform =\n");
        f.write("1.0 0.0 0.0 0.0\n");
        f.write("0.0 1.0 0.0 0.0\n");
        f.write("0.0 0.0 1.0 0.0 ;\n");
        f.close();
    }

    MriVolData vol;
    QVector<FiffCoordTrans> trans;
    bool ok = MriMghIO::read(path, vol, trans, m_tempDir.path());
    QVERIFY(ok);
    QVERIFY(!vol.talairachXfmPath.isEmpty());
    QCOMPARE(vol.talairachXfmPath, QString("talairach.xfm"));

    // Talairach transform should have been parsed
    QCOMPARE(trans.size(), 1);
    QCOMPARE(trans[0].from, FIFFV_COORD_MRI);
    QCOMPARE(trans[0].to, FIFFV_COORD_MRI_DISPLAY);
}

//=============================================================================================================

void TestMriIO::testMghUnsupportedDataType()
{
    // Create MGH with an unsupported data type (MRI_BITMAP = 5)
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream << qint32(MRI_MGH_VERSION)
           << qint32(4) << qint32(4) << qint32(2)
           << qint32(1) << qint32(MRI_BITMAP) << qint32(0);
    stream << qint16(0);
    // Pad header
    for (int i = data.size(); i < MRI_MGH_DATA_OFFSET; ++i) stream << quint8(0);
    // Write some dummy voxel data (1 byte per voxel for bitmap)
    for (int i = 0; i < 4 * 4 * 2; ++i) stream << quint8(0);

    QString path = m_tempDir.path() + "/synth_bitmap.mgh";
    QVERIFY(writeToFile(path, data));

    MriVolData vol;
    QVector<FiffCoordTrans> trans;
    bool ok = MriMghIO::read(path, vol, trans);
    QVERIFY(!ok);  // Should fail for unsupported type
}

//=============================================================================================================

void TestMriIO::testMghCorruptedMgz()
{
    // Write random bytes with .mgz extension — decompression should fail
    QString path = m_tempDir.path() + "/corrupt.mgz";
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        QByteArray garbage(1024, 'X');
        f.write(garbage);
        f.close();
    }

    MriVolData vol;
    QVector<FiffCoordTrans> trans;
    bool ok = MriMghIO::read(path, vol, trans);
    QVERIFY(!ok);
}

//=============================================================================================================

void TestMriIO::testMghEmptyFile()
{
    // Empty .mgh file
    QString path = m_tempDir.path() + "/empty.mgh";
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.close();
    }

    MriVolData vol;
    QVector<FiffCoordTrans> trans;
    bool ok = MriMghIO::read(path, vol, trans);
    QVERIFY(!ok);

    // Empty .mgz file
    QString pathMgz = m_tempDir.path() + "/empty.mgz";
    {
        QFile f(pathMgz);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.close();
    }
    ok = MriMghIO::read(pathMgz, vol, trans);
    QVERIFY(!ok);
}

//=============================================================================================================

void TestMriIO::testCorFifWriteReadSynthetic()
{
    // Create a small synthetic volume AND round-trip through COR.fif
    // This test does not require sample data
    QByteArray mghData = createSyntheticMgh(4, 4, 3, MRI_UCHAR, true);
    QString mghPath = m_tempDir.path() + "/synth_roundtrip.mgh";
    QVERIFY(writeToFile(mghPath, mghData));

    MriVolData vol;
    QVector<FiffCoordTrans> trans;
    bool ok = MriMghIO::read(mghPath, vol, trans);
    QVERIFY(ok);
    QCOMPARE(vol.slices.size(), 3);

    // Write to COR.fif
    QString corPath = m_tempDir.path() + "/COR_synth.fif";
    ok = MriCorFifIO::write(corPath, vol.slices, trans);
    QVERIFY(ok);
    QVERIFY(QFile::exists(corPath));

    // Read back and verify structure
    QFile file(corPath);
    FiffStream::SPtr stream(new FiffStream(&file));
    QVERIFY(stream->open());

    FiffTag::SPtr tag;
    int sliceCount = 0;
    stream->device()->seek(0);
    while (!stream->device()->atEnd()) {
        stream->read_tag(tag);
        if (!tag) break;
        if (tag->kind == FIFF_BLOCK_START) {
            fiff_int_t blockKind = *tag->toInt();
            if (blockKind == FIFFB_MRI_SLICE) sliceCount++;
        }
    }
    QCOMPARE(sliceCount, 3);
    stream->close();
}

//=============================================================================================================

void TestMriIO::cleanupTestCase()
{
    // m_tempDir cleans up automatically
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMriIO)
#include "test_mri_io.moc"
