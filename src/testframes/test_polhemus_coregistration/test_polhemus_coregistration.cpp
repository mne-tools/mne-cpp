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
 * The expectations use Qt's vector math as a reference independent of the
 * Eigen types the library works in.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/devices/polhemus/polhemus_coregistration.h>
#include <acq/devices/polhemus/acquired_points.h>

#include <digitizer.h>

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

using namespace mne;

//=============================================================================================================

namespace
{

Eigen::Vector3f toEigen(const QVector3D& v)
{
    return Eigen::Vector3f(v.x(), v.y(), v.z());
}

Eigen::Quaternionf toEigen(const QQuaternion& q)
{
    return Eigen::Quaternionf(q.scalar(), q.x(), q.y(), q.z());
}

QVector3D toQt(const Eigen::Vector3f& v)
{
    return QVector3D(v.x(), v.y(), v.z());
}

QQuaternion toQt(const Eigen::Quaternionf& q)
{
    return QQuaternion(q.w(), q.x(), q.y(), q.z());
}

QMatrix4x4 toQt(const Eigen::Matrix4f& m)
{
    QMatrix4x4 out;
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            out(r, c) = m(r, c);
        }
    }
    return out;
}

bool opticalRayInWorld(const PolhemusCoregistration& coreg, QVector3D& origin, QVector3D& direction)
{
    Eigen::Vector3f o = Eigen::Vector3f::Zero();
    Eigen::Vector3f d = Eigen::Vector3f::Zero();
    const bool ok = coreg.opticalRayInWorld(o, d);
    origin = toQt(o);
    direction = toQt(d);
    return ok;
}

bool opticalUpInWorld(const PolhemusCoregistration& coreg, QVector3D& up)
{
    Eigen::Vector3f u = Eigen::Vector3f::Zero();
    const bool ok = coreg.opticalUpInWorld(u);
    up = toQt(u);
    return ok;
}

} // namespace

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
    void sessionState_keepsTheStations();

    void liveTracking_stationsMirrorTipAndGimbalGuard();
    void liveCapture_fiducialsHeadShapeVertexAndButton();
    void headFrameFallback_buildsFrameFromCapturedFiducials();
    void pivotCalibration_recoversTipOffset();
    void pivotCalibration_rejectsTooFewOrSingleAxisSamples();
    void opticalCalibration_recoversAxis_data();
    void opticalCalibration_recoversAxis();
    void opticalSession_roundTripAndRestoreReplacesFiducials();
    void digitizer_feedsTheSimulatedStylusAndTracker();
    void digitizer_reportsAMissingPort();

