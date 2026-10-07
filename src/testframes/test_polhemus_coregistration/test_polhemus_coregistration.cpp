//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_polhemus_coregistration.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
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
#include <QSignalSpy>
#include <QtMath>
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
    void defaultsAndConfiguration();
    void liveDataOperationsRequireSamples();
    void pivotAndRegistrationReset();

    void registration_recoversKnownTransform_data();
    void registration_recoversKnownTransform();

    void registration_rejectsDegenerateFiducials_data();
    void registration_rejectsDegenerateFiducials();

    void registration_requiresAllFiducials();

    void sessionState_roundTrip();

    void liveTracking_stationsMirrorTipAndGimbalGuard();
    void liveCapture_fiducialsHeadShapeVertexAndButton();
    void headFrameFallback_buildsFrameFromCapturedFiducials();
    void pivotCalibration_recoversTipOffset();
    void pivotCalibration_rejectsTooFewOrSingleAxisSamples();
    void opticalCalibration_recoversAxis_data();
    void opticalCalibration_recoversAxis();
    void opticalSession_roundTripAndRestoreReplacesFiducials();

private:
    static void feed(PolhemusConnection& conn, int station, const QVector3D& pos, const QQuaternion& ori);
    static void calibrateOptics(PolhemusCoregistration& coreg, PolhemusConnection& conn, const QVector3D& center,
                                const QVector3D& axis);
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

void TestPolhemusCoregistration::defaultsAndConfiguration()
{
    PolhemusCoregistration coreg;

    QCOMPARE(coreg.trackerStation(), 2);
    QCOMPARE(coreg.penStation(), 1);
    QCOMPARE(coreg.probeStation(), 3);
    QVERIFY(coreg.deviceToWorld().isIdentity());
    QVERIFY(coreg.headToWorld().isIdentity());
    QVERIFY(coreg.headToDevice().isIdentity());
    QVERIFY(coreg.worldToModel().isIdentity());
    QVERIFY(!coreg.registrationValid());
    QVERIFY(!coreg.haveLivePenPosition());
    QVERIFY(!coreg.haveLiveProbePosition());
    QVERIFY(coreg.connection() == nullptr);
    QVERIFY(coreg.acquiredPoints() != nullptr);

    coreg.setTrackerStation(5);
    coreg.setPenStation(6);
    coreg.setProbeStation(7);
    QCOMPARE(coreg.trackerStation(), 5);
    QCOMPARE(coreg.penStation(), 6);
    QCOMPARE(coreg.probeStation(), 7);

    const QVector3D tipOffset(0.001f, -0.002f, 0.003f);
    coreg.setPenTipOffset(tipOffset);
    coreg.setTipOffsetEnabled(true);
    QCOMPARE(coreg.penTipOffset(), tipOffset);
    QVERIFY(coreg.tipOffsetEnabled());

    coreg.setAxisMirror(true, true);
    QVERIFY(coreg.mirrorX());
    QVERIFY(coreg.mirrorY());

    const QVector3D trackerOffset(0.01f, 0.02f, -0.03f);
    const QQuaternion trackerRotation = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 25.0f);
    coreg.setTrackerToDeviceOffset(trackerOffset, trackerRotation);
    QCOMPARE(coreg.trackerToDeviceTranslation(), trackerOffset);
    QCOMPARE(coreg.trackerToDeviceRotation(), trackerRotation);

    coreg.setKnownTrackerToObjectiveDistance(0.15f);
    QCOMPARE(coreg.knownTrackerToObjectiveDistance(), 0.15f);
}

//=============================================================================================================

