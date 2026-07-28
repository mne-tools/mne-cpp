//=============================================================================================================
/**
 * @file     test_polhemus_coregistration.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.9
 * @date     July, 2026
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
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    Tests for PolhemusCoregistration.
 *
 * The registration is a Kabsch / Procrustes fit of three fiducials, which has
 * a property that makes it checkable without any hardware: if the model points
 * are produced from the pen points by a known rigid transform, the fit has to
 * recover exactly that transform. Every expectation here is derived that way
 * rather than from what the code currently returns, so the test can fail.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/polhemus/polhemus_coregistration.h>
#include <utils/polhemus/acquired_points.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QSettings>
#include <QTemporaryDir>
#include <QVector3D>
#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestPolhemusCoregistration
 *
 * @brief The TestPolhemusCoregistration class provides tests for PolhemusCoregistration.
 */
class TestPolhemusCoregistration : public QObject
{
    Q_OBJECT

public:
    TestPolhemusCoregistration() = default;

private:
    //=========================================================================================================
    /**
     * Loads a coregistration with the given pen and model fiducials.
     *
     * Fiducials normally arrive from the tracker, so they are injected through
     * the session-restore path, which is the only hardware free way in and is
     * itself worth covering.
     */
    static void seedFiducials(PolhemusCoregistration& coreg,
                              QSettings& settings,
                              const QVector3D& penLpa,
                              const QVector3D& penNas,
                              const QVector3D& penRpa,
                              const QVector3D& modelLpa,
                              const QVector3D& modelNas,
                              const QVector3D& modelRpa);

    //=========================================================================================================
    /**
     * Writes a QVector3D the way saveSessionState does, so restoreSessionState
     * reads it back.
     */
    static void writeVec3(QSettings& settings, const QString& key, const QVector3D& v);

private slots:
    void registration_recoversKnownTransform_data();
    void registration_recoversKnownTransform();

    void registration_rejectsDegenerateFiducials_data();
    void registration_rejectsDegenerateFiducials();

    void registration_requiresAllFiducials();

