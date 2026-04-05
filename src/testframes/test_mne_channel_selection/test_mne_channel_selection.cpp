//=============================================================================================================
/**
 * @file     test_mne_channel_selection.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Tests for MNEChSelection, MNEFilterDef, and CovDescription data types.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtTest>

#include <mne/mne_ch_selection.h>
#include <mne/mne_filter_def.h>
#include <mne/mne_process_description.h>

using namespace MNELIB;

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
        QVERIFY(sel.chdef.isEmpty());
        QCOMPARE(sel.ndef, 0);
        QVERIFY(sel.chspick.isEmpty());
        QVERIFY(sel.chspick_nospace.isEmpty());
        QCOMPARE(sel.pick.size(), 0);
        QCOMPARE(sel.pick_deriv.size(), 0);
        QCOMPARE(sel.nderiv, 0);
        QCOMPARE(sel.ch_kind.size(), 0);
        QCOMPARE(sel.nchan, 0);
        QCOMPARE(sel.kind, 0);
    }

    void testChSelectionAssignment()
    {
        MNEChSelection sel;
        sel.name = "Frontal";
        sel.chdef << "MEG 01*" << "MEG 02*";
        sel.ndef = 2;

        sel.chspick << "MEG 0111" << "MEG 0112" << "MEG 0211" << "MEG 0212";
        sel.chspick_nospace << "MEG0111" << "MEG0112" << "MEG0211" << "MEG0212";
        sel.nchan = 4;

        sel.pick.resize(4);
        sel.pick << 0, 1, 2, 3;

        sel.ch_kind.resize(4);
        sel.ch_kind.setConstant(1); // FIFFV_MEG_CH kind

        QCOMPARE(sel.name, QString("Frontal"));
        QCOMPARE(sel.ndef, 2);
        QCOMPARE(sel.nchan, 4);
        QCOMPARE(sel.chspick.size(), 4);
        QCOMPARE(sel.ch_kind.size(), 4);
        QCOMPARE(sel.ch_kind(0), 1);
        QCOMPARE(sel.ch_kind(3), 1);
    }

    void testChSelectionCopy()
    {
        MNEChSelection orig;
        orig.name = "TestSelection";
        orig.chdef << "EEG*";
        orig.ndef = 1;
        orig.nchan = 3;

        MNEChSelection copy = orig;
        QCOMPARE(copy.name, QString("TestSelection"));
        QCOMPARE(copy.ndef, 1);
        QCOMPARE(copy.nchan, 3);
        QCOMPARE(copy.chdef.size(), 1);
        QCOMPARE(copy.chdef.at(0), QString("EEG*"));

        // Modify copy, verify original unchanged
        copy.name = "Modified";
        QCOMPARE(orig.name, QString("TestSelection"));
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
        QVERIFY(covDef.events.isEmpty());
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
        covDef.events << 1 << 2 << 3;
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
        QVERIFY(covDesc.defs.isEmpty());
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
        def1.events << 1;
        def1.tmin = -0.2f;
        def1.tmax = 0.5f;
        def1.doBaseline = true;
        def1.bmin = -0.2f;
        def1.bmax = 0.0f;

        CovDefinition def2;
        def2.events << 2 << 3;
        def2.tmin = -0.1f;
        def2.tmax = 0.3f;

        covDesc.defs.append(def1);
        covDesc.defs.append(def2);

        QCOMPARE(covDesc.defs.size(), 2);
        QCOMPARE(covDesc.defs[0].events.size(), 1);
        QCOMPARE(covDesc.defs[1].events.size(), 2);
        QCOMPARE(covDesc.removeSampleMean, false);
    }
};

QTEST_GUILESS_MAIN(TestMNEChannelSelection)
#include "test_mne_channel_selection.moc"