void TestPolhemusCoregistration::liveDataOperationsRequireSamples()
{
    PolhemusCoregistration coreg;

    QVERIFY(!coreg.captureCurrentPenPositionAsFiducial(FiducialId::LPA));
    QVERIFY(!coreg.captureCurrentPenPositionAsHeadShape());
    QVERIFY(!coreg.captureCurrentPenPositionAsVertex());
    QVERIFY(!coreg.captureOpticalCalibSample());
    QVERIFY(!coreg.captureObjectiveCenter());
    QVERIFY(!coreg.solveOpticalCalibration());

    QVector3D origin;
    QVector3D direction;
    QVector3D up;
    float correctionDeg = -1.0f;
    QVERIFY(!coreg.opticalRayInWorld(origin, direction));
    QVERIFY(!coreg.opticalUpInWorld(up));
    QVERIFY(!coreg.applyOpticalAxisFineAdjust(QVector3D(0.0f, 0.0f, 1.0f), correctionDeg));

    coreg.clearOpticalCalibSamples();
    coreg.clearObjectiveCenter();
    coreg.clearOpticalFineAdjust();
    QCOMPARE(coreg.opticalCalibSampleCount(), 0);
    QVERIFY(!coreg.opticalCalibrationValid());
    QVERIFY(!coreg.hasObjectiveCenter());
    QVERIFY(!coreg.opticalFineAdjustApplied());
}

//=============================================================================================================

void TestPolhemusCoregistration::pivotAndRegistrationReset()
{
    PolhemusCoregistration coreg;
    QSignalSpy pivotSpy(&coreg, &PolhemusCoregistration::pivotStateChanged);
    QSignalSpy registrationSpy(&coreg, &PolhemusCoregistration::registrationChanged);

    QCOMPARE(coreg.pivotState(), PolhemusCoregistration::PivotState::Idle);
    coreg.startPivotCalibration();
    QCOMPARE(coreg.pivotState(), PolhemusCoregistration::PivotState::WaitingForStart);
    QCOMPARE(coreg.pivotSampleCount(), 0);
    QCOMPARE(pivotSpy.count(), 1);

    coreg.cancelPivotCalibration();
    QCOMPARE(coreg.pivotState(), PolhemusCoregistration::PivotState::Idle);
    QCOMPARE(coreg.pivotSampleCount(), 0);
    QCOMPARE(pivotSpy.count(), 2);

    coreg.setModelVertex(QVector3D(0.0f, 0.1f, 0.1f));
    QVERIFY(coreg.hasModelVertex());
    coreg.resetRegistration();
    QVERIFY(!coreg.registrationValid());
    QVERIFY(coreg.headToWorld().isIdentity());
    QVERIFY(coreg.headToDevice().isIdentity());
    QVERIFY(coreg.worldToModel().isIdentity());
    QVERIFY(!coreg.hasPenVertex());
    QVERIFY(!coreg.hasAllPenFiducials());
    QCOMPARE(registrationSpy.count(), 1);
}

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
    settings.setValue("hasPenFid/CZ", true);
    writeVec3(settings, "penFid/NAS", penLpa);
    writeVec3(settings, "penFid/RPA", penNas);
    writeVec3(settings, "penFid/CZ", penRpa);

    settings.setValue("hasModelFid/LPA", false);
    settings.setValue("hasModelFid/NAS", true);
    settings.setValue("hasModelFid/RPA", true);
    settings.setValue("hasModelFid/CZ", true);
    writeVec3(settings, "modelFid/NAS", modelLpa);
    writeVec3(settings, "modelFid/RPA", modelNas);
    writeVec3(settings, "modelFid/CZ", modelRpa);

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
    QTest::newRow("identity") << QVector3D(0, 0, 1) << 0.0f << QVector3D(0.0f, 0.0f, 0.0f);
    QTest::newRow("yaw 30") << QVector3D(0, 0, 1) << 30.0f << QVector3D(0.0f, 0.0f, 0.0f);
    QTest::newRow("pitch 45") << QVector3D(1, 0, 0) << 45.0f << QVector3D(0.0f, 0.0f, 0.0f);
    QTest::newRow("roll 90") << QVector3D(0, 1, 0) << 90.0f << QVector3D(0.0f, 0.0f, 0.0f);
    QTest::newRow("translation") << QVector3D(0, 0, 1) << 0.0f << QVector3D(0.05f, -0.03f, 0.10f);
    QTest::newRow("yaw + shift") << QVector3D(0, 0, 1) << 60.0f << QVector3D(0.02f, 0.04f, -0.01f);
    QTest::newRow("oblique axis") << QVector3D(1, 1, 1) << 120.0f << QVector3D(-0.07f, 0.01f, 0.03f);
    QTest::newRow("near 180") << QVector3D(0, 1, 0) << 179.0f << QVector3D(0.0f, 0.0f, 0.0f);
}

