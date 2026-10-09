//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_standard_montage.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    Tests for standard montage definitions.
 */

#include <utils/montage/standard_montage.h>

#include <QtTest>
#include <QObject>
#include <Eigen/Core>
#include <cmath>

using namespace UTILSLIB;
using namespace Eigen;

class TestStandardMontage : public QObject
{
    Q_OBJECT

private slots:
    void testMontage1020Count()
    {
        auto montage = StandardMontage::getMontage(StandardMontage::System::Standard_1020);
        QCOMPARE(montage.size(), 21);
    }

    void testMontage1010Count()
    {
        auto montage = StandardMontage::getMontage(StandardMontage::System::Standard_1010);
        QVERIFY(montage.size() > 21);
        QVERIFY(montage.size() >= 60);
    }

    void testElectrodeNames1020()
    {
        QStringList names = StandardMontage::getElectrodeNames(StandardMontage::System::Standard_1020);
        QCOMPARE(names.size(), 21);
        QVERIFY(names.contains("Cz"));
        QVERIFY(names.contains("Fp1"));
        QVERIFY(names.contains("O2"));
        QVERIFY(names.contains("A1"));
    }

    void testElectrodeNames1010()
    {
        QStringList names = StandardMontage::getElectrodeNames(StandardMontage::System::Standard_1010);
        QVERIFY(names.contains("FCz"));
        QVERIFY(names.contains("CPz"));
        QVERIFY(names.contains("AF7"));
        QVERIFY(names.contains("PO8"));
    }

    void testFindElectrode()
    {
        Vector3d pos;
        QVERIFY(StandardMontage::findElectrode("Cz", pos));
        // Cz should be at midline, superior position
        QVERIFY(std::abs(pos.x()) < 0.001); // midline
        QVERIFY(pos.z() > 0.05);            // superior
    }

    void testFindElectrodeCaseInsensitive()
    {
        Vector3d pos1, pos2;
        QVERIFY(StandardMontage::findElectrode("cz", pos1));
        QVERIFY(StandardMontage::findElectrode("CZ", pos2));
        QVERIFY((pos1 - pos2).norm() < 1e-10);
    }

    void testFindElectrodeNotFound()
    {
        Vector3d pos;
        QVERIFY(!StandardMontage::findElectrode("NONEXISTENT", pos));
    }

    void testPositionsReasonableRange()
    {
        // All electrodes should be within ~12 cm of origin
        auto montage = StandardMontage::getMontage(StandardMontage::System::Standard_1020);
        for (const auto& ep : montage) {
            double r = ep.pos.norm();
            QVERIFY2(r > 0.01 && r < 0.15,
                     qPrintable(QString("%1: r=%2 out of range").arg(ep.name).arg(r)));
        }
    }

    void testSymmetry()
    {
        // Fp1 and Fp2 should be symmetric about midline
        Vector3d fp1, fp2;
        QVERIFY(StandardMontage::findElectrode("Fp1", fp1));
        QVERIFY(StandardMontage::findElectrode("Fp2", fp2));

        QVERIFY(std::abs(fp1.x() + fp2.x()) < 0.005); // x symmetric
        QVERIFY(std::abs(fp1.y() - fp2.y()) < 0.005); // y same
        QVERIFY(std::abs(fp1.z() - fp2.z()) < 0.005); // z same
    }

    void testElectrodeCountMethod()
    {
        QCOMPARE(StandardMontage::electrodeCount(StandardMontage::System::Standard_1020), 21);
        QVERIFY(StandardMontage::electrodeCount(StandardMontage::System::Standard_1010) >= 60);
    }

    void testPositionsMatchPython()
    {
        // Reference values produced by mne.channels.make_standard_montage("standard_1005")
        // (mne 1.11.0): 343 electrodes from Fp1 to A2, without the fiducials.
        const QList<ElectrodePosition> montage = StandardMontage::getMontage(StandardMontage::System::Standard_1005);
        QCOMPARE(montage.size(), 343);
        QCOMPARE(montage.first().name, QStringLiteral("Fp1"));
        QCOMPARE(montage.last().name, QStringLiteral("A2"));
        Vector3d sum = Vector3d::Zero();
        for (const ElectrodePosition& electrode : montage)
            sum += electrode.pos;
        QVERIFY((sum - Vector3d(0.23258589999999968, -5.762538499999998, 7.280988999999997)).norm() < 1e-12);

        const QList<QPair<QString, Vector3d>> expected{{"Cz", {0.0004009, -0.009167, 0.100244}},
                                                       {"A1", {-0.0860761, -0.0249897, -0.067986}},
                                                       {"Fp1", {-0.0294367, 0.0839171, -0.00699}},
                                                       {"O2", {0.0298426, -0.112156, 0.0088}}};
        for (const auto& [name, position] : expected) {
            Vector3d pos;
            QVERIFY(StandardMontage::findElectrode(name, pos));
            QVERIFY2((pos - position).norm() < 1e-12, qPrintable(name));
        }
        // The smaller systems are subsets of the same table
        for (const auto system : {StandardMontage::System::Standard_1020, StandardMontage::System::Standard_1010}) {
            for (const ElectrodePosition& electrode : StandardMontage::getMontage(system)) {
                Vector3d pos;
                QVERIFY(StandardMontage::findElectrode(electrode.name, pos));
                QCOMPARE(electrode.pos, pos);
            }
        }
        QCOMPARE(StandardMontage::electrodeCount(StandardMontage::System::Standard_1010), 73);
    }

    void testUniqueNames()
    {
        QStringList names = StandardMontage::getElectrodeNames(StandardMontage::System::Standard_1010);
        QSet<QString> unique(names.begin(), names.end());
        QCOMPARE(unique.size(), names.size());
    }
};

QTEST_GUILESS_MAIN(TestStandardMontage)
#include "test_standard_montage.moc"
