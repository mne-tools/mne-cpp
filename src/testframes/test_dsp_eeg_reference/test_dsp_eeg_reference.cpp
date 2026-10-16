//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_dsp_eeg_reference.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    Tests for EEG re-referencing functions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/eeg_reference.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_file.h>

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
            ch.unit = 107; // FIFF_UNIT_V
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
        QVERIFY(data.row(0).isApprox(RowVectorXd::Constant(10, 30.0), 1e-10));  // EEG3
        QVERIFY(data.row(1).isApprox(RowVectorXd::Constant(10, 100.0), 1e-10)); // MEG1
        QVERIFY(data.row(2).isApprox(RowVectorXd::Constant(10, 200.0), 1e-10)); // MEG2
        QVERIFY(data.row(3).isApprox(RowVectorXd::Constant(10, -10.0), 1e-10)); // EEG1-EEG2
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
        // EEG3 is bad and, as in mne.set_eeg_reference, left alone
        QVERIFY(data.row(2).isApprox(RowVectorXd::Constant(10, 30.0), 1e-10));
        // MEG unchanged
        QVERIFY(data.row(3).isApprox(RowVectorXd::Constant(10, 100.0), 1e-10));
    }

    //=========================================================================================================
    /**
     * @brief Re-referenced data equals MNE-Python's.
     *
     * Reference values produced by mne 1.11, column 2 of raw.get_data():
     *   X = np.vstack([np.sin(np.arange(6) + k) * (k + 1) for k in range(5)])
     *   info = mne.create_info(["E1", "E2", "E3", "E4", "M1"], 100.0, ["eeg"] * 4 + ["mag"]); info["bads"] = ["E3"]
     *   raw.set_eeg_reference(ref), mne.add_reference_channels(raw, ["REF"]),
     *   mne.set_bipolar_reference(raw, ["E1", "E2"], ["E2", "E4"]) and (..., ["E1"], ["E4"], drop_refs=False)
     */
    void testMatchesMnePython_data()
    {
        QTest::addColumn<QString>("op");
        QTest::addColumn<QStringList>("names");
        QTest::addColumn<QList<double>>("expected");
        QTest::newRow("average") << "average" << QStringList{"E1", "E2", "E3", "E4", "M1"}
                                 << QList<double>{1.790683979, 1.163626568, -2.270407486, -2.954310547, -1.397077491};
        QTest::newRow("E2") << "E2" << QStringList{"E1", "E2", "E3", "E4", "M1"}
                            << QList<double>{0.6270574107, 0.0, -2.270407486, -4.117937115, -1.397077491};
        QTest::newRow("E1+E4") << "E1,E4" << QStringList{"E1", "E2", "E3", "E4", "M1"}
                               << QList<double>{2.372497263, 1.745439852, -2.270407486, -2.372497263, -1.397077491};
        QTest::newRow("add REF") << "addref" << QStringList{"E1", "E2", "E3", "E4", "M1", "REF"}
                                 << QList<double>{0.9092974268, 0.2822400161, -2.270407486, -3.835697099, -1.397077491, 0.0};
        QTest::newRow("bipolar") << "bipolar" << QStringList{"E3", "M1", "E1-E2", "E2-E4"}
                                 << QList<double>{-2.270407486, -1.397077491, 0.6270574107, 4.117937115};
        QTest::newRow("bipolar keep") << "bipolarkeep" << QStringList{"E1", "E2", "E3", "E4", "M1", "E1-E4"}
                                      << QList<double>{0.9092974268, 0.2822400161, -2.270407486, -3.835697099, -1.397077491, 4.744994525};
    }

    void testMatchesMnePython()
    {
        QFETCH(QString, op);
        QFETCH(QStringList, names);
        QFETCH(QList<double>, expected);

        FiffInfo info;
        info.sfreq = 100.0;
        const QStringList chNames = {"E1", "E2", "E3", "E4", "M1"};
        for (int i = 0; i < chNames.size(); ++i) {
            FiffChInfo ch;
            ch.scanNo = ch.logNo = i + 1;
            ch.kind = i < 4 ? FIFFV_EEG_CH : FIFFV_MEG_CH;
            ch.ch_name = chNames[i];
            info.chs.append(ch);
            info.ch_names.append(chNames[i]);
        }
        info.nchan = chNames.size();
        info.bads = {"E3"};
        MatrixXd data(5, 6);
        for (int k = 0; k < 5; ++k)
            for (int t = 0; t < 6; ++t)
                data(k, t) = std::sin(t + k) * (k + 1);

        if (op == "addref")
            addReferenceChannels(data, info, {"REF"});
        else if (op == "bipolar")
            setBipolarReference(data, info, {"E1", "E2"}, {"E2", "E4"});
        else if (op == "bipolarkeep")
            setBipolarReference(data, info, {"E1"}, {"E4"}, false);
        else
            setEegReference(data, info, op.split(','));

        QCOMPARE(info.ch_names, names);
        QCOMPARE(data.rows(), static_cast<Eigen::Index>(expected.size()));
        for (int i = 0; i < expected.size(); ++i)
            QVERIFY2(std::abs(data(i, 2) - expected[i]) <= 1e-9,
                     qPrintable(QString("%1: %2 vs mne %3").arg(names[i]).arg(data(i, 2), 0, 'g', 10).arg(expected[i], 0, 'g', 10)));
    }

    //=========================================================================================================
    /**
     * @brief projection=true adds the projector mne.set_eeg_reference("average", projection=True) adds:
     * desc "Average EEG reference", kind 10, inactive, good EEG columns at 1/sqrt(n). The data is unchanged,
     * a second call adds nothing and a channel reference is refused.
     */
    void testAverageProjector()
    {
        FiffInfo info = makeTestInfo();
        info.bads.append("EEG3");
        MatrixXd data = makeTestData();
        const MatrixXd orig = data;

        setEegReference(data, info, QStringList(), true);
        setEegReference(data, info, QStringList(), true);
        setEegReference(data, info, {"EEG1"}, true);

        QCOMPARE(data, orig);
        QCOMPARE(info.projs.size(), 1);
        const FiffProj& proj = info.projs.first();
        QCOMPARE(proj.desc, QString("Average EEG reference"));
        QCOMPARE(proj.kind, static_cast<fiff_int_t>(FIFFV_PROJ_ITEM_EEG_AVREF));
        QVERIFY(!proj.active);
        QCOMPARE(proj.data->col_names, QStringList({"EEG1", "EEG2"}));
        QVERIFY(proj.data->data.isApprox(MatrixXd::Constant(1, 2, 1.0 / std::sqrt(2.0))));
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
