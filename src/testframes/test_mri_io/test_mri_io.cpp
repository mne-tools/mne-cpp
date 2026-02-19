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

    // MriCorFifIO round-trip test
    void testCorFifWriteRead();

    void cleanupTestCase();

private:
    /** Find the MNE sample data subjects directory. */
    QString findSubjectsDir();

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

void TestMriIO::cleanupTestCase()
{
    // m_tempDir cleans up automatically
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMriIO)
#include "test_mri_io.moc"
