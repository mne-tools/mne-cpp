//=============================================================================================================
/**
 * @file     test_tool_inverse_computation.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     June, 2026
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
 * @brief    Tests for inverse tool functions (collapse, scale, file format readers, helpers).
 */

//=============================================================================================================
// Include tool sources
//=============================================================================================================

// --- mne_compute_mne ---
#define main _compute_mne_main_unused
#include "../../tools/inverse/mne_compute_mne/main.cpp"
#undef main

// --- mne_compute_raw_inverse (helpers) ---
#define main _raw_inv_main_unused
#include "../../tools/inverse/mne_compute_raw_inverse/main.cpp"
#undef main

// --- mne_volume_data2mri ---
#define main _vol2mri_main_unused
#include "../../tools/inverse/mne_volume_data2mri/main.cpp"
#undef main

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QTemporaryDir>

//=============================================================================================================

class TestToolInverseComputation : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // --- collapseData tests ---
    void testCollapseMax();
    void testCollapseMaxNegative();
    void testCollapseL1();
    void testCollapseL2();
    void testCollapseSingleTime();
    void testCollapseReplication();

    // --- scaleData tests ---
    void testScaleDataScaleTo();
    void testScaleDataScaleBy();
    void testScaleDataSiCurrents();
    void testScaleDataZeroMax();

    // --- isRawFile tests ---
    void testIsRawFileWithRawData();
    void testIsRawFileWithEvokedData();
    void testIsRawFileNonexistent();

    // --- sanitizeForFilename tests ---
    void testSanitizeNormal();
    void testSanitizeSpecialChars();
    void testSanitizeSpaces();
    void testSanitizeEmpty();

    // --- composeOutName tests ---
    void testComposeOutNameRaw();
    void testComposeOutNameStc();
    void testComposeOutNameWithTag();
    void testComposeOutNameNoTag();

    // --- labelHemisphere tests ---
    void testLabelHemisphereLeft();
    void testLabelHemisphereRight();
    void testLabelHemisphereUnknown();
    void testLabelHemispherePrefix();

    // --- findLabelsInDir tests ---
    void testFindLabelsInDirEmpty();
    void testFindLabelsInDirSorted();
    void testFindLabelsInDirRealData();

    // --- readWFile / readStcFile tests ---
    void testReadWFileSynthetic();
    void testReadStcFileSynthetic();
    void testReadWFileEmpty();

    void cleanupTestCase();

private:
    QString m_sResourcePath;
    QTemporaryDir m_tempDir;
};

//=============================================================================================================

