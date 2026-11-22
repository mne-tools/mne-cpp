//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_channel_selection.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     April, 2026
 * @brief    Tests for MNEChSelection, MNEFilterDef, and CovDescription data types.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/mne/mne_ch_selection.h>
#include <mne/mne/mne_types.h>
#include <mne/mne/mne_filter_def.h>
#include <mne/mne/mne_process_description.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

using namespace mne;

//=============================================================================================================
// TEST CLASS
//=============================================================================================================

class TestMNEChannelSelection : public QObject
{
    Q_OBJECT

private slots:

    //=========================================================================================================
    // MNEChSelection tests
    //=========================================================================================================

    void testChSelectionDefaults()
    {
        MNEChSelection sel;
        QCOMPARE(sel.name, QString());
        QVERIFY(sel.chdef.empty());
        QVERIFY(sel.chspick.empty());
        QVERIFY(sel.chspick_nospace.empty());
        QCOMPARE(sel.pick.size(), 0);
        QCOMPARE(sel.pick_deriv.size(), 0);
        QCOMPARE(sel.nderiv(), 0);
        QCOMPARE(sel.ch_kind.size(), 0);
        QCOMPARE(sel.nchan(), 0);
        QCOMPARE(sel.kind, 0);
    }

    void testChSelectionAssignment()
    {
        MNEChSelection sel;
        sel.name = "Frontal";
        sel.chdef.push_back("MEG 01*");
        sel.chdef.push_back("MEG 02*");

        sel.chspick.push_back("MEG 0111");
        sel.chspick.push_back("MEG 0112");
        sel.chspick.push_back("MEG 0211");
        sel.chspick.push_back("MEG 0212");
        sel.chspick_nospace.push_back("MEG 0111");
        sel.chspick_nospace.push_back("MEG 0112");
        sel.chspick_nospace.push_back("MEG 0211");
        sel.chspick_nospace.push_back("MEG 0212");

        sel.pick.resize(4);
        sel.pick << 0, 1, 2, 3;

        sel.ch_kind.resize(4);
        sel.ch_kind.setConstant(1); // FIFFV_MEG_CH kind

        QCOMPARE(sel.name, QString("Frontal"));
        QCOMPARE(sel.chdef.size(), 2);
        QCOMPARE(sel.nchan(), 4);
        QCOMPARE(sel.chspick.size(), 4);
        QCOMPARE(sel.ch_kind.size(), 4);
        QCOMPARE(sel.ch_kind(0), 1);
        QCOMPARE(sel.ch_kind(3), 1);
    }

    void testChSelectionCopy()
    {
        MNEChSelection orig;
        orig.name = "TestSelection";
        orig.chdef.push_back("EEG*");
        orig.chspick = {"EEG 001", "EEG 002", "EEG 003"};

        MNEChSelection copy = orig;
        QCOMPARE(copy.name, QString("TestSelection"));
        QCOMPARE(copy.nchan(), 3);
        QCOMPARE(copy.chdef.size(), 1);
        QCOMPARE(copy.chdef.at(0), QString("EEG*"));

        // Modify copy, verify original unchanged
        copy.name = "Modified";
        QCOMPARE(orig.name, QString("TestSelection"));
    }

    void testChSelectionAssignChannels()
    {
        std::vector<mne::FiffChInfo> chs(4);
        chs[0].ch_name = "MEG0113";
        chs[0].kind = FIFFV_MEG_CH;
        chs[1].ch_name = "EEG 001-REF";
        chs[1].kind = FIFFV_EEG_CH;
        chs[2].ch_name = "eog";
        chs[2].kind = FIFFV_EOG_CH;
        chs[3].ch_name = "STI 014";
        chs[3].kind = FIFFV_STIM_CH;

        // Like MNE-C, a definition matches ignoring case and its own spaces, and by the part of a name before a dash
        auto sel = MNEChSelection::these("fit", StringList{"MEG 0113", "EEG 001", "EOG", "MISSING"});
        QCOMPARE(sel.kind, MNE_CH_SELECTION_USER);
        QCOMPARE(sel.assign_channels(chs), 3);
        QCOMPARE(sel.pick, Eigen::Vector4i(0, 1, 2, -1));
        QCOMPARE(sel.ch_kind, Eigen::Vector4i(FIFFV_MEG_CH, FIFFV_EEG_CH, FIFFV_EOG_CH, -1));
        QCOMPARE(sel.pick_deriv, Eigen::Vector4i::Constant(-1));
        QCOMPARE(sel.nderiv(), 0);

        // The derived channels are those picked from the derivations
        sel.pick_deriv << -1, -1, 0, 1;
        QCOMPARE(sel.nderiv(), 2);
        QCOMPARE(sel.chspick_nospace[0], std::string("MEG0113"));

        // The spaces of the recorded name are kept, as in MNE-C
        chs[0].ch_name = "MEG 0113";
        auto spaceless = MNEChSelection::these("fit", StringList{"MEG0113"});
        QCOMPARE(spaceless.assign_channels(chs), 0);
    }

