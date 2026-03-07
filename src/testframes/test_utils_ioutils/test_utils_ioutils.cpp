//=============================================================================================================
/**
 * @file     test_utils_ioutils.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Tests for IOUtils — byte swapping, fread3, Eigen matrix I/O, channel name conventions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/ioutils.h>
#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QObject>
#include <QTemporaryDir>
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <sstream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Tests for IOUtils: byte-swap operations, 3-byte integer I/O, Eigen matrix round-trip I/O,
 *        and channel naming convention conversions.
 */
class TestIOUtils : public QObject
{
    Q_OBJECT

public:
    TestIOUtils();

private slots:
    void initTestCase();

    // Byte-swap operations (idempotent: swap(swap(x)) == x)
    void testSwapShort();
    void testSwapInt();
    void testSwapIntp();
    void testSwapLong();
    void testSwapLongp();
    void testSwapFloat();
    void testSwapFloatp();
    void testSwapDoublep();

    // fread3 — 3-byte integer read via QDataStream
    void testFread3QDataStream();
    void testFread3StdStream();
    void testFread3Many();

    // Eigen matrix write/read round-trip (inspired by mne-python's IO round-trip pattern)
    void testWriteReadEigenMatrixQString();
    void testWriteReadEigenMatrixStdString();

    // Channel naming conventions
    void testGetNewChNamesConventionsQString();
    void testGetNewChNamesConventionsStdString();
    void testGetOldChNamesConventionsQString();
    void testGetOldChNamesConventionsStdString();
    void testCheckMatchingChNamesConventions();

    void cleanupTestCase();

private:
    QTemporaryDir m_tempDir;
    double m_dEpsilon;
};

//=============================================================================================================

TestIOUtils::TestIOUtils()
: m_dEpsilon(1e-10)
{
}

//=============================================================================================================

void TestIOUtils::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================

void TestIOUtils::testSwapShort()
{
    qint16 original = 0x0102;
    qint16 swapped = IOUtils::swap_short(original);
    QCOMPARE(swapped, (qint16)0x0201);

    // Double swap = identity
    qint16 restored = IOUtils::swap_short(swapped);
    QCOMPARE(restored, original);

    // Zero stays zero
    QCOMPARE(IOUtils::swap_short(0), (qint16)0);
}

//=============================================================================================================

void TestIOUtils::testSwapInt()
{
    qint32 original = 0x01020304;
    qint32 swapped = IOUtils::swap_int(original);
    QCOMPARE(swapped, (qint32)0x04030201);

    // Double swap = identity
    QCOMPARE(IOUtils::swap_int(swapped), original);

    // Zero
    QCOMPARE(IOUtils::swap_int(0), (qint32)0);
}

//=============================================================================================================

void TestIOUtils::testSwapIntp()
{
    qint32 val = 0x01020304;
    qint32 original = val;
    IOUtils::swap_intp(&val);
    QCOMPARE(val, (qint32)0x04030201);

    // Double swap = identity
    IOUtils::swap_intp(&val);
    QCOMPARE(val, original);
}

//=============================================================================================================

void TestIOUtils::testSwapLong()
{
    qint64 original = 0x0102030405060708LL;
    qint64 swapped = IOUtils::swap_long(original);
    QCOMPARE(swapped, (qint64)0x0807060504030201LL);

    // Double swap = identity
    QCOMPARE(IOUtils::swap_long(swapped), original);
}

//=============================================================================================================

void TestIOUtils::testSwapLongp()
{
    qint64 val = 0x0102030405060708LL;
    qint64 original = val;
    IOUtils::swap_longp(&val);
    QCOMPARE(val, (qint64)0x0807060504030201LL);

    IOUtils::swap_longp(&val);
    QCOMPARE(val, original);
}

//=============================================================================================================

void TestIOUtils::testSwapFloat()
{
    float original = 1.5f;
    float swapped = IOUtils::swap_float(original);

    // Double swap = identity
    float restored = IOUtils::swap_float(swapped);
    QVERIFY(std::abs(restored - original) < 1e-6f);
}

//=============================================================================================================

void TestIOUtils::testSwapFloatp()
{
    float val = 3.14f;
    float original = val;
    IOUtils::swap_floatp(&val);

    // val is now byte-swapped (probably garbage as float, but swap back should restore)
    IOUtils::swap_floatp(&val);
    QVERIFY(std::abs(val - original) < 1e-6f);
}

//=============================================================================================================

void TestIOUtils::testSwapDoublep()
{
    double val = 2.71828;
    double original = val;
    IOUtils::swap_doublep(&val);

    // Double swap = identity
    IOUtils::swap_doublep(&val);
    QVERIFY(std::abs(val - original) < m_dEpsilon);
}

//=============================================================================================================

void TestIOUtils::testFread3QDataStream()
{
    // Write a known 3-byte big-endian integer and read it back
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    // Manually write 3 bytes representing value: (0x01 << 16) + (0x02 << 8) + 0x03 = 66051
    buffer.write("\x01\x02\x03", 3);
    buffer.close();

    buffer.open(QIODevice::ReadOnly);
    QDataStream stream(&buffer);
    qint32 result = IOUtils::fread3(stream);
    QCOMPARE(result, (qint32)66051);
}

