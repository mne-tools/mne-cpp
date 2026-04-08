//=============================================================================================================
/**
 * @file     test_tool_preproc_helpers.cpp
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
 * @brief    Tests for preprocessing tool helper functions.
 */

//=============================================================================================================
// Include tool sources
//=============================================================================================================

// --- mne_insert_4D_comp ---
#define main _insert_4d_main_unused
#include "../../tools/preprocessing/mne_insert_4D_comp/main.cpp"
#undef main

// --- mne_rename_channels ---
#define main _rename_ch_main_unused
#include "../../tools/preprocessing/mne_rename_channels/main.cpp"
#undef main

// --- mne_mark_bad_channels ---
#define main _mark_bad_main_unused
#include "../../tools/preprocessing/mne_mark_bad_channels/main.cpp"
#undef main

// --- mne_create_comp_data ---
#define main _create_comp_main_unused
#include "../../tools/preprocessing/mne_create_comp_data/main.cpp"
#undef main

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QTextStream>

//=============================================================================================================

class TestToolPreprocHelpers : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // --- readSensorPositions tests ---
    void testReadSensorPositionsBasic();
    void testReadSensorPositionsComments();
    void testReadSensorPositionsMmToMeters();

    // --- procrustes tests ---
    void testProcrustesIdentity();
    void testProcrustesTranslation();
    void testProcrustesRotation();
    void testProcrustesOrthogonal();

    // --- readRefData tests ---
    void testReadRefDataBasic();
    void testReadRefDataDimensions();
    void testReadRefDataComments();

    // --- explainKind tests ---
    void testExplainKindMEG();
    void testExplainKindEEG();
    void testExplainKindEOG();
    void testExplainKindEMG();
    void testExplainKindECG();
    void testExplainKindMISC();
    void testExplainKindSTIM();
    void testExplainKindUnknown();

    // --- readAliases tests ---
    void testReadAliasesBasic();
    void testReadAliasesReverse();
    void testReadAliasesWithKind();
    void testReadAliasesTruncation();
    void testReadAliasesComments();

    // --- readBadChannelList tests ---
    void testReadBadChannelListBasic();
    void testReadBadChannelListComments();
    void testReadBadChannelListEmpty();

    // --- readAsciiMatrix tests ---
    void testReadAsciiMatrixBasic();
    void testReadAsciiMatrixNames();
    void testReadAsciiMatrixValues();

    void cleanupTestCase();

private:
    QString m_sResourcePath;
    QTemporaryDir m_tmpDir;
};

//=============================================================================================================

void TestToolPreprocHelpers::initTestCase()
{
    QString binDir = QCoreApplication::applicationDirPath();
    m_sResourcePath = binDir + "/../resources/data/mne-cpp-test-data/";
    QVERIFY(m_tmpDir.isValid());
}

//=============================================================================================================
// readSensorPositions tests
//=============================================================================================================

void TestToolPreprocHelpers::testReadSensorPositionsBasic()
{
    QString fname = m_tmpDir.path() + "/sensors.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "CH01  10.0  20.0  30.0\n";
    out << "CH02  40.0  50.0  60.0\n";
    out << "CH03  70.0  80.0  90.0\n";
    f.close();

    MatrixX3f pos;
    QStringList names;
    QVERIFY(readSensorPositions(fname, pos, names));
    QCOMPARE(pos.rows(), (Eigen::Index)3);
    QCOMPARE(names.size(), 3);
    QCOMPARE(names[0], QString("CH01"));
    QCOMPARE(names[1], QString("CH02"));
    QCOMPARE(names[2], QString("CH03"));
}

void TestToolPreprocHelpers::testReadSensorPositionsComments()
{
    QString fname = m_tmpDir.path() + "/sensors_comments.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "# comment line\n";
    out << "CH01  10.0  20.0  30.0\n";
    out << "\n";
    out << "# another comment\n";
    out << "CH02  40.0  50.0  60.0\n";
    f.close();

    MatrixX3f pos;
    QStringList names;
    QVERIFY(readSensorPositions(fname, pos, names));
    QCOMPARE(pos.rows(), (Eigen::Index)2);
    QCOMPARE(names.size(), 2);
}

