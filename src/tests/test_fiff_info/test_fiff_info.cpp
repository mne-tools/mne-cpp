// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2010-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

#include <algorithm>
#include <QtTest/QtTest>
#include <mne/base/string_utils.h>
#include <Eigen/Dense>

#include <cstring>
#include <new>

#include <mne/base/log.h>

#include <mne/fiff/fiff_ch_name.h>
#include <mne/fiff/fiff_info_base.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_proj.h>
#include <mne/fiff/fiff_named_matrix.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_cov.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_dig_point_set.h>

using namespace mne;
using namespace Eigen;

class TestFiffInfo : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;

    bool hasData() const
    {
        return !m_sDataPath.isEmpty();
    }

    FiffChInfo makeCh(const QString& name, int kind, int unit, int coilType)
    {
        FiffChInfo ch;
        ch.ch_name = name.toStdString();
        ch.kind = kind;
        ch.unit = unit;
        ch.chpos.coil_type = coilType;
        ch.range = 1.0;
        ch.cal = 1.0;
        return ch;
    }

    FiffInfoBase makeSyntheticInfoBase(int nCh = 4)
    {
        FiffInfoBase info;
        info.nchan = nCh;
        for (int i = 0; i < nCh; ++i) {
            QString name = QString("MEG %1").arg(i, 4, 10, QChar('0'));
            info.chs.push_back(makeCh(name, FIFFV_MEG_CH, FIFF_UNIT_T, FIFFV_COIL_VV_MAG_T3));
            info.ch_names.push_back(name.toStdString());
        }
        info.bads.clear();
        return info;
    }

