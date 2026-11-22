//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_channels_standard_montage.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    Tests for standard montage definitions.
 */

#include <mne/channels/standard_montage.h>

#include <QtTest>
#include <QObject>
#include <Eigen/Core>
#include <algorithm>
#include <cmath>
#include <set>

using namespace mne;
using namespace Eigen;

class TestStandardMontage : public QObject
{
    Q_OBJECT

private slots:
    void testMontage1020Count()
    {
        QCOMPARE(StandardMontage::montage(StandardMontage::System::Standard_1020).size(), std::size_t(21));
    }

    void testMontage1010Count()
    {
        QCOMPARE(StandardMontage::montage(StandardMontage::System::Standard_1010).size(), std::size_t(73));
    }

    void testElectrodeNames1020()
    {
        const StringList names = StandardMontage::electrode_names(StandardMontage::System::Standard_1020);
        QCOMPARE(names.size(), std::size_t(21));
        for (const char* name : {"Cz", "Fp1", "O2", "A1"})
            QVERIFY(std::ranges::contains(names, name));
    }

    void testElectrodeNames1010()
    {
        const StringList names = StandardMontage::electrode_names(StandardMontage::System::Standard_1010);
        for (const char* name : {"FCz", "CPz", "AF7", "PO8"})
            QVERIFY(std::ranges::contains(names, name));
    }

    void testFindElectrode()
    {
        const auto cz = StandardMontage::find_electrode("Cz");
        QVERIFY(cz);
        QVERIFY(std::abs(cz->x()) < 0.001); // midline
        QVERIFY(cz->z() > 0.05);            // superior
    }

    void testFindElectrodeCaseInsensitive()
    {
        const auto lower = StandardMontage::find_electrode("cz");
        const auto upper = StandardMontage::find_electrode("CZ");
        QVERIFY(lower && upper);
        QCOMPARE(*lower, *upper);
        // The fiducials are in the table, though not in the montage
        QVERIFY(StandardMontage::find_electrode("lpa"));
    }

    void testFindElectrodeNotFound()
    {
        QVERIFY(!StandardMontage::find_electrode("NONEXISTENT"));
        QVERIFY(!StandardMontage::find_electrode(""));
    }

    void testPositionsReasonableRange()
    {
        // All electrodes should be within ~12 cm of origin
        for (const ElectrodePosition& electrode : StandardMontage::montage(StandardMontage::System::Standard_1020)) {
            const double r = electrode.pos.norm();
            QVERIFY2(r > 0.01 && r < 0.15, electrode.name.c_str());
        }
    }

    void testSymmetry()
    {
        // Fp1 and Fp2 should be symmetric about midline
        const auto fp1 = StandardMontage::find_electrode("Fp1");
        const auto fp2 = StandardMontage::find_electrode("Fp2");
        QVERIFY(fp1 && fp2);
        QVERIFY(std::abs(fp1->x() + fp2->x()) < 0.005);
        QVERIFY(std::abs(fp1->y() - fp2->y()) < 0.005);
        QVERIFY(std::abs(fp1->z() - fp2->z()) < 0.005);
    }

    void testElectrodeCountMethod()
    {
        for (const auto system : {StandardMontage::System::Standard_1020, StandardMontage::System::Standard_1010, StandardMontage::System::Standard_1005})
            QCOMPARE(StandardMontage::electrode_count(system), StandardMontage::montage(system).size());
    }

    void testPositionsMatchPython()
    {
        // Reference values produced by mne.channels.make_standard_montage("standard_1005")
        // (mne 1.11.0): 343 electrodes from Fp1 to A2, without the fiducials.
        const std::vector<ElectrodePosition> montage = StandardMontage::montage(StandardMontage::System::Standard_1005);
        QCOMPARE(montage.size(), std::size_t(343));
        QCOMPARE(montage.front().name, std::string("Fp1"));
        QCOMPARE(montage.back().name, std::string("A2"));
        Vector3d sum = Vector3d::Zero();
        for (const ElectrodePosition& electrode : montage)
            sum += electrode.pos;
        QVERIFY((sum - Vector3d(0.23258589999999968, -5.762538499999998, 7.280988999999997)).norm() < 1e-12);

        const std::vector<ElectrodePosition> expected{{"Cz", {0.0004009, -0.009167, 0.100244}},
                                                      {"A1", {-0.0860761, -0.0249897, -0.067986}},
                                                      {"Fp1", {-0.0294367, 0.0839171, -0.00699}},
                                                      {"O2", {0.0298426, -0.112156, 0.0088}}};
        for (const auto& [name, position] : expected) {
            const auto pos = StandardMontage::find_electrode(name);
            QVERIFY(pos);
            QVERIFY2((*pos - position).norm() < 1e-12, name.c_str());
        }
        // The smaller systems are subsets of the same table
        for (const auto system : {StandardMontage::System::Standard_1020, StandardMontage::System::Standard_1010}) {
            for (const ElectrodePosition& electrode : StandardMontage::montage(system)) {
                const auto pos = StandardMontage::find_electrode(electrode.name);
                QVERIFY(pos);
                QCOMPARE(electrode.pos, *pos);
            }
        }
    }

    void testUniqueNames()
    {
        const StringList names = StandardMontage::electrode_names(StandardMontage::System::Standard_1010);
        QCOMPARE(std::set<std::string>(names.begin(), names.end()).size(), names.size());
    }
};

QTEST_GUILESS_MAIN(TestStandardMontage)
#include "test_channels_standard_montage.moc"
