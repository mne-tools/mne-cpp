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
#include <fiff/fiff_annotations.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_digitizer_data.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_explain.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QBuffer>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/SparseCore>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <sstream>
#include <cmath>
#include <cstring>
#include <memory>
#include <numbers>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
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
    void refusesUnwritablePath();
    void writesAnnotationsAndDevice();

private:
    std::shared_ptr<FiffDirNode> findBlock(int kind) const;

    QTemporaryDir m_dir;
    QString m_path;
    std::unique_ptr<FiffStream> m_stream;
    Matrix3f m_rot;
    Vector3f m_move;
};

//=============================================================================================================

void TestFiffStreamPython::initTestCase()
{
    QVERIFY(m_dir.isValid());
    m_path = m_dir.filePath("stream-roundtrip.fif");

    m_rot = AngleAxisf(static_cast<float>(std::numbers::pi / 6.0), Vector3f::UnitZ()).toRotationMatrix();
    m_move = Vector3f(0.01f, 0.02f, 0.03f);

    {
        auto w = FiffStream::start_file(m_path.toStdU16String());
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
        named.row_names.push_back("R1");
        named.row_names.push_back("R2");
        named.col_names.push_back("C1");
        named.col_names.push_back("C2");
        named.col_names.push_back("C3");
        named.data.resize(2, 3);
        named.data << 0.5, -1.0, 2.0, 3.5, 0.0, -4.25;
        w->write_named_matrix(FIFF_MNE_CTF_COMP_DATA, named);

        std::vector<FiffProj> projs;
        for (int k = 0; k < 2; ++k) {
            FiffNamedMatrix vec;
            vec.nrow = 1;
            vec.ncol = 3;
            vec.col_names.push_back("MEG 0113");
            vec.col_names.push_back("MEG 0112");
            vec.col_names.push_back("MEG 0111");
            vec.data = RowVector3d(k == 0 ? 0.6 : 0.0, 0.8, k == 0 ? 0.0 : 0.6);
            projs.push_back(FiffProj(FIFFV_PROJ_ITEM_FIELD, k == 0, std::format("PCA-{}", k + 1), vec));
        }
        w->write_proj(projs);

        w->write_coord_trans(FiffCoordTrans(FIFFV_COORD_DEVICE, FIFFV_COORD_HEAD, m_rot, m_move));
        w->end_file();
    }

    m_stream = std::make_unique<FiffStream>(m_path.toStdU16String());
    QVERIFY(m_stream->open());
}

//=============================================================================================================

std::shared_ptr<FiffDirNode> TestFiffStreamPython::findBlock(int kind) const
{
    const std::vector<std::shared_ptr<FiffDirNode>> nodes = m_stream->dirtree()->dir_tree_find(kind);
    return nodes.empty() ? std::shared_ptr<FiffDirNode>() : nodes.front();
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
    const std::optional<FiffTag> tag = m_stream->dirtree()->find_tag(*m_stream, kind);
    QVERIFY(tag);
    QVERIFY(tag->is_matrix());
    const MatrixXd back = MatrixXd(tag->to_sparse_float_matrix());
    QCOMPARE(back.rows(), 3);
    QCOMPARE(back.cols(), 4);
    QVERIFY2((back - sparseReference().cast<double>()).cwiseAbs().maxCoeff() == 0.0,
             qPrintable(QStringLiteral("sparse round trip differs")));
}

//=============================================================================================================

void TestFiffStreamPython::intMatrix()
{
    const std::optional<FiffTag> tag = m_stream->dirtree()->find_tag(*m_stream, FIFF_MNE_SOURCE_SPACE_TRIANGLES);
    QVERIFY(tag);
    MatrixXi expected(2, 3);
    expected << 1, 2, 3, 4, 5, 6;
    // Like the float decoders, toIntMatrix returns the transpose of the
    // stored row-major matrix; callers transpose it back.
    QCOMPARE(MatrixXi(tag->to_int_matrix().transpose()), expected);
}

//=============================================================================================================

