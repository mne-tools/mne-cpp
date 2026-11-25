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

#include <mne/math/mat_file.h>
#include <mne/math/matrix_io.h>
#include <mne/base/log.h>

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
#include <cstdint>
#include <cstring>
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
 * @brief Tests for the Eigen text matrix I/O (round trips, vectors, comments, malformed input) and the MAT-file writer.
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
    void matFileLayout();
    void matFileErrors();

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

void TestMatrixIo::matFileLayout()
{
    MatrixXd x(2, 3);
    x << 1, 2, 3, 4, 5, 6;
    MatrixXd times(1, 2);
    times << 0.5, -0.25;
    QVERIFY(write_mat_file(file("out.mat"), "test", {{"x", x}, {"times_long", times}}));

    std::ifstream in(file("out.mat"), std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    // Header 128, 'x' 8 + 96 (short name packed into its tag), 'times_long' 8 + 80 (name padded to 16)
    QCOMPARE(bytes.size(), std::size_t{320});
    QVERIFY(bytes.starts_with("MATLAB 5.0 MAT-file, created by test "));
    QCOMPARE(bytes.substr(124, 4), std::string("\x00\x01IM", 4));

    auto u32 = [&](std::size_t at) {
        std::uint32_t v = 0;
        for (int i = 3; i >= 0; --i) {
            v = (v << 8) | static_cast<unsigned char>(bytes[at + static_cast<std::size_t>(i)]);
        }
        return v;
    };
    auto f64 = [&](std::size_t at) {
        std::uint64_t v = 0;
        for (int i = 7; i >= 0; --i) {
            v = (v << 8) | static_cast<unsigned char>(bytes[at + static_cast<std::size_t>(i)]);
        }
        double d = 0.0;
        std::memcpy(&d, &v, sizeof d);
        return d;
    };
    QCOMPARE(u32(128), 14u);             // miMATRIX
    QCOMPARE(u32(132), 96u);             // payload
    QCOMPARE(u32(144), 6u);              // mxDOUBLE_CLASS
    QCOMPARE(u32(160), 2u);              // rows
    QCOMPARE(u32(164), 3u);              // cols
    QCOMPARE(u32(168), (1u << 16) | 1u); // small miINT8 element of 1 byte
    QCOMPARE(bytes[172], 'x');
    QCOMPARE(u32(176), 9u); // miDOUBLE
    QCOMPARE(u32(180), 48u);
    const double columnMajor[] = {1, 4, 2, 5, 3, 6};
    for (std::size_t i = 0; i < 6; ++i) {
        QCOMPARE(f64(184 + 8 * i), columnMajor[i]);
    }
    QCOMPARE(u32(232), 14u);
    QCOMPARE(u32(236), 80u);
    QCOMPARE(u32(272), 1u); // miINT8 with a full tag
    QCOMPARE(u32(276), 10u);
    QCOMPARE(bytes.substr(280, 10), std::string("times_long"));
    QCOMPARE(f64(304), 0.5);
    QCOMPARE(f64(312), -0.25);
}

//=============================================================================================================

void TestMatrixIo::matFileErrors()
{
    const MatrixXd one = MatrixXd::Ones(1, 1);
    const auto unnamed = write_mat_file(file("unnamed.mat"), "test", {{"", one}});
    QVERIFY(!unnamed);
    QVERIFY(unnamed.error().code == ErrorCode::Format);
    const auto unwritable = write_mat_file(file("no_such_dir") / "out.mat", "test", {{"one", one}});
    QVERIFY(!unwritable);
    QVERIFY(unwritable.error().code == ErrorCode::Io);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMatrixIo)
#include "test_math_matrix_io.moc"
