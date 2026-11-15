//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fiff_coverage.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     June, 2026
 * @brief    Tests for untested FIFF library methods.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <memory>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_info_base.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_digitizer_data.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_ctf_comp.h>
#include <fiff/fiff_cov.h>
#include <base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/LU>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================

class TestFiffCoverage : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // --- FiffStream: get_evoked_entries ---
    void testGetEvokedEntries();
    void testGetEvokedEntriesCount();

    // --- FiffStream: read_digitizer_data ---
    void testReadDigitizerData();
    void testReadDigitizerDataPoints();

    // --- FiffStream: read_meas_info_base ---
    void testReadMeasInfoBase();
    void testReadMeasInfoBaseChannels();

    // --- FiffStream: write_evoked_set + read round-trip ---
    void testWriteReadEvokedSetRoundTrip();
    void testWriteReadEvokedSetPreserveData();

    // --- FiffStream: write_bad_channels ---
    void testWriteBadChannels();

    // --- FiffStream: open_update + attach_env ---
    void testOpenUpdateAttachEnv();

    // --- FiffStream: read_named_matrix ---
    void testReadNamedMatrix();

    // --- FiffStream: read_ctf_comp + write_ctf_comp round-trip ---
    void testCtfCompRoundTrip();

    // --- FiffCoordTrans ---
    void testReadMeasTransform();
    void testCoordTransInverse();
    void testCoordTransApply();

    // --- FiffNamedMatrix ---
    void testNamedMatrixConstruction();
    void testNamedMatrixTranspose();

    // --- FiffCov ---
    void testFiffCovFromFile();
    void testFiffCovDimensions();

    void cleanupTestCase();

private:
    QString rawPath() const
    {
        return m_sTestDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
    }
    QString avePath() const
    {
        return m_sTestDataPath + "/MEG/sample/sample_audvis-ave.fif";
    }
    QString covPath() const
    {
        return m_sTestDataPath + "/MEG/sample/sample_audvis-cov.fif";
    }
    QString fwdPath() const
    {
        return m_sTestDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
    }

    QString m_sTestDataPath;
    QTemporaryDir m_tmpDir;
};

//=============================================================================================================

void TestFiffCoverage::initTestCase()
{
    mne::Log::install_qt_message_handler();
    m_sTestDataPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data";
    QVERIFY2(QFile::exists(rawPath()),
             qPrintable(QString("Test data not found: %1").arg(rawPath())));
    QVERIFY(m_tmpDir.isValid());
}

//=============================================================================================================
// get_evoked_entries
//=============================================================================================================

void TestFiffCoverage::testGetEvokedEntries()
{
    QVERIFY2(QFile::exists(avePath()),
             qPrintable(QString("Evoked test data not found: %1").arg(avePath())));

    FiffStream stream(avePath().toStdU16String());
    QVERIFY(stream.open());

    // Find evoked nodes in the directory tree
    std::vector<std::shared_ptr<FiffDirNode>> evoked_nodes = stream.dirtree()->dir_tree_find(FIFFB_EVOKED);
    mne::StringList comments;
    std::vector<fiff_int_t> aspect_kinds;
    std::string t;
    QVERIFY(stream.get_evoked_entries(evoked_nodes, comments, aspect_kinds, t));

    stream.close();
    QVERIFY(comments.size() > 0);
    QCOMPARE(comments.size(), aspect_kinds.size());
}

void TestFiffCoverage::testGetEvokedEntriesCount()
{
    QVERIFY2(QFile::exists(avePath()),
             qPrintable(QString("Evoked test data not found: %1").arg(avePath())));

    QFile file(avePath());
    FiffStream stream(avePath().toStdU16String());
    QVERIFY(stream.open());

    std::vector<std::shared_ptr<FiffDirNode>> evoked_nodes = stream.dirtree()->dir_tree_find(FIFFB_EVOKED);
    mne::StringList comments;
    std::vector<fiff_int_t> aspect_kinds;
    std::string t;
    QVERIFY(stream.get_evoked_entries(evoked_nodes, comments, aspect_kinds, t));
    stream.close();

    // The sample evoked file should have at least 1 entry
    QVERIFY(comments.size() >= 1);
    // Aspect kinds should be valid (typically FIFFV_ASPECT_AVERAGE=100)
    for (const auto& aspect_kind : aspect_kinds) {
        QVERIFY(aspect_kind >= 0);
    }
}