//=============================================================================================================

void TestPolhemusCoregistration::registration_recoversKnownTransform()
{
    QFETCH(QVector3D, axis);
    QFETCH(float, angleDeg);
    QFETCH(QVector3D, translation);

    // Head scale fiducials in metres, well spread so the degeneracy guard
    // does not trip.
    const QVector3D penLpa(-0.075f, 0.000f, 0.000f);
    const QVector3D penNas(0.000f, 0.095f, 0.000f);
    const QVector3D penRpa(0.075f, 0.000f, 0.000f);

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

    struct
    {
        const char* name;
        QVector3D from;
        QVector3D to;
    } cases[] = {
        {"LPA", penLpa, modelLpa},
        {"NAS", penNas, modelNas},
        {"RPA", penRpa, modelRpa}};

    for (const auto& c : cases) {
        const QVector3D mapped = actual.map(c.from);
        const float err = (mapped - c.to).length();
        QVERIFY2(err < 1.0e-4f,
                 qPrintable(QString("%1 maps to (%2, %3, %4), expected (%5, %6, %7), error %8 m")
                                .arg(c.name)
                                .arg(mapped.x())
                                .arg(mapped.y())
                                .arg(mapped.z())
                                .arg(c.to.x())
                                .arg(c.to.y())
                                .arg(c.to.z())
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
                            .arg(distBefore)
                            .arg(distAfter)));
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
    const QVector3D penLpa(-0.075f, 0.001f, 0.002f);
    const QVector3D penNas(0.003f, 0.095f, 0.004f);
    const QVector3D penRpa(0.075f, 0.005f, 0.006f);

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

void TestPolhemusCoregistration::feed(PolhemusConnection& conn, int station, const QVector3D& pos, const QQuaternion& ori)
{
    emit conn.pointReceived(station, pos, ori);
}

//=============================================================================================================

void TestPolhemusCoregistration::liveTracking_stationsMirrorTipAndGimbalGuard()
{
    PolhemusCoregistration coreg;
    PolhemusConnection conn;
    coreg.setConnection(&conn);
    QSignalSpy deviceSpy(&coreg, &PolhemusCoregistration::devicePoseChanged);
    QSignalSpy penSpy(&coreg, &PolhemusCoregistration::penPoseChanged);
    QSignalSpy probeSpy(&coreg, &PolhemusCoregistration::probePoseChanged);

    // Tracker (station 2): deviceToWorld = T(pos) R(ori) T(offset) R(offsetRot)
    const QQuaternion trackerOri = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 30.0f);
    const QQuaternion offsetRot = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 90.0f);
    coreg.setTrackerToDeviceOffset(QVector3D(0.01f, 0.0f, 0.0f), offsetRot);
    feed(conn, 2, QVector3D(0.1f, 0.2f, 0.3f), trackerOri);
    QCOMPARE(deviceSpy.count(), 1);
    const QVector3D deviceOrigin = coreg.deviceToWorld().map(QVector3D());
    const QVector3D expectedOrigin = QVector3D(0.1f, 0.2f, 0.3f) + trackerOri.rotatedVector(QVector3D(0.01f, 0, 0));
    QVERIFY((deviceOrigin - expectedOrigin).length() < 1e-6f);
    const QVector3D deviceY = coreg.deviceToWorld().mapVector(QVector3D(0, 1, 0));
    QVERIFY((deviceY - (trackerOri * offsetRot).rotatedVector(QVector3D(0, 1, 0))).length() < 1e-6f);

    // Pen (station 1): mirrored X/Y, tip offset rotated by the sensor orientation
    const QQuaternion penOri = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 90.0f);
    coreg.setAxisMirror(true, true);
    coreg.setPenTipOffset(QVector3D(0.0f, 0.0f, -0.1f));
    coreg.setTipOffsetEnabled(true);
    feed(conn, 1, QVector3D(0.05f, 0.06f, 0.07f), penOri);
    QVERIFY(coreg.haveLivePenPosition());
    QVERIFY((coreg.penPosition() - QVector3D(-0.05f, 0.04f, 0.07f)).length() < 1e-6f);
    QCOMPARE(penSpy.count(), 1);

    // Near gimbal lock (pitch 85 deg about Y) the pose is frozen, but the signal still fires
    feed(conn, 1, QVector3D(0.5f, 0.5f, 0.5f), QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 85.0f));
    QVERIFY((coreg.penPosition() - QVector3D(-0.05f, 0.04f, 0.07f)).length() < 1e-6f);
    QCOMPARE(penSpy.count(), 2);

    // Probe (station 3) and an unknown station
    feed(conn, 3, QVector3D(0.2f, 0.1f, 0.0f), penOri);
    QVERIFY(coreg.haveLiveProbePosition());
    QCOMPARE(coreg.probePosition(), QVector3D(-0.2f, -0.1f, 0.0f));
    QCOMPARE(coreg.probeOrientation(), penOri);
    feed(conn, 4, QVector3D(), QQuaternion());
    QCOMPARE(deviceSpy.count() + penSpy.count() + probeSpy.count(), 4);

    // After detaching, samples are ignored
    coreg.setConnection(nullptr);
    feed(conn, 1, QVector3D(), QQuaternion());
    QCOMPARE(penSpy.count(), 2);
}