    //=========================================================================================================
    // MNEFilterDef tests
    //=========================================================================================================

    void testFilterDefDefaults()
    {
        MNEFilterDef filt;
        QCOMPARE(filt.filter_on, false);
        QCOMPARE(filt.size, 0);
        QCOMPARE(filt.taper_size, 0);
        QCOMPARE(filt.highpass, 0.0f);
        QCOMPARE(filt.highpass_width, 0.0f);
        QCOMPARE(filt.lowpass, 0.0f);
        QCOMPARE(filt.lowpass_width, 0.0f);
        QCOMPARE(filt.eog_highpass, 0.0f);
        QCOMPARE(filt.eog_highpass_width, 0.0f);
        QCOMPARE(filt.eog_lowpass, 0.0f);
        QCOMPARE(filt.eog_lowpass_width, 0.0f);
    }

    void testFilterDefBandpass()
    {
        MNEFilterDef filt;
        filt.filter_on = true;
        filt.size = 4096;
        filt.taper_size = 128;
        filt.highpass = 1.0f;
        filt.highpass_width = 0.5f;
        filt.lowpass = 40.0f;
        filt.lowpass_width = 5.0f;

        QCOMPARE(filt.filter_on, true);
        QCOMPARE(filt.size, 4096);
        QCOMPARE(filt.taper_size, 128);
        QVERIFY(qFuzzyCompare(filt.highpass, 1.0f));
        QVERIFY(qFuzzyCompare(filt.lowpass, 40.0f));
    }

    void testFilterDefEogFilter()
    {
        MNEFilterDef filt;
        // EOG channels often need different filter settings
        filt.eog_highpass = 0.1f;
        filt.eog_highpass_width = 0.05f;
        filt.eog_lowpass = 30.0f;
        filt.eog_lowpass_width = 3.0f;

        QVERIFY(qFuzzyCompare(filt.eog_highpass, 0.1f));
        QVERIFY(qFuzzyCompare(filt.eog_lowpass, 30.0f));
        // Verify main filter not affected
        QCOMPARE(filt.highpass, 0.0f);
        QCOMPARE(filt.lowpass, 0.0f);
    }

    //=========================================================================================================
    // CovDefinition / CovDescription tests
    //=========================================================================================================

    void testCovDefinitionDefaults()
    {
        CovDefinition covDef;
        QVERIFY(covDef.events.empty());
        QCOMPARE(covDef.ignore, 0u);
        QCOMPARE(covDef.delay, 0.0f);
        QVERIFY(qFuzzyCompare(covDef.tmin + 1.0f, -0.2f + 1.0f)); // Compare with offset to avoid near-zero issues
        QCOMPARE(covDef.tmax, 0.0f);
        QCOMPARE(covDef.bmin, 0.0f);
        QCOMPARE(covDef.bmax, 0.0f);
        QCOMPARE(covDef.doBaseline, false);
    }

    void testCovDefinitionAssignment()
    {
        CovDefinition covDef;
        covDef.events.push_back(1);
        covDef.events.push_back(2);
        covDef.events.push_back(3);
        covDef.tmin = -0.1f;
        covDef.tmax = 0.5f;
        covDef.bmin = -0.1f;
        covDef.bmax = 0.0f;
        covDef.doBaseline = true;

        QCOMPARE(covDef.events.size(), 3);
        QCOMPARE(covDef.events[0], 1u);
        QCOMPARE(covDef.doBaseline, true);
    }

    void testCovDescriptionDefaults()
    {
        CovDescription covDesc;
        QVERIFY(covDesc.defs.empty());
        QCOMPARE(covDesc.removeSampleMean, true);
        QCOMPARE(covDesc.fixSkew, false);
        QCOMPARE(covDesc.filename, QString());
        QCOMPARE(covDesc.eventFile, QString());
        QCOMPARE(covDesc.logFile, QString());
    }

    void testCovDescriptionMultipleDefs()
    {
        CovDescription covDesc;
        covDesc.filename = "/tmp/cov_out.fif";
        covDesc.removeSampleMean = false;

        CovDefinition def1;
        def1.events.push_back(1);
        def1.tmin = -0.2f;
        def1.tmax = 0.5f;
        def1.doBaseline = true;
        def1.bmin = -0.2f;
        def1.bmax = 0.0f;

        CovDefinition def2;
        def2.events.push_back(2);
        def2.events.push_back(3);
        def2.tmin = -0.1f;
        def2.tmax = 0.3f;

        covDesc.defs.push_back(def1);
        covDesc.defs.push_back(def2);

        QCOMPARE(covDesc.defs.size(), 2);
        QCOMPARE(covDesc.defs[0].events.size(), 1);
        QCOMPARE(covDesc.defs[1].events.size(), 2);
        QCOMPARE(covDesc.removeSampleMean, false);
    }
};

QTEST_GUILESS_MAIN(TestMNEChannelSelection)
#include "test_mne_channel_selection.moc"