void TestFiffStreamPython::namedMatrix()
{
    QVERIFY(findBlock(FIFFB_MNE_NAMED_MATRIX));
    FiffNamedMatrix back;
    QVERIFY(m_stream->read_named_matrix(m_stream->dirtree(), FIFF_MNE_CTF_COMP_DATA, back));
    QCOMPARE(back.nrow, 2);
    QCOMPARE(back.ncol, 3);
    QCOMPARE(back.row_names, StringList({"R1", "R2"}));
    QCOMPARE(back.col_names, StringList({"C1", "C2", "C3"}));
    MatrixXd expected(2, 3);
    expected << 0.5, -1.0, 2.0, 3.5, 0.0, -4.25;
    QCOMPARE(back.data, expected);
}

//=============================================================================================================

void TestFiffStreamPython::projectors()
{
    const std::vector<FiffProj> back = m_stream->read_proj(m_stream->dirtree());
    QCOMPARE(back.size(), 2);
    QCOMPARE(back[0].desc, QString("PCA-1"));
    QCOMPARE(back[1].desc, QString("PCA-2"));
    QCOMPARE(back[0].active, true);
    QCOMPARE(back[1].active, false);
    QCOMPARE(back[0].kind, static_cast<fiff_int_t>(FIFFV_PROJ_ITEM_FIELD));
    // Channel names are read without spaces (FIFF convention in MNE-CPP).
    QCOMPARE(back[0].data.col_names, StringList({"MEG 0113", "MEG 0112", "MEG 0111"}));
    QVERIFY((back[0].data.data - RowVector3d(0.6, 0.8, 0.0)).cwiseAbs().maxCoeff() < 1e-7);
    QVERIFY((back[1].data.data - RowVector3d(0.0, 0.8, 0.6)).cwiseAbs().maxCoeff() < 1e-7);
}

//=============================================================================================================

void TestFiffStreamPython::coordTrans()
{
    const std::optional<FiffTag> tag = m_stream->dirtree()->find_tag(*m_stream, FIFF_COORD_TRANS);
    QVERIFY(tag);
    const FiffCoordTrans t = tag->to_coord_trans();
    QCOMPARE(t.from, static_cast<fiff_int_t>(FIFFV_COORD_DEVICE));
    QCOMPARE(t.to, static_cast<fiff_int_t>(FIFFV_COORD_HEAD));
    QVERIFY((t.trans.block<3, 3>(0, 0) - m_rot).cwiseAbs().maxCoeff() < 1e-7f);
    QVERIFY((t.trans.block<3, 1>(0, 3) - m_move).cwiseAbs().maxCoeff() < 1e-7f);
    // The stored inverse must undo the transform.
    QVERIFY((t.trans * t.invtrans - Matrix4f::Identity()).cwiseAbs().maxCoeff() < 1e-6f);

    // A truncated struct payload must yield the default value, not read past the tag data.
    FiffTag truncated;
    truncated.assign(tag->bytes().first(8));
    truncated.type = FIFFT_COORD_TRANS_STRUCT;
    QCOMPARE(truncated.to_coord_trans().from, FiffCoordTrans().from);
    truncated.type = FIFFT_DIG_POINT_STRUCT;
    QCOMPARE(truncated.to_dig_point().kind, FiffDigPoint().kind);
    truncated.type = FIFFT_ID_STRUCT;
    QCOMPARE(truncated.to_fiff_id().version, FiffId().version);
    truncated.type = FIFFT_CH_INFO_STRUCT;
    QCOMPARE(truncated.to_ch_info().ch_name, FiffChInfo().ch_name);
}

//=============================================================================================================

