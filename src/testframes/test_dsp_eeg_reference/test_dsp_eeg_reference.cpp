//=============================================================================================================
/**
 * @file     test_dsp_eeg_reference.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    Tests for EEG re-referencing functions.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/eeg_reference.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Test class for EEG re-referencing functions.
 */
class TestDspEegReference : public QObject
{
    Q_OBJECT

private:
    /**
     * @brief Build a small FiffInfo with a mix of EEG and MEG channels.
     *
     * Layout: 5 channels total
     *   [0] EEG1 (EEG)
     *   [1] EEG2 (EEG)
     *   [2] EEG3 (EEG)
     *   [3] MEG1 (MEG)
     *   [4] MEG2 (MEG)
     */
    static FiffInfo makeTestInfo()
    {
        FiffInfo info;
        info.nchan = 5;
        info.sfreq = 1000.0;

        QStringList names = {"EEG1", "EEG2", "EEG3", "MEG1", "MEG2"};
        QList<int> kinds = {FIFFV_EEG_CH, FIFFV_EEG_CH, FIFFV_EEG_CH,
                            FIFFV_MEG_CH, FIFFV_MEG_CH};

        for (int i = 0; i < 5; ++i) {
            FiffChInfo ch;
            ch.scanNo = i + 1;
            ch.logNo = i + 1;
            ch.kind = kinds[i];
            ch.ch_name = names[i];
            ch.range = 1.0f;
            ch.cal = 1.0f;
            ch.unit = 107;  // FIFF_UNIT_V
            ch.unit_mul = 0;
            ch.coord_frame = 0;
            info.chs.append(ch);
            info.ch_names.append(names[i]);
        }

        return info;
    }

    /**
     * @brief Build a 5x10 test data matrix with known values.
     *
     * Row 0 (EEG1): all 10.0
     * Row 1 (EEG2): all 20.0
     * Row 2 (EEG3): all 30.0
     * Row 3 (MEG1): all 100.0
     * Row 4 (MEG2): all 200.0
     */
    static MatrixXd makeTestData()
    {
        MatrixXd data(5, 10);
        data.row(0).setConstant(10.0);
        data.row(1).setConstant(20.0);
        data.row(2).setConstant(30.0);
        data.row(3).setConstant(100.0);
        data.row(4).setConstant(200.0);
        return data;
    }

private slots:
    //=========================================================================================================
    /**
     * @brief Average reference subtracts EEG mean from all EEG channels.
     */
    void testAverageReference()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();

        // EEG mean = (10 + 20 + 30) / 3 = 20
        setEegReference(data, info);