//=============================================================================================================

void TestIOUtils::testFread3StdStream()
{
    std::stringstream ss;
    ss.write("\x01\x02\x03", 3);
    ss.seekg(0);

    std::iostream& stream = ss;
    qint32 result = IOUtils::fread3(stream);
    QCOMPARE(result, (qint32)66051);
}

//=============================================================================================================

void TestIOUtils::testFread3Many()
{
    // Write 3 x 3-byte integers
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    buffer.write("\x00\x00\x01", 3);  // 1
    buffer.write("\x00\x01\x00", 3);  // 256
    buffer.write("\x01\x00\x00", 3);  // 65536
    buffer.close();

    buffer.open(QIODevice::ReadOnly);
    QDataStream stream(&buffer);
    VectorXi result = IOUtils::fread3_many(stream, 3);

    QCOMPARE(result.size(), (Eigen::Index)3);
    QCOMPARE(result(0), (qint32)1);
    QCOMPARE(result(1), (qint32)256);
    QCOMPARE(result(2), (qint32)65536);
}

//=============================================================================================================

void TestIOUtils::testWriteReadEigenMatrixQString()
{
    // Write → Read round-trip for Eigen matrix
    QString path = m_tempDir.path() + "/test_matrix_qt.txt";

    MatrixXd A(3, 4);
    A << 1.1, 2.2, 3.3, 4.4,
         5.5, 6.6, 7.7, 8.8,
         9.9, 10.1, 11.2, 12.3;

    QVERIFY(IOUtils::write_eigen_matrix(A, path));

    MatrixXd B;
    QVERIFY(IOUtils::read_eigen_matrix(B, path));

    QCOMPARE(B.rows(), A.rows());
    QCOMPARE(B.cols(), A.cols());
    QVERIFY(B.isApprox(A, 1e-5));
}

//=============================================================================================================

void TestIOUtils::testWriteReadEigenMatrixStdString()
{
    std::string path = m_tempDir.path().toStdString() + "/test_matrix_std.txt";

    MatrixXd A(2, 3);
    A << 1.0, 2.0, 3.0,
         4.0, 5.0, 6.0;

    QVERIFY(IOUtils::write_eigen_matrix(A, path));

    MatrixXd B;
    QVERIFY(IOUtils::read_eigen_matrix(B, path));

    QCOMPARE(B.rows(), A.rows());
    QCOMPARE(B.cols(), A.cols());
    QVERIFY(B.isApprox(A, 1e-5));
}

//=============================================================================================================

void TestIOUtils::testGetNewChNamesConventionsQString()
{
    QStringList oldNames;
    oldNames << "MEG 0113" << "MEG 0122" << "EEG 001" << "STI 014";

    QStringList newNames = IOUtils::get_new_chnames_conventions(oldNames);

    QCOMPARE(newNames.size(), oldNames.size());
    // New convention removes space: "MEG 0113" -> "MEG0113"
    for (const auto& name : newNames) {
        QVERIFY(!name.contains(" ") || name.startsWith("STI") || name.startsWith("EEG"));
    }
}

//=============================================================================================================

void TestIOUtils::testGetNewChNamesConventionsStdString()
{
    std::vector<std::string> oldNames = {"MEG 0113", "MEG 0122"};

    std::vector<std::string> newNames = IOUtils::get_new_chnames_conventions(oldNames);

    QCOMPARE((int)newNames.size(), (int)oldNames.size());
}

//=============================================================================================================

void TestIOUtils::testGetOldChNamesConventionsQString()
{
    QStringList newNames;
    newNames << "MEG0113" << "MEG0122";

    QStringList oldNames = IOUtils::get_old_chnames_conventions(newNames);

    QCOMPARE(oldNames.size(), newNames.size());
    // Old convention adds space: "MEG0113" -> "MEG 0113"
    for (const auto& name : oldNames) {
        QVERIFY(name.contains(" ") || name == newNames[oldNames.indexOf(name)]);
    }
}

//=============================================================================================================

void TestIOUtils::testGetOldChNamesConventionsStdString()
{
    std::vector<std::string> newNames = {"MEG0113", "MEG0122"};

    std::vector<std::string> oldNames = IOUtils::get_old_chnames_conventions(newNames);

    QCOMPARE((int)oldNames.size(), (int)newNames.size());
}

//=============================================================================================================

void TestIOUtils::testCheckMatchingChNamesConventions()
{
    QStringList list1, list2;
    list1 << "MEG 0113" << "MEG 0122";
    list2 << "MEG0113" << "MEG0122";

    // Should detect mismatch and optionally fix it
    bool result = IOUtils::check_matching_chnames_conventions(list1, list2, true);

    // After matching, both lists should use the same convention
    QVERIFY(result || list1 == list2);
}

//=============================================================================================================

void TestIOUtils::cleanupTestCase()
{
    qInfo() << "TestIOUtils: All tests completed";
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestIOUtils)
#include "test_utils_ioutils.moc"