//=============================================================================================================
// read_digitizer_data
//=============================================================================================================

void TestFiffCoverage::testReadDigitizerData()
{
    FiffStream stream(rawPath().toStdU16String());
    QVERIFY(stream.open());

    const auto digData = stream.read_digitizer_data(stream.dirtree());
    stream.close();

    QVERIFY(digData.has_value());
    QCOMPARE(digData->points.size(), std::size_t{146});
    QCOMPARE(digData->active.size(), digData->points.size());
}

void TestFiffCoverage::testReadDigitizerDataPoints()
{
    FiffRawData raw(rawPath().toStdString());

    // Check that digitizer points from the raw info are accessible
    QVERIFY(raw.info.dig.size() > 0);
    // At least fiducials (3 cardinal + HPI coils)
    int nCardinal = 0;
    for (const FiffDigPoint& p : raw.info.dig) {
        if (p.kind == FIFFV_POINT_CARDINAL)
            nCardinal++;
    }
    QVERIFY(nCardinal >= 3); // LPA, nasion, RPA
}

//=============================================================================================================
// read_meas_info_base
//=============================================================================================================

void TestFiffCoverage::testReadMeasInfoBase()
{
    // read_meas_info_base requires FIFFB_MNE_PARENT_MEAS_FILE (found in fwd files, not raw)
    QVERIFY2(QFile::exists(fwdPath()),
             qPrintable(QString("Forward solution not found: %1").arg(fwdPath())));

    FiffStream stream(fwdPath().toStdU16String());
    QVERIFY(stream.open());

    FiffInfoBase infoBase;
    bool ok = stream.read_meas_info_base(stream.dirtree(), infoBase);
    stream.close();

    QVERIFY(ok);
    QVERIFY(infoBase.nchan > 0);
}

void TestFiffCoverage::testReadMeasInfoBaseChannels()
{
    QVERIFY2(QFile::exists(fwdPath()),
             qPrintable(QString("Forward solution not found: %1").arg(fwdPath())));

    FiffStream stream(fwdPath().toStdU16String());
    QVERIFY(stream.open());

    FiffInfoBase infoBase;
    QVERIFY(stream.read_meas_info_base(stream.dirtree(), infoBase));
    stream.close();

    // Should have channel info
    QCOMPARE(infoBase.chs.size(), infoBase.nchan);
    QVERIFY(infoBase.nchan > 300); // Sample data has 376 channels
}

//=============================================================================================================
// write_evoked_set + read round-trip
//=============================================================================================================

void TestFiffCoverage::testWriteReadEvokedSetRoundTrip()
{
    QVERIFY2(QFile::exists(avePath()),
             qPrintable(QString("Evoked test data not found: %1").arg(avePath())));

    // Read original
    FiffEvokedSet origSet(avePath().toStdU16String());
    QVERIFY(origSet.evoked.size() > 0);

    // Write to temp file using save() which handles open/write/close
    QString tmpPath = m_tmpDir.path() + "/evoked_roundtrip.fif";
    QVERIFY(origSet.save(tmpPath.toStdU16String()));

    // Read back
    FiffEvokedSet readSet(tmpPath.toStdU16String());

    QCOMPARE(readSet.evoked.size(), origSet.evoked.size());
}

void TestFiffCoverage::testWriteReadEvokedSetPreserveData()
{
    QVERIFY2(QFile::exists(avePath()),
             qPrintable(QString("Evoked test data not found: %1").arg(avePath())));

    FiffEvokedSet origSet(avePath().toStdU16String());

    // Write and read back using save()
    QString tmpPath = m_tmpDir.path() + "/evoked_preserve.fif";
    QVERIFY(origSet.save(tmpPath.toStdU16String()));

    FiffEvokedSet readSet(tmpPath.toStdU16String());

    // Check data dimensions match
    for (std::size_t i = 0; i < std::min(origSet.evoked.size(), readSet.evoked.size()); i++) {
        QCOMPARE(readSet.evoked[i].data.rows(), origSet.evoked[i].data.rows());
        QCOMPARE(readSet.evoked[i].data.cols(), origSet.evoked[i].data.cols());
    }
}