void TestToolInverseComputation::initTestCase()
{
    QString binDir = QCoreApplication::applicationDirPath();
    m_sResourcePath = binDir + "/../resources/data/mne-cpp-test-data/";
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================
// collapseData tests
//=============================================================================================================

void TestToolInverseComputation::testCollapseMax()
{
    MatrixXd data(3, 4);
    data << 1, -5, 3, 2,    // max abs = -5 at col 1
            4,  1, 2, 3,    // max abs = 4 at col 0
            1,  2, 3, -4;   // max abs = -4 at col 3

    collapseData(data, COLLAPSE_MAX);

    // After collapse, all columns should be identical
    for (int t = 0; t < 4; t++) {
        QCOMPARE(data(0, t), -5.0);  // Preserves sign
        QCOMPARE(data(1, t), 4.0);
        QCOMPARE(data(2, t), -4.0);
    }
}

void TestToolInverseComputation::testCollapseMaxNegative()
{
    MatrixXd data(1, 3);
    data << -10, -3, -7;
    collapseData(data, COLLAPSE_MAX);
    QCOMPARE(data(0, 0), -10.0);  // Max absolute, preserving sign
}

void TestToolInverseComputation::testCollapseL1()
{
    MatrixXd data(2, 4);
    data << 1, -1, 3, -3,     // |1|+|1|+|3|+|3| = 8, /4 = 2
            2, 2, 2, 2;       // |2|+|2|+|2|+|2| = 8, /4 = 2

    collapseData(data, COLLAPSE_L1);

    for (int t = 0; t < 4; t++) {
        QVERIFY(qAbs(data(0, t) - 2.0) < 1e-10);
        QVERIFY(qAbs(data(1, t) - 2.0) < 1e-10);
    }
}

void TestToolInverseComputation::testCollapseL2()
{
    MatrixXd data(1, 2);
    data << 3.0, 4.0;
    // RMS = sqrt((9 + 16) / 2) = sqrt(12.5)
    double expected = std::sqrt(12.5);

    collapseData(data, COLLAPSE_L2);

    QVERIFY(qAbs(data(0, 0) - expected) < 1e-10);
}

void TestToolInverseComputation::testCollapseSingleTime()
{
    // Single time point → collapse is no-op
    MatrixXd data(3, 1);
    data << 5, 10, 15;
    MatrixXd orig = data;

    collapseData(data, COLLAPSE_MAX);

    QCOMPARE(data(0, 0), orig(0, 0));
    QCOMPARE(data(1, 0), orig(1, 0));
    QCOMPARE(data(2, 0), orig(2, 0));
}

void TestToolInverseComputation::testCollapseReplication()
{
    // After collapse, all columns should contain the same values
    MatrixXd data = MatrixXd::Random(5, 10);
    collapseData(data, COLLAPSE_L1);

    for (int t = 1; t < 10; t++) {
        for (int s = 0; s < 5; s++) {
            QCOMPARE(data(s, t), data(s, 0));
        }
    }
}

//=============================================================================================================
// scaleData tests
//=============================================================================================================

void TestToolInverseComputation::testScaleDataScaleTo()
{
    MatrixXd data(2, 3);
    data << 1, 2, 3,
            4, 5, 6;
    // Max abs = 6, scale to 100 → factor = 100/6

    scaleData(data, 100.0, 0.0, false);

    double maxVal = data.cwiseAbs().maxCoeff();
    QVERIFY(qAbs(maxVal - 100.0) < 1e-6);
}

void TestToolInverseComputation::testScaleDataScaleBy()
{
    MatrixXd data(1, 3);
    data << 10, 20, 30;

    scaleData(data, 0.0, 2.0, false);

    QCOMPARE(data(0, 0), 20.0);
    QCOMPARE(data(0, 1), 40.0);
    QCOMPARE(data(0, 2), 60.0);
}

void TestToolInverseComputation::testScaleDataSiCurrents()
{
    MatrixXd data(1, 2);
    data << 1e-9, 2e-9;
    MatrixXd orig = data;

    scaleData(data, 100.0, 2.0, true);  // siCurrents → no change

    QCOMPARE(data(0, 0), orig(0, 0));
    QCOMPARE(data(0, 1), orig(0, 1));
}

void TestToolInverseComputation::testScaleDataZeroMax()
{
    // If all zeros → scaleTo does nothing (avoids division by zero)
    MatrixXd data = MatrixXd::Zero(2, 3);
    scaleData(data, 50.0, 0.0, false);
    QVERIFY(data.isZero());
}

//=============================================================================================================
// isRawFile tests
//=============================================================================================================

void TestToolInverseComputation::testIsRawFileWithRawData()
{
    QString rawPath = m_sResourcePath + "MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Raw test data not available");

    QVERIFY(isRawFile(rawPath));
}

void TestToolInverseComputation::testIsRawFileWithEvokedData()
{
    QString avePath = m_sResourcePath + "MEG/sample/sample_audvis-ave.fif";
    if (!QFile::exists(avePath))
        QSKIP("Evoked test data not available");

    QVERIFY(!isRawFile(avePath));
}

void TestToolInverseComputation::testIsRawFileNonexistent()
{
    QVERIFY(!isRawFile("/nonexistent/path.fif"));
}

//=============================================================================================================
// sanitizeForFilename tests
//=============================================================================================================

void TestToolInverseComputation::testSanitizeNormal()
{
    QCOMPARE(sanitizeForFilename("hello_world"), QString("hello_world"));
}

