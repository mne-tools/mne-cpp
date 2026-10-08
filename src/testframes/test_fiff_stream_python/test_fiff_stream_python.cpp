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
#include <fiff/fiff_cov.h>
#include <fiff/fiff_digitizer_data.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_constants.h>

#include <cmath>
#include <cstring>
#include <memory>

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
    void copiesProcessingHistory();
    void attachesEnvironment();
    void readsFloatCovariance();
    void readsMeasInfo();
    void readsTagTypes();
    void printsDirectoryTree();

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

void TestFiffStreamPython::copiesProcessingHistory()
{
    // data/sss_history_raw.fif (make_proc_history_fixture.py) carries three MaxFilter
    // runs; mne-python reads 10th-order SSS info for the first two and cross-talk
    // compensation (5 entries) for the second.
    const QString src = QStringLiteral(MNE_FIFF_STREAM_DATA_DIR "/sss_history_raw.fif");
    auto historyOf = [](const QString& path, QList<FiffDirNode::SPtr>& runs) {
        auto file = std::make_shared<QFile>(path);
        FiffStream::SPtr stream(new FiffStream(file.get()));
        if (!stream->open())
            return false;
        const QList<FiffDirNode::SPtr> hist = stream->dirtree()->dir_tree_find(FIFFB_PROCESSING_HISTORY);
        if (hist.isEmpty())
            return false;
        runs = hist[0]->dir_tree_find(FIFFB_PROCESSING_RECORD);
        stream->close();
        return true;
    };
    QList<FiffDirNode::SPtr> srcRuns;
    QVERIFY(historyOf(src, srcRuns));
    QCOMPARE(srcRuns.size(), 3);

    const QString dst = m_dir.filePath("history-copy.fif");
    {
        QFile out(dst);
        FiffStream::SPtr w = FiffStream::start_file(out);
        QVERIFY(w);
        fiff_int_t nchan = 3;
        w->write_int(FIFF_NCHAN, &nchan);
        w->end_file();
    }
    QVERIFY(FiffStream::copyProcessingHistory(src, dst));

    QList<FiffDirNode::SPtr> dstRuns;
    QVERIFY(historyOf(dst, dstRuns));
    QCOMPARE(dstRuns.size(), srcRuns.size());
    for (int k = 0; k < srcRuns.size(); ++k) {
        QCOMPARE(dstRuns[k]->nchild(), srcRuns[k]->nchild());
        QCOMPARE(dstRuns[k]->dir_tree_find(FIFFB_SSS_INFO).size(), srcRuns[k]->dir_tree_find(FIFFB_SSS_INFO).size());
        QCOMPARE(dstRuns[k]->dir_tree_find(FIFFB_SSS_CAL_ADJUST).size(), srcRuns[k]->dir_tree_find(FIFFB_SSS_CAL_ADJUST).size());
        QCOMPARE(dstRuns[k]->dir_tree_find(FIFFB_CHANNEL_DECOUPLER).size(), srcRuns[k]->dir_tree_find(FIFFB_CHANNEL_DECOUPLER).size());
    }
}

//=============================================================================================================

void TestFiffStreamPython::attachesEnvironment()
{
    const QString path = m_dir.filePath("env.fif");
    {
        QFile out(path);
        FiffStream::SPtr w = FiffStream::start_file(out);
        QVERIFY(w);
        w->start_block(FIFFB_MEAS);
        fiff_int_t nchan = 3;
        w->write_int(FIFF_NCHAN, &nchan);
        w->end_block(FIFFB_MEAS);
        w->end_file();
    }
    {
        QFile f(path);
        FiffStream::SPtr u = FiffStream::open_update(f);
        QVERIFY(u);
        QVERIFY(u->attach_env(QStringLiteral("/data/stüdy"), QStringLiteral("mne_process --raw a.fif")));
        u->close();
    }

    QFile f(path);
    FiffStream::SPtr r(new FiffStream(&f));
    QVERIFY(r->open());
    const QList<FiffDirNode::SPtr> env = r->dirtree()->dir_tree_find(FIFFB_MNE_ENV);
    QCOMPARE(env.size(), 1);
    FiffTag::UPtr tag;
    QVERIFY(env[0]->find_tag(r, FIFF_MNE_ENV_WORKING_DIR, tag));
    // Strings are written as UTF-8: the tag holds the bytes, not the characters, of a non-ASCII path.
    QCOMPARE(tag->toString(), QStringLiteral("/data/stüdy"));
    QVERIFY(env[0]->find_tag(r, FIFF_MNE_ENV_COMMAND_LINE, tag));
    QCOMPARE(tag->toString(), QString("mne_process --raw a.fif"));
    // The measurement block written before is still there.
    const QList<FiffDirNode::SPtr> meas = r->dirtree()->dir_tree_find(FIFFB_MEAS);
    QCOMPARE(meas.size(), 1);
    QVERIFY(meas[0]->find_tag(r, FIFF_NCHAN, tag));
    QCOMPARE(*tag->toInt(), 3);
    r->close();
}