void TestToolPreprocHelpers::testReadSensorPositionsMmToMeters()
{
    QString fname = m_tmpDir.path() + "/sensors_mm.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "CH01  1000.0  2000.0  3000.0\n";
    f.close();

    MatrixX3f pos;
    QStringList names;
    QVERIFY(readSensorPositions(fname, pos, names));
    // mm -> meters conversion: 1000mm = 1.0m
    QVERIFY(qAbs(pos(0, 0) - 1.0f) < 1e-4f);
    QVERIFY(qAbs(pos(0, 1) - 2.0f) < 1e-4f);
    QVERIFY(qAbs(pos(0, 2) - 3.0f) < 1e-4f);
}

//=============================================================================================================
// procrustes tests
//=============================================================================================================

void TestToolPreprocHelpers::testProcrustesIdentity()
{
    MatrixX3f src(4, 3);
    src << 1, 0, 0,
           0, 1, 0,
           0, 0, 1,
           1, 1, 1;

    Matrix4f T = procrustes(src, src);
    // Should be identity
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            QVERIFY(qAbs(T(i, j) - (i == j ? 1.0f : 0.0f)) < 1e-4f);
}

void TestToolPreprocHelpers::testProcrustesTranslation()
{
    MatrixX3f src(3, 3);
    src << 0, 0, 0,
           1, 0, 0,
           0, 1, 0;

    Vector3f offset(5.0f, 10.0f, 15.0f);
    MatrixX3f tgt = src.rowwise() + offset.transpose();

    Matrix4f T = procrustes(src, tgt);
    // Translation should be (5, 10, 15)
    QVERIFY(qAbs(T(0, 3) - 5.0f) < 1e-3f);
    QVERIFY(qAbs(T(1, 3) - 10.0f) < 1e-3f);
    QVERIFY(qAbs(T(2, 3) - 15.0f) < 1e-3f);
}

void TestToolPreprocHelpers::testProcrustesRotation()
{
    // 90 degree rotation about Z axis
    MatrixX3f src(3, 3);
    src << 1, 0, 0,
           0, 1, 0,
           0, 0, 1;

    MatrixX3f tgt(3, 3);
    tgt << 0, 1, 0,
          -1, 0, 0,
           0, 0, 1;

    Matrix4f T = procrustes(src, tgt);

    // Apply rotation to source, check it matches target
    for (int i = 0; i < 3; i++) {
        Vector3f transformed = T.block<3, 3>(0, 0) * src.row(i).transpose() + T.block<3, 1>(0, 3);
        for (int j = 0; j < 3; j++)
            QVERIFY(qAbs(transformed(j) - tgt(i, j)) < 1e-3f);
    }
}

void TestToolPreprocHelpers::testProcrustesOrthogonal()
{
    // Result rotation matrix should be orthogonal (R^T * R = I)
    MatrixX3f src(4, 3);
    src << 1, 0, 0,
           0, 2, 0,
           0, 0, 3,
           1, 1, 1;

    MatrixX3f tgt(4, 3);
    tgt << 0, 1, 0,
          -2, 0, 0,
           0, 0, 3,
          -1, 1, 1;

    Matrix4f T = procrustes(src, tgt);
    Matrix3f R = T.block<3, 3>(0, 0);
    Matrix3f product = R.transpose() * R;

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            QVERIFY(qAbs(product(i, j) - (i == j ? 1.0f : 0.0f)) < 1e-3f);
}

//=============================================================================================================
// readRefData tests
//=============================================================================================================

void TestToolPreprocHelpers::testReadRefDataBasic()
{
    QString fname = m_tmpDir.path() + "/refdata.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "1.0 2.0 3.0\n";
    out << "4.0 5.0 6.0\n";
    out << "7.0 8.0 9.0\n";
    f.close();

    MatrixXf refData;
    int nChan, nSamp;
    QVERIFY(readRefData(fname, refData, nChan, nSamp));
    QCOMPARE(nSamp, 3);
    QCOMPARE(nChan, 3);
}

void TestToolPreprocHelpers::testReadRefDataDimensions()
{
    QString fname = m_tmpDir.path() + "/refdata_dim.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "1.0 2.0\n";
    out << "3.0 4.0\n";
    out << "5.0 6.0\n";
    out << "7.0 8.0\n";
    out << "9.0 10.0\n";
    f.close();

    MatrixXf refData;
    int nChan, nSamp;
    QVERIFY(readRefData(fname, refData, nChan, nSamp));
    // 5 rows (samples), 2 columns (channels)
    QCOMPARE(nSamp, 5);
    QCOMPARE(nChan, 2);
    QCOMPARE(refData.rows(), (Eigen::Index)2);
    QCOMPARE(refData.cols(), (Eigen::Index)5);
}