private:
    static void feed(PolhemusCoregistration& coreg, int station, const QVector3D& pos, const QQuaternion& ori);
    static void calibrateOptics(PolhemusCoregistration& coreg, const QVector3D& center, const QVector3D& axis);
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
    QVERIFY(toQt(coreg.deviceToWorld()).isIdentity());
    QVERIFY(toQt(coreg.headToWorld()).isIdentity());
    QVERIFY(toQt(coreg.headToDevice()).isIdentity());
    QVERIFY(toQt(coreg.worldToModel()).isIdentity());
    QVERIFY(!coreg.registrationValid());
    QVERIFY(!coreg.haveLivePenPosition());
    QVERIFY(!coreg.haveLiveProbePosition());
    QVERIFY(coreg.acquiredPoints() != nullptr);

    coreg.setTrackerStation(5);
    coreg.setPenStation(6);
    coreg.setProbeStation(7);
    QCOMPARE(coreg.trackerStation(), 5);
    QCOMPARE(coreg.penStation(), 6);
    QCOMPARE(coreg.probeStation(), 7);

    const QVector3D tipOffset(0.001f, -0.002f, 0.003f);
    coreg.setPenTipOffset(toEigen(tipOffset));
    coreg.setTipOffsetEnabled(true);
    QCOMPARE(toQt(coreg.penTipOffset()), tipOffset);
    QVERIFY(coreg.tipOffsetEnabled());

    coreg.setAxisMirror(true, true);
    QVERIFY(coreg.mirrorX());
    QVERIFY(coreg.mirrorY());

    const QVector3D trackerOffset(0.01f, 0.02f, -0.03f);
    const QQuaternion trackerRotation = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 25.0f);
    coreg.setTrackerToDeviceOffset(toEigen(trackerOffset), toEigen(trackerRotation));
    QCOMPARE(toQt(coreg.trackerToDeviceTranslation()), trackerOffset);
    QCOMPARE(toQt(coreg.trackerToDeviceRotation()), trackerRotation);

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
    QVERIFY(!opticalRayInWorld(coreg, origin, direction));
    QVERIFY(!opticalUpInWorld(coreg, up));
    QVERIFY(!coreg.applyOpticalAxisFineAdjust(toEigen(QVector3D(0.0f, 0.0f, 1.0f)), correctionDeg));

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

    coreg.setModelVertex(toEigen(QVector3D(0.0f, 0.1f, 0.1f)));
    QVERIFY(coreg.hasModelVertex());
    coreg.resetRegistration();
    QVERIFY(!coreg.registrationValid());
    QVERIFY(toQt(coreg.headToWorld()).isIdentity());
    QVERIFY(toQt(coreg.headToDevice()).isIdentity());
    QVERIFY(toQt(coreg.worldToModel()).isIdentity());
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
    const QMatrix4x4 actual = toQt(coreg.worldToModel());

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

void TestPolhemusCoregistration::sessionState_keepsTheStations()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    // Every station comes back, the probe station included
    PolhemusCoregistration first;
    first.setTrackerStation(4);
    first.setPenStation(5);
    first.setProbeStation(7);
    QSettings saved(tmpDir.path() + "/stations.ini", QSettings::IniFormat);
    first.saveSessionState(saved);
    PolhemusCoregistration second;
    second.restoreSessionState(saved);
    QCOMPARE(second.trackerStation(), 4);
    QCOMPARE(second.penStation(), 5);
    QCOMPARE(second.probeStation(), 7);

    // A state without stations leaves the default assignment, not pen and tracker swapped
    QSettings partial(tmpDir.path() + "/partial.ini", QSettings::IniFormat);
    partial.setValue("polhemus/mirrorX", true);
    partial.sync();
    PolhemusCoregistration third;
    third.restoreSessionState(partial);
    QCOMPARE(third.trackerStation(), 2);
    QCOMPARE(third.penStation(), 1);
    QCOMPARE(third.probeStation(), 3);
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
    const QVector3D mappedFirst = toQt(first.worldToModel()).map(probe);
    const QVector3D mappedSecond = toQt(second.worldToModel()).map(probe);
    QVERIFY2((mappedFirst - mappedSecond).length() < 1.0e-5f,
             "registration differs after a save and restore cycle");
}

//=============================================================================================================

void TestPolhemusCoregistration::feed(PolhemusCoregistration& coreg, int station, const QVector3D& pos, const QQuaternion& ori)
{
    coreg.on_pose(station, toEigen(pos), toEigen(ori));
}

//=============================================================================================================

