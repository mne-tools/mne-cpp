//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fiff_stream_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Round-trips FiffStream tag writers through the readers and mne-python.
 *
 * The existing writer tests only check that a tag was written. Here every
 * structure is written to one file, read back by MNE-CPP and compared field by
 * field. mne-python's tag reader (mne._fiff.tag.read_tag) read the same file to
 * confirm the on-disk layout, giving the values asserted below:
 *
 *   sparse CCS / RCS 3 x 4 [[1,0,2,0],[0,0,3,4],[5,0,0,6]]   -> scipy csc / csr equal to it
 *   int matrix 2 x 3 [[1,2,3],[4,5,6]]                      -> int32 array equal to it
 *   named matrix 2 x 3 (rows R1 R2, cols C1 C2 C3)          -> _read_named_matrix
 *   projectors (active PCA-1, inactive PCA-2)                -> _read_proj desc / active / data
 *   MEG -> head transform (30 deg about z, move 1, 2, 3 cm)  -> Transform('meg', 'head')
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_constants.h>

#include <cmath>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/SparseCore>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

namespace
{

MatrixXf sparseReference()
{
    MatrixXf m(3, 4);
    m << 1, 0, 2, 0,
        0, 0, 3, 4,
        5, 0, 0, 6;
    return m;
}

} // namespace

//=============================================================================================================
/**
 * Round-trips FiffStream tag writers through their readers.
 */
class TestFiffStreamPython : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void sparseMatrices_data();
    void sparseMatrices();
    void intMatrix();
    void namedMatrix();
    void projectors();
    void coordTrans();

private:
    FiffDirNode::SPtr findBlock(int kind) const;

    QTemporaryDir m_dir;
    QString m_path;
    QFile m_file;
    FiffStream::SPtr m_stream;
    Matrix3f m_rot;
    Vector3f m_move;
};

//=============================================================================================================

void TestFiffStreamPython::initTestCase()
{
    QVERIFY(m_dir.isValid());
    m_path = m_dir.filePath("stream-roundtrip.fif");

    m_rot = AngleAxisf(static_cast<float>(M_PI / 6.0), Vector3f::UnitZ()).toRotationMatrix();
    m_move = Vector3f(0.01f, 0.02f, 0.03f);

    {
        QFile out(m_path);
        FiffStream::SPtr w = FiffStream::start_file(out);
        QVERIFY(w);
        const SparseMatrix<float> sp = sparseReference().sparseView();
        w->write_float_sparse_ccs(FIFF_MNE_FORWARD_SOLUTION, sp);
        w->write_float_sparse_rcs(FIFF_MNE_FORWARD_SOLUTION_GRAD, sp);

        MatrixXi im(2, 3);
        im << 1, 2, 3, 4, 5, 6;
        w->write_int_matrix(FIFF_MNE_SOURCE_SPACE_TRIANGLES, im);

        FiffNamedMatrix named;
        named.nrow = 2;
        named.ncol = 3;
        named.row_names << "R1" << "R2";
        named.col_names << "C1" << "C2" << "C3";
        named.data.resize(2, 3);
        named.data << 0.5, -1.0, 2.0, 3.5, 0.0, -4.25;
        w->write_named_matrix(FIFF_MNE_CTF_COMP_DATA, named);

        QList<FiffProj> projs;
        for (int k = 0; k < 2; ++k) {
            FiffNamedMatrix vec;
            vec.nrow = 1;
            vec.ncol = 3;
            vec.col_names << "MEG 0113" << "MEG 0112" << "MEG 0111";
            vec.data = RowVector3d(k == 0 ? 0.6 : 0.0, 0.8, k == 0 ? 0.0 : 0.6);
            projs.append(FiffProj(FIFFV_PROJ_ITEM_FIELD, k == 0, QStringLiteral("PCA-%1").arg(k + 1), vec));
        }
        w->write_proj(projs);

        w->write_coord_trans(FiffCoordTrans(FIFFV_COORD_DEVICE, FIFFV_COORD_HEAD, m_rot, m_move));
        w->end_file();
    }

    m_file.setFileName(m_path);
    m_stream = FiffStream::SPtr(new FiffStream(&m_file));
    QVERIFY(m_stream->open());
}

//=============================================================================================================

FiffDirNode::SPtr TestFiffStreamPython::findBlock(int kind) const
{
    const QList<FiffDirNode::SPtr> nodes = m_stream->dirtree()->dir_tree_find(kind);
    return nodes.isEmpty() ? FiffDirNode::SPtr() : nodes.first();
}