//=============================================================================================================
// write_bad_channels
//=============================================================================================================

void TestFiffCoverage::testWriteBadChannels()
{
    // Write bad channels to a FIFF file and verify
    QString tmpPath = m_tmpDir.path() + "/bads.fif";
    {
        auto stream = FiffStream::start_file(tmpPath.toStdU16String());
        QVERIFY(stream != nullptr);

        StringList bads;
        bads.push_back("MEG 0111");
        bads.push_back("MEG 0113");
        bads.push_back("MEG 0121");
        stream->write_bad_channels(bads);
        stream->end_file();
        stream->close(); // flush to disk
    }

    // Verify file was created and is non-empty
    QVERIFY(QFile::exists(tmpPath));
    QVERIFY2(QFileInfo(tmpPath).size() > 0,
             "Written FIFF file is empty - stream not flushed");
}

//=============================================================================================================
// open_update + attach_env
//=============================================================================================================

void TestFiffCoverage::testOpenUpdateAttachEnv()
{
    // Create a minimal FIFF file first
    QString tmpPath = m_tmpDir.path() + "/update_test.fif";
    {
        auto s = FiffStream::start_file(tmpPath.toStdU16String());
        QVERIFY(s != nullptr);
        fiff_int_t val = 42;
        s->write_int(FIFF_NCHAN, &val);
        s->end_file();
        s->close(); // flush to disk
    }

    // Open for update
    auto stream = FiffStream::open_update(tmpPath.toStdU16String());
    QVERIFY2(stream != nullptr, "open_update returned null");
    // Should be able to read the file
    QVERIFY(stream->nent() > 0);
    stream->close();
}

//=============================================================================================================
// read_named_matrix
//=============================================================================================================

void TestFiffCoverage::testReadNamedMatrix()
{
    QVERIFY2(QFile::exists(fwdPath()),
             qPrintable(QString("Forward solution not found: %1").arg(fwdPath())));

    QFile file(fwdPath());
    FiffStream stream(fwdPath().toStdU16String());
    QVERIFY(stream.open());

    // The forward solution file should contain named matrices
    // Search for a named matrix in the directory tree
    std::shared_ptr<FiffDirNode> megNode;
    std::vector<std::shared_ptr<FiffDirNode>> nodes = stream.dirtree()->dir_tree_find(FIFFB_MNE_FORWARD_SOLUTION);
    if (!nodes.empty()) {
        megNode = nodes[0];
        FiffNamedMatrix namedMat;
        bool ok = stream.read_named_matrix(megNode, FIFF_MNE_FORWARD_SOLUTION, namedMat);
        if (ok) {
            QVERIFY(namedMat.nrow > 0);
            QVERIFY(namedMat.ncol > 0);
            QCOMPARE(namedMat.data.rows(), (Eigen::Index)namedMat.nrow);
            QCOMPARE(namedMat.data.cols(), (Eigen::Index)namedMat.ncol);
        }
    }

    stream.close();
}

//=============================================================================================================
// CTF compensation round-trip
//=============================================================================================================