//=============================================================================================================

void TestPolhemusCoregistration::liveCapture_fiducialsHeadShapeVertexAndButton()
{
    PolhemusCoregistration coreg;
    PolhemusConnection conn;
    coreg.setConnection(&conn);
    QSignalSpy buttonSpy(&coreg, &PolhemusCoregistration::penButtonPressed);

    const QVector3D lpa(-0.075f, 0.0f, 0.0f), nas(0.0f, 0.095f, 0.0f), rpa(0.075f, 0.0f, 0.0f);
    const QVector<std::pair<FiducialId, QVector3D>> fids = {
        {FiducialId::LPA, lpa + QVector3D(0, 0, 0.01f)}, {FiducialId::LPA, lpa}, {FiducialId::NAS, nas}, {FiducialId::RPA, rpa}};
    for (const auto& [id, pos] : fids) {
        feed(conn, 1, pos, QQuaternion());
        QVERIFY(coreg.captureCurrentPenPositionAsFiducial(id));
    }
    // Re-capturing LPA replaced the first one
    QCOMPARE(coreg.acquiredPoints()->countOf(PointKind::Fiducial), 3);
    QCOMPARE(coreg.acquiredPoints()->fiducial(FiducialId::LPA), lpa);
    QVERIFY(coreg.hasAllPenFiducials());

    for (int i = 0; i < 2; ++i) {
        feed(conn, 1, QVector3D(0.01f * i, 0.02f, 0.09f), QQuaternion());
        QVERIFY(coreg.captureCurrentPenPositionAsHeadShape());
    }
    QCOMPARE(coreg.acquiredPoints()->countOf(PointKind::HeadShape), 2);
    QCOMPARE(coreg.acquiredPoints()->points().last().label, QStringLiteral("HSP-2"));

    feed(conn, 1, QVector3D(0.0f, 0.02f, 0.1f), QQuaternion());
    QVERIFY(coreg.captureCurrentPenPositionAsVertex());
    QVERIFY(coreg.hasPenVertex());
    QCOMPARE(coreg.penVertex(), QVector3D(0.0f, 0.02f, 0.1f));

    // The pen button is forwarded only for the pen station, with the tip-adjusted position
    emit conn.penButtonPressed(3, QVector3D(1, 1, 1), QQuaternion());
    QCOMPARE(buttonSpy.count(), 0);
    emit conn.penButtonPressed(1, QVector3D(0.03f, 0.0f, 0.0f), QQuaternion());
    QCOMPARE(buttonSpy.count(), 1);
    QCOMPARE(buttonSpy.at(0).at(0).value<QVector3D>(), QVector3D(0.03f, 0.0f, 0.0f));
}

//=============================================================================================================