private slots:
    void initTestCase()
    {
        QString base = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data";
        if (QFile::exists(base + "/MEG/sample/sample_audvis_trunc_raw.fif"))
            m_sDataPath = base;
    }
    //=========================================================================
    // Channel names
    //=========================================================================
    void channelNames_readAsStored()
    {
        if (m_sDataPath.isEmpty())
            QSKIP("No test data");
        // As mne-python reads them: names, bads and projector columns keep their spaces
        const FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());
        QCOMPARE(raw.info.ch_names.front(), std::string("MEG 0113"));
        QCOMPARE(raw.info.chs.front().ch_name, std::string("MEG 0113"));
        QCOMPARE(raw.info.bads, (mne::StringList{"MEG 2443", "EEG 053"}));
        QVERIFY(!raw.info.projs.empty());
        QCOMPARE(raw.info.projs.front().data.col_names.front(), std::string("MEG 0111"));
    }

    void channelNames_matchAcrossConventions()
    {
        QVERIFY(same_channel("MEG 0113", "MEG0113"));
        QVERIFY(same_channel("meg0113", "MEG 0113"));
        QVERIFY(same_channel("STI 014", "STI 014"));
        QVERIFY(same_channel("", "  "));
        QVERIFY(!same_channel("MEG 0113", "MEG 0112"));
        QVERIFY(!same_channel("MEG 011", "MEG 0113"));

        // An exact name wins over one that only matches across conventions
        const mne::StringList names{"EEG001", "EEG 001", "STI 014"};
        QCOMPARE(channel_index(names, "EEG 001"), 1);
        QCOMPARE(channel_index(names, "STI014"), 2);
        QCOMPARE(channel_index(names, "EEG 002"), -1);

        // Selections and look-ups accept either convention
        FiffInfoBase info = makeSyntheticInfoBase(3); // MEG 0000 .. MEG 0002
        info.bads = {"MEG0002"};
        QCOMPARE(info.channelIndex("MEG0001"), 1);
        QVERIFY(info.isBad("MEG 0002"));
        QVERIFY(!info.isBad("MEG 0001"));
        QCOMPARE(FiffInfoBase::pick_channels(info.ch_names, {"meg0002"}).size(), 1);
        QCOMPARE(FiffInfoBase::pick_channels(info.ch_names, {}, {"MEG0001"}).size(), 2);
    }

    //=========================================================================
    // FiffInfoBase
    //=========================================================================
    void infoBase_defaultCtor()
    {
        FiffInfoBase info;
        QVERIFY(info.empty());
    }

    void infoBase_copyCtor()
    {
        FiffInfoBase info = makeSyntheticInfoBase(3);
        FiffInfoBase copy(info);
        QCOMPARE(copy.nchan, 3);
        QCOMPARE(copy.ch_names.size(), 3);
    }

    void infoBase_clear()
    {
        FiffInfoBase info = makeSyntheticInfoBase(4);
        info.clear();
        QVERIFY(info.empty());
    }

    void info_clearThroughBase()
    {
        // A FiffInfo cleared through its base clears its own fields too.
        FiffInfo info;
        info.sfreq = 1000.0f;
        info.device_info.type = "synthetic";
        info.acq_pars = "ACQch001 110113";
        FiffInfoBase& base = info;
        base.clear();
        QCOMPARE(info.sfreq, -1.0f);
        QVERIFY(info.device_info.empty());
        QVERIFY(info.acq_pars.empty());
    }

    void infoBase_channelType()
    {
        FiffInfoBase info = makeSyntheticInfoBase(2);
        QString type = QString::fromStdString(info.channel_type(0));
        QVERIFY(!type.isEmpty());
    }

    void infoBase_pickChannels()
    {
        StringList allNames;
        allNames.push_back("MEG 0001");
        allNames.push_back("MEG 0002");
        allNames.push_back("MEG 0003");
        allNames.push_back("EEG 001");
        StringList include;
        include.push_back("MEG 0001");
        include.push_back("MEG 0003");
        StringList exclude;

        RowVectorXi picks = FiffInfoBase::pick_channels(allNames, include, exclude);
        QCOMPARE(picks.size(), 2);
        QCOMPARE(picks(0), 0);
        QCOMPARE(picks(1), 2);
    }

    void infoBase_pickChannelsExclude()
    {
        StringList allNames;
        allNames.push_back("MEG 0001");
        allNames.push_back("MEG 0002");
        allNames.push_back("MEG 0003");
        StringList include; // empty = include all
        StringList exclude;
        exclude.push_back("MEG 0002");

        RowVectorXi picks = FiffInfoBase::pick_channels(allNames, include, exclude);
        QCOMPARE(picks.size(), 2);
    }

    void infoBase_pickInfo()
    {
        FiffInfoBase info = makeSyntheticInfoBase(4);
        RowVectorXi picks(2);
        picks << 0, 2;
        FiffInfoBase picked = info.pick_info(&picks);
        QCOMPARE(picked.nchan, 2);
        QCOMPARE(picked.ch_names.size(), 2);
    }

    void infoBase_getChannelTypes()
    {
        FiffInfoBase info = makeSyntheticInfoBase(3);
        StringList types = info.get_channel_types();
        QVERIFY(types.size() > 0);
        QVERIFY(std::ranges::contains(types, "mag"));
    }

    void infoBase_equality()
    {
        FiffInfoBase info1 = makeSyntheticInfoBase(2);
        FiffInfoBase info2 = makeSyntheticInfoBase(2);
        QVERIFY(info1 == info2);
        // ez was left uninitialized, so this held only when the stack happened to be finite.
        alignas(FiffChPos) unsigned char storage[sizeof(FiffChPos)];
        std::memset(storage, 0xff, sizeof(storage));
        FiffChPos* pos = new (storage) FiffChPos;
        QVERIFY(pos->r0.isZero() && pos->ex.isZero() && pos->ey.isZero() && pos->ez.isZero());
        pos->~FiffChPos();
    }

    void info_defaultIsDeterministic()
    {
        // proj_id and the microseconds of meas_date were left uninitialized, so writers stored stack garbage
        alignas(FiffInfo) unsigned char storage[sizeof(FiffInfo)];
        std::memset(storage, 0xab, sizeof(storage));
        FiffInfo* info = new (storage) FiffInfo;
        QCOMPARE(info->proj_id, -1);
        QCOMPARE(info->meas_date[0], -1);
        QCOMPARE(info->meas_date[1], 0);
        info->~FiffInfo();

        FiffInfo cleared;
        cleared.meas_date[1] = 7;
        cleared.clear();
        QCOMPARE(cleared.meas_date[1], 0);
    }

    void infoBase_pickTypes()
    {
        FiffInfoBase info = makeSyntheticInfoBase(4);
        RowVectorXi megPicks = info.pick_types("mag", false, false);
        QVERIFY(megPicks.size() > 0);
        // A default-constructed info has nchan -1 and no channels; this used to throw bad_alloc.
        QCOMPARE(FiffInfoBase().pick_types("mag", true, true).size(), Index(0));
    }

    void info_picksMatchMnePython()
    {
        if (!hasData())
            QSKIP("No test data");
        // Reference values produced by mne 1.11 on sample_audvis_trunc_raw.fif (bads MEG 2443, EEG 053, 376 channels):
        //   mne.pick_types(info, meg, eeg, stim) (exclude="bads"), pick_types(..., exclude=[]),
        //   mne.pick_channels(ch_names, include=["EEG 001", "MEG 0113", "STI 014"]), pick_channels(exclude=bads),
        //   mne.pick_info(info, [3, 10, 316]) -> MEG 0122, MEG 0142, EEG 002 and no bads
        const FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());
        const FiffInfo& info = raw.info;
        QCOMPARE(info.nchan, 376);
        QCOMPARE(info.bads, StringList({"MEG 2443", "EEG 053"}));

        const RowVectorXi meg = info.pick_types(true, false, false, {}, info.bads);
        QCOMPARE(meg.size(), Index(305));
        QCOMPARE(meg(304), 305);
        const RowVectorXi eeg = info.pick_types(false, true, false, {}, info.bads);
        QCOMPARE(eeg.size(), Index(59));
        QCOMPARE(eeg(0), 315);
        QCOMPARE(eeg(58), 374);
        const RowVectorXi stim = info.pick_types(false, false, true, {}, info.bads);
        QCOMPARE(stim.size(), Index(9));
        QCOMPARE(stim(0), 306);
        QCOMPARE(stim(8), 314);
        QCOMPARE(info.pick_types(true, true, false).size(), Index(366));

        QCOMPARE(FiffInfoBase::pick_channels(info.ch_names, {"EEG 001", "MEG 0113", "STI 014"}), RowVectorXi((RowVectorXi(3) << 0, 312, 315).finished()));
        QCOMPARE(FiffInfoBase::pick_channels(info.ch_names, {}, info.bads).size(), Index(374));

        const RowVectorXi sel = (RowVectorXi(3) << 3, 10, 316).finished();
        const FiffInfo sub = info.pick_info(sel);
        QCOMPARE(sub.ch_names, StringList({"MEG 0122", "MEG 0142", "EEG 002"}));
        QCOMPARE(sub.nchan, 3);
        QVERIFY(sub.bads.empty());
    }

    //=========================================================================
    // FiffProj
    //=========================================================================
    void proj_defaultCtor()
    {
        FiffProj proj;
        QCOMPARE(proj.active, false);
        QCOMPARE(proj.kind, (fiff_int_t)-1);
    }

    void proj_parameterizedCtor()
    {
        FiffNamedMatrix data;
        data.nrow = 1;
        data.ncol = 3;
        data.data = MatrixXd::Ones(1, 3) / std::sqrt(3.0);
        data.col_names.push_back("CH1");
        data.col_names.push_back("CH2");
        data.col_names.push_back("CH3");
        data.row_names.push_back("Proj1");

        FiffProj proj(FIFFV_MNE_PROJ_ITEM_EEG_AVREF, true, "TestProj", data);
        QCOMPARE(proj.active, true);
        QVERIFY(proj.data.nrow == 1);
    }

    void proj_copyCtor()
    {
        FiffProj proj;
        proj.active = true;
        proj.desc = "test";
        FiffProj copy(proj);
        QCOMPARE(copy.active, true);
        QCOMPARE(copy.desc, QString("test"));
    }

    void proj_activateProjs()
    {
        FiffProj proj;
        proj.active = false;
        std::vector<FiffProj> projs;
        projs.push_back(proj);
        FiffProj::activate_projs(projs);
        QCOMPARE(projs[0].active, true);
    }

    void proj_makeProjector()
    {
        FiffNamedMatrix data;
        data.nrow = 1;
        data.ncol = 3;
        data.data = MatrixXd::Ones(1, 3) / std::sqrt(3.0);
        data.col_names.push_back("CH1");
        data.col_names.push_back("CH2");
        data.col_names.push_back("CH3");
        data.row_names.push_back("PCA1");

        FiffProj proj(FIFFV_MNE_PROJ_ITEM_EEG_AVREF, true, "PCA", data);
        std::vector<FiffProj> projs;
        projs.push_back(proj);

        StringList chNames;
        chNames.push_back("CH1");
        chNames.push_back("CH2");
        chNames.push_back("CH3");
        MatrixXd P;
        int nProj = FiffProj::make_projector(projs, chNames, P);
        QVERIFY(nProj >= 0);
        if (nProj > 0) {
            QCOMPARE(P.rows(), 3);
            QCOMPARE(P.cols(), 3);
        }
    }

    //=========================================================================
    // FiffInfo from raw file
    //=========================================================================
    void info_readFromRaw()
    {
        QString rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawPath)) {
            QSKIP("Sample raw file not found");
        }
        FiffRawData raw(rawPath.toStdString());
        QVERIFY(raw.info.nchan > 0);
        QVERIFY(raw.info.sfreq > 0);

        RowVectorXi megPicks = raw.info.pick_types("mag", false, false);
        QVERIFY(megPicks.size() > 0);

        RowVectorXi gradPicks = raw.info.pick_types("grad", false, false);
        QVERIFY(gradPicks.size() > 0);

        RowVectorXi eegPicks = raw.info.pick_types("", true, false);
        QVERIFY(eegPicks.size() > 0);

        FiffInfoBase pickedInfo = raw.info.pick_info(megPicks);
        QCOMPARE(pickedInfo.nchan, megPicks.size());
    }

    void info_channelTypes()
    {
        QString rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawPath)) {
            QSKIP("Sample raw file not found");
        }
        FiffRawData raw(rawPath.toStdString());

        StringList chTypes = raw.info.get_channel_types();
        QVERIFY(chTypes.size() > 0);
        QVERIFY(std::ranges::contains(chTypes, "mag") || std::ranges::contains(chTypes, "grad") || std::ranges::contains(chTypes, "eeg"));
    }

    //=========================================================================
    // DATA-DRIVEN: Full channel picking from real raw info
    //=========================================================================
    void data_pickTypes_allCombinations()
    {
        if (!hasData())
            QSKIP("No test data");
        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

        // Pick magnetometers
        RowVectorXi magPicks = raw.info.pick_types("mag", false, false);
        QVERIFY(magPicks.size() > 0);

        // Pick gradiometers
        RowVectorXi gradPicks = raw.info.pick_types("grad", false, false);
        QVERIFY(gradPicks.size() > 0);

        // Pick EEG
        RowVectorXi eegPicks = raw.info.pick_types("", true, false);
        QVERIFY(eegPicks.size() > 0);

        // Pick STI
        RowVectorXi stiPicks = raw.info.pick_types("", false, true);
        QVERIFY(stiPicks.size() > 0);

        // All combined should cover more
        int totalPicked = magPicks.size() + gradPicks.size() + eegPicks.size() + stiPicks.size();
        QVERIFY(totalPicked > 0);
        QVERIFY(totalPicked <= raw.info.nchan);
    }

    void data_pickInfo_fromRaw()
    {
        if (!hasData())
            QSKIP("No test data");
        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

        // Pick first 50 channels
        RowVectorXi picks(50);
        for (int i = 0; i < 50; ++i)
            picks(i) = i;

        FiffInfo pickedInfo = raw.info.pick_info(picks);
        QCOMPARE(pickedInfo.nchan, 50);
        QCOMPARE(pickedInfo.ch_names.size(), 50);
        QCOMPARE(pickedInfo.chs.size(), 50);
    }

    //=========================================================================
    // DATA-DRIVEN: SSP projectors from real raw data
    //=========================================================================
    void data_proj_fromRealInfo()
    {
        if (!hasData())
            QSKIP("No test data");
        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

        std::vector<FiffProj>& projs = raw.info.projs;
        if (projs.empty())
            QSKIP("No projectors in file");

        Log::debug("Found {} projectors:", projs.size());
        for (const FiffProj& p : projs) {
            Log::debug("  {} active= {} kind= {}", p.desc, p.active, p.kind);
        }

        // Activate all
        FiffProj::activate_projs(projs);
        for (const FiffProj& p : projs)
            QVERIFY(p.active);

        // Make projector matrix
        MatrixXd P;
        int nProj = FiffProj::make_projector(projs, raw.info.ch_names, P);
        QVERIFY(nProj > 0);
        QCOMPARE(P.rows(), (Eigen::Index)raw.info.nchan);
        QCOMPARE(P.cols(), (Eigen::Index)raw.info.nchan);

        // P should be idempotent (P*P = P)
        MatrixXd PP = P * P;
        QVERIFY(PP.isApprox(P, 1e-10));
    }

    //=========================================================================
    // DATA-DRIVEN: FiffCov from file
    //=========================================================================
    void data_cov_readAndPick()
    {
        if (!hasData())
            QSKIP("No test data");
        QString path = m_sDataPath + "/MEG/sample/sample_audvis-cov.fif";
        QFile file(path);
        if (!file.exists())
            QSKIP("Covariance file not found");

        FiffCov cov(path.toStdString());
        QVERIFY(!cov.empty());
        QVERIFY(cov.data.rows() > 0);
        QCOMPARE(cov.data.rows(), cov.data.cols());
        QVERIFY(cov.names.size() > 0);

        // Pick subset
        int nPick = std::min(30, static_cast<int>(cov.names.size()));
        StringList pickNames = StringList(cov.names.begin() + 0, cov.names.begin() + 0 + nPick);
        FiffCov picked = cov.pick_channels(pickNames);
        QCOMPARE(picked.names.size(), nPick);
    }

    //=========================================================================
    // DATA-DRIVEN: Read evoked and exercise info
    //=========================================================================
    void data_evokedInfo()
    {
        if (!hasData())
            QSKIP("No test data");
        QString path = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";
        QFile file(path);
        if (!file.exists())
            QSKIP("Evoked file not found");

        FiffEvokedSet evokedSet(path.toStdU16String());
        QVERIFY(evokedSet.evoked.size() > 0);
        QVERIFY(evokedSet.info.nchan > 0);

        // Exercise pick_channels
        StringList picks;
        for (int i = 0; i < qMin(20, evokedSet.info.nchan); ++i)
            picks.push_back(evokedSet.info.ch_names[i]);

        FiffEvokedSet pickedSet = evokedSet.pick_channels(picks);
        QCOMPARE(pickedSet.info.nchan, picks.size());
    }

    //=========================================================================
    // DATA-DRIVEN: Digitization points from raw info
    //=========================================================================
    void data_digPoints()
    {
        if (!hasData())
            QSKIP("No test data");
        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

        FiffDigPointSet digSet = raw.info.dig;
        if (digSet.size() == 0)
            QSKIP("No dig points");

        Log::debug("Found {} digitization points", digSet.size());

        // Check that dig points have valid coordinates
        for (int i = 0; i < std::ssize(digSet); ++i) {
            FiffDigPoint dp = digSet[i];
            QVERIFY(dp.kind >= 0);
        }
    }

    //=========================================================================
    // DATA-DRIVEN: FiffInfo comp data
    //=========================================================================
    void data_compInfo()
    {
        if (!hasData())
            QSKIP("No test data");
        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

        // Get current compensation
        fiff_int_t comp = raw.info.get_current_comp();
        Log::debug("Current compensation grade: {}", comp);

        // Set compensation (exercise the code path)
        raw.info.set_current_comp(0);
        QCOMPARE(raw.info.get_current_comp(), (fiff_int_t)0);

        // Restore
        raw.info.set_current_comp(comp);
        QCOMPARE(raw.info.get_current_comp(), comp);
    }

    //=========================================================================
    // HPI coil excitation frequencies
    //=========================================================================
    void hpiCoilFreqs_readFromFile()
    {
        if (m_sDataPath.isEmpty())
            QSKIP("Test data not available");

        QFile file(m_sDataPath + "/MEG/sample/test_hpiFit_raw.fif");
        if (!file.exists())
            QSKIP("test_hpiFit_raw.fif not available");

        FiffRawData raw((m_sDataPath + "/MEG/sample/test_hpiFit_raw.fif").toStdString());

        // This file carries a FIFFB_HPI_MEAS block with one FIFFB_HPI_COIL
        // sub block per coil, each holding a FIFF_HPI_COIL_FREQ tag.
        QCOMPARE(raw.info.hpi_coil_freqs.size(), 4);
        QCOMPARE(qRound(raw.info.hpi_coil_freqs.at(0)), 154);
        QCOMPARE(qRound(raw.info.hpi_coil_freqs.at(1)), 158);
        QCOMPARE(qRound(raw.info.hpi_coil_freqs.at(2)), 161);
        QCOMPARE(qRound(raw.info.hpi_coil_freqs.at(3)), 166);
    }

    void hpiCoilFreqs_emptyWithoutHpiBlock()
    {
        if (m_sDataPath.isEmpty())
            QSKIP("Test data not available");

        FiffRawData raw((m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif").toStdString());

        // No HPI measurement block in this file, so the list stays empty
        // rather than being filled with garbage.
        QVERIFY(raw.info.hpi_coil_freqs.empty());
    }

    void cleanupTestCase()
    {
    }
};

QTEST_GUILESS_MAIN(TestFiffInfo)
#include "test_fiff_info.moc"

#include <vector>