void TestToolInverseComputation::testSanitizeSpecialChars()
{
    QString result = sanitizeForFilename("My File (v2.1)!");
    QVERIFY(!result.contains(' '));
    QVERIFY(!result.contains('('));
    QVERIFY(!result.contains(')'));
    QVERIFY(!result.contains('!'));
}

void TestToolInverseComputation::testSanitizeSpaces()
{
    QString result = sanitizeForFilename("hello  world");
    QCOMPARE(result, QString("hello_world"));
}

void TestToolInverseComputation::testSanitizeEmpty()
{
    QCOMPARE(sanitizeForFilename(""), QString("unknown"));
    QCOMPARE(sanitizeForFilename("!!!"), QString("unknown"));
}

//=============================================================================================================
// composeOutName tests
//=============================================================================================================

void TestToolInverseComputation::testComposeOutNameRaw()
{
    QString result = composeOutName("/data/lh.V1.label", "dSPM", true);
    QVERIFY(result.endsWith("_raw.fif"));
    QVERIFY(result.contains("dSPM"));
}

void TestToolInverseComputation::testComposeOutNameStc()
{
    QString result = composeOutName("/data/lh.V1.label", "MNE", false);
    QVERIFY(result.endsWith("-stc.fif"));
    QVERIFY(result.contains("MNE"));
}

void TestToolInverseComputation::testComposeOutNameWithTag()
{
    QString result = composeOutName("/path/to/my.label", "tag", false);
    QVERIFY(result.contains("-tag"));
}

void TestToolInverseComputation::testComposeOutNameNoTag()
{
    QString result = composeOutName("/path/to/my.label", "", false);
    QVERIFY(!result.contains("--"));
    QVERIFY(result.endsWith("-stc.fif"));
}

//=============================================================================================================
// labelHemisphere tests
//=============================================================================================================

void TestToolInverseComputation::testLabelHemisphereLeft()
{
    QCOMPARE(labelHemisphere("/path/to/lh.V1.label"), 0);
    QCOMPARE(labelHemisphere("label-lh.label"), 0);
}

void TestToolInverseComputation::testLabelHemisphereRight()
{
    QCOMPARE(labelHemisphere("/path/to/rh.V1.label"), 1);
    QCOMPARE(labelHemisphere("label-rh.label"), 1);
}

void TestToolInverseComputation::testLabelHemisphereUnknown()
{
    QCOMPARE(labelHemisphere("/path/to/unknown.label"), -1);
    QCOMPARE(labelHemisphere("no_hemisphere.txt"), -1);
}

void TestToolInverseComputation::testLabelHemispherePrefix()
{
    QCOMPARE(labelHemisphere("lh.aparc.label"), 0);
    QCOMPARE(labelHemisphere("rh.aparc.label"), 1);
}

//=============================================================================================================
// findLabelsInDir tests
//=============================================================================================================

void TestToolInverseComputation::testFindLabelsInDirEmpty()
{
    QString emptyDir = m_tempDir.path() + "/empty_labels";
    QDir().mkpath(emptyDir);
    QStringList labels = findLabelsInDir(emptyDir);
    QCOMPARE(labels.size(), 0);
}

void TestToolInverseComputation::testFindLabelsInDirSorted()
{
    QString labelDir = m_tempDir.path() + "/test_labels";
    QDir().mkpath(labelDir);

    // Create label files out of order
    QFile(labelDir + "/rh.V1.label").open(QIODevice::WriteOnly);
    QFile(labelDir + "/lh.V1.label").open(QIODevice::WriteOnly);
    QFile(labelDir + "/lh.MT.label").open(QIODevice::WriteOnly);
    QFile(labelDir + "/not_a_label.txt").open(QIODevice::WriteOnly);

    QStringList labels = findLabelsInDir(labelDir);
    QCOMPARE(labels.size(), 3);  // Only .label files
    // Should be sorted
    QVERIFY(labels[0] < labels[1]);
    QVERIFY(labels[1] < labels[2]);
}