    void sessionState_roundTrip();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

void TestPolhemusCoregistration::writeVec3(QSettings& settings, const QString& key, const QVector3D& v)
{
    settings.setValue(key + "/x", v.x());
    settings.setValue(key + "/y", v.y());
    settings.setValue(key + "/z", v.z());
}

//=============================================================================================================

void TestPolhemusCoregistration::seedFiducials(PolhemusCoregistration& coreg,
                                               QSettings& settings,
                                               const QVector3D& penLpa,
                                               const QVector3D& penNas,
                                               const QVector3D& penRpa,
                                               const QVector3D& modelLpa,
                                               const QVector3D& modelNas,
                                               const QVector3D& modelRpa)
{
    // Index 0..2 of the stored arrays, named the way the persistence code
    // names them. Note these labels do not line up with FiducialId, which
    // starts at LPA = 1; save and load agree with each other, which is what
    // matters and what sessionState_roundTrip pins down.
    settings.setValue("hasPenFid/LPA", false);
    settings.setValue("hasPenFid/NAS", true);
    settings.setValue("hasPenFid/RPA", true);
    settings.setValue("hasPenFid/CZ",  true);
    writeVec3(settings, "penFid/NAS", penLpa);
    writeVec3(settings, "penFid/RPA", penNas);
    writeVec3(settings, "penFid/CZ",  penRpa);

    settings.setValue("hasModelFid/LPA", false);
    settings.setValue("hasModelFid/NAS", true);
    settings.setValue("hasModelFid/RPA", true);
    settings.setValue("hasModelFid/CZ",  true);
    writeVec3(settings, "modelFid/NAS", modelLpa);
    writeVec3(settings, "modelFid/RPA", modelNas);
    writeVec3(settings, "modelFid/CZ",  modelRpa);

    settings.sync();

    // restoreSessionState reports whether a valid registration came back, not
    // whether the file was read, so a settings blob that only carries
    // fiducials returns false while still loading them. That is the contract,
    // hence no QVERIFY on the return here.
    coreg.restoreSessionState(settings, QString());
    QVERIFY2(coreg.hasAllPenFiducials(),
             "fiducials did not load, the rest of the test would be meaningless");
}

//=============================================================================================================

void TestPolhemusCoregistration::registration_recoversKnownTransform_data()
{
    QTest::addColumn<QVector3D>("axis");
    QTest::addColumn<float>("angleDeg");
    QTest::addColumn<QVector3D>("translation");

    // A rigid transform applied to the pen fiducials to make the model ones.
    // Whatever the fit produces has to map pen onto model again, so each row
    // is an independent check of the Kabsch implementation.
    QTest::newRow("identity")      << QVector3D(0, 0, 1) <<   0.0f << QVector3D(0.0f,   0.0f,   0.0f);
    QTest::newRow("yaw 30")        << QVector3D(0, 0, 1) <<  30.0f << QVector3D(0.0f,   0.0f,   0.0f);
    QTest::newRow("pitch 45")      << QVector3D(1, 0, 0) <<  45.0f << QVector3D(0.0f,   0.0f,   0.0f);
    QTest::newRow("roll 90")       << QVector3D(0, 1, 0) <<  90.0f << QVector3D(0.0f,   0.0f,   0.0f);
    QTest::newRow("translation")   << QVector3D(0, 0, 1) <<   0.0f << QVector3D(0.05f, -0.03f,  0.10f);
    QTest::newRow("yaw + shift")   << QVector3D(0, 0, 1) <<  60.0f << QVector3D(0.02f,  0.04f, -0.01f);
    QTest::newRow("oblique axis")  << QVector3D(1, 1, 1) << 120.0f << QVector3D(-0.07f, 0.01f,  0.03f);
    QTest::newRow("near 180")      << QVector3D(0, 1, 0) << 179.0f << QVector3D(0.0f,   0.0f,   0.0f);
}

//=============================================================================================================

void TestPolhemusCoregistration::registration_recoversKnownTransform()
{
    QFETCH(QVector3D, axis);
    QFETCH(float, angleDeg);
    QFETCH(QVector3D, translation);

    // Head scale fiducials in metres, well spread so the degeneracy guard
    // does not trip.
    const QVector3D penLpa(-0.075f,  0.000f, 0.000f);
    const QVector3D penNas( 0.000f,  0.095f, 0.000f);
    const QVector3D penRpa( 0.075f,  0.000f, 0.000f);

    QMatrix4x4 expected;
    expected.translate(translation);
    expected.rotate(QQuaternion::fromAxisAndAngle(axis.normalized(), angleDeg));

    const QVector3D modelLpa = expected.map(penLpa);
    const QVector3D modelNas = expected.map(penNas);
    const QVector3D modelRpa = expected.map(penRpa);

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QSettings settings(tmpDir.path() + "/coreg.ini", QSettings::IniFormat);

    PolhemusCoregistration coreg;
    seedFiducials(coreg, settings, penLpa, penNas, penRpa, modelLpa, modelNas, modelRpa);

    QVERIFY(coreg.computeRegistration());
    QVERIFY(coreg.registrationValid());

    // The paired SVD path writes its result into worldToModel: that is the
    // pen-to-model fit. headToDevice is a different quantity, derived from
    // the tracker's device-to-world transform, and stays identity here since
    // no tracker is attached.
    //
    // The fit is only meaningful through what it does to points, so check the
    // mapping rather than the matrix entries, which lets an equally valid but
    // differently expressed transform pass.
    const QMatrix4x4 actual = coreg.worldToModel();

    struct { const char* name; QVector3D from; QVector3D to; } cases[] = {
        {"LPA", penLpa, modelLpa},
        {"NAS", penNas, modelNas},
        {"RPA", penRpa, modelRpa}
    };

    for(const auto& c : cases) {
        const QVector3D mapped = actual.map(c.from);
        const float err = (mapped - c.to).length();
        QVERIFY2(err < 1.0e-4f,
                 qPrintable(QString("%1 maps to (%2, %3, %4), expected (%5, %6, %7), error %8 m")
                            .arg(c.name)
                            .arg(mapped.x()).arg(mapped.y()).arg(mapped.z())
                            .arg(c.to.x()).arg(c.to.y()).arg(c.to.z())
                            .arg(err)));
    }

    // A rigid transform preserves distances. This catches a fit that happens
    // to land the three fiducials while scaling or shearing everything else,
    // which the three point checks above cannot see on their own.
    const QVector3D probe(0.01f, 0.02f, 0.03f);
    const QVector3D probe2(-0.04f, 0.01f, 0.05f);
    const float distBefore = (probe - probe2).length();
    const float distAfter = (actual.map(probe) - actual.map(probe2)).length();
    QVERIFY2(std::fabs(distBefore - distAfter) < 1.0e-5f,
             qPrintable(QString("transform is not rigid: %1 m became %2 m")
                        .arg(distBefore).arg(distAfter)));
}

//=============================================================================================================

void TestPolhemusCoregistration::registration_rejectsDegenerateFiducials_data()
{
    QTest::addColumn<QVector3D>("lpa");
    QTest::addColumn<QVector3D>("nas");
    QTest::addColumn<QVector3D>("rpa");
    QTest::addColumn<bool>("expectSuccess");

    // The guard rejects fiducials closer than 20 mm to each other, because a
    // fit from points that close is dominated by digitizer noise.
    QTest::newRow("well spread")
        << QVector3D(-0.075f, 0.0f, 0.0f) << QVector3D(0.0f, 0.095f, 0.0f) << QVector3D(0.075f, 0.0f, 0.0f)
        << true;
    QTest::newRow("just above 20 mm")
        << QVector3D(-0.011f, 0.0f, 0.0f) << QVector3D(0.0f, 0.021f, 0.0f) << QVector3D(0.011f, 0.0f, 0.0f)
        << true;
    QTest::newRow("all coincident")
        << QVector3D(0.0f, 0.0f, 0.0f) << QVector3D(0.0f, 0.0f, 0.0f) << QVector3D(0.0f, 0.0f, 0.0f)
        << false;
    QTest::newRow("two coincident")
        << QVector3D(-0.075f, 0.0f, 0.0f) << QVector3D(0.075f, 0.0f, 0.0f) << QVector3D(0.075f, 0.0f, 0.0f)
        << false;
    QTest::newRow("5 mm apart")
        << QVector3D(-0.005f, 0.0f, 0.0f) << QVector3D(0.0f, 0.005f, 0.0f) << QVector3D(0.005f, 0.0f, 0.0f)
        << false;
}

//=============================================================================================================

void TestPolhemusCoregistration::registration_rejectsDegenerateFiducials()
{
    QFETCH(QVector3D, lpa);
    QFETCH(QVector3D, nas);
    QFETCH(QVector3D, rpa);
    QFETCH(bool, expectSuccess);

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QSettings settings(tmpDir.path() + "/coreg.ini", QSettings::IniFormat);

    PolhemusCoregistration coreg;
    // Model side identical to the pen side, so only the spread decides.
    seedFiducials(coreg, settings, lpa, nas, rpa, lpa, nas, rpa);

    QCOMPARE(coreg.computeRegistration(), expectSuccess);
    QCOMPARE(coreg.registrationValid(), expectSuccess);
}

//=============================================================================================================

void TestPolhemusCoregistration::registration_requiresAllFiducials()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QSettings settings(tmpDir.path() + "/coreg.ini", QSettings::IniFormat);