void TestToolPreprocHelpers::testReadRefDataComments()
{
    QString fname = m_tmpDir.path() + "/refdata_comments.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "# comment\n";
    out << "1.0 2.0\n";
    out << "\n";
    out << "3.0 4.0\n";
    f.close();

    MatrixXf refData;
    int nChan, nSamp;
    QVERIFY(readRefData(fname, refData, nChan, nSamp));
    QCOMPARE(nSamp, 2);
    QCOMPARE(nChan, 2);
}

//=============================================================================================================
// explainKind tests
//=============================================================================================================

void TestToolPreprocHelpers::testExplainKindMEG()
{
    QCOMPARE(explainKind(FIFFV_MEG_CH), QString("MEG"));
}

void TestToolPreprocHelpers::testExplainKindEEG()
{
    QCOMPARE(explainKind(FIFFV_EEG_CH), QString("EEG"));
}

void TestToolPreprocHelpers::testExplainKindEOG()
{
    QCOMPARE(explainKind(FIFFV_EOG_CH), QString("EOG"));
}

void TestToolPreprocHelpers::testExplainKindEMG()
{
    QCOMPARE(explainKind(FIFFV_EMG_CH), QString("EMG"));
}

void TestToolPreprocHelpers::testExplainKindECG()
{
    QCOMPARE(explainKind(FIFFV_ECG_CH), QString("ECG"));
}

void TestToolPreprocHelpers::testExplainKindMISC()
{
    QCOMPARE(explainKind(FIFFV_MISC_CH), QString("MISC"));
}

void TestToolPreprocHelpers::testExplainKindSTIM()
{
    QCOMPARE(explainKind(FIFFV_STIM_CH), QString("STI"));
}

void TestToolPreprocHelpers::testExplainKindUnknown()
{
    QCOMPARE(explainKind(9999), QString("Unknown"));
}

//=============================================================================================================
// readAliases tests
//=============================================================================================================

void TestToolPreprocHelpers::testReadAliasesBasic()
{
    QString fname = m_tmpDir.path() + "/aliases.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "MEG0111:MEG0111_new\n";
    out << "MEG0121:MEG0121_new\n";
    f.close();

    QList<ChannelAlias> aliases = readAliases(fname, false);
    QCOMPARE(aliases.size(), 2);
    QCOMPARE(aliases[0].from, QString("MEG0111"));
    QCOMPARE(aliases[0].to, QString("MEG0111_new"));
    QCOMPARE(aliases[0].toKind, -1);
    QCOMPARE(aliases[1].from, QString("MEG0121"));
    QCOMPARE(aliases[1].to, QString("MEG0121_new"));
}

void TestToolPreprocHelpers::testReadAliasesReverse()
{
    QString fname = m_tmpDir.path() + "/aliases_rev.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "OLD_NAME:NEW_NAME\n";
    f.close();

    QList<ChannelAlias> aliases = readAliases(fname, true);
    QCOMPARE(aliases.size(), 1);
    // When reversed, from = NEW_NAME, to = OLD_NAME
    QCOMPARE(aliases[0].from, QString("NEW_NAME"));
    QCOMPARE(aliases[0].to, QString("OLD_NAME"));
}

void TestToolPreprocHelpers::testReadAliasesWithKind()
{
    QString fname = m_tmpDir.path() + "/aliases_kind.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "STI014:TRIGGER:3" << "\n";  // kind = 3 (STIM)
    f.close();

    QList<ChannelAlias> aliases = readAliases(fname, false);
    QCOMPARE(aliases.size(), 1);
    QCOMPARE(aliases[0].from, QString("STI014"));
    QCOMPARE(aliases[0].to, QString("TRIGGER"));
    QCOMPARE(aliases[0].toKind, 3);
}

void TestToolPreprocHelpers::testReadAliasesTruncation()
{
    QString fname = m_tmpDir.path() + "/aliases_long.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "SHORT:VERYLONGCHANNELNAME16\n";
    f.close();

    QList<ChannelAlias> aliases = readAliases(fname, false);
    QCOMPARE(aliases.size(), 1);
    // Name should be truncated to 15 chars
    QCOMPARE(aliases[0].to.length(), 15);
}