void TestFiffStreamPython::copiesProcessingHistory()
{
    // data/sss_history_raw.fif (make_proc_history_fixture.py) carries three MaxFilter
    // runs; mne-python reads 10th-order SSS info for the first two and cross-talk
    // compensation (5 entries) for the second.
    const QString src = QStringLiteral(MNE_FIFF_STREAM_DATA_DIR "/sss_history_raw.fif");
    auto historyOf = [](const QString& path, std::vector<std::shared_ptr<FiffDirNode>>& runs) {
        FiffStream stream(path.toStdU16String());
        if (!stream.open())
            return false;
        const std::vector<std::shared_ptr<FiffDirNode>> hist = stream.dirtree()->dir_tree_find(FIFFB_PROCESSING_HISTORY);
        if (hist.empty())
            return false;
        runs = hist[0]->dir_tree_find(FIFFB_PROCESSING_RECORD);
        stream.close();
        return true;
    };
    std::vector<std::shared_ptr<FiffDirNode>> srcRuns;
    QVERIFY(historyOf(src, srcRuns));
    QCOMPARE(srcRuns.size(), 3);

    const QString dst = m_dir.filePath("history-copy.fif");
    {
        auto w = FiffStream::start_file(dst.toStdU16String());
        QVERIFY(w);
        fiff_int_t nchan = 3;
        w->write_int(FIFF_NCHAN, &nchan);
        w->end_file();
    }
    QVERIFY(FiffStream::copy_processing_history(src.toStdU16String(), dst.toStdU16String()));

    std::vector<std::shared_ptr<FiffDirNode>> dstRuns;
    QVERIFY(historyOf(dst, dstRuns));
    QCOMPARE(dstRuns.size(), srcRuns.size());
    for (int k = 0; k < std::ssize(srcRuns); ++k) {
        QCOMPARE(dstRuns[k]->nchild(), srcRuns[k]->nchild());
        QCOMPARE(dstRuns[k]->dir_tree_find(FIFFB_SSS_INFO).size(), srcRuns[k]->dir_tree_find(FIFFB_SSS_INFO).size());
        QCOMPARE(dstRuns[k]->dir_tree_find(FIFFB_SSS_CAL_ADJUST).size(), srcRuns[k]->dir_tree_find(FIFFB_SSS_CAL_ADJUST).size());
        QCOMPARE(dstRuns[k]->dir_tree_find(FIFFB_CHANNEL_DECOUPLER).size(), srcRuns[k]->dir_tree_find(FIFFB_CHANNEL_DECOUPLER).size());
    }

    // Nothing is copied from a missing source or one without history, nor into a missing destination
    const QByteArray before = [&dst]() {
        QFile f(dst);
        return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
    }();
    QVERIFY(!FiffStream::copy_processing_history(m_dir.filePath("missing.fif").toStdU16String(), dst.toStdU16String()));
    const QString plain = m_dir.filePath("history-none.fif");
    {
        auto w = FiffStream::start_file(plain.toStdU16String());
        QVERIFY(w);
        fiff_int_t nchan = 1;
        w->write_int(FIFF_NCHAN, &nchan);
        w->end_file();
    }
    QVERIFY(!FiffStream::copy_processing_history(plain.toStdU16String(), dst.toStdU16String()));
    QVERIFY(!FiffStream::copy_processing_history(src.toStdU16String(), m_dir.filePath("missing-dst.fif").toStdU16String()));
    QVERIFY(!QFile::exists(m_dir.filePath("missing-dst.fif")));
    QFile after(dst);
    QVERIFY(after.open(QIODevice::ReadOnly));
    QCOMPARE(after.readAll(), before);
}

//=============================================================================================================

void TestFiffStreamPython::attachesEnvironment()
{
    const QString path = m_dir.filePath("env.fif");
    {
        auto w = FiffStream::start_file(path.toStdU16String());
        QVERIFY(w);
        w->start_block(FIFFB_MEAS);
        fiff_int_t nchan = 3;
        w->write_int(FIFF_NCHAN, &nchan);
        w->end_block(FIFFB_MEAS);
        w->end_file();
    }
    {
        auto u = FiffStream::open_update(path.toStdU16String());
        QVERIFY(u);
        QVERIFY(u->attach_env(QStringLiteral("/data/stüdy").toStdString(), QStringLiteral("mne_process --raw a.fif").toStdString()));
        u->close();
    }

    FiffStream r(path.toStdU16String());
    QVERIFY(r.open());
    const std::vector<std::shared_ptr<FiffDirNode>> env = r.dirtree()->dir_tree_find(FIFFB_MNE_ENV);
    QCOMPARE(env.size(), 1);
    const auto workingDir = env[0]->find_tag(r, FIFF_MNE_ENV_WORKING_DIR);
    QVERIFY(workingDir);
    // Strings are written as UTF-8: the tag holds the bytes, not the characters, of a non-ASCII path.
    QCOMPARE(workingDir->to_string(), QStringLiteral("/data/stüdy"));
    const auto commandLine = env[0]->find_tag(r, FIFF_MNE_ENV_COMMAND_LINE);
    QVERIFY(commandLine);
    QCOMPARE(commandLine->to_string(), QString("mne_process --raw a.fif"));
    // The measurement block written before is still there.
    const std::vector<std::shared_ptr<FiffDirNode>> meas = r.dirtree()->dir_tree_find(FIFFB_MEAS);
    QCOMPARE(meas.size(), 1);
    const auto nchan = meas[0]->find_tag(r, FIFF_NCHAN);
    QVERIFY(nchan);
    QCOMPARE(*nchan->to_int(), 3);
    r.close();
}