void TestPolhemusCoregistration::liveTracking_stationsMirrorTipAndGimbalGuard()
{
    PolhemusCoregistration coreg;
    QSignalSpy deviceSpy(&coreg, &PolhemusCoregistration::devicePoseChanged);
    QSignalSpy penSpy(&coreg, &PolhemusCoregistration::penPoseChanged);
    QSignalSpy probeSpy(&coreg, &PolhemusCoregistration::probePoseChanged);

    // Tracker (station 2): deviceToWorld = T(pos) R(ori) T(offset) R(offsetRot)
    const QQuaternion trackerOri = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 30.0f);
    const QQuaternion offsetRot = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 90.0f);
    coreg.setTrackerToDeviceOffset(Eigen::Vector3f(0.01f, 0.0f, 0.0f), toEigen(offsetRot));
    feed(coreg, 2, QVector3D(0.1f, 0.2f, 0.3f), trackerOri);
    QCOMPARE(deviceSpy.count(), 1);
    const QVector3D deviceOrigin = toQt(coreg.deviceToWorld()).map(QVector3D());
    const QVector3D expectedOrigin = QVector3D(0.1f, 0.2f, 0.3f) + trackerOri.rotatedVector(QVector3D(0.01f, 0, 0));
    QVERIFY((deviceOrigin - expectedOrigin).length() < 1e-6f);
    const QVector3D deviceY = toQt(coreg.deviceToWorld()).mapVector(QVector3D(0, 1, 0));
    QVERIFY((deviceY - (trackerOri * offsetRot).rotatedVector(QVector3D(0, 1, 0))).length() < 1e-6f);

    // Pen (station 1): mirrored X/Y, tip offset rotated by the sensor orientation
    const QQuaternion penOri = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 90.0f);
    coreg.setAxisMirror(true, true);
    coreg.setPenTipOffset(toEigen(QVector3D(0.0f, 0.0f, -0.1f)));
    coreg.setTipOffsetEnabled(true);
    feed(coreg, 1, QVector3D(0.05f, 0.06f, 0.07f), penOri);
    QVERIFY(coreg.haveLivePenPosition());
    QVERIFY((toQt(coreg.penPosition()) - QVector3D(-0.05f, 0.04f, 0.07f)).length() < 1e-6f);
    QCOMPARE(penSpy.count(), 1);

    // Near gimbal lock (pitch 85 deg about Y) the pose is frozen, but the signal still fires
    feed(coreg, 1, QVector3D(0.5f, 0.5f, 0.5f), QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 85.0f));
    QVERIFY((toQt(coreg.penPosition()) - QVector3D(-0.05f, 0.04f, 0.07f)).length() < 1e-6f);
    QCOMPARE(penSpy.count(), 2);

    // Probe (station 3) and an unknown station
    feed(coreg, 3, QVector3D(0.2f, 0.1f, 0.0f), penOri);
    QVERIFY(coreg.haveLiveProbePosition());
    QCOMPARE(toQt(coreg.probePosition()), QVector3D(-0.2f, -0.1f, 0.0f));
    QCOMPARE(toQt(coreg.probeOrientation()), penOri);
    feed(coreg, 4, QVector3D(), QQuaternion());
    QCOMPARE(deviceSpy.count() + penSpy.count() + probeSpy.count(), 4);
}

//=============================================================================================================

void TestPolhemusCoregistration::liveCapture_fiducialsHeadShapeVertexAndButton()
{
    PolhemusCoregistration coreg;
    QSignalSpy buttonSpy(&coreg, &PolhemusCoregistration::penButtonPressed);

    const QVector3D lpa(-0.075f, 0.0f, 0.0f), nas(0.0f, 0.095f, 0.0f), rpa(0.075f, 0.0f, 0.0f);
    const QVector<std::pair<FiducialId, QVector3D>> fids = {
        {FiducialId::LPA, lpa + QVector3D(0, 0, 0.01f)}, {FiducialId::LPA, lpa}, {FiducialId::NAS, nas}, {FiducialId::RPA, rpa}};
    for (const auto& [id, pos] : fids) {
        feed(coreg, 1, pos, QQuaternion());
        QVERIFY(coreg.captureCurrentPenPositionAsFiducial(id));
    }
    // Re-capturing LPA replaced the first one
    QCOMPARE(coreg.acquiredPoints()->countOf(PointKind::Fiducial), 3);
    QCOMPARE(toQt(coreg.acquiredPoints()->fiducial(FiducialId::LPA)), lpa);
    QVERIFY(coreg.hasAllPenFiducials());

    for (int i = 0; i < 2; ++i) {
        feed(coreg, 1, QVector3D(0.01f * i, 0.02f, 0.09f), QQuaternion());
        QVERIFY(coreg.captureCurrentPenPositionAsHeadShape());
    }
    QCOMPARE(coreg.acquiredPoints()->countOf(PointKind::HeadShape), 2);
    QCOMPARE(coreg.acquiredPoints()->points().last().label, QStringLiteral("HSP-2"));

    feed(coreg, 1, QVector3D(0.0f, 0.02f, 0.1f), QQuaternion());
    QVERIFY(coreg.captureCurrentPenPositionAsVertex());
    QVERIFY(coreg.hasPenVertex());
    QCOMPARE(toQt(coreg.penVertex()), QVector3D(0.0f, 0.02f, 0.1f));

    // The pen button is forwarded only for the pen station, with the tip-adjusted position
    coreg.on_button(3, toEigen(QVector3D(1, 1, 1)), Eigen::Quaternionf::Identity());
    QCOMPARE(buttonSpy.count(), 0);
    coreg.on_button(1, toEigen(QVector3D(0.03f, 0.0f, 0.0f)), Eigen::Quaternionf::Identity());
    QCOMPARE(buttonSpy.count(), 1);
    QCOMPARE(toQt(buttonSpy.at(0).at(0).value<Eigen::Vector3f>()), QVector3D(0.03f, 0.0f, 0.0f));
}