void TestPolhemusCoregistration::headFrameFallback_buildsFrameFromCapturedFiducials()
{
    PolhemusCoregistration coreg;
    PolhemusConnection conn;
    coreg.setConnection(&conn);
    // A rotated head: NAS along world +y, LPA along world -x around origin (0.01, 0.02, 0.03)
    const QVector3D o(0.01f, 0.02f, 0.03f);
    const QVector<std::pair<FiducialId, QVector3D>> fids = {
        {FiducialId::LPA, o + QVector3D(-0.07f, 0, 0)}, {FiducialId::NAS, o + QVector3D(0.01f, 0.1f, 0)}, {FiducialId::RPA, o + QVector3D(0.07f, 0, 0)}};
    for (const auto& [id, pos] : fids) {
        feed(conn, 1, pos, QQuaternion());
        QVERIFY(coreg.captureCurrentPenPositionAsFiducial(id));
    }
    feed(conn, 2, QVector3D(0.2f, 0.0f, 0.0f), QQuaternion());

    QVERIFY(coreg.computeRegistration());
    // Head frame: origin midway between the ears, x towards NAS, y towards LPA (orthogonalised), z = x cross y
    const QMatrix4x4 h = coreg.headToWorld();
    QVERIFY((h.map(QVector3D()) - o).length() < 1e-6f);
    const QVector3D ex = QVector3D(0.01f, 0.1f, 0).normalized();
    QVERIFY((h.mapVector(QVector3D(1, 0, 0)) - ex).length() < 1e-5f);
    const QVector3D ey = h.mapVector(QVector3D(0, 1, 0));
    QVERIFY(std::fabs(QVector3D::dotProduct(ey, ex)) < 1e-5f);
    QVERIFY(ey.x() < 0.0f);
    QVERIFY((h.mapVector(QVector3D(0, 0, 1)) - QVector3D(0, 0, 1)).length() < 1e-5f);
    QVERIFY(coreg.worldToModel().isIdentity());
    QVERIFY((coreg.headToDevice().map(QVector3D()) - (o - QVector3D(0.2f, 0, 0))).length() < 1e-6f);

    // NAS on the ear line: no frame can be built
    PolhemusCoregistration flat;
    flat.setConnection(&conn);
    const QVector<std::pair<FiducialId, QVector3D>> line = {
        {FiducialId::LPA, QVector3D(-0.07f, 0, 0)}, {FiducialId::NAS, QVector3D(0.03f, 0, 0)}, {FiducialId::RPA, QVector3D(0.07f, 0, 0)}};
    for (const auto& [id, pos] : line) {
        feed(conn, 1, pos, QQuaternion());
        QVERIFY(flat.captureCurrentPenPositionAsFiducial(id));
    }
    QVERIFY(!flat.computeRegistration());
}

//=============================================================================================================

void TestPolhemusCoregistration::pivotCalibration_recoversTipOffset()
{
    PolhemusCoregistration coreg;
    PolhemusConnection conn;
    coreg.setConnection(&conn);
    QSignalSpy doneSpy(&coreg, &PolhemusCoregistration::pivotCalibrationDone);
    QSignalSpy sampleSpy(&coreg, &PolhemusCoregistration::pivotSampleCollected);

    // The pen pivots about a fixed tip T; the sensor sits at T - R_i * offset
    const QVector3D tip(0.1f, 0.2f, 0.05f);
    const QVector3D offset(0.01f, -0.02f, -0.12f);
    coreg.startPivotCalibration();
    emit conn.penButtonPressed(1, tip, QQuaternion());
    QCOMPARE(coreg.pivotState(), PolhemusCoregistration::PivotState::Collecting);

    for (int i = 0; i < 14; ++i) {
        const float phi = qDegreesToRadians(25.0f * i);
        const QQuaternion r = QQuaternion::fromAxisAndAngle(QVector3D(std::cos(phi), std::sin(phi), 0.3f), 30.0f);
        feed(conn, 1, tip - r.rotatedVector(offset), r);
        // A repeat of the same orientation and a gimbal-locked pose are not collected
        feed(conn, 1, tip - r.rotatedVector(offset), r);
        feed(conn, 1, tip, QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 85.0f));
    }
    QCOMPARE(coreg.pivotSampleCount(), 14);
    QCOMPARE(sampleSpy.count(), 14);
    QVERIFY(sampleSpy.last().at(1).toFloat() > 30.0f);

    emit conn.penButtonPressed(1, tip, QQuaternion());
    QCOMPARE(coreg.pivotState(), PolhemusCoregistration::PivotState::Done);
    QCOMPARE(doneSpy.count(), 1);
    QVERIFY2((coreg.penTipOffset() - offset).length() < 1e-5f,
             qPrintable(QString("offset %1 %2 %3").arg(coreg.penTipOffset().x()).arg(coreg.penTipOffset().y()).arg(coreg.penTipOffset().z())));
    QVERIFY(coreg.pivotResidualMm() < 0.01f);
}