void TestToolInverseComputation::testFindLabelsInDirRealData()
{
    QString labelDir = m_sResourcePath + "subjects/sample/label/";
    if (!QDir(labelDir).exists())
        QSKIP("Label test data not available");

    QStringList labels = findLabelsInDir(labelDir);
    QVERIFY(labels.size() >= 1);  // At least lh.V1.label
}

//=============================================================================================================
// readWFile / readStcFile tests
//=============================================================================================================

void TestToolInverseComputation::testReadWFileSynthetic()
{
    // Create a synthetic W file (big-endian format)
    QString wPath = m_tempDir.path() + "/test.w";
    QFile file(wPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QDataStream out(&file);
    out.setByteOrder(QDataStream::BigEndian);

    quint16 header = 0;
    out << header;

    // Number of vertices (3 bytes)
    qint32 nvert = 3;
    out << (quint8)((nvert >> 16) & 0xFF);
    out << (quint8)((nvert >> 8) & 0xFF);
    out << (quint8)(nvert & 0xFF);

    // Vertex 0
    out << (quint8)0 << (quint8)0 << (quint8)0;
    float val0 = 1.5f; out << val0;

    // Vertex 100
    out << (quint8)0 << (quint8)0 << (quint8)100;
    float val1 = 2.5f; out << val1;

    // Vertex 500
    out << (quint8)0 << (quint8)1 << (quint8)244;  // 500 = 1*256 + 244
    float val2 = 3.5f; out << val2;

    file.close();

    VectorXd data;
    VectorXi vertices;
    QVERIFY(readWFile(wPath, data, vertices));
    QCOMPARE(data.size(), (Eigen::Index)3);
    QCOMPARE(vertices.size(), (Eigen::Index)3);
    QCOMPARE(vertices(0), 0);
    QCOMPARE(vertices(1), 100);
    QCOMPARE(vertices(2), 500);
    QVERIFY(qAbs(data(0) - 1.5) < 0.01);
    QVERIFY(qAbs(data(1) - 2.5) < 0.01);
    QVERIFY(qAbs(data(2) - 3.5) < 0.01);
}

void TestToolInverseComputation::testReadStcFileSynthetic()
{
    // Create a synthetic STC file (big-endian format)
    QString stcPath = m_tempDir.path() + "/test.stc";
    QFile file(stcPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QDataStream out(&file);
    out.setByteOrder(QDataStream::BigEndian);

    float tmin = 0.0f, tstep = 0.001f;
    out << tmin << tstep;

    qint32 nvert = 2;
    out << nvert;
    qint32 v0 = 10, v1 = 20;
    out << v0 << v1;

    qint32 ntime = 3;
    out << ntime;

    // Data: [t=0: v0, v1], [t=1: v0, v1], [t=2: v0, v1]
    float vals[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    for (int i = 0; i < 6; i++) out << vals[i];

    file.close();

    MatrixXd data;
    VectorXi vertices;
    float tminOut, tstepOut;
    QVERIFY(readStcFile(stcPath, data, vertices, tminOut, tstepOut));
    QCOMPARE(vertices.size(), (Eigen::Index)2);
    QCOMPARE(vertices(0), 10);
    QCOMPARE(vertices(1), 20);
    QVERIFY(qAbs(tminOut) < 1e-6);
    QVERIFY(qAbs(tstepOut - 0.001f) < 1e-6f);
    QCOMPARE(data.rows(), (Eigen::Index)2);
    QCOMPARE(data.cols(), (Eigen::Index)3);
    QVERIFY(qAbs(data(0, 0) - 1.0) < 0.01);
    QVERIFY(qAbs(data(1, 2) - 6.0) < 0.01);
}

void TestToolInverseComputation::testReadWFileEmpty()
{
    VectorXd data;
    VectorXi vertices;
    QVERIFY(!readWFile("/nonexistent/file.w", data, vertices));
}

//=============================================================================================================

void TestToolInverseComputation::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestToolInverseComputation)
#include "test_tool_inverse_computation.moc"