    PolhemusCoregistration coreg;

    // Nothing captured at all.
    QVERIFY(!coreg.computeRegistration());

    // Only two of the three captured, which is not enough to fix a rigid
    // transform and must be refused rather than fitted.
    settings.setValue("hasPenFid/NAS", true);
    settings.setValue("hasPenFid/RPA", true);
    settings.setValue("hasPenFid/CZ", false);
    writeVec3(settings, "penFid/NAS", QVector3D(-0.075f, 0.0f, 0.0f));
    writeVec3(settings, "penFid/RPA", QVector3D(0.0f, 0.095f, 0.0f));
    settings.sync();

    coreg.restoreSessionState(settings, QString());
    QVERIFY(!coreg.hasAllPenFiducials());
    QVERIFY(!coreg.computeRegistration());
}

//=============================================================================================================

void TestPolhemusCoregistration::sessionState_roundTrip()
{
    const QVector3D penLpa(-0.075f,  0.001f, 0.002f);
    const QVector3D penNas( 0.003f,  0.095f, 0.004f);
    const QVector3D penRpa( 0.075f,  0.005f, 0.006f);

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QSettings writeSettings(tmpDir.path() + "/coreg.ini", QSettings::IniFormat);

    PolhemusCoregistration first;
    seedFiducials(first, writeSettings, penLpa, penNas, penRpa, penLpa, penNas, penRpa);
    first.setTrackerStation(3);
    first.setPenStation(4);
    QVERIFY(first.computeRegistration());

    QSettings saved(tmpDir.path() + "/saved.ini", QSettings::IniFormat);
    first.saveSessionState(saved, QString());
    saved.sync();

    PolhemusCoregistration second;
    // Here the saved state does carry a valid registration, so the return
    // value is expected to be true, unlike in seedFiducials.
    QVERIFY(second.restoreSessionState(saved, QString()));

    // What was written has to come back. Comparing only a flag would pass even
    // if every coordinate were lost.
    QCOMPARE(second.hasAllPenFiducials(), first.hasAllPenFiducials());

    QVERIFY(second.computeRegistration());

    const QVector3D probe(0.01f, 0.02f, 0.03f);
    const QVector3D mappedFirst = first.worldToModel().map(probe);
    const QVector3D mappedSecond = second.worldToModel().map(probe);
    QVERIFY2((mappedFirst - mappedSecond).length() < 1.0e-5f,
             "registration differs after a save and restore cycle");
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestPolhemusCoregistration)
#include "test_polhemus_coregistration.moc"