void TestFiffStreamPython::readsFloatCovariance()
{
    // data/float_cov.fif (make_float_cov_fixture.py): single-precision covariances as MNE-C writes them.
    FiffStream stream(QString(QStringLiteral(MNE_FIFF_STREAM_DATA_DIR "/float_cov.fif")).toStdU16String());
    QVERIFY(stream.open());

    FiffCov noise;
    QVERIFY(stream.read_cov(stream.dirtree(), FIFFV_MNE_NOISE_COV, noise));
    QCOMPARE(noise.dim, 3);
    QCOMPARE(noise.nfree, 42);
    QVERIFY(!noise.diag);
    QCOMPARE(noise.names, StringList({"A", "B", "C"}));
    Matrix3d full;
    full << 1.164402008, 0.134921417, 0.535389721,
        0.134921417, 2.983242273, -0.174825236,
        0.535389721, -0.174825236, 3.042061329;
    QVERIFY((noise.data - full).cwiseAbs().maxCoeff() < 1e-6);
    QVERIFY((noise.eig - Vector3d(1.006871979, 2.928603736, 3.254229896)).cwiseAbs().maxCoeff() < 1e-6);
    // Rows of eigvec are the eigenvectors, as in mne-python (whitener = diag(1/sqrt(eig)) * eigvec).
    QCOMPARE(noise.eigvec.rows(), 3);
    QVERIFY((noise.eigvec * full - noise.eig.asDiagonal() * noise.eigvec).cwiseAbs().maxCoeff() < 1e-5);
    QCOMPARE(noise.bads, StringList({"B"}));
    QCOMPARE(noise.projs.size(), 1);
    QCOMPARE(noise.projs[0].desc, QString("ECG-1"));
    QCOMPARE(noise.projs[0].kind, static_cast<fiff_int_t>(FIFFV_PROJ_ITEM_FIELD));
    QVERIFY(!noise.projs[0].active);
    QCOMPARE(noise.projs[0].data.col_names, StringList({"A", "B", "C"}));
    QVERIFY((noise.projs[0].data.data - RowVector3d(0.0, 0.6, 0.8)).cwiseAbs().maxCoeff() < 1e-7);

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
    FiffStream stream(QString(QStringLiteral(MNE_FIFF_STREAM_DATA_DIR "/meas_info.fif")).toStdU16String());
    QVERIFY(stream.open());
    FiffInfo info;
    std::shared_ptr<FiffDirNode> meas;
    QVERIFY(stream.read_meas_info(stream.dirtree(), info, meas));

    // The Isotrak's own FIFF_MNE_COORD_FRAME overrides the head-frame default.
    const FiffDigitizerData digData = stream.read_digitizer_data(stream.dirtree()).value();
    QCOMPARE(digData.coord_frame, FIFFV_COORD_MRI);
    QCOMPARE(digData.points.size(), std::size_t{3});
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
    FiffStream stream(QString(QStringLiteral(MNE_FIFF_STREAM_DATA_DIR "/tag_types.fif")).toStdU16String());
    QVERIFY(stream.open());
    auto tag = [&](int kind) {
        std::optional<FiffTag> t;
        t = stream.dirtree()->find_tag(stream, kind);
        return t;
    };

    std::optional<FiffTag> t = tag(901);
    QVERIFY(t);
    QCOMPARE(t->to_double()[0], 1.25);
    QCOMPARE(t->to_double()[1], -2.5e-12);
    QCOMPARE(t->to_double()[2], 3.0e200);
    t = tag(902);
    QCOMPARE(t->to_short()[0], qint16(-3));
    QCOMPARE(t->to_short()[2], qint16(-32000));
    t = tag(903);
    QCOMPARE(t->to_unsigned_short()[1], quint16(40000));
    QCOMPARE(t->to_unsigned_short()[2], quint16(65535));
    t = tag(904);
    QCOMPARE(*t->to_julian(), 2461318); // 2026-10-04
    t = tag(905);
    QCOMPARE(t->to_dau_pack16()[0], qint16(-7));
    QCOMPARE(t->to_dau_pack16()[1], qint16(12345));
    // Complex and matrix payloads have no typed accessor; read the swapped data directly.
    auto floats = [](const FiffTag& x) {
        return reinterpret_cast<const float*>(x.data());
    };
    t = tag(906);
    QCOMPARE(floats(*t)[0], 1.5f);
    QCOMPARE(floats(*t)[1], -2.0f);
    QCOMPARE(floats(*t)[3], 4.0f);

    // Dense matrices keep their trailing dimensions (ncol, nrow, ndim) in host order.
    qint32 ndim;
    std::vector<qint32> dims;
    t = tag(907);
    QVERIFY(t->matrix_dimensions(ndim, dims));
    QCOMPARE(dims, std::vector<qint32>({3, 2}));
    QCOMPARE(reinterpret_cast<const double*>(t->data())[5], 6.5);
    t = tag(908);
    QVERIFY(t->matrix_dimensions(ndim, dims));
    QCOMPARE(dims, std::vector<qint32>({2, 1}));
    QCOMPARE(floats(*t)[3], -4.0f);

    MatrixXd expected(4, 4);
    expected << 1, 0, 2, 0, 0, 0, 3, 4, 5, 0, 0, 6, 0, 7, 0, 0;
    for (int kind : {909, 910}) {
        t = tag(kind);
        QVERIFY(t);
        QCOMPARE(MatrixXd(t->to_sparse_float_matrix()), expected);
    }

    // getInfo names the type of each tag as make_tag_types_fixture.py wrote it.
    const QList<QPair<int, QString>> infos{{901, "Simple type FIFFT_DOUBLE"}, {902, "Simple type FIFFT_SHORT"}, {903, "Simple type FIFFT_USHORT"}, {904, "Simple type FIFFT_JULIAN"}, {905, "Simple type FIFFT_DAU_PACK16"}, {906, "Simple type FIFFT_COMPLEX_FLOAT"}, {907, "Matrix of type FIFFT_DOUBLE"}, {908, "Matrix of type FIFFT_COMPLEX_FLOAT"}, {909, "Matrix of type FIFFT_FLOAT"}};
    for (const auto& [kind, info] : infos)
        QCOMPARE(tag(kind)->info(), info);
    FiffTag unknown;
    unknown.type = 9999;
    QCOMPARE(unknown.info(), QString("Structure unknown"));

    // write_tag must turn every payload back into file byte order.
    const QString copyPath = m_dir.filePath("tag-types-copy.fif");
    {
        auto w = FiffStream::start_file(copyPath.toStdU16String());
        QVERIFY(w);
        for (int kind = 901; kind <= 910; ++kind)
            w->write_tag(*tag(kind));
        w->end_file();
    }
    FiffStream copy(copyPath.toStdU16String());
    QVERIFY(copy.open());
    for (int kind = 901; kind <= 910; ++kind) {
        const std::optional<FiffTag> back = copy.dirtree()->find_tag(copy, kind);
        QVERIFY(back);
        const std::optional<FiffTag> orig = tag(kind);
        QVERIFY2(back->size() == orig->size() && std::memcmp(back->data(), orig->data(), static_cast<size_t>(orig->size())) == 0,
                 qPrintable(QStringLiteral("tag %1 changed in write_tag").arg(kind)));
    }
    copy.close();
    stream.close();
}

