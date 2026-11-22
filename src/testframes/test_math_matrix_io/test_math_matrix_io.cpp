//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_math_matrix_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Tests for mne::write_matrix and mne::read_matrix.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <math/matrix_io.h>
#include <base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Tests for the Eigen text matrix I/O: round trips, vectors, comments and malformed input.
 */
class TestMatrixIo : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void roundTripIsExact();
    void vectorsAndExpressions();
    void commentsAndBlankLinesAreSkipped();
    void nonFiniteValuesRoundTrip();
    void malformedInputFails();

private:
    std::filesystem::path file(const std::string& name) const;
    void writeText(const std::string& name, const std::string& text) const;

    QTemporaryDir m_tempDir;
};

//=============================================================================================================

std::filesystem::path TestMatrixIo::file(const std::string& name) const
{
    return std::filesystem::path(m_tempDir.path().toStdString()) / name;
}

//=============================================================================================================

void TestMatrixIo::writeText(const std::string& name, const std::string& text) const
{
    std::ofstream(file(name)) << text;
}

//=============================================================================================================

void TestMatrixIo::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================

void TestMatrixIo::roundTripIsExact()
{
    MatrixXd a(3, 4);
    a << 0.1, 2.0 / 3.0, -3.3e-12, 4.4,
        5.5, 6.6, 7.7, 8.8,
        9.9, 10.1, 11.2, 1e300;
    QVERIFY(write_matrix(a, file("a.txt"), "three by four"));

    MatrixXd b;
    QVERIFY(read_matrix(b, file("a.txt")));
    QCOMPARE(b.rows(), a.rows());
    QCOMPARE(b.cols(), a.cols());
    QVERIFY(b == a); // shortest round-trip representation: bit-identical

    MatrixXf f = a.cast<float>().topLeftCorner(2, 2);
    QVERIFY(write_matrix(f, file("f.txt")));
    MatrixXf g;
    QVERIFY(read_matrix(g, file("f.txt")));
    QVERIFY(g == f);
}

//=============================================================================================================

void TestMatrixIo::vectorsAndExpressions()
{
    MatrixXd m(2, 3);
    m << 1, 2, 3, 4, 5, 6;
    QVERIFY(write_matrix(m.transpose(), file("t.txt"))); // any dense expression
    MatrixXd t;
    QVERIFY(read_matrix(t, file("t.txt")));
    QVERIFY(t == m.transpose());

    QVERIFY(write_matrix(m, file("m.txt")));
    RowVectorXd firstRow;
    QVERIFY(read_matrix(firstRow, file("m.txt")));
    QVERIFY(firstRow == m.row(0));
    VectorXd firstCol;
    QVERIFY(read_matrix(firstCol, file("m.txt")));
    QVERIFY(firstCol == m.col(0));

    Matrix<double, 2, 3> fixed;
    QVERIFY(read_matrix(fixed, file("m.txt")));
    QVERIFY(fixed == m);
    Matrix3d wrongSize = Matrix3d::Zero();
    QVERIFY(!read_matrix(wrongSize, file("m.txt")));
    QVERIFY(wrongSize.isZero());

    writeText("empty.txt", "");
    MatrixXd empty(2, 2);
    QVERIFY(read_matrix(empty, file("empty.txt")));
    QCOMPARE(empty.size(), Index(0));
    VectorXd emptyVector(3);
    QVERIFY(read_matrix(emptyVector, file("empty.txt")));
    QCOMPARE(emptyVector.size(), Index(0));
}

//=============================================================================================================

void TestMatrixIo::commentsAndBlankLinesAreSkipped()
{
    writeText("c.txt", "# header\n\n 1\t2  3 \n# middle\n4 5 6\n\n");
    MatrixXd c;
    QVERIFY(read_matrix(c, file("c.txt")));
    QCOMPARE(c.rows(), Index(2));
    QCOMPARE(c.cols(), Index(3));
    QCOMPARE(c(1, 2), 6.0);

    std::istringstream text("0.5 -1e3\n2 3\n");
    Matrix2d fromStream;
    QVERIFY(read_matrix(fromStream, text));
    QCOMPARE(fromStream(0, 1), -1000.0);
    std::istringstream ragged("1 2\n3\n");
    QVERIFY(!read_matrix(c, ragged));
}

//=============================================================================================================

void TestMatrixIo::nonFiniteValuesRoundTrip()
{
    RowVector3d v(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), -1.5);
    QVERIFY(write_matrix(v, file("nan.txt")));
    RowVectorXd w;
    QVERIFY(read_matrix(w, file("nan.txt")));
    QCOMPARE(w.size(), Index(3));
    QVERIFY(std::isnan(w(0)));
    QVERIFY(std::isinf(w(1)) && w(1) > 0);
    QCOMPARE(w(2), -1.5);
}

//=============================================================================================================

void TestMatrixIo::malformedInputFails()
{
    MatrixXd untouched = MatrixXd::Ones(1, 1);
    QVERIFY(!read_matrix(untouched, file("missing.txt")));

    writeText("ragged.txt", "1 2 3\n4 5\n");
    QVERIFY(!read_matrix(untouched, file("ragged.txt")));

    writeText("text.txt", "1 2 x\n");
    QVERIFY(!read_matrix(untouched, file("text.txt")));

    writeText("comma.txt", "1,5 2\n");
    QVERIFY(!read_matrix(untouched, file("comma.txt")));

    QVERIFY(untouched == MatrixXd::Ones(1, 1));
    QVERIFY(!write_matrix(untouched, file("no_such_dir") / "out.txt"));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMatrixIo)
#include "test_math_matrix_io.moc"