        // EEG1: 10 - 20 = -10
        QVERIFY(data.row(0).isApprox(RowVectorXd::Constant(10, -10.0), 1e-10));
        // EEG2: 20 - 20 = 0
        QVERIFY(data.row(1).isApprox(RowVectorXd::Constant(10, 0.0), 1e-10));
        // EEG3: 30 - 20 = 10
        QVERIFY(data.row(2).isApprox(RowVectorXd::Constant(10, 10.0), 1e-10));
    }

    //=========================================================================================================
    /**
     * @brief Non-EEG channels are unchanged after average re-referencing.
     */
    void testAverageReferencePreservesNonEeg()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();

        setEegReference(data, info);

        // MEG channels unchanged
        QVERIFY(data.row(3).isApprox(RowVectorXd::Constant(10, 100.0), 1e-10));
        QVERIFY(data.row(4).isApprox(RowVectorXd::Constant(10, 200.0), 1e-10));
    }

    //=========================================================================================================
    /**
     * @brief Re-reference to a single named channel.
     */
    void testSpecificChannelReference()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();

        // Re-reference to EEG2 (value 20)
        setEegReference(data, info, QStringList{"EEG2"});

        // EEG1: 10 - 20 = -10
        QVERIFY(data.row(0).isApprox(RowVectorXd::Constant(10, -10.0), 1e-10));
        // EEG2: 20 - 20 = 0
        QVERIFY(data.row(1).isApprox(RowVectorXd::Constant(10, 0.0), 1e-10));
        // EEG3: 30 - 20 = 10
        QVERIFY(data.row(2).isApprox(RowVectorXd::Constant(10, 10.0), 1e-10));
        // MEG unchanged
        QVERIFY(data.row(3).isApprox(RowVectorXd::Constant(10, 100.0), 1e-10));
    }

    //=========================================================================================================
    /**
     * @brief Re-reference to mean of multiple named channels.
     */
    void testMultipleChannelReference()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();

        // Re-reference to mean of EEG1 and EEG3: (10+30)/2 = 20
        setEegReference(data, info, QStringList{"EEG1", "EEG3"});

        // EEG1: 10 - 20 = -10
        QVERIFY(data.row(0).isApprox(RowVectorXd::Constant(10, -10.0), 1e-10));
        // EEG2: 20 - 20 = 0
        QVERIFY(data.row(1).isApprox(RowVectorXd::Constant(10, 0.0), 1e-10));
        // EEG3: 30 - 20 = 10
        QVERIFY(data.row(2).isApprox(RowVectorXd::Constant(10, 10.0), 1e-10));
    }

    //=========================================================================================================
    /**
     * @brief addReferenceChannels adds zero rows and updates info.
     */
    void testAddReferenceChannels()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();

        addReferenceChannels(data, info, QStringList{"REF1", "REF2"});

        // Data should have 7 rows now
        QCOMPARE(data.rows(), static_cast<Eigen::Index>(7));
        QCOMPARE(data.cols(), static_cast<Eigen::Index>(10));

        // Original rows unchanged
        QVERIFY(data.row(0).isApprox(RowVectorXd::Constant(10, 10.0), 1e-10));
        QVERIFY(data.row(4).isApprox(RowVectorXd::Constant(10, 200.0), 1e-10));

        // New rows are zeros
        QVERIFY(data.row(5).isApprox(RowVectorXd::Zero(10), 1e-10));
        QVERIFY(data.row(6).isApprox(RowVectorXd::Zero(10), 1e-10));

        // Info updated
        QCOMPARE(info.nchan, 7);
        QCOMPARE(info.chs.size(), 7);
        QCOMPARE(info.ch_names.size(), 7);
        QCOMPARE(info.ch_names[5], QString("REF1"));
        QCOMPARE(info.ch_names[6], QString("REF2"));
        QCOMPARE(info.chs[5].kind, static_cast<fiff_int_t>(FIFFV_EEG_CH));
        QCOMPARE(info.chs[6].kind, static_cast<fiff_int_t>(FIFFV_EEG_CH));
    }

    //=========================================================================================================
    /**
     * @brief setBipolarReference computes anode - cathode.
     */
    void testBipolarReference()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();

        // EEG1 - EEG2: 10 - 20 = -10
        setBipolarReference(data, info,
                            QStringList{"EEG1"},
                            QStringList{"EEG2"},
                            false);

        // With dropOriginals=false, original rows are kept plus one bipolar row
        QCOMPARE(data.rows(), static_cast<Eigen::Index>(6));

        // Bipolar row at the end: 10 - 20 = -10
        QVERIFY(data.row(5).isApprox(RowVectorXd::Constant(10, -10.0), 1e-10));

        // Bipolar channel name
        QCOMPARE(info.ch_names.last(), QString("EEG1-EEG2"));
        QCOMPARE(info.nchan, 6);
    }

    //=========================================================================================================
    /**
     * @brief setBipolarReference with dropOriginals removes anode/cathode channels.
     */
    void testBipolarReferenceDropOriginals()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();

        // EEG1 - EEG2 with drop originals
        setBipolarReference(data, info,
                            QStringList{"EEG1"},
                            QStringList{"EEG2"},
                            true);

        // 5 original - 2 dropped + 1 bipolar = 4 rows
        QCOMPARE(data.rows(), static_cast<Eigen::Index>(4));
        QCOMPARE(info.nchan, 4);

        // Remaining channels: EEG3, MEG1, MEG2, EEG1-EEG2
        QCOMPARE(info.ch_names[0], QString("EEG3"));
        QCOMPARE(info.ch_names[1], QString("MEG1"));
        QCOMPARE(info.ch_names[2], QString("MEG2"));
        QCOMPARE(info.ch_names[3], QString("EEG1-EEG2"));

        // Verify data values
        QVERIFY(data.row(0).isApprox(RowVectorXd::Constant(10, 30.0), 1e-10));   // EEG3
        QVERIFY(data.row(1).isApprox(RowVectorXd::Constant(10, 100.0), 1e-10));  // MEG1
        QVERIFY(data.row(2).isApprox(RowVectorXd::Constant(10, 200.0), 1e-10));  // MEG2
        QVERIFY(data.row(3).isApprox(RowVectorXd::Constant(10, -10.0), 1e-10));  // EEG1-EEG2
    }

    //=========================================================================================================
    /**
     * @brief setBipolarReference with dropOriginals=false keeps originals.
     */
    void testBipolarReferencekeepOriginals()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();

        setBipolarReference(data, info,
                            QStringList{"EEG2"},
                            QStringList{"EEG3"},
                            false);

        // 5 original + 1 bipolar = 6 rows
        QCOMPARE(data.rows(), static_cast<Eigen::Index>(6));
        QCOMPARE(info.nchan, 6);

        // Original rows preserved
        QVERIFY(data.row(0).isApprox(RowVectorXd::Constant(10, 10.0), 1e-10));
        QVERIFY(data.row(1).isApprox(RowVectorXd::Constant(10, 20.0), 1e-10));
        QVERIFY(data.row(2).isApprox(RowVectorXd::Constant(10, 30.0), 1e-10));

        // Bipolar row: EEG2 - EEG3 = 20 - 30 = -10
        QVERIFY(data.row(5).isApprox(RowVectorXd::Constant(10, -10.0), 1e-10));
        QCOMPARE(info.ch_names[5], QString("EEG2-EEG3"));
    }

    //=========================================================================================================
    /**
     * @brief Bad channels are excluded from average reference computation.
     */
    void testBadChannelsExcluded()
    {
        FiffInfo info = makeTestInfo();
        MatrixXd data = makeTestData();

        // Mark EEG3 as bad
        info.bads.append("EEG3");

        // Average ref should use only EEG1 and EEG2: (10 + 20) / 2 = 15
        setEegReference(data, info);

        // EEG1: 10 - 15 = -5
        QVERIFY(data.row(0).isApprox(RowVectorXd::Constant(10, -5.0), 1e-10));
        // EEG2: 20 - 15 = 5
        QVERIFY(data.row(1).isApprox(RowVectorXd::Constant(10, 5.0), 1e-10));
        // EEG3 is bad but still gets re-referenced: 30 - 15 = 15
        QVERIFY(data.row(2).isApprox(RowVectorXd::Constant(10, 15.0), 1e-10));
        // MEG unchanged
        QVERIFY(data.row(3).isApprox(RowVectorXd::Constant(10, 100.0), 1e-10));
    }

    //=========================================================================================================
    /**
     * @brief Edge case: empty data matrix does not crash.
     */
    void testEmptyData()
    {
        FiffInfo info;
        info.nchan = 0;
        MatrixXd data(0, 0);

        // Should not crash
        setEegReference(data, info);
        QCOMPARE(data.rows(), static_cast<Eigen::Index>(0));
        QCOMPARE(data.cols(), static_cast<Eigen::Index>(0));

        addReferenceChannels(data, info, QStringList{});
        QCOMPARE(data.rows(), static_cast<Eigen::Index>(0));

        setBipolarReference(data, info, QStringList{}, QStringList{});
        QCOMPARE(data.rows(), static_cast<Eigen::Index>(0));
    }
};

//=============================================================================================================

QTEST_MAIN(TestDspEegReference)

//=============================================================================================================

#include "test_dsp_eeg_reference.moc"