//=============================================================================================================

void TestFiffStreamPython::printsDirectoryTree()
{
    // mne-python fiff_open on test_mne_ctf_comp_python/data/ctf_grade0_raw.fif: blocks depth-first (its root 0 is
    // MNE-CPP's FIFFB_ROOT), with a run of 36 FIFF_CH_INFO (203) tags in the meas info block.
    FiffStream stream(QString(QStringLiteral(MNE_CTF_COMP_DATA_DIR "/ctf_grade0_raw.fif")).toStdU16String());
    QVERIFY(stream.open());

    std::ostringstream out;
    stream.dirtree()->print(out);
    const QString text = QString::fromStdString(out.str());

    // Every line is whole: a block with its id, one run of equal tag kinds, or a closing brace (MNE-C fiff_dir_tree)
    const QRegularExpression blockLine(QStringLiteral(R"(^( *)(\d+) = (.+?) +\{ (\t\d+\.\d+ 0x[0-9a-f]+ -?\d+ -?\d+ )?$)"));
    const QRegularExpression tagLine(QStringLiteral(R"(^( *)(\d+) = (.+?) *( \[(\d+)\])?$)"));
    const QList<int> blocks{FIFFB_ROOT, 100, 101, 109, 107, 106, 370, 371, 357, 371, 357, 371, 357, 371, 357, 371, 357, 102};
    QList<int> printedBlocks;
    int printedTags = 0;
    int depth = 0;
    const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const QRegularExpressionMatch block = blockLine.match(line);
        const QRegularExpressionMatch tag = tagLine.match(line);
        if (block.hasMatch()) {
            QCOMPARE(block.captured(1).size(), 5 * depth++);
            QCOMPARE(block.captured(3), FiffExplain::block(block.captured(2).toInt()));
            printedBlocks << block.captured(2).toInt();
        } else if (tag.hasMatch()) {
            QCOMPARE(tag.captured(1).size(), 5 * (depth - 1) + 2);
            QCOMPARE(tag.captured(3), FiffExplain::tag(tag.captured(2).toInt()));
            ++printedTags;
        } else {
            QCOMPARE(line, QString(5 * --depth, QLatin1Char(' ')) + QLatin1Char('}'));
        }
    }
    QCOMPARE(depth, 0);
    QCOMPARE(printedBlocks, blocks);
    QVERIFY(lines.filter(QRegularExpression(QStringLiteral(R"(^ +203 = channel +\[36\]$)"))).size() == 1);
    // One line per run of equal tag kinds: mne-python's tree has 50; the root's FIFF_FILE_ID entry is its id here.
    QCOMPARE(printedTags, 49);

    QCOMPARE(FiffExplain::tag(FIFF_CH_INFO), QStringLiteral("channel"));
    QCOMPARE(FiffExplain::block(FIFFB_XFIT_AUX), QStringLiteral("xfit aux"));
    QCOMPARE(FiffExplain::tag(FIFF_CH_POS), QStringLiteral("channel pos"));
    QCOMPARE(FiffExplain::tag(-5), QString());
    QCOMPARE(FiffExplain::unit(FIFF_UNIT_T), QStringLiteral("T"));
    QCOMPARE(FiffExplain::unit(FIFF_UNIT_T_M), QStringLiteral("T/m"));
    QCOMPARE(FiffExplain::unit(FIFF_UNIT_NONE), QString());
    QCOMPARE(FiffExplain::unitMultiplier(FIFF_UNITM_F), QStringLiteral("f"));
    QCOMPARE(FiffExplain::channelKind(FIFFV_EEG_CH), QStringLiteral("EEG"));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