//=============================================================================================================

void TestPolhemusCoregistration::pivotCalibration_rejectsTooFewOrSingleAxisSamples()
{
    PolhemusCoregistration coreg;
    PolhemusConnection conn;
    coreg.setConnection(&conn);
    const QVector3D offset(0.0f, 0.0f, -0.12f);

    // Fewer than 10 samples
    coreg.startPivotCalibration();
    emit conn.penButtonPressed(1, QVector3D(), QQuaternion());
    for (int i = 0; i < 5; ++i) {
        const QQuaternion r = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 10.0f * i);
        feed(conn, 1, -r.rotatedVector(offset), r);
    }
    emit conn.penButtonPressed(1, QVector3D(), QQuaternion());
    QCOMPARE(coreg.pivotState(), PolhemusCoregistration::PivotState::Idle);

    // Rotations about the pen's own axis leave tip depth and offset length indistinguishable
    coreg.startPivotCalibration();
    emit conn.penButtonPressed(1, QVector3D(), QQuaternion());
    for (int i = 0; i < 12; ++i) {
        const QQuaternion r = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 10.0f * i);
        feed(conn, 1, -r.rotatedVector(offset), r);
    }
    QCOMPARE(coreg.pivotSampleCount(), 12);
    emit conn.penButtonPressed(1, QVector3D(), QQuaternion());
    QCOMPARE(coreg.pivotState(), PolhemusCoregistration::PivotState::Idle);
    QCOMPARE(coreg.penTipOffset(), QVector3D());
}

//=============================================================================================================

void TestPolhemusCoregistration::calibrateOptics(PolhemusCoregistration& coreg, PolhemusConnection& conn,
                                                 const QVector3D& center, const QVector3D& axis)
{
    // Focus points on the optical axis, seen from five different tracker poses
    for (int k = 0; k < 5; ++k) {
        const QQuaternion trackerOri = QQuaternion::fromAxisAndAngle(QVector3D(0.2f * k, 1.0f, 0.5f), 15.0f + 7.0f * k);
        const QVector3D trackerPos(0.3f + 0.01f * k, -0.1f, 0.4f - 0.02f * k);
        feed(conn, 2, trackerPos, trackerOri);
        feed(conn, 1, trackerPos + trackerOri.rotatedVector(center + (0.10f + 0.06f * k) * axis), QQuaternion());
        QVERIFY(coreg.captureOpticalCalibSample());
    }
    QCOMPARE(coreg.opticalCalibSampleCount(), 5);
}

//=============================================================================================================

void TestPolhemusCoregistration::opticalCalibration_recoversAxis_data()
{
    QTest::addColumn<int>("mode");
    QTest::newRow("captured objective center") << 0;
    QTest::newRow("known distance") << 1;
    QTest::newRow("unconstrained") << 2;
}

//=============================================================================================================