//=============================================================================================================

void TestPolhemusCoregistration::headFrameFallback_buildsFrameFromCapturedFiducials()
{
    PolhemusCoregistration coreg;
    // A rotated head: NAS along world +y, LPA along world -x around origin (0.01, 0.02, 0.03)
    const QVector3D o(0.01f, 0.02f, 0.03f);
    const QVector<std::pair<FiducialId, QVector3D>> fids = {
        {FiducialId::LPA, o + QVector3D(-0.07f, 0, 0)}, {FiducialId::NAS, o + QVector3D(0.01f, 0.1f, 0)}, {FiducialId::RPA, o + QVector3D(0.07f, 0, 0)}};
    for (const auto& [id, pos] : fids) {
        feed(coreg, 1, pos, QQuaternion());
        QVERIFY(coreg.captureCurrentPenPositionAsFiducial(id));
    }
    feed(coreg, 2, QVector3D(0.2f, 0.0f, 0.0f), QQuaternion());

    QVERIFY(coreg.computeRegistration());
    // Head frame: origin midway between the ears, x towards NAS, y towards LPA (orthogonalised), z = x cross y
    const QMatrix4x4 h = toQt(coreg.headToWorld());
    QVERIFY((h.map(QVector3D()) - o).length() < 1e-6f);
    const QVector3D ex = QVector3D(0.01f, 0.1f, 0).normalized();
    QVERIFY((h.mapVector(QVector3D(1, 0, 0)) - ex).length() < 1e-5f);
    const QVector3D ey = h.mapVector(QVector3D(0, 1, 0));
    QVERIFY(std::fabs(QVector3D::dotProduct(ey, ex)) < 1e-5f);
    QVERIFY(ey.x() < 0.0f);
    QVERIFY((h.mapVector(QVector3D(0, 0, 1)) - QVector3D(0, 0, 1)).length() < 1e-5f);
    QVERIFY(toQt(coreg.worldToModel()).isIdentity());
    QVERIFY((toQt(coreg.headToDevice()).map(QVector3D()) - (o - QVector3D(0.2f, 0, 0))).length() < 1e-6f);

    // NAS on the ear line: no frame can be built
    PolhemusCoregistration flat;
    const QVector<std::pair<FiducialId, QVector3D>> line = {
        {FiducialId::LPA, QVector3D(-0.07f, 0, 0)}, {FiducialId::NAS, QVector3D(0.03f, 0, 0)}, {FiducialId::RPA, QVector3D(0.07f, 0, 0)}};
    for (const auto& [id, pos] : line) {
        feed(flat, 1, pos, QQuaternion());
        QVERIFY(flat.captureCurrentPenPositionAsFiducial(id));
    }
    QVERIFY(!flat.computeRegistration());
}