void TestFiffStreamPython::refusesUnwritablePath()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("no/such/dir/out.fif"));
    FiffInfo info;
    FiffChInfo ch;
    ch.ch_name = "EEG 001";
    ch.kind = FIFFV_EEG_CH;
    info.chs.push_back(ch);
    info.ch_names.push_back(ch.ch_name);
    info.nchan = 1;
    info.sfreq = 100.0f;
    RowVectorXd cals;
    QVERIFY(!FiffStream::start_writing_raw(path.toStdString(), info, cals));
    QVERIFY(!FiffCoordTrans().write(path.toStdString()));
    QVERIFY(!FiffDigPointSet().write(path.toStdString()));
    QVERIFY(!QFileInfo::exists(path));

    // The channel list, not the nchan field (-1 when never set), decides what is written.
    // Without a source file, digitizer points, acquisition parameters and the CTF head transform come from info
    // (mne.io.read_raw_fif reads the same three points, both strings and the 4 mm CTF shift).
    info.nchan = -1;
    for (int k = 0; k < 3; ++k) {
        FiffDigPoint point;
        point.kind = FIFFV_POINT_CARDINAL;
        point.ident = k + 1;
        point.coord_frame = FIFFV_COORD_HEAD;
        point.r[0] = 0.01f * (k + 1);
        point.r[1] = point.r[2] = 0.0f;
        info.dig.push_back(point);
    }
    info.acq_pars = "ACQch001 110113";
    info.acq_stim = "STIM 1";
    Matrix4f ctf = Matrix4f::Identity();
    ctf(0, 3) = 0.004f;
    info.ctf_head_t = FiffCoordTrans(FIFFV_MNE_COORD_CTF_HEAD, FIFFV_COORD_HEAD, ctf);
    QFile written(dir.filePath(QStringLiteral("one_channel_raw.fif")));
    auto stream = FiffStream::start_writing_raw(dir.filePath(QStringLiteral("one_channel_raw.fif")).toStdString(), info, cals);
    QVERIFY(stream);
    QCOMPARE(cals.size(), Eigen::Index(1));
    stream->write_raw_buffer(MatrixXd::Constant(1, 10, 2e-6), cals);
    stream->finish_writing_raw();
    FiffRawData raw1(written.fileName().toStdString());
    QCOMPARE(raw1.info.nchan, 1);
    QCOMPARE(raw1.last_samp, 9);
    QCOMPARE(raw1.info.dig.size(), 3);
    QCOMPARE(raw1.info.dig[2].ident, 3);
    QCOMPARE(raw1.info.dig[2].r[0], 0.03f);
    QCOMPARE(raw1.info.acq_pars, info.acq_pars);
    QCOMPARE(raw1.info.acq_stim, info.acq_stim);
    QCOMPARE(raw1.info.ctf_head_t.trans, ctf);

    // A source file that has gone away only loses the blocks copied from it, not the measurement info.
    info.filename = (dir.filePath(QStringLiteral("deleted_raw.fif"))).toStdString();
    QFile orphan(dir.filePath(QStringLiteral("orphan_raw.fif")));
    auto orphanStream = FiffStream::start_writing_raw(dir.filePath(QStringLiteral("orphan_raw.fif")).toStdString(), info, cals);
    QVERIFY(orphanStream);
    orphanStream->write_raw_buffer(MatrixXd::Constant(1, 10, 2e-6), cals);
    orphanStream->finish_writing_raw();
    FiffRawData raw2(orphan.fileName().toStdString());
    QCOMPARE(raw2.info.nchan, 1);
    QCOMPARE(raw2.last_samp, 9);
    QCOMPARE(raw2.info.dig.size(), 3);
}