//=============================================================================================================

void TestFiffStreamPython::sparseMatrices_data()
{
    QTest::addColumn<int>("kind");
    QTest::newRow("CCS") << static_cast<int>(FIFF_MNE_FORWARD_SOLUTION);
    QTest::newRow("RCS") << static_cast<int>(FIFF_MNE_FORWARD_SOLUTION_GRAD);
}

void TestFiffStreamPython::sparseMatrices()
{
    QFETCH(int, kind);
    FiffTag::UPtr tag;
    QVERIFY(m_stream->dirtree()->find_tag(m_stream, kind, tag));
    QVERIFY(tag->isMatrix());
    const MatrixXd back = MatrixXd(tag->toSparseFloatMatrix());
    QCOMPARE(back.rows(), 3);
    QCOMPARE(back.cols(), 4);
    QVERIFY2((back - sparseReference().cast<double>()).cwiseAbs().maxCoeff() == 0.0,
             qPrintable(QStringLiteral("sparse round trip differs")));
}

//=============================================================================================================

void TestFiffStreamPython::intMatrix()
{
    FiffTag::UPtr tag;
    QVERIFY(m_stream->dirtree()->find_tag(m_stream, FIFF_MNE_SOURCE_SPACE_TRIANGLES, tag));
    MatrixXi expected(2, 3);
    expected << 1, 2, 3, 4, 5, 6;
    // Like the float decoders, toIntMatrix returns the transpose of the
    // stored row-major matrix; callers transpose it back.
    QCOMPARE(MatrixXi(tag->toIntMatrix().transpose()), expected);
}

//=============================================================================================================

void TestFiffStreamPython::namedMatrix()
{
    QVERIFY(findBlock(FIFFB_MNE_NAMED_MATRIX));
    FiffNamedMatrix back;
    QVERIFY(m_stream->read_named_matrix(m_stream->dirtree(), FIFF_MNE_CTF_COMP_DATA, back));
    QCOMPARE(back.nrow, 2);
    QCOMPARE(back.ncol, 3);
    QCOMPARE(back.row_names, QStringList({"R1", "R2"}));
    QCOMPARE(back.col_names, QStringList({"C1", "C2", "C3"}));
    MatrixXd expected(2, 3);
    expected << 0.5, -1.0, 2.0, 3.5, 0.0, -4.25;
    QCOMPARE(back.data, expected);
}

//=============================================================================================================

void TestFiffStreamPython::projectors()
{
    const QList<FiffProj> back = m_stream->read_proj(m_stream->dirtree());
    QCOMPARE(back.size(), 2);
    QCOMPARE(back[0].desc, QString("PCA-1"));
    QCOMPARE(back[1].desc, QString("PCA-2"));
    QCOMPARE(back[0].active, true);
    QCOMPARE(back[1].active, false);
    QCOMPARE(back[0].kind, static_cast<fiff_int_t>(FIFFV_PROJ_ITEM_FIELD));
    // Channel names are read without spaces (FIFF convention in MNE-CPP).
    QCOMPARE(back[0].data->col_names, QStringList({"MEG0113", "MEG0112", "MEG0111"}));
    QVERIFY((back[0].data->data - RowVector3d(0.6, 0.8, 0.0)).cwiseAbs().maxCoeff() < 1e-7);
    QVERIFY((back[1].data->data - RowVector3d(0.0, 0.8, 0.6)).cwiseAbs().maxCoeff() < 1e-7);
}

//=============================================================================================================

void TestFiffStreamPython::coordTrans()
{
    FiffTag::UPtr tag;
    QVERIFY(m_stream->dirtree()->find_tag(m_stream, FIFF_COORD_TRANS, tag));
    const FiffCoordTrans t = tag->toCoordTrans();
    QCOMPARE(t.from, static_cast<fiff_int_t>(FIFFV_COORD_DEVICE));
    QCOMPARE(t.to, static_cast<fiff_int_t>(FIFFV_COORD_HEAD));
    QVERIFY((t.trans.block<3, 3>(0, 0) - m_rot).cwiseAbs().maxCoeff() < 1e-7f);
    QVERIFY((t.trans.block<3, 1>(0, 3) - m_move).cwiseAbs().maxCoeff() < 1e-7f);
    // The stored inverse must undo the transform.
    QVERIFY((t.trans * t.invtrans - Matrix4f::Identity()).cwiseAbs().maxCoeff() < 1e-6f);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffStreamPython)
#include "test_fiff_stream_python.moc"