void TestFiffStreamPython::readsFloatCovariance()
{
    // data/float_cov.fif (make_float_cov_fixture.py): single-precision covariances as MNE-C writes them.
    QFile file(QStringLiteral(MNE_FIFF_STREAM_DATA_DIR "/float_cov.fif"));
    FiffStream stream(&file);
    QVERIFY(stream.open());

    FiffCov noise;
    QVERIFY(stream.read_cov(stream.dirtree(), FIFFV_MNE_NOISE_COV, noise));
    QCOMPARE(noise.dim, 3);
    QCOMPARE(noise.nfree, 42);
    QVERIFY(!noise.diag);
    QCOMPARE(noise.names, QStringList({"A", "B", "C"}));
    Matrix3d full;
    full << 1.164402008, 0.134921417, 0.535389721,
        0.134921417, 2.983242273, -0.174825236,
        0.535389721, -0.174825236, 3.042061329;
    QVERIFY((noise.data - full).cwiseAbs().maxCoeff() < 1e-6);
    QVERIFY((noise.eig - Vector3d(1.006871979, 2.928603736, 3.254229896)).cwiseAbs().maxCoeff() < 1e-6);
    // Rows of eigvec are the eigenvectors, as in mne-python (whitener = diag(1/sqrt(eig)) * eigvec).
    QCOMPARE(noise.eigvec.rows(), 3);
    QVERIFY((noise.eigvec * full - noise.eig.asDiagonal() * noise.eigvec).cwiseAbs().maxCoeff() < 1e-5);
    QCOMPARE(noise.bads, QStringList({"B"}));
    QCOMPARE(noise.projs.size(), 1);
    QCOMPARE(noise.projs[0].desc, QString("ECG-1"));
    QCOMPARE(noise.projs[0].kind, static_cast<fiff_int_t>(FIFFV_PROJ_ITEM_FIELD));
    QVERIFY(!noise.projs[0].active);
    QCOMPARE(noise.projs[0].data->col_names, QStringList({"A", "B", "C"}));
    QVERIFY((noise.projs[0].data->data - RowVector3d(0.0, 0.6, 0.8)).cwiseAbs().maxCoeff() < 1e-7);

    FiffCov source;
    QVERIFY(stream.read_cov(stream.dirtree(), FIFFV_MNE_SOURCE_COV, source));
    QVERIFY(source.diag);
    QCOMPARE(source.nfree, -1);
    QCOMPARE(source.data.col(0), Vector4d(1.5, 2.5, 3.5, 4.5));

    FiffCov missing;
    QVERIFY(!stream.read_cov(stream.dirtree(), FIFFV_MNE_DEPTH_PRIOR_COV, missing));
    stream.close();
}

//=============================================================================================================

void TestFiffStreamPython::readsMeasInfo()
{
    // data/meas_info.fif (make_meas_info_fixture.py); expected values are mne-python's read_info.
    QFile file(QStringLiteral(MNE_FIFF_STREAM_DATA_DIR "/meas_info.fif"));
    FiffStream stream(&file);
    QVERIFY(stream.open());
    FiffInfo info;
    FiffDirNode::SPtr meas;
    QVERIFY(stream.read_meas_info(stream.dirtree(), info, meas));

    // The Isotrak's own FIFF_MNE_COORD_FRAME overrides the head-frame default.
    FiffDigitizerData digData;
    QVERIFY(stream.read_digitizer_data(stream.dirtree(), digData));
    QCOMPARE(digData.coord_frame, FIFFV_COORD_MRI);
    QCOMPARE(digData.npoint, 3);
    QCOMPARE(digData.points[2].coord_frame, FIFFV_COORD_MRI);
    QCOMPARE(digData.points[2].ident, 3);
    stream.close();

    QCOMPARE(info.nchan, 2);
    QCOMPARE(info.meas_id.version, 65540);
    QCOMPARE(info.meas_id.machid[0], 11);
    QCOMPARE(info.meas_id.machid[1], 22);
    // Without FIFF_MEAS_DATE the date comes from the measurement id.
    QCOMPARE(info.meas_date[0], 1700000000);
    QCOMPARE(info.meas_date[1], 250000);
    QCOMPARE(info.utc_offset, QString("+0100"));
    QCOMPARE(info.gantry_angle, 68);
    QCOMPARE(info.acq_pars, QString("pars"));
    QCOMPARE(info.acq_stim, QString("stim"));

    // ctf_head_t only in the HPI result block.
    QCOMPARE(info.ctf_head_t.from, FIFFV_MNE_COORD_CTF_HEAD);
    Matrix4f devCtf;
    devCtf << 0, 0, -1, 0.022f, -1, 0, 0, -0.006f, 0, 1, 0, 0.03f, 0, 0, 0, 1;
    QVERIFY((info.dev_ctf_t.trans - devCtf).cwiseAbs().maxCoeff() < 1e-6f);

    QCOMPARE(info.dig.size(), 3);
    for (const FiffDigPoint& d : info.dig)
        QCOMPARE(d.coord_frame, FIFFV_COORD_MRI);
    QVERIFY((Map<const Vector3f>(info.dig[1].r) - Vector3f(0.0f, 0.1f, 0.0f)).norm() < 1e-7f);
    QCOMPARE(info.dig_trans.from, FIFFV_COORD_MRI);
    QCOMPARE(info.dig_trans.to, FIFFV_COORD_HEAD);
    QVERIFY(std::abs(info.dig_trans.trans(2, 3) - 0.04f) < 1e-7f);
}