//=============================================================================================================

void TestFiffStreamPython::writesAnnotationsAndDevice()
{
    // mne.io.read_raw_fif reads this file with device_info {type: synthetic, model: Oddball 8, serial: seed=3} and
    // annotations onset [1.5, 2.25, 3.0], duration [0.0, 0.5, 0.25], description
    // ['Stimulus 1', 'a:b', ''], ch_names [(), ('EEG 001',), ()], meas_date 2026-11-03 10:00:00.250000 UTC.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    FiffInfo info;
    FiffChInfo ch;
    ch.ch_name = "EEG 001";
    ch.kind = FIFFV_EEG_CH;
    ch.unit = FIFF_UNIT_V;
    ch.cal = 1.0f;
    ch.range = 1.0f;
    info.chs.push_back(ch);
    info.ch_names.push_back(ch.ch_name);
    info.nchan = 1;
    info.sfreq = 100.0f;
    info.linefreq = 50.0f;
    info.meas_date[0] = 1793700000;
    info.meas_date[1] = 250000;
    info.device_info.type = "synthetic";
    info.device_info.model = "Oddball 8";
    info.device_info.serial = "seed=3";

    FiffAnnotations annotations;
    annotations.append(1.5, 0.0, "Stimulus 1");
    annotations.append(2.25, 0.5, "a:b", {"EEG 001"});
    annotations.append(3.0, 0.25, "");

    const QString path = dir.filePath(QStringLiteral("annotated_raw.fif"));
    RowVectorXd cals;
    auto stream = FiffStream::start_writing_raw(path.toStdString(), info, cals);
    QVERIFY(stream);
    stream->write_raw_buffer(MatrixXd::Constant(1, 400, 1e-6), cals);
    stream->end_block(FIFFB_RAW_DATA);
    stream->write_annotations(annotations, info.meas_date);
    stream->end_block(FIFFB_MEAS);
    stream->end_file();
    stream->close();

    FiffRawData raw(path.toStdString());
    QCOMPARE(raw.info.device_info.type, info.device_info.type);
    QCOMPARE(raw.info.device_info.model, info.device_info.model);
    QCOMPARE(raw.info.device_info.serial, info.device_info.serial);
    QVERIFY(raw.info.device_info.site.empty());
    QCOMPARE(raw.info.meas_date[1], 250000);

    FiffStream reader(path.toStdU16String());
    QVERIFY(reader.open());
    FiffAnnotations back;
    fiff_int_t origTime[2] = {0, 0};
    QVERIFY(reader.read_annotations(reader.dirtree(), back, origTime));
    QCOMPARE(origTime[0], info.meas_date[0]);
    QCOMPARE(origTime[1], info.meas_date[1]);
    QCOMPARE(back.size(), std::size_t{3});
    QCOMPARE(back[0].description, std::string("Stimulus 1"));
    QCOMPARE(back[1].description, std::string("a:b"));
    QVERIFY(back[2].description.empty());
    QCOMPARE(back[1].onset, 2.25);
    QCOMPARE(back[1].duration, 0.5);
    QVERIFY(back[0].ch_names.empty());
    QCOMPARE(back[1].ch_names, std::vector<std::string>{"EEG 001"});

    // Without devices and orig_time nothing of either is written.
    info.device_info = FiffDeviceInfo();
    FiffStream plain(std::make_unique<mne::MemoryByteStream>());
    QCOMPARE(plain.write_device_info(info.device_info), fiff_long_t(-1));
    QCOMPARE(plain.byte_stream().size(), std::uint64_t(0));

    // Writers of native samples declare their packing. An unknown line frequency is not written (mne-python's
    // info["line_freq"] stays None instead of reading -1).
    info.linefreq = -1.0f;
    QFile packedFile(dir.filePath(QStringLiteral("packed_raw.fif")));
    auto packed = FiffStream::start_writing_raw(dir.filePath(QStringLiteral("packed_raw.fif")).toStdString(), info, cals, defaultMatrixXi, false,
                                                FIFFT_DAU_PACK16);
    QVERIFY(packed);
    packed->finish_writing_raw();
    FiffStream packedReader(packedFile.fileName().toStdU16String());
    QVERIFY(packedReader.open());
    const std::optional<FiffTag> pack = packedReader.dirtree()->dir_tree_find(FIFFB_MEAS_INFO).front()->find_tag(packedReader, FIFF_DATA_PACK);
    QVERIFY(pack);
    QCOMPARE(*pack->to_int(), FIFFT_DAU_PACK16);
    const std::optional<FiffTag> lineFreq = packedReader.dirtree()->dir_tree_find(FIFFB_MEAS_INFO).front()->find_tag(packedReader, FIFF_LINE_FREQ);
    QVERIFY(!lineFreq);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffStreamPython)
#include "test_fiff_stream_python.moc"