void TestPolhemusCoregistration::opticalCalibration_recoversAxis()
{
    QFETCH(int, mode);
    PolhemusCoregistration coreg;
    PolhemusConnection conn;
    coreg.setConnection(&conn);
    QSignalSpy changedSpy(&coreg, &PolhemusCoregistration::opticalCalibrationChanged);

    const QVector3D center(0.05f, 0.18f, -0.03f);
    const QVector3D axis = QVector3D(0.1f, 1.0f, -0.3f).normalized();
    coreg.setKnownTrackerToObjectiveDistance(mode == 1 ? center.length() : 0.0f);

    calibrateOptics(coreg, conn, center, axis);
    if (mode == 0) {
        const QQuaternion trackerOri = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 40.0f);
        feed(conn, 2, QVector3D(0.1f, 0.1f, 0.1f), trackerOri);
        feed(conn, 1, QVector3D(0.1f, 0.1f, 0.1f) + trackerOri.rotatedVector(center), QQuaternion());
        QVERIFY(coreg.captureObjectiveCenter());
        QVERIFY((coreg.objectiveCenterLocal() - center).length() < 1e-5f);
    }
    QVERIFY(coreg.solveOpticalCalibration());
    QVERIFY(coreg.opticalCalibrationValid());
    QCOMPARE(changedSpy.count(), 1);

    QVERIFY2(QVector3D::dotProduct(coreg.opticalAxisLocal(), axis) > 1.0f - 1e-5f,
             qPrintable(QString("axis %1 %2 %3").arg(coreg.opticalAxisLocal().x()).arg(coreg.opticalAxisLocal().y()).arg(coreg.opticalAxisLocal().z())));
    QVERIFY(coreg.opticalCalibResidualMm() < 0.05f);
    QVERIFY(std::fabs(coreg.opticalCalibDepthSpreadMm() - 240.0f) < 0.1f);
    // The centre lies on the axis; with a captured centre or a known distance it is the true one
    const QVector3D c = coreg.opticalCenterLocal();
    const QVector3D rel = c - center;
    QVERIFY((rel - QVector3D::dotProduct(rel, axis) * axis).length() < 1e-5f);
    if (mode != 2) {
        QVERIFY2(rel.length() < 1e-4f, qPrintable(QString("centre off by %1 m").arg(rel.length())));
    }

    // World ray and up vector for the current tracker pose
    const QQuaternion trackerOri = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 20.0f);
    const QVector3D trackerPos(0.2f, 0.0f, 0.1f);
    feed(conn, 2, trackerPos, trackerOri);
    QVector3D origin, direction, up;
    QVERIFY(coreg.opticalRayInWorld(origin, direction));
    QVERIFY((origin - (trackerPos + trackerOri.rotatedVector(c))).length() < 1e-5f);
    QVERIFY((direction - trackerOri.rotatedVector(axis)).length() < 1e-4f);
    QVERIFY(coreg.opticalUpInWorld(up));
    QVERIFY(std::fabs(QVector3D::dotProduct(up, direction)) < 1e-5f);
    QVERIFY(QVector3D::dotProduct(up, trackerOri.rotatedVector(QVector3D(0, 0, 1))) > 0.9f);

    // Fine adjustment towards a target 2 degrees off the axis, then undo
    const QVector3D tilted = QQuaternion::fromAxisAndAngle(up, 2.0f).rotatedVector(direction);
    float correction = 0.0f;
    QVERIFY(coreg.applyOpticalAxisFineAdjust(origin + 0.3f * tilted, correction));
    QVERIFY(std::fabs(correction - 2.0f) < 0.01f);
    QVERIFY(coreg.opticalFineAdjustApplied());
    QCOMPARE(coreg.opticalFineAdjustDeg(), correction);
    QVERIFY(coreg.opticalRayInWorld(origin, direction));
    QVERIFY((direction - tilted).length() < 1e-4f);
    QVERIFY(!coreg.applyOpticalAxisFineAdjust(origin + 0.3f * QQuaternion::fromAxisAndAngle(up, 20.0f).rotatedVector(direction), correction));
    QVERIFY(correction > 10.0f);
    QVERIFY(!coreg.applyOpticalAxisFineAdjust(origin, correction));
    coreg.clearOpticalFineAdjust();
    QVERIFY(!coreg.opticalFineAdjustApplied());
    QVERIFY((coreg.opticalAxisLocal() - axis).length() < 1e-4f);

    coreg.clearOpticalCalibSamples();
    QVERIFY(!coreg.opticalCalibrationValid());
    QVERIFY(!coreg.solveOpticalCalibration());
}

//=============================================================================================================