//=============================================================================================================

void TestFiffStreamPython::readsTagTypes()
{
    // data/tag_types.fif (make_tag_types_fixture.py): big-endian payloads of every swapped type.
    QFile file(QStringLiteral(MNE_FIFF_STREAM_DATA_DIR "/tag_types.fif"));
    FiffStream stream(&file);
    QVERIFY(stream.open());
    auto tag = [&](int kind) {
        FiffTag::UPtr t;
        stream.dirtree()->find_tag(&stream, kind, t);
        return t;
    };

    FiffTag::UPtr t = tag(901);
    QVERIFY(t);
    QCOMPARE(t->toDouble()[0], 1.25);
    QCOMPARE(t->toDouble()[1], -2.5e-12);
    QCOMPARE(t->toDouble()[2], 3.0e200);
    t = tag(902);
    QCOMPARE(t->toShort()[0], qint16(-3));
    QCOMPARE(t->toShort()[2], qint16(-32000));
    t = tag(903);
    QCOMPARE(t->toUnsignedShort()[1], quint16(40000));
    QCOMPARE(t->toUnsignedShort()[2], quint16(65535));
    t = tag(904);
    QCOMPARE(*t->toJulian(), 2461318); // 2026-10-04
    t = tag(905);
    QCOMPARE(t->toDauPack16()[0], qint16(-7));
    QCOMPARE(t->toDauPack16()[1], qint16(12345));
    // Complex and matrix payloads have no typed accessor; read the swapped data directly.
    auto floats = [](const FiffTag::UPtr& x) {
        return reinterpret_cast<const float*>(x->data());
    };
    t = tag(906);
    QCOMPARE(floats(t)[0], 1.5f);
    QCOMPARE(floats(t)[1], -2.0f);
    QCOMPARE(floats(t)[3], 4.0f);

    // Dense matrices keep their trailing dimensions (ncol, nrow, ndim) in host order.
    qint32 ndim;
    QVector<qint32> dims;
    t = tag(907);
    QVERIFY(t->getMatrixDimensions(ndim, dims));
    QCOMPARE(dims, QVector<qint32>({3, 2}));
    QCOMPARE(reinterpret_cast<const double*>(t->data())[5], 6.5);
    t = tag(908);
    QVERIFY(t->getMatrixDimensions(ndim, dims));
    QCOMPARE(dims, QVector<qint32>({2, 1}));
    QCOMPARE(floats(t)[3], -4.0f);

    MatrixXd expected(4, 4);
    expected << 1, 0, 2, 0, 0, 0, 3, 4, 5, 0, 0, 6, 0, 7, 0, 0;
    for (int kind : {909, 910}) {
        t = tag(kind);
        QVERIFY(t);
        QCOMPARE(MatrixXd(t->toSparseFloatMatrix()), expected);
    }

    // getInfo names the type of each tag as make_tag_types_fixture.py wrote it.
    const QList<QPair<int, QString>> infos{{901, "Simple type FIFFT_DOUBLE"}, {902, "Simple type FIFFT_SHORT"}, {903, "Simple type FIFFT_USHORT"}, {904, "Simple type FIFFT_JULIAN"}, {905, "Simple type FIFFT_DAU_PACK16"}, {906, "Simple type FIFFT_COMPLEX_FLOAT"}, {907, "Matrix of type FIFFT_DOUBLE"}, {908, "Matrix of type FIFFT_COMPLEX_FLOAT"}, {909, "Matrix of type FIFFT_FLOAT"}};
    for (const auto& [kind, info] : infos)
        QCOMPARE(tag(kind)->getInfo(), info);
    FiffTag unknown;
    unknown.type = 9999;
    QCOMPARE(unknown.getInfo(), QString("Structure unknown"));

    // write_tag must turn every payload back into file byte order.
    const QString copyPath = m_dir.filePath("tag-types-copy.fif");
    {
        QFile out(copyPath);
        FiffStream::SPtr w = FiffStream::start_file(out);
        QVERIFY(w);
        for (int kind = 901; kind <= 910; ++kind)
            w->write_tag(tag(kind));
        w->end_file();
    }
    QFile copyFile(copyPath);
    FiffStream copy(&copyFile);
    QVERIFY(copy.open());
    for (int kind = 901; kind <= 910; ++kind) {
        FiffTag::UPtr back;
        QVERIFY(copy.dirtree()->find_tag(&copy, kind, back));
        const FiffTag::UPtr orig = tag(kind);
        QVERIFY2(back->size() == orig->size() && std::memcmp(back->data(), orig->data(), static_cast<size_t>(orig->size())) == 0,
                 qPrintable(QStringLiteral("tag %1 changed in write_tag").arg(kind)));
    }
    copy.close();
    stream.close();
}