void TestFiffCoverage::testCtfCompRoundTrip()
{
    // Read CTF comp data from raw file (if present)
    FiffStream stream(rawPath().toStdU16String());
    QVERIFY(stream.open());

    std::vector<FiffCtfComp> comps;
    FiffInfo info;
    std::shared_ptr<FiffDirNode> nodeInfo;
    stream.read_meas_info(stream.dirtree(), info, nodeInfo);
    comps = info.comps;
    stream.close();

    if (comps.empty()) {
        // Create synthetic CTF comp data
        FiffCtfComp comp;
        comp.ctfkind = 101;
        comp.kind = 101;
        comp.save_calibrated = false;
        FiffNamedMatrix& data = comp.data;
        data.nrow = 2;
        data.ncol = 3;
        data.data = MatrixXd::Ones(2, 3);
        data.row_names.push_back("MEG 0111");
        data.row_names.push_back("MEG 0121");
        data.col_names.push_back("REF01");
        data.col_names.push_back("REF02");
        data.col_names.push_back("REF03");
        comps.push_back(comp);
    }

    // Write to temp file
    QString tmpPath = m_tmpDir.path() + "/ctf_comp.fif";
    {
        auto outStream = FiffStream::start_file(tmpPath.toStdU16String());
        QVERIFY(outStream != nullptr);
        outStream->write_ctf_comp(comps);
        outStream->end_file();
        outStream->close(); // flush to disk
    }

    // Verify file was created
    QVERIFY(QFile::exists(tmpPath));
    QVERIFY2(QFileInfo(tmpPath).size() > 0,
             "Written FIFF file is empty - stream not flushed");

    // Read compensators are calibrated in memory and written back uncalibrated (mne _write_ctf_comp),
    // so a read-write-read round trip returns the same matrix
    std::vector<FiffChInfo> chs;
    for (const auto& [name, cal] : {std::pair{"MEG 0111", 2.0f}, std::pair{"MEG 0121", 4.0f}, std::pair{"REF01", 0.5f}, std::pair{"REF02", 8.0f}, std::pair{"REF03", 1.0f}}) {
        FiffChInfo ch;
        ch.ch_name = name;
        ch.range = 1.0f;
        ch.cal = cal;
        chs.push_back(ch);
    }
    FiffCtfComp raw;
    raw.ctfkind = 1194410578;
    raw.save_calibrated = false;
    raw.data = FiffNamedMatrix(2, 3, {"MEG 0111", "MEG 0121"}, {"REF01", "REF02", "REF03"}, MatrixXd::Constant(2, 3, 1.0));
    const auto writeComps = [](const QString& path, const std::vector<FiffCtfComp>& list) {
        auto w = FiffStream::start_file(path.toStdU16String());
        w->write_ctf_comp(list);
        w->end_file();
        w->close();
    };
    const auto readComps = [&chs](const QString& path) {
        FiffStream r(path.toStdU16String());
        std::vector<FiffCtfComp> read;
        if (r.open())
            read = r.read_ctf_comp(r.dirtree(), chs);
        return read;
    };
    const QString firstPath = m_tmpDir.path() + "/ctf_comp_raw.fif";
    writeComps(firstPath, {raw});
    const std::vector<FiffCtfComp> first = readComps(firstPath);
    QCOMPARE(first.size(), 1);
    QCOMPARE(first[0].kind, 1);
    MatrixXd calibrated(2, 3);
    calibrated << 2.0 / 0.5, 2.0 / 8.0, 2.0, 4.0 / 0.5, 4.0 / 8.0, 4.0;
    QVERIFY(first[0].data.data.isApprox(calibrated));
    const QString secondPath = m_tmpDir.path() + "/ctf_comp_again.fif";
    writeComps(secondPath, first);
    const std::vector<FiffCtfComp> second = readComps(secondPath);
    QCOMPARE(second.size(), 1);
    QVERIFY2(second[0].data.data.isApprox(calibrated), qPrintable(QString::number(second[0].data.data(0, 0))));

    // A compensator naming a channel the data lack makes the set unusable (mne raises), not half-read
    FiffCtfComp unknown = raw;
    unknown.ctfkind = 1194476114;
    unknown.data = FiffNamedMatrix(1, 1, {"MEG 0111"}, {"REF99"}, MatrixXd::Ones(1, 1));
    const QString brokenPath = m_tmpDir.path() + "/ctf_comp_unknown.fif";
    writeComps(brokenPath, {raw, unknown});
    QVERIFY(readComps(brokenPath).empty());
}

//=============================================================================================================
// FiffCoordTrans
//=============================================================================================================

void TestFiffCoverage::testReadMeasTransform()
{
    const auto read = FiffCoordTrans::read(rawPath().toStdString(), FIFFV_COORD_DEVICE, FIFFV_COORD_HEAD);
    QVERIFY(read);
    const FiffCoordTrans& t = *read;
    {
        QCOMPARE(t.from, (int)FIFFV_COORD_DEVICE);
        QCOMPARE(t.to, (int)FIFFV_COORD_HEAD);
        // Transform should be non-identity (device is not at head position)
        QVERIFY(!t.trans.isIdentity());
    }
}