//=============================================================================================================

void TestPolhemusCoregistration::pivotCalibration_recoversTipOffset()
{
    PolhemusCoregistration coreg;
    QSignalSpy doneSpy(&coreg, &PolhemusCoregistration::pivotCalibrationDone);
    QSignalSpy sampleSpy(&coreg, &PolhemusCoregistration::pivotSampleCollected);

    // The pen pivots about a fixed tip T; the sensor sits at T - R_i * offset
    const QVector3D tip(0.1f, 0.2f, 0.05f);
    const QVector3D offset(0.01f, -0.02f, -0.12f);
    coreg.startPivotCalibration();
    coreg.on_button(1, toEigen(tip), Eigen::Quaternionf::Identity());
    QCOMPARE(coreg.pivotState(), PolhemusCoregistration::PivotState::Collecting);

    for (int i = 0; i < 14; ++i) {
        const float phi = qDegreesToRadians(25.0f * i);
        const QQuaternion r = QQuaternion::fromAxisAndAngle(QVector3D(std::cos(phi), std::sin(phi), 0.3f), 30.0f);
        feed(coreg, 1, tip - r.rotatedVector(offset), r);
        // A repeat of the same orientation and a gimbal-locked pose are not collected
        feed(coreg, 1, tip - r.rotatedVector(offset), r);
        feed(coreg, 1, tip, QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 85.0f));
    }
    QCOMPARE(coreg.pivotSampleCount(), 14);
    QCOMPARE(sampleSpy.count(), 14);
    QVERIFY(sampleSpy.last().at(1).toFloat() > 30.0f);

    coreg.on_button(1, toEigen(tip), Eigen::Quaternionf::Identity());
    QCOMPARE(coreg.pivotState(), PolhemusCoregistration::PivotState::Done);
    QCOMPARE(doneSpy.count(), 1);
    QVERIFY2((toQt(coreg.penTipOffset()) - offset).length() < 1e-5f,
             qPrintable(QString("offset %1 %2 %3").arg(toQt(coreg.penTipOffset()).x()).arg(toQt(coreg.penTipOffset()).y()).arg(toQt(coreg.penTipOffset()).z())));
    QVERIFY(coreg.pivotResidualMm() < 0.01f);
}

//=============================================================================================================

void TestPolhemusCoregistration::pivotCalibration_rejectsTooFewOrSingleAxisSamples()
{
    PolhemusCoregistration coreg;
    const QVector3D offset(0.0f, 0.0f, -0.12f);

    // Fewer than 10 samples
    coreg.startPivotCalibration();
    coreg.on_button(1, toEigen(QVector3D()), Eigen::Quaternionf::Identity());
    for (int i = 0; i < 5; ++i) {
        const QQuaternion r = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 10.0f * i);
        feed(coreg, 1, -r.rotatedVector(offset), r);
    }
    coreg.on_button(1, toEigen(QVector3D()), Eigen::Quaternionf::Identity());
    QCOMPARE(coreg.pivotState(), PolhemusCoregistration::PivotState::Idle);

    // Rotations about the pen's own axis leave tip depth and offset length indistinguishable
    coreg.startPivotCalibration();
    coreg.on_button(1, toEigen(QVector3D()), Eigen::Quaternionf::Identity());
    for (int i = 0; i < 12; ++i) {
        const QQuaternion r = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 10.0f * i);
        feed(coreg, 1, -r.rotatedVector(offset), r);
    }
    QCOMPARE(coreg.pivotSampleCount(), 12);
    coreg.on_button(1, toEigen(QVector3D()), Eigen::Quaternionf::Identity());
    QCOMPARE(coreg.pivotState(), PolhemusCoregistration::PivotState::Idle);
    QCOMPARE(toQt(coreg.penTipOffset()), QVector3D());
}

//=============================================================================================================

