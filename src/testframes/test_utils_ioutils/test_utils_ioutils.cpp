//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_utils_ioutils.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