void TestFiffCoverage::testCoordTransInverse()
{
    // Create a known transform and verify its inverse
    FiffCoordTrans t;
    t.from = FIFFV_COORD_MRI;
    t.to = FIFFV_COORD_HEAD;
    t.trans = Matrix4f::Identity();
    t.trans(0, 3) = 0.1f; // 100mm translation on X
    t.trans(1, 3) = 0.05f;
    // inverted() uses the invtrans field, so compute and set it
    t.invtrans = t.trans.inverse();

    FiffCoordTrans ti = t.inverted();
    QCOMPARE(ti.from, (int)FIFFV_COORD_HEAD);
    QCOMPARE(ti.to, (int)FIFFV_COORD_MRI);

    // T * T^-1 should be identity
    Matrix4f product = t.trans * ti.trans;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            QVERIFY(qAbs(product(i, j) - (i == j ? 1.0f : 0.0f)) < 1e-5f);
}

void TestFiffCoverage::testCoordTransApply()
{
    FiffCoordTrans t;
    t.from = FIFFV_COORD_MRI;
    t.to = FIFFV_COORD_HEAD;
    t.trans = Matrix4f::Identity();
    t.trans(0, 3) = 0.1f;

    // Apply to a point using apply_trans
    MatrixX3f point(1, 3);
    point << 0.0f, 0.0f, 0.0f;
    MatrixX3f transformed = t.apply_trans(point, true); // forward
    QVERIFY(qAbs(transformed(0, 0) - 0.1f) < 1e-5f);
    QVERIFY(qAbs(transformed(0, 1) - 0.0f) < 1e-5f);
    QVERIFY(qAbs(transformed(0, 2) - 0.0f) < 1e-5f);
}

//=============================================================================================================
// FiffNamedMatrix
//=============================================================================================================

void TestFiffCoverage::testNamedMatrixConstruction()
{
    StringList rowNames{"row1", "row2"};
    StringList colNames{"col1", "col2", "col3"};
    MatrixXd data = MatrixXd::Random(2, 3);

    FiffNamedMatrix nm(2, 3, rowNames, colNames, data);
    QCOMPARE(nm.nrow, 2);
    QCOMPARE(nm.ncol, 3);
    QCOMPARE(nm.row_names.size(), 2);
    QCOMPARE(nm.col_names.size(), 3);
    QCOMPARE(nm.data.rows(), (Eigen::Index)2);
    QCOMPARE(nm.data.cols(), (Eigen::Index)3);
}

void TestFiffCoverage::testNamedMatrixTranspose()
{
    StringList rowNames{"A", "B"};
    StringList colNames{"X", "Y", "Z"};
    MatrixXd data(2, 3);
    data << 1, 2, 3, 4, 5, 6;

    FiffNamedMatrix nm(2, 3, rowNames, colNames, data);
    nm.transpose_named_matrix(); // in-place transpose

    QCOMPARE(nm.nrow, 3);
    QCOMPARE(nm.ncol, 2);
    QCOMPARE(nm.row_names, colNames);
    QCOMPARE(nm.col_names, rowNames);
    QVERIFY(qAbs(nm.data(0, 0) - 1.0) < 1e-10);
    QVERIFY(qAbs(nm.data(2, 1) - 6.0) < 1e-10);
}

//=============================================================================================================
// FiffCov
//=============================================================================================================

void TestFiffCoverage::testFiffCovFromFile()
{
    QVERIFY2(QFile::exists(covPath()),
             qPrintable(QString("Covariance test data not found: %1").arg(covPath())));

    FiffCov cov(covPath().toStdString());
    QVERIFY(cov.data.size() > 0);
    QVERIFY(cov.names.size() > 0);
}

void TestFiffCoverage::testFiffCovDimensions()
{
    QVERIFY2(QFile::exists(covPath()),
             qPrintable(QString("Covariance test data not found: %1").arg(covPath())));

    FiffCov cov(covPath().toStdString());
    // Covariance matrix should be square
    QCOMPARE(cov.data.rows(), cov.data.cols());
    QCOMPARE(cov.data.rows(), (Eigen::Index)cov.names.size());
}

//=============================================================================================================

void TestFiffCoverage::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffCoverage)
#include "test_fiff_coverage.moc"

#include <vector>