void TestPolhemusCoregistration::calibrateOptics(PolhemusCoregistration& coreg, const QVector3D& center, const QVector3D& axis)
{
    // Focus points on the optical axis, seen from five different tracker poses
    for (int k = 0; k < 5; ++k) {
        const QQuaternion trackerOri = QQuaternion::fromAxisAndAngle(QVector3D(0.2f * k, 1.0f, 0.5f), 15.0f + 7.0f * k);
        const QVector3D trackerPos(0.3f + 0.01f * k, -0.1f, 0.4f - 0.02f * k);
        feed(coreg, 2, trackerPos, trackerOri);
        feed(coreg, 1, trackerPos + trackerOri.rotatedVector(center + (0.10f + 0.06f * k) * axis), QQuaternion());
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
    QSignalSpy changedSpy(&coreg, &PolhemusCoregistration::opticalCalibrationChanged);

    const QVector3D center(0.05f, 0.18f, -0.03f);
    const QVector3D axis = QVector3D(0.1f, 1.0f, -0.3f).normalized();
    coreg.setKnownTrackerToObjectiveDistance(mode == 1 ? center.length() : 0.0f);

    calibrateOptics(coreg, center, axis);
    if (mode == 0) {
        const QQuaternion trackerOri = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 40.0f);
        feed(coreg, 2, QVector3D(0.1f, 0.1f, 0.1f), trackerOri);
        feed(coreg, 1, QVector3D(0.1f, 0.1f, 0.1f) + trackerOri.rotatedVector(center), QQuaternion());
        QVERIFY(coreg.captureObjectiveCenter());
        QVERIFY((toQt(coreg.objectiveCenterLocal()) - center).length() < 1e-5f);
    }
    QVERIFY(coreg.solveOpticalCalibration());
    QVERIFY(coreg.opticalCalibrationValid());
    QCOMPARE(changedSpy.count(), 1);

    QVERIFY2(QVector3D::dotProduct(toQt(coreg.opticalAxisLocal()), axis) > 1.0f - 1e-5f,
             qPrintable(QString("axis %1 %2 %3").arg(toQt(coreg.opticalAxisLocal()).x()).arg(toQt(coreg.opticalAxisLocal()).y()).arg(toQt(coreg.opticalAxisLocal()).z())));
    QVERIFY(coreg.opticalCalibResidualMm() < 0.05f);
    QVERIFY(std::fabs(coreg.opticalCalibDepthSpreadMm() - 240.0f) < 0.1f);
    // The centre lies on the axis; with a captured centre or a known distance it is the true one
    const QVector3D c = toQt(coreg.opticalCenterLocal());
    const QVector3D rel = c - center;
    QVERIFY((rel - QVector3D::dotProduct(rel, axis) * axis).length() < 1e-5f);
    if (mode != 2) {
        QVERIFY2(rel.length() < 1e-4f, qPrintable(QString("centre off by %1 m").arg(rel.length())));
    }

    // World ray and up vector for the current tracker pose
    const QQuaternion trackerOri = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 20.0f);
    const QVector3D trackerPos(0.2f, 0.0f, 0.1f);
    feed(coreg, 2, trackerPos, trackerOri);
    QVector3D origin, direction, up;
    QVERIFY(opticalRayInWorld(coreg, origin, direction));
    QVERIFY((origin - (trackerPos + trackerOri.rotatedVector(c))).length() < 1e-5f);
    QVERIFY((direction - trackerOri.rotatedVector(axis)).length() < 1e-4f);
    QVERIFY(opticalUpInWorld(coreg, up));
    QVERIFY(std::fabs(QVector3D::dotProduct(up, direction)) < 1e-5f);
    QVERIFY(QVector3D::dotProduct(up, trackerOri.rotatedVector(QVector3D(0, 0, 1))) > 0.9f);

    // Fine adjustment towards a target 2 degrees off the axis, then undo
    const QVector3D tilted = QQuaternion::fromAxisAndAngle(up, 2.0f).rotatedVector(direction);
    float correction = 0.0f;
    QVERIFY(coreg.applyOpticalAxisFineAdjust(toEigen(origin + 0.3f * tilted), correction));
    QVERIFY(std::fabs(correction - 2.0f) < 0.01f);
    QVERIFY(coreg.opticalFineAdjustApplied());
    QCOMPARE(coreg.opticalFineAdjustDeg(), correction);
    QVERIFY(opticalRayInWorld(coreg, origin, direction));
    QVERIFY((direction - tilted).length() < 1e-4f);
    QVERIFY(!coreg.applyOpticalAxisFineAdjust(toEigen(origin + 0.3f * QQuaternion::fromAxisAndAngle(up, 20.0f).rotatedVector(direction)), correction));
    QVERIFY(correction > 10.0f);
    QVERIFY(!coreg.applyOpticalAxisFineAdjust(toEigen(origin), correction));
    coreg.clearOpticalFineAdjust();
    QVERIFY(!coreg.opticalFineAdjustApplied());
    QVERIFY((toQt(coreg.opticalAxisLocal()) - axis).length() < 1e-4f);

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
    const QVector3D center(0.0f, 0.2f, 0.0f);
    const QVector3D axis = QVector3D(0.0f, 1.0f, 0.2f).normalized();
    calibrateOptics(first, center, axis);
    feed(first, 1, QVector3D(0.3f, 0.0f, 0.4f) + center, QQuaternion());
    feed(first, 2, QVector3D(0.3f, 0.0f, 0.4f), QQuaternion());
    QVERIFY(first.captureObjectiveCenter());
    QVERIFY(first.solveOpticalCalibration());
    QVector3D origin, direction;
    QVERIFY(opticalRayInWorld(first, origin, direction));
    float correction = 0.0f;
    QVERIFY(first.applyOpticalAxisFineAdjust(toEigen(origin + 0.3f * QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 1.0f).rotatedVector(direction)), correction));
    first.setModelVertex(toEigen(QVector3D(0.0f, 0.0f, 0.12f)));
    feed(first, 1, QVector3D(0.0f, 0.01f, 0.11f), QQuaternion());
    QVERIFY(first.captureCurrentPenPositionAsVertex());
    const QVector<std::pair<FiducialId, QVector3D>> fids = {
        {FiducialId::LPA, QVector3D(-0.07f, 0, 0)}, {FiducialId::NAS, QVector3D(0, 0.1f, 0)}, {FiducialId::RPA, QVector3D(0.07f, 0, 0)}};
    for (const auto& [id, pos] : fids) {
        feed(first, 1, pos, QQuaternion());
        QVERIFY(first.captureCurrentPenPositionAsFiducial(id));
    }
    QVERIFY(first.computeRegistration());

    QSettings saved(tmpDir.path() + "/optical.ini", QSettings::IniFormat);
    first.saveSessionState(saved);
    saved.sync();

    // The restoring side already holds different live fiducials; the restored ones must replace them
    PolhemusCoregistration second;
    for (const auto& [id, pos] : fids) {
        feed(second, 1, pos + QVector3D(0.0f, 0.0f, 0.05f), QQuaternion());
        QVERIFY(second.captureCurrentPenPositionAsFiducial(id));
    }
    QVERIFY(second.restoreSessionState(saved));
    QVERIFY(second.restoreSessionState(saved));
    QCOMPARE(second.acquiredPoints()->countOf(PointKind::Fiducial), 3);
    for (const auto& [id, pos] : fids) {
        QCOMPARE(toQt(second.acquiredPoints()->fiducial(id)), pos);
    }

    QCOMPARE(second.opticalCalibrationValid(), true);
    QCOMPARE(second.opticalCalibSampleCount(), 5);
    QCOMPARE(second.hasObjectiveCenter(), true);
    QVERIFY((toQt(second.objectiveCenterLocal()) - toQt(first.objectiveCenterLocal())).length() < 1e-6f);
    QVERIFY((toQt(second.opticalAxisLocal()) - toQt(first.opticalAxisLocal())).length() < 1e-6f);
    QVERIFY((toQt(second.opticalCenterLocal()) - toQt(first.opticalCenterLocal())).length() < 1e-6f);
    QCOMPARE(second.opticalFineAdjustApplied(), true);
    QVERIFY(std::fabs(second.opticalFineAdjustDeg() - first.opticalFineAdjustDeg()) < 1e-6f);
    QVERIFY(std::fabs(second.opticalCalibResidualMm() - first.opticalCalibResidualMm()) < 1e-4f);
    QVERIFY(std::fabs(second.opticalCalibDepthSpreadMm() - first.opticalCalibDepthSpreadMm()) < 1e-3f);
    QCOMPARE(second.hasPenVertex(), true);
    QCOMPARE(second.hasModelVertex(), true);
    QCOMPARE(toQt(second.penVertex()), toQt(first.penVertex()));
    QCOMPARE(toQt(second.modelVertex()), toQt(first.modelVertex()));
    QCOMPARE(toQt(second.headToWorld()), toQt(first.headToWorld()));
    second.clearOpticalFineAdjust();
    QVERIFY((toQt(second.opticalAxisLocal()) - axis).length() < 1e-4f);
    QVERIFY(second.solveOpticalCalibration());

    QSettings empty(tmpDir.path() + "/empty.ini", QSettings::IniFormat);
    QVERIFY(!second.restoreSessionState(empty));
}