void TestToolPreprocHelpers::testReadAliasesComments()
{
    QString fname = m_tmpDir.path() + "/aliases_comments.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "# This is a comment\n";
    out << "CH1:CH1_NEW\n";
    out << "\n";
    out << "# Another comment\n";
    out << "CH2:CH2_NEW\n";
    f.close();

    QList<ChannelAlias> aliases = readAliases(fname, false);
    QCOMPARE(aliases.size(), 2);
}

//=============================================================================================================
// readBadChannelList tests
//=============================================================================================================

void TestToolPreprocHelpers::testReadBadChannelListBasic()
{
    QString fname = m_tmpDir.path() + "/bads.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "MEG0111\n";
    out << "MEG0121\n";
    out << "MEG0131\n";
    f.close();

    QStringList bads = readBadChannelList(fname);
    QCOMPARE(bads.size(), 3);
    QCOMPARE(bads[0], QString("MEG0111"));
    QCOMPARE(bads[1], QString("MEG0121"));
    QCOMPARE(bads[2], QString("MEG0131"));
}

void TestToolPreprocHelpers::testReadBadChannelListComments()
{
    QString fname = m_tmpDir.path() + "/bads_comments.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "# Bad channels\n";
    out << "MEG0111\n";
    out << "\n";
    out << "# More bads\n";
    out << "MEG0121\n";
    f.close();

    QStringList bads = readBadChannelList(fname);
    QCOMPARE(bads.size(), 2);
}

void TestToolPreprocHelpers::testReadBadChannelListEmpty()
{
    QString fname = m_tmpDir.path() + "/bads_empty.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "# Only comments\n";
    out << "\n";
    f.close();

    QStringList bads = readBadChannelList(fname);
    QCOMPARE(bads.size(), 0);
}

//=============================================================================================================
// readAsciiMatrix tests
//=============================================================================================================

void TestToolPreprocHelpers::testReadAsciiMatrixBasic()
{
    QString fname = m_tmpDir.path() + "/matrix.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "3 2\n";
    out << "col1 col2\n";
    out << "row1 1.0 2.0\n";
    out << "row2 3.0 4.0\n";
    out << "row3 5.0 6.0\n";
    f.close();

    MatrixXd data;
    QStringList rowNames, colNames;
    QVERIFY(readAsciiMatrix(fname, data, rowNames, colNames));
    QCOMPARE(data.rows(), (Eigen::Index)3);
    QCOMPARE(data.cols(), (Eigen::Index)2);
}

void TestToolPreprocHelpers::testReadAsciiMatrixNames()
{
    QString fname = m_tmpDir.path() + "/matrix_names.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "2 3\n";
    out << "A B C\n";
    out << "X 1.0 2.0 3.0\n";
    out << "Y 4.0 5.0 6.0\n";
    f.close();

    MatrixXd data;
    QStringList rowNames, colNames;
    QVERIFY(readAsciiMatrix(fname, data, rowNames, colNames));
    QCOMPARE(colNames.size(), 3);
    QCOMPARE(colNames[0], QString("A"));
    QCOMPARE(colNames[1], QString("B"));
    QCOMPARE(colNames[2], QString("C"));
    QCOMPARE(rowNames.size(), 2);
    QCOMPARE(rowNames[0], QString("X"));
    QCOMPARE(rowNames[1], QString("Y"));
}

void TestToolPreprocHelpers::testReadAsciiMatrixValues()
{
    QString fname = m_tmpDir.path() + "/matrix_vals.txt";
    QFile f(fname);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&f);
    out << "2 2\n";
    out << "c1 c2\n";
    out << "r1  1.5  -2.5\n";
    out << "r2  3.14  0.0\n";
    f.close();

    MatrixXd data;
    QStringList rowNames, colNames;
    QVERIFY(readAsciiMatrix(fname, data, rowNames, colNames));
    QVERIFY(qAbs(data(0, 0) - 1.5) < 1e-10);
    QVERIFY(qAbs(data(0, 1) - (-2.5)) < 1e-10);
    QVERIFY(qAbs(data(1, 0) - 3.14) < 1e-10);
    QVERIFY(qAbs(data(1, 1) - 0.0) < 1e-10);
}

//=============================================================================================================

void TestToolPreprocHelpers::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestToolPreprocHelpers)
#include "test_tool_preproc_helpers.moc"