//=============================================================================================================

namespace
{
QStringList g_messages;
void collectMessages(QtMsgType, const QMessageLogContext&, const QString& msg)
{
    g_messages << msg;
}
} // namespace

void TestFiffStreamPython::printsDirectoryTree()
{
    // mne-python fiff_open on test_mne_ctf_comp_python/data/ctf_grade0_raw.fif: blocks depth-first (its root 0 is
    // MNE-CPP's FIFFB_ROOT), with a run of 36 FIFF_CH_INFO (203) tags in the meas info block.
    QFile file(QStringLiteral(MNE_CTF_COMP_DATA_DIR "/ctf_grade0_raw.fif"));
    FiffStream stream(&file);
    QVERIFY(stream.open());

    g_messages.clear();
    const QtMessageHandler previous = qInstallMessageHandler(collectMessages);
    stream.dirtree()->print(0);
    qInstallMessageHandler(previous);

    const QList<int> blocks{FIFFB_ROOT, 100, 101, 109, 107, 106, 370, 371, 357, 371, 357, 371, 357, 371, 357, 371, 357, 102};
    QList<int> printedBlocks;
    int printedTags = 0;
    // Blocks without a description (the CTF compensation 370/371/357) are reported as "Cannot explain".
    const QRegularExpression entry(QStringLiteral("^(?:(\\d+) = |Cannot explain: (\\d+)$)"));
    for (int i = 0; i < g_messages.size(); ++i) {
        const QRegularExpressionMatch m = entry.match(g_messages[i]);
        // A block line is the one followed by " { ".
        if (i + 1 < g_messages.size() && g_messages[i + 1] == QStringLiteral(" { "))
            printedBlocks << (m.captured(1).isEmpty() ? m.captured(2) : m.captured(1)).toInt();
        else if (m.hasMatch())
            ++printedTags;
    }
    QCOMPARE(printedBlocks, blocks);
    QVERIFY(g_messages.contains(QStringLiteral("203 = %1").arg(QString::fromLatin1(FiffDirNode::get_tag_explanation(FIFF_CH_INFO)))));
    QVERIFY(g_messages.contains(QStringLiteral(" [36]\n")));
    QCOMPARE(QString::fromLatin1(FiffDirNode::get_tag_explanation(-5)), QStringLiteral("unknown"));
    QCOMPARE(QString::fromLatin1(FiffDirNode::get_unit_name(FIFF_UNIT_T)), QStringLiteral("T"));
    QCOMPARE(QString::fromLatin1(FiffDirNode::get_unit_name(FIFF_UNIT_T_M)), QStringLiteral("T/m"));
    QCOMPARE(QString::fromLatin1(FiffDirNode::get_unit_name(FIFF_UNIT_V)), QStringLiteral("V"));
    QCOMPARE(QString::fromLatin1(FiffDirNode::get_unit_name(FIFF_UNIT_NONE)), QStringLiteral("NA"));
    g_messages.clear();
    qInstallMessageHandler(collectMessages);
    FiffDirNode::explain(-5);
    FiffDirNode::explain_block(-5);
    qInstallMessageHandler(previous);
    QCOMPARE(g_messages, QStringList({QStringLiteral("Cannot explain: -5"), QStringLiteral("Cannot explain: -5")}));
    // One line per run of equal tag kinds: mne-python's tree has 50; the root's FIFF_FILE_ID entry is its id here.
    QCOMPARE(printedTags, 49);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffStreamPython)
#include "test_fiff_stream_python.moc"
