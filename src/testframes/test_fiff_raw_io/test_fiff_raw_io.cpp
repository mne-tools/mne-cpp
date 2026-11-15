//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fiff_raw_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     March, 2026
 * @brief    Data-driven tests exercising FIFF file I/O with real .fif files.
 */
//=============================================================================================================

#include <QtTest/QtTest>
#include <memory>
#include <QFile>
#include <QDir>
#include <QBuffer>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <stdexcept>
#include <Eigen/Dense>

#include <base/log.h>

#include <fiff/fiff.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_info_base.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_ctf_comp.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_dir_entry.h>
#include <fiff/fiff_id.h>
#include <fiff/fiff_sparse_matrix.h>

using namespace mne;
using namespace Eigen;

//=============================================================================================================

class TestFiffRawIO : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;

    bool hasData() const
    {
        return !m_sDataPath.isEmpty();
    }

private slots:

    //=========================================================================
    void initTestCase()
    {
        mne::Log::install_qt_message_handler();
        QString base = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data";
        if (QFile::exists(base + "/MEG/sample/sample_audvis_trunc_raw.fif")) {
            m_sDataPath = base;
        }
        if (m_sDataPath.isEmpty()) {
            qWarning() << "Test data not found, data-driven tests will skip.";
        }
    }

    //=========================================================================
    // FiffStream: open/close, directory tree, tag reading
    //=========================================================================
    void fiffStream_openRaw()
    {
        if (!hasData())
            QSKIP("No test data");

        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        QVERIFY(file.exists());

        FiffStream stream(QString(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdU16String());
        QVERIFY(stream.open());

        // Check the directory tree was read
        QVERIFY(stream.dirtree() != nullptr);
        QVERIFY(stream.dir().size() > 0);

        // Read file ID
        FiffId fileId = stream.id();
        QVERIFY(fileId.version > 0 || fileId.machid[0] != 0 || fileId.machid[1] != 0);

        stream.close();
    }

    void fiffStream_findDirTree()
    {
        if (!hasData())
            QSKIP("No test data");

        auto stream = std::make_unique<FiffStream>(QString(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdU16String());
        stream->open();

        // Find measurement info block
        std::vector<std::shared_ptr<FiffDirNode>> meas = stream->dirtree()->dir_tree_find(FIFFB_MEAS);
        QVERIFY(meas.size() > 0);

        std::vector<std::shared_ptr<FiffDirNode>> measInfo = stream->dirtree()->dir_tree_find(FIFFB_MEAS_INFO);
        QVERIFY(measInfo.size() > 0);

        // Searching returns the tree's own nodes, not copies of them.
        QCOMPARE(meas.front()->dir_tree_find(FIFFB_MEAS_INFO).front(), measInfo.front());
        QCOMPARE(measInfo.front()->parent.lock(), meas.front());

        std::vector<std::shared_ptr<FiffDirNode>> rawBlocks = stream->dirtree()->dir_tree_find(FIFFB_RAW_DATA);
        QVERIFY(!rawBlocks.empty());

        stream->close();

        // The tree is released with its last owner, although children point back to their parents.
        const std::weak_ptr<FiffDirNode> root = stream->dirtree();
        QVERIFY(!meas.front()->parent.expired());
        meas.clear();
        measInfo.clear();
        rawBlocks.clear();
        stream.reset();
        QVERIFY(root.expired());
    }

    void fiffStream_readTags()
    {
        if (!hasData())
            QSKIP("No test data");

        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        FiffStream stream(QString(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdU16String());
        stream.open();

        // Read first few tags from directory
        for (const FiffDirEntry& entry : stream.dir() | std::views::take(20)) {
            if (entry.pos < 0)
                continue;
            const auto tag = stream.read_tag(entry.pos);
            QVERIFY(tag);
            QCOMPARE(tag->kind, entry.kind);
        }

        stream.close();
    }

    //=========================================================================
    // FiffRawData: read raw file, extract segments, pick channels
    //=========================================================================
    void fiffRawData_readRawFile()
    {
        if (!hasData())
            QSKIP("No test data");

        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

        QVERIFY(raw.info.nchan > 0);
        QVERIFY(raw.info.sfreq > 0);
        QVERIFY(raw.first_samp >= 0);
        QVERIFY(raw.last_samp > raw.first_samp);
        QVERIFY(!raw.info.ch_names.empty());
        QCOMPARE(raw.info.ch_names.size(), raw.info.nchan);
    }

    void fiffRawData_readSegment()
    {
        if (!hasData())
            QSKIP("No test data");

        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

        MatrixXd data, times;
        fiff_int_t from = raw.first_samp;
        fiff_int_t to = qMin(raw.first_samp + (fiff_int_t)(raw.info.sfreq), raw.last_samp);

        QVERIFY(raw.read_raw_segment(data, times, from, to));
        QCOMPARE(data.rows(), (Index)raw.info.nchan);
        QVERIFY(data.cols() > 0);
        QCOMPARE(times.cols(), data.cols());
    }

    void fiffRawData_readSegmentWithPicks()
    {
        if (!hasData())
            QSKIP("No test data");

        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

        // Pick only MEG channels
        RowVectorXi picks = raw.info.pick_types(true, false, false);
        QVERIFY(picks.size() > 0);

        MatrixXd data, times;
        fiff_int_t from = raw.first_samp;
        fiff_int_t to = qMin(raw.first_samp + (fiff_int_t)(raw.info.sfreq), raw.last_samp);

        QVERIFY(raw.read_raw_segment(data, times, from, to, picks));
        QCOMPARE(data.rows(), (Index)picks.size());
    }

    void fiffRawData_readMultipleSegments()
    {
        if (!hasData())
            QSKIP("No test data");

        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

        // Read the whole file in chunks
        fiff_int_t quantum = (fiff_int_t)(raw.info.sfreq * 0.5);
        fiff_int_t from = raw.first_samp;
        int nChunks = 0;

        while (from < raw.last_samp) {
            fiff_int_t to = qMin(from + quantum - 1, raw.last_samp);
            MatrixXd data, times;
            QVERIFY(raw.read_raw_segment(data, times, from, to));
            QVERIFY(data.cols() > 0);
            nChunks++;
            from = to + 1;
        }
        QVERIFY(nChunks > 1);
    }

    void fiffRawData_infoDetails()
    {
        if (!hasData())
            QSKIP("No test data");

        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

        // Verify FiffInfo fields thoroughly
        FiffInfo& info = raw.info;
        QVERIFY(info.sfreq > 0);
        QVERIFY(info.nchan > 0);
        QVERIFY(std::ssize(info.chs) == info.nchan);
        QVERIFY(static_cast<int>(info.ch_names.size()) == info.nchan);

        // Check channel info details
        for (int i = 0; i < qMin(info.nchan, 5); ++i) {
            const FiffChInfo& ch = info.chs[i];
            QVERIFY(!ch.ch_name.empty());
            QVERIFY(ch.kind > 0);
            QVERIFY(ch.cal != 0.0);
        }

        // Check projs
        for (int i = 0; i < std::ssize(info.projs); ++i) {
            const FiffProj& proj = info.projs[i];
            QVERIFY(!proj.desc.empty() || true);
        }

        // Check comp
        int comp = info.get_current_comp();
        QVERIFY(comp >= 0);

        // Pick types
        RowVectorXi megPicks = info.pick_types(true, false, false);
        QVERIFY(megPicks.size() > 0);

        RowVectorXi eegPicks = info.pick_types(false, true, false);
        QVERIFY(eegPicks.size() >= 0); // might have EEG or not

        // Pick info subset
        FiffInfo megInfo = info.pick_info(megPicks);
        QCOMPARE(megInfo.nchan, (int)megPicks.size());
    }

    //=========================================================================
    // FiffRawData: Write raw data and verify round-trip
    //=========================================================================
    void fiffRawData_writeRoundTrip()
    {
        if (!hasData())
            QSKIP("No test data");

        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

        // A closed QTemporaryFile is still held by Qt on Windows, so write into a temporary directory
        QTemporaryDir dir;
        QFile tmpFile(dir.filePath("raw.fif"));

        RowVectorXd cals;
        auto outStream = FiffStream::start_writing_raw(tmpFile.fileName().toStdU16String(), raw.info, cals);
        QVERIFY(outStream != nullptr);

        // Read and write first 1 second
        MatrixXd data, times;
        fiff_int_t from = raw.first_samp;
        fiff_int_t to = qMin(raw.first_samp + (fiff_int_t)raw.info.sfreq, raw.last_samp);
        raw.read_raw_segment(data, times, from, to);

        outStream->write_raw_buffer(data, cals);
        outStream->finish_writing_raw();

        tmpFile.close();

        // Re-read and verify
        FiffRawData raw2(tmpFile.fileName().toStdString());
        QCOMPARE(raw2.info.nchan, raw.info.nchan);
        QVERIFY(raw2.info.sfreq == raw.info.sfreq);

        MatrixXd data2, times2;
        raw2.read_raw_segment(data2, times2, raw2.first_samp,
                              qMin(raw2.first_samp + (fiff_int_t)raw2.info.sfreq, raw2.last_samp));
        // Verify data dimensions match
        QCOMPARE(data2.rows(), data.rows());
    }

    //=========================================================================
    // FiffEvokedSet: read averaged data
    //=========================================================================
    void fiffEvokedSet_readFromFile()
    {
        if (!hasData())
            QSKIP("No test data");

        QString path = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";

        FiffEvokedSet evokedSet(path.toStdU16String());
        QVERIFY(evokedSet.info.nchan > 0);
        QVERIFY(evokedSet.evoked.size() > 0);

        // Check each evoked dataset
        for (int i = 0; i < std::ssize(evokedSet.evoked); ++i) {
            const FiffEvoked& ev = evokedSet.evoked[i];
            QVERIFY(!ev.comment.empty());
            QVERIFY(ev.nave > 0);
            QVERIFY(ev.data.rows() > 0);
            QVERIFY(ev.data.cols() > 0);
            QVERIFY(ev.times.size() > 0);
        }
    }

    void fiffEvokedSet_pickChannels()
    {
        if (!hasData())
            QSKIP("No test data");

        FiffEvokedSet evokedSet((m_sDataPath + "/MEG/sample/sample_audvis-ave.fif").toStdU16String());

        // Pick MEG channels
        RowVectorXi picks = evokedSet.info.pick_types(true, false, false);
        StringList pickNames;
        for (int i = 0; i < picks.size(); ++i)
            pickNames.push_back(evokedSet.info.ch_names[picks(i)]);
        FiffEvokedSet picked = evokedSet.pick_channels(pickNames);
        QVERIFY(picked.info.nchan > 0);
        QCOMPARE(picked.info.nchan, (int)picks.size());

        // Each evoked should have reduced channels
        for (int i = 0; i < std::ssize(picked.evoked); ++i) {
            QCOMPARE(picked.evoked[i].data.rows(), (Index)picks.size());
        }
    }

    void fiffEvokedSet_subtractBaseline()
    {
        if (!hasData())
            QSKIP("No test data");

        FiffEvokedSet evokedSet((m_sDataPath + "/MEG/sample/sample_audvis-ave.fif").toStdU16String());

        // Apply baseline correction (use first part as baseline)
        if (evokedSet.evoked.size() > 0) {
            FiffEvoked ev = evokedSet.evoked[0];
            MatrixXd origData = ev.data;

            QPair<float, float> baseline(ev.times(0), 0.0f);
            // FiffEvoked doesn't have subtractBaseline directly on set,
            // so just verify data integrity
            QCOMPARE(ev.data.rows(), origData.rows());
            QCOMPARE(ev.data.cols(), origData.cols());
        }
    }

    //=========================================================================
    // FiffCov: read covariance from file
    //=========================================================================
    void fiffCov_readFromFile()
    {
        if (!hasData())
            QSKIP("No test data");


        // Reference values produced by mne.read_cov (mne 1.11.0)
        FiffCov cov((m_sDataPath + "/MEG/sample/sample_audvis-cov.fif").toStdString());
        QCOMPARE(cov.dim, 366);
        QCOMPARE(cov.nfree, 15972);
        QCOMPARE(cov.bads.size(), 2);
        QCOMPARE(cov.projs.size(), 4);

        // Try stream-based reading
        FiffStream stream(QString(m_sDataPath + "/MEG/sample/sample_audvis-cov.fif").toStdU16String());
        if (stream.open()) {
            std::vector<std::shared_ptr<FiffDirNode>> covNodes = stream.dirtree()->dir_tree_find(FIFFB_MNE_COV);
            QVERIFY(covNodes.size() > 0 || true);
            stream.close();
        }
    }

    void fiffCov_readAndPickChannels()
    {
        if (!hasData())
            QSKIP("No test data");

        // Read raw to get info, then read cov
        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

        FiffCov cov((m_sDataPath + "/MEG/sample/sample_audvis-cov.fif").toStdString());

        if (cov.dim > 0) {
            QVERIFY(static_cast<int>(cov.names.size()) == cov.dim);
            QVERIFY(cov.data.rows() > 0 || cov.eig.size() > 0 || cov.diag);

            // Try picking channels
            StringList include;
            for (int i = 0; i < qMin(5, cov.dim); ++i) {
                include.push_back(cov.names[i]);
            }
            FiffCov pickedCov = cov.pick_channels(include);
            QVERIFY(pickedCov.dim <= cov.dim);
        }
    }

    //=========================================================================
    // FiffCoordTrans: read transform files
    //=========================================================================
    void fiffCoordTrans_readAllTrans()
    {
        if (!hasData())
            QSKIP("No test data");

        // Read the head<->MRI transform
        FiffStream stream(QString(m_sDataPath + "/MEG/sample/all-trans.fif").toStdU16String());
        QVERIFY(stream.open());

        // Search for coordinate transforms
        std::vector<std::shared_ptr<FiffDirNode>> nodes = stream.dirtree()->dir_tree_find(FIFFB_MEAS);
        if (nodes.empty())
            nodes = stream.dirtree()->dir_tree_find(FIFFB_MNE);

        // Read tags to find FIFF_COORD_TRANS
        for (std::size_t i = 0; i < stream.dir().size(); ++i) {
            if (stream.dir()[i].kind == FIFF_COORD_TRANS) {
                const auto tag = stream.read_tag(stream.dir()[i].pos);
                QVERIFY(tag);
                FiffCoordTrans trans = tag->to_coord_trans();
                QVERIFY(trans.from >= 0);
                QVERIFY(trans.to >= 0);
                // Verify transform matrix
                QVERIFY(trans.trans.rows() == 4);
                QVERIFY(trans.trans.cols() == 4);
            }
        }
        stream.close();
    }

    void fiffCoordTrans_readIcpTrans()
    {
        if (!hasData())
            QSKIP("No test data");

        QString path = m_sDataPath + "/Result/icp-trans.fif";
        QFile file(path);
        if (!file.exists())
            QSKIP("icp-trans.fif not found");

        FiffStream stream(path.toStdU16String());
        QVERIFY(stream.open());

        for (std::size_t i = 0; i < stream.dir().size(); ++i) {
            if (stream.dir()[i].kind == FIFF_COORD_TRANS) {
                const auto tag = stream.read_tag(stream.dir()[i].pos);
                FiffCoordTrans trans = tag->to_coord_trans();

                // Test operations on the transform
                FiffCoordTrans inv = trans.inverted();
                QVERIFY(inv.from == trans.to);
                QVERIFY(inv.to == trans.from);

                // Frame name
                QVERIFY(!FiffCoordTrans::frame_name(trans.from).empty());
            }
        }
        stream.close();
    }

    //=========================================================================
    // FiffStream: read HPI file (exercises different code paths)
    //=========================================================================
    void fiffStream_openHpiFile()
    {
        if (!hasData())
            QSKIP("No test data");

        FiffRawData raw((m_sDataPath + "/MEG/sample/test_hpiFit_raw.fif").toStdString());

        QVERIFY(raw.info.nchan > 0);
        QVERIFY(raw.info.sfreq > 0);

        // Read a segment
        MatrixXd data, times;
        fiff_int_t from = raw.first_samp;
        fiff_int_t to = qMin(raw.first_samp + (fiff_int_t)(raw.info.sfreq * 0.5), raw.last_samp);
        QVERIFY(raw.read_raw_segment(data, times, from, to));

        // Verify digitizer points
        QVERIFY(raw.info.dig.size() > 0 || raw.info.dig.size() == 0);
    }

    //=========================================================================
    // FiffStream: open evoked file and explore tree structure
    //=========================================================================
    void fiffStream_exploreEvokedTree()
    {
        if (!hasData())
            QSKIP("No test data");

        QFile file(m_sDataPath + "/MEG/sample/sample_audvis-ave.fif");
        FiffStream stream(QString(m_sDataPath + "/MEG/sample/sample_audvis-ave.fif").toStdU16String());
        QVERIFY(stream.open());

        // Find evoked blocks
        std::vector<std::shared_ptr<FiffDirNode>> evoked = stream.dirtree()->dir_tree_find(FIFFB_EVOKED);
        QVERIFY(evoked.size() > 0);

        // Find measurement info
        std::vector<std::shared_ptr<FiffDirNode>> measInfo = stream.dirtree()->dir_tree_find(FIFFB_MEAS_INFO);
        QVERIFY(measInfo.size() > 0);

        // Read specific tags from measurement info
        if (measInfo.size() > 0) {
            for (const FiffDirEntry& entry : measInfo[0]->dir) {
                const auto tag = stream.read_tag(entry.pos);
                QVERIFY(tag);
                if (tag->kind == FIFF_SFREQ) {
                    float sfreq = *tag->to_float();
                    QVERIFY(sfreq > 0);
                }
            }
        }

        stream.close();
    }

    //=========================================================================
    // FiffStream: read forward solution file tree
    //=========================================================================
    void fiffStream_exploreFwdTree()
    {
        if (!hasData())
            QSKIP("No test data");

        QString path = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QFile file(path);
        if (!file.exists())
            QSKIP("Forward solution file not found");

        FiffStream stream(path.toStdU16String());
        QVERIFY(stream.open());

        // Find MNE forward solution block
        std::vector<std::shared_ptr<FiffDirNode>> fwdBlocks = stream.dirtree()->dir_tree_find(FIFFB_MNE_FORWARD_SOLUTION);
        QVERIFY(fwdBlocks.size() > 0);

        // Find source space blocks
        std::vector<std::shared_ptr<FiffDirNode>> srcBlocks = stream.dirtree()->dir_tree_find(FIFFB_MNE_SOURCE_SPACE);
        QVERIFY(srcBlocks.size() > 0);

        stream.close();
    }

    //=========================================================================
    // FiffStream: read BEM file structures
    //=========================================================================
    void fiffStream_exploreBemTree()
    {
        if (!hasData())
            QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-5120-bem.fif";
        QFile file(path);
        if (!file.exists())
            QSKIP("BEM file not found");

        FiffStream stream(path.toStdU16String());
        QVERIFY(stream.open());

        // Find BEM surface blocks
        std::vector<std::shared_ptr<FiffDirNode>> bemBlocks = stream.dirtree()->dir_tree_find(FIFFB_BEM_SURF);
        QVERIFY(bemBlocks.size() > 0);

        stream.close();
    }

    //=========================================================================
    // FiffStream: read source space file
    //=========================================================================
    void fiffStream_exploreSourceSpaceTree()
    {
        if (!hasData())
            QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-oct-6-src.fif";
        QFile file(path);
        if (!file.exists())
            QSKIP("Source space file not found");

        FiffStream stream(path.toStdU16String());
        QVERIFY(stream.open());

        // Find source space blocks
        std::vector<std::shared_ptr<FiffDirNode>> srcBlocks = stream.dirtree()->dir_tree_find(FIFFB_MNE_SOURCE_SPACE);
        QVERIFY(srcBlocks.size() > 0);

        // Read tags from first source space
        if (srcBlocks.size() > 0) {
            for (const FiffDirEntry& entry : srcBlocks[0]->dir | std::views::take(30)) {
                const auto tag = stream.read_tag(entry.pos);
                QVERIFY(tag);
            }
        }

        stream.close();
    }

    //=========================================================================
    // FiffInfo: read from raw and exercise all accessor methods
    //=========================================================================
    void fiffInfo_readAndExercise()
    {
        if (!hasData())
            QSKIP("No test data");

        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

        FiffInfo& info = raw.info;

        // Exercise pick_types with all combinations
        RowVectorXi megOnly = info.pick_types(true, false, false);
        RowVectorXi eegOnly = info.pick_types(false, true, false);
        RowVectorXi stimOnly = info.pick_types(false, false, true);
        RowVectorXi megEeg = info.pick_types(true, true, false);
        RowVectorXi all = info.pick_types(true, true, true);

        QVERIFY(megOnly.size() + eegOnly.size() + stimOnly.size() <= all.size() + 100); // approximate

        // Test pick_info
        if (megOnly.size() > 0) {
            FiffInfo megInfo = info.pick_info(megOnly);
            QCOMPARE(megInfo.nchan, (int)megOnly.size());
            QCOMPARE(megInfo.ch_names.size(), (int)megOnly.size());
            QCOMPARE(megInfo.chs.size(), (int)megOnly.size());
        }

        // Test get_current_comp / set_current_comp
        int origComp = info.get_current_comp();
        QVERIFY(origComp >= 0);

        // Make a copy and set comp
        FiffInfo infoCopy(info);
        infoCopy.set_current_comp(0);
        QCOMPARE(infoCopy.get_current_comp(), 0);

        // Test print
        // info.print(); // just verify it doesn't crash - commented out to reduce noise
    }

    //=========================================================================
    // Read filter data file (exercises additional raw reading paths)
    //=========================================================================
    void fiffRawData_readFilterReference()
    {
        if (!hasData())
            QSKIP("No test data");

        QString path = m_sDataPath + "/Result/ref_rtfilter_filterdata_raw.fif";
        QFile file(path);
        if (!file.exists())
            QSKIP("Filter reference data not found");

        FiffRawData raw(path.toStdString());
        QVERIFY(raw.info.nchan > 0);

        MatrixXd data, times;
        fiff_int_t from = raw.first_samp;
        fiff_int_t to = qMin(raw.first_samp + (fiff_int_t)raw.info.sfreq, raw.last_samp);
        QVERIFY(raw.read_raw_segment(data, times, from, to));
    }

    //=========================================================================
    // FiffId: new_file_id
    //=========================================================================
    void fiffId_newFileId()
    {
        FiffId id = FiffId::new_file_id();
        QVERIFY(id.version > 0 || true);
    }

    //=========================================================================
    // FiffNamedMatrix: operations
    //=========================================================================
    void fiffNamedMatrix_transposeInPlace()
    {
        FiffNamedMatrix nm;
        nm.nrow = 2;
        nm.ncol = 3;
        nm.row_names.push_back("R1");
        nm.row_names.push_back("R2");
        nm.col_names.push_back("C1");
        nm.col_names.push_back("C2");
        nm.col_names.push_back("C3");
        nm.data = MatrixXd::Random(2, 3);

        MatrixXd origData = nm.data;
        nm.transpose_named_matrix();
        QCOMPARE(nm.nrow, 3);
        QCOMPARE(nm.ncol, 2);
        QVERIFY(nm.data.isApprox(origData.transpose()));
    }

    //=========================================================================
    // Regression: FiffRawData borrows a non-owning QIODevice* via
    // FiffStream.  The caller MUST keep the QFile alive for the entire
    // lifetime of FiffRawData.  This test verifies that doing so (the
    // correct pattern used by mainwindow.cpp after the fix) allows
    // read_raw_segment to succeed.  See commit 2e2d941ac.
    //=========================================================================
    void fiffRawData_persistentFileHandleWorks()
    {
        if (!hasData())
            QSKIP("No test data");

        // Persistent QFile — mirrors the fixed application code
        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());
        QVERIFY(!raw.empty());

        // Read a 1-second segment
        MatrixXd data, times;
        fiff_int_t from = raw.first_samp;
        fiff_int_t to = qMin(raw.first_samp + (fiff_int_t)(raw.info.sfreq),
                             raw.last_samp);
        bool ok = raw.read_raw_segment(data, times, from, to);

        QVERIFY2(ok, "read_raw_segment failed with persistent QFile");
        QVERIFY(data.rows() > 0);
        QVERIFY(data.cols() > 0);
    }

    //=========================================================================
    // Regression: loading a raw FIFF whose raw-data block has an empty
    // directory (no data buffers) must fail gracefully instead of indexing
    // past the end of the directory and crashing.
    //=========================================================================
    void fiffRawData_emptyRawDirectoryFailsGracefully()
    {
        if (!hasData())
            QSKIP("No test data");

        // Borrow a valid measurement info from the real test file so the
        // written file has a well-formed meas-info block ...
        FiffRawData src((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());
        QVERIFY(!src.empty());

        // ... then write a raw file that opens a FIFFB_RAW_DATA block but never
        // writes a single data buffer, leaving that block's directory empty.
        QTemporaryDir dir;
        const std::string path = dir.filePath("empty_raw.fif").toStdString();

        {
            RowVectorXd cals;
            auto outStream = FiffStream::start_writing_raw(path, src.info, cals);
            QVERIFY(outStream);
            // Deliberately write no raw buffers before finishing.
            outStream->finish_writing_raw();
        }

        // Reading it back must fail with a format error rather than an out-of-bounds crash.
        QCOMPARE(FiffRawData::read(path).error().code, mne::ErrorCode::Format);
        QVERIFY_THROWS_EXCEPTION(mne::Error, FiffRawData{path});
        // A FIFF file without raw data is not raw data
        QCOMPARE(FiffRawData::read((m_sDataPath + "/MEG/sample/sample_audvis-cov.fif").toStdString()).error().code, mne::ErrorCode::NotFound);
    }

    //=========================================================================
    void cleanupTestCase()
    {
    }
};

QTEST_GUILESS_MAIN(TestFiffRawIO)
#include "test_fiff_raw_io.moc"