void TestPolhemusCoregistration::opticalSession_roundTripAndRestoreReplacesFiducials()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    PolhemusCoregistration first;
    PolhemusConnection conn;
    first.setConnection(&conn);
    const QVector3D center(0.0f, 0.2f, 0.0f);
    const QVector3D axis = QVector3D(0.0f, 1.0f, 0.2f).normalized();
    calibrateOptics(first, conn, center, axis);
    feed(conn, 1, QVector3D(0.3f, 0.0f, 0.4f) + center, QQuaternion());
    feed(conn, 2, QVector3D(0.3f, 0.0f, 0.4f), QQuaternion());
    QVERIFY(first.captureObjectiveCenter());
    QVERIFY(first.solveOpticalCalibration());
    QVector3D origin, direction;
    QVERIFY(first.opticalRayInWorld(origin, direction));
    float correction = 0.0f;
    QVERIFY(first.applyOpticalAxisFineAdjust(origin + 0.3f * QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 1.0f).rotatedVector(direction), correction));
    first.setModelVertex(QVector3D(0.0f, 0.0f, 0.12f));
    feed(conn, 1, QVector3D(0.0f, 0.01f, 0.11f), QQuaternion());
    QVERIFY(first.captureCurrentPenPositionAsVertex());
    const QVector<std::pair<FiducialId, QVector3D>> fids = {
        {FiducialId::LPA, QVector3D(-0.07f, 0, 0)}, {FiducialId::NAS, QVector3D(0, 0.1f, 0)}, {FiducialId::RPA, QVector3D(0.07f, 0, 0)}};
    for (const auto& [id, pos] : fids) {
        feed(conn, 1, pos, QQuaternion());
        QVERIFY(first.captureCurrentPenPositionAsFiducial(id));
    }
    QVERIFY(first.computeRegistration());

    QSettings saved(tmpDir.path() + "/optical.ini", QSettings::IniFormat);
    first.saveSessionState(saved);
    saved.sync();

    // The restoring side already holds different live fiducials; the restored ones must replace them
    PolhemusCoregistration second;
    second.setConnection(&conn);
    for (const auto& [id, pos] : fids) {
        feed(conn, 1, pos + QVector3D(0.0f, 0.0f, 0.05f), QQuaternion());
        QVERIFY(second.captureCurrentPenPositionAsFiducial(id));
    }
    QVERIFY(second.restoreSessionState(saved));
    QVERIFY(second.restoreSessionState(saved));
    QCOMPARE(second.acquiredPoints()->countOf(PointKind::Fiducial), 3);
    for (const auto& [id, pos] : fids) {
        QCOMPARE(second.acquiredPoints()->fiducial(id), pos);
    }

    QCOMPARE(second.opticalCalibrationValid(), true);
    QCOMPARE(second.opticalCalibSampleCount(), 5);
    QCOMPARE(second.hasObjectiveCenter(), true);
    QVERIFY((second.objectiveCenterLocal() - first.objectiveCenterLocal()).length() < 1e-6f);
    QVERIFY((second.opticalAxisLocal() - first.opticalAxisLocal()).length() < 1e-6f);
    QVERIFY((second.opticalCenterLocal() - first.opticalCenterLocal()).length() < 1e-6f);
    QCOMPARE(second.opticalFineAdjustApplied(), true);
    QVERIFY(std::fabs(second.opticalFineAdjustDeg() - first.opticalFineAdjustDeg()) < 1e-6f);
    QVERIFY(std::fabs(second.opticalCalibResidualMm() - first.opticalCalibResidualMm()) < 1e-4f);
    QVERIFY(std::fabs(second.opticalCalibDepthSpreadMm() - first.opticalCalibDepthSpreadMm()) < 1e-3f);
    QCOMPARE(second.hasPenVertex(), true);
    QCOMPARE(second.hasModelVertex(), true);
    QCOMPARE(second.penVertex(), first.penVertex());
    QCOMPARE(second.modelVertex(), first.modelVertex());
    QCOMPARE(second.headToWorld(), first.headToWorld());
    second.clearOpticalFineAdjust();
    QVERIFY((second.opticalAxisLocal() - axis).length() < 1e-4f);
    QVERIFY(second.solveOpticalCalibration());

    QSettings empty(tmpDir.path() + "/empty.ini", QSettings::IniFormat);
    QVERIFY(!second.restoreSessionState(empty));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestPolhemusCoregistration)
#include "test_polhemus_coregistration.moc"