//=============================================================================================================

void TestPolhemusCoregistration::digitizer_feedsTheSimulatedStylusAndTracker()
{
    MNEALIGN::Digitizer digitizer;
    PolhemusCoregistration coreg;
    connect(&digitizer, &MNEALIGN::Digitizer::pointReceived, &coreg, &PolhemusCoregistration::on_pose);
    QSignalSpy connectedSpy(&digitizer, &MNEALIGN::Digitizer::connectedChanged);
    QSignalSpy pointSpy(&digitizer, &MNEALIGN::Digitizer::pointReceived);
    QSignalSpy deviceSpy(&coreg, &PolhemusCoregistration::devicePoseChanged);

    QVERIFY(digitizer.open(QString()));
    QVERIFY(digitizer.isConnected());
    QVERIFY(digitizer.description().contains("simulated"));
    QVERIFY(digitizer.open(QString())); // already open
    QCOMPARE(connectedSpy.count(), 1);

    QVERIFY(QTest::qWaitFor([&] { return coreg.haveLivePenPosition() && deviceSpy.count() > 0; }, 5000));
    // The simulated stylus sweeps a head of 10 cm radius
    QVERIFY(std::abs(coreg.penPosition().head<2>().norm() - 0.10f) < 1e-3f);
    QVERIFY(QTest::qWaitFor([&] { return pointSpy.count() >= 20; }, 5000));
    for (const QList<QVariant>& point : pointSpy) {
        QVERIFY(point.at(0).toInt() == 1 || point.at(0).toInt() == 2);
    }

    digitizer.close();
    QVERIFY(!digitizer.isConnected());
    QCOMPARE(connectedSpy.count(), 2);
    QVERIFY(digitizer.description().isEmpty());
    const qsizetype points = pointSpy.count();
    QTest::qWait(100);
    QCOMPARE(pointSpy.count(), points);
}

//=============================================================================================================

void TestPolhemusCoregistration::digitizer_reportsAMissingPort()
{
    MNEALIGN::Digitizer digitizer;
    QSignalSpy errorSpy(&digitizer, &MNEALIGN::Digitizer::errorOccurred);
    QVERIFY(!digitizer.open(QStringLiteral("/no/such/port")));
    QVERIFY(!digitizer.isConnected());
    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(errorSpy.first().first().toString().contains("/no/such/port"));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestPolhemusCoregistration)
#include "test_polhemus_coregistration.moc"
