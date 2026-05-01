//=============================================================================================================
/**
 * @file     test_standard_montage.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
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
        QVERIFY(std::abs(pos.x()) < 0.001);  // midline
        QVERIFY(pos.z() > 0.05);              // superior
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

        QVERIFY(std::abs(fp1.x() + fp2.x()) < 0.005);  // x symmetric
        QVERIFY(std::abs(fp1.y() - fp2.y()) < 0.005);   // y same
        QVERIFY(std::abs(fp1.z() - fp2.z()) < 0.005);   // z same
    }

    void testElectrodeCountMethod()
    {
        QCOMPARE(StandardMontage::electrodeCount(StandardMontage::System::Standard_1020), 21);
        QVERIFY(StandardMontage::electrodeCount(StandardMontage::System::Standard_1010) >= 60);
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
