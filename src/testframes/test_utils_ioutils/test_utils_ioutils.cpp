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
 * @brief    Tests for IOUtils — Eigen matrix I/O.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/ioutils.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QObject>
#include <QTemporaryDir>
#include <QDebug>
#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Tests for IOUtils: Eigen matrix write/read round-trip I/O.
 */
class TestIOUtils : public QObject
{
    Q_OBJECT

public:
    TestIOUtils();

private slots:
    void initTestCase();

    // Eigen matrix write/read round-trip
    void testWriteReadEigenMatrixQString();
    void testWriteReadEigenMatrixStdString();

    void cleanupTestCase();

private:
    QTemporaryDir m_tempDir;
};

//=============================================================================================================

TestIOUtils::TestIOUtils()
{
}

//=============================================================================================================

void TestIOUtils::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
    QVERIFY(m_tempDir.isValid());
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

void TestIOUtils::cleanupTestCase()
{
    qInfo() << "TestIOUtils: All tests completed";
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestIOUtils)
#include "test_utils_ioutils.moc"
