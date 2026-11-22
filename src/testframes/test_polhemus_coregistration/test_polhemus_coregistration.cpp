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

#include <mne/acq/devices/polhemus/polhemus_coregistration.h>
#include <mne/acq/devices/polhemus/acquired_points.h>
#include <mne/base/json.h>

#include <digitizer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QMatrix4x4>
#include <QSignalSpy>
#include <QtMath>
#include <QQuaternion>
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

bool optical_ray_in_world(const PolhemusCoregistration& coreg, QVector3D& origin, QVector3D& direction)
{
    Eigen::Vector3f o = Eigen::Vector3f::Zero();
    Eigen::Vector3f d = Eigen::Vector3f::Zero();
    const bool ok = coreg.optical_ray_in_world(o, d);
    origin = toQt(o);
    direction = toQt(d);
    return ok;
}

bool optical_up_in_world(const PolhemusCoregistration& coreg, QVector3D& up)
{
    Eigen::Vector3f u = Eigen::Vector3f::Zero();
    const bool ok = coreg.optical_up_in_world(u);
    up = toQt(u);
    return ok;
}

/** Counts the events a coregistration reports through its listener. */
struct Events
{
    explicit Events(PolhemusCoregistration& coreg)
    {
        PolhemusCoregistration::Listener listener;
        listener.device_pose_changed = [this](const Eigen::Matrix4f&) {
            ++device_poses;
        };
        listener.pen_pose_changed = [this](const Eigen::Vector3f&, const Eigen::Quaternionf&) {
            ++pen_poses;
        };
        listener.probe_pose_changed = [this](const Eigen::Vector3f&, const Eigen::Quaternionf&) {
            ++probe_poses;
        };
        listener.pen_button_pressed = [this](const Eigen::Vector3f& position, const Eigen::Quaternionf&) {
            button_presses.push_back(position);
        };
        listener.registration_changed = [this] {
            ++registrations;
        };
        listener.pivot_state_changed = [this](PolhemusCoregistration::PivotState) {
            ++pivot_states;
        };
        listener.pivot_calibration_done = [this](const Eigen::Vector3f&, float) {
            ++pivot_done;
        };
        listener.pivot_sample_collected = [this](int, float span) {
            pivot_spans.push_back(span);
        };
        listener.optical_calibration_changed = [this] {
            ++optical_changes;
        };
        coreg.set_listener(std::move(listener));
    }
    Events(const Events&) = delete;
    Events& operator=(const Events&) = delete;

    int device_poses = 0;
    int pen_poses = 0;
    int probe_poses = 0;
    std::vector<Eigen::Vector3f> button_presses;
    int registrations = 0;
    int pivot_states = 0;
    int pivot_done = 0;
    std::vector<float> pivot_spans;
    int optical_changes = 0;
};

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
                              const QVector3D& penLpa,
                              const QVector3D& penNas,
                              const QVector3D& penRpa,
                              const QVector3D& modelLpa,
                              const QVector3D& modelNas,
                              const QVector3D& modelRpa);

    //=========================================================================================================
    /**
     * Stores a fiducial the way session_state() does, so restore_session_state() reads it back.
     */
    static void writeFid(JsonObject& state, const char* group, const char* name, const QVector3D& v);

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
    void sessionState_namesTheFiducials();

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

    QCOMPARE(coreg.tracker_station(), 2);
    QCOMPARE(coreg.pen_station(), 1);
    QCOMPARE(coreg.probe_station(), 3);
    QVERIFY(toQt(coreg.device_to_world()).isIdentity());
    QVERIFY(toQt(coreg.head_to_world()).isIdentity());
    QVERIFY(toQt(coreg.head_to_device()).isIdentity());
    QVERIFY(toQt(coreg.world_to_model()).isIdentity());
    QVERIFY(!coreg.registration_valid());
    QVERIFY(!coreg.have_live_pen_position());
    QVERIFY(!coreg.have_live_probe_position());

    coreg.set_tracker_station(5);
    coreg.set_pen_station(6);
    coreg.set_probe_station(7);
    QCOMPARE(coreg.tracker_station(), 5);
    QCOMPARE(coreg.pen_station(), 6);
    QCOMPARE(coreg.probe_station(), 7);

    const QVector3D tipOffset(0.001f, -0.002f, 0.003f);
    coreg.set_pen_tip_offset(toEigen(tipOffset));
    coreg.set_tip_offset_enabled(true);
    QCOMPARE(toQt(coreg.pen_tip_offset()), tipOffset);
    QVERIFY(coreg.tip_offset_enabled());

    coreg.set_axis_mirror(true, true);
    QVERIFY(coreg.mirror_x());
    QVERIFY(coreg.mirror_y());

    const QVector3D trackerOffset(0.01f, 0.02f, -0.03f);
    const QQuaternion trackerRotation = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 25.0f);
    coreg.set_tracker_to_device_offset(toEigen(trackerOffset), toEigen(trackerRotation));
    QCOMPARE(toQt(coreg.tracker_to_device_translation()), trackerOffset);
    QCOMPARE(toQt(coreg.tracker_to_device_rotation()), trackerRotation);

    coreg.set_known_tracker_to_objective_distance(0.15f);
    QCOMPARE(coreg.known_tracker_to_objective_distance(), 0.15f);
}

//=============================================================================================================

void TestPolhemusCoregistration::liveDataOperationsRequireSamples()
{
    PolhemusCoregistration coreg;

    QVERIFY(!coreg.capture_current_pen_position_as_fiducial(FiducialId::LPA));
    QVERIFY(!coreg.capture_current_pen_position_as_head_shape());
    QVERIFY(!coreg.capture_current_pen_position_as_vertex());
    QVERIFY(!coreg.capture_optical_calib_sample());
    QVERIFY(!coreg.capture_objective_center());
    QVERIFY(!coreg.solve_optical_calibration());

    QVector3D origin;
    QVector3D direction;
    QVector3D up;
    float correctionDeg = -1.0f;
    QVERIFY(!optical_ray_in_world(coreg, origin, direction));
    QVERIFY(!optical_up_in_world(coreg, up));
    QVERIFY(!coreg.apply_optical_axis_fine_adjust(toEigen(QVector3D(0.0f, 0.0f, 1.0f)), correctionDeg));

    coreg.clear_optical_calib_samples();
    coreg.clear_objective_center();
    coreg.clear_optical_fine_adjust();
    QCOMPARE(coreg.optical_calib_sample_count(), 0);
    QVERIFY(!coreg.optical_calibration_valid());
    QVERIFY(!coreg.has_objective_center());
    QVERIFY(!coreg.optical_fine_adjust_applied());
}

//=============================================================================================================

void TestPolhemusCoregistration::pivotAndRegistrationReset()
{
    PolhemusCoregistration coreg;
    Events events(coreg);

    QCOMPARE(coreg.pivot_state(), PolhemusCoregistration::PivotState::Idle);
    coreg.start_pivot_calibration();
    QCOMPARE(coreg.pivot_state(), PolhemusCoregistration::PivotState::WaitingForStart);
    QCOMPARE(coreg.pivot_sample_count(), 0);
    QCOMPARE(events.pivot_states, 1);

    coreg.cancel_pivot_calibration();
    QCOMPARE(coreg.pivot_state(), PolhemusCoregistration::PivotState::Idle);
    QCOMPARE(coreg.pivot_sample_count(), 0);
    QCOMPARE(events.pivot_states, 2);

    coreg.set_model_vertex(toEigen(QVector3D(0.0f, 0.1f, 0.1f)));
    QVERIFY(coreg.has_model_vertex());
    coreg.reset_registration();
    QVERIFY(!coreg.registration_valid());
    QVERIFY(toQt(coreg.head_to_world()).isIdentity());
    QVERIFY(toQt(coreg.head_to_device()).isIdentity());
    QVERIFY(toQt(coreg.world_to_model()).isIdentity());
    QVERIFY(!coreg.has_pen_vertex());
    QVERIFY(!coreg.has_all_pen_fiducials());
    QCOMPARE(events.registrations, 1);
}

//=============================================================================================================

void TestPolhemusCoregistration::writeFid(JsonObject& state, const char* group, const char* name, const QVector3D& v)
{
    if (!state[group].is_object())
        state[group] = JsonObject();
    JsonObject fids = state[group].to_object();
    JsonObject xyz;
    xyz["x"] = static_cast<double>(v.x());
    xyz["y"] = static_cast<double>(v.y());
    xyz["z"] = static_cast<double>(v.z());
    fids[name] = xyz;
    state[group] = fids;
}

//=============================================================================================================

void TestPolhemusCoregistration::seedFiducials(PolhemusCoregistration& coreg,
                                               const QVector3D& penLpa,
                                               const QVector3D& penNas,
                                               const QVector3D& penRpa,
                                               const QVector3D& modelLpa,
                                               const QVector3D& modelNas,
                                               const QVector3D& modelRpa)
{
    JsonObject state;
    writeFid(state, "penFid", "LPA", penLpa);
    writeFid(state, "penFid", "NAS", penNas);
    writeFid(state, "penFid", "RPA", penRpa);
    writeFid(state, "modelFid", "LPA", modelLpa);
    writeFid(state, "modelFid", "NAS", modelNas);
    writeFid(state, "modelFid", "RPA", modelRpa);

    // restore_session_state reports whether a valid registration came back, not
    // whether the state was read, so a state that only carries fiducials
    // returns false while still loading them. That is the contract,
    // hence no QVERIFY on the return here.
    coreg.restore_session_state(state);
    QVERIFY2(coreg.has_all_pen_fiducials(),
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

    PolhemusCoregistration coreg;
    seedFiducials(coreg, penLpa, penNas, penRpa, modelLpa, modelNas, modelRpa);

    QVERIFY(coreg.compute_registration());
    QVERIFY(coreg.registration_valid());

    // The paired SVD path writes its result into world_to_model: that is the
    // pen-to-model fit. head_to_device is a different quantity, derived from
    // the tracker's device-to-world transform, and stays identity here since
    // no tracker is attached.
    //
    // The fit is only meaningful through what it does to points, so check the
    // mapping rather than the matrix entries, which lets an equally valid but
    // differently expressed transform pass.
    const QMatrix4x4 actual = toQt(coreg.world_to_model());

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

    PolhemusCoregistration coreg;
    // Model side identical to the pen side, so only the spread decides.
    seedFiducials(coreg, lpa, nas, rpa, lpa, nas, rpa);

    QCOMPARE(coreg.compute_registration(), expectSuccess);
    QCOMPARE(coreg.registration_valid(), expectSuccess);
}

//=============================================================================================================

void TestPolhemusCoregistration::registration_requiresAllFiducials()
{
    PolhemusCoregistration coreg;

    // Nothing captured at all.
    QVERIFY(!coreg.compute_registration());

    // Only two of the three captured, which is not enough to fix a rigid
    // transform and must be refused rather than fitted.
    JsonObject state;
    writeFid(state, "penFid", "LPA", QVector3D(-0.075f, 0.0f, 0.0f));
    writeFid(state, "penFid", "NAS", QVector3D(0.0f, 0.095f, 0.0f));
    coreg.restore_session_state(state);
    QVERIFY(!coreg.has_all_pen_fiducials());
    QVERIFY(!coreg.compute_registration());
}

//=============================================================================================================

void TestPolhemusCoregistration::sessionState_keepsTheStations()
{
    // Every station comes back, the probe station included
    PolhemusCoregistration first;
    first.set_tracker_station(4);
    first.set_pen_station(5);
    first.set_probe_station(7);
    PolhemusCoregistration second;
    second.restore_session_state(first.session_state());
    QCOMPARE(second.tracker_station(), 4);
    QCOMPARE(second.pen_station(), 5);
    QCOMPARE(second.probe_station(), 7);

    // A state without stations leaves the default assignment, not pen and tracker swapped
    JsonObject partial;
    partial["mirrorX"] = true;
    PolhemusCoregistration third;
    third.restore_session_state(partial);
    QCOMPARE(third.tracker_station(), 2);
    QCOMPARE(third.pen_station(), 1);
    QCOMPARE(third.probe_station(), 3);
}

//=============================================================================================================

void TestPolhemusCoregistration::sessionState_namesTheFiducials()
{
    // A fiducial captured as LPA is stored under LPA, not under the name of the next one
    PolhemusCoregistration coreg;
    const QVector3D lpa(-0.075f, 0.0f, 0.0f), nas(0.0f, 0.095f, 0.0f), rpa(0.075f, 0.0f, 0.0f);
    for (const auto& [id, pos] : {std::pair{FiducialId::LPA, lpa}, std::pair{FiducialId::NAS, nas}, std::pair{FiducialId::RPA, rpa}}) {
        feed(coreg, 1, pos, QQuaternion());
        QVERIFY(coreg.capture_current_pen_position_as_fiducial(id));
        coreg.set_model_fiducial(id, toEigen(pos));
    }
    const JsonObject saved = coreg.session_state();
    for (const auto& [name, pos] : {std::pair{"LPA", lpa}, std::pair{"NAS", nas}, std::pair{"RPA", rpa}}) {
        QCOMPARE(static_cast<float>(saved["penFid"][name]["x"].to_double()), pos.x());
        QCOMPARE(static_cast<float>(saved["penFid"][name]["y"].to_double()), pos.y());
        QCOMPARE(static_cast<float>(saved["modelFid"][name]["x"].to_double()), pos.x());
    }
    QCOMPARE(saved["penFid"].to_object().size(), std::size_t(3));
}

//=============================================================================================================

void TestPolhemusCoregistration::sessionState_roundTrip()
{
    const QVector3D penLpa(-0.075f, 0.001f, 0.002f);
    const QVector3D penNas(0.003f, 0.095f, 0.004f);
    const QVector3D penRpa(0.075f, 0.005f, 0.006f);

    PolhemusCoregistration first;
    seedFiducials(first, penLpa, penNas, penRpa, penLpa, penNas, penRpa);
    first.set_tracker_station(3);
    first.set_pen_station(4);
    QVERIFY(first.compute_registration());

    // Through text, the way an application stores it
    const JsonObject saved = JsonDocument::from_json(JsonDocument(first.session_state()).to_json()).object();

    PolhemusCoregistration second;
    // Here the saved state does carry a valid registration, so the return
    // value is expected to be true, unlike in seedFiducials.
    QVERIFY(second.restore_session_state(saved));

    // What was written has to come back. Comparing only a flag would pass even
    // if every coordinate were lost.
    QCOMPARE(second.has_all_pen_fiducials(), first.has_all_pen_fiducials());

    QVERIFY(second.compute_registration());

    const QVector3D probe(0.01f, 0.02f, 0.03f);
    const QVector3D mappedFirst = toQt(first.world_to_model()).map(probe);
    const QVector3D mappedSecond = toQt(second.world_to_model()).map(probe);
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
    Events events(coreg);

    // Tracker (station 2): device_to_world = T(pos) R(ori) T(offset) R(offsetRot)
    const QQuaternion tracker_ori = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 30.0f);
    const QQuaternion offsetRot = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 90.0f);
    coreg.set_tracker_to_device_offset(Eigen::Vector3f(0.01f, 0.0f, 0.0f), toEigen(offsetRot));
    feed(coreg, 2, QVector3D(0.1f, 0.2f, 0.3f), tracker_ori);
    QCOMPARE(events.device_poses, 1);
    const QVector3D deviceOrigin = toQt(coreg.device_to_world()).map(QVector3D());
    const QVector3D expectedOrigin = QVector3D(0.1f, 0.2f, 0.3f) + tracker_ori.rotatedVector(QVector3D(0.01f, 0, 0));
    QVERIFY((deviceOrigin - expectedOrigin).length() < 1e-6f);
    const QVector3D deviceY = toQt(coreg.device_to_world()).mapVector(QVector3D(0, 1, 0));
    QVERIFY((deviceY - (tracker_ori * offsetRot).rotatedVector(QVector3D(0, 1, 0))).length() < 1e-6f);

    // Pen (station 1): mirrored X/Y, tip offset rotated by the sensor orientation
    const QQuaternion penOri = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 90.0f);
    coreg.set_axis_mirror(true, true);
    coreg.set_pen_tip_offset(toEigen(QVector3D(0.0f, 0.0f, -0.1f)));
    coreg.set_tip_offset_enabled(true);
    feed(coreg, 1, QVector3D(0.05f, 0.06f, 0.07f), penOri);
    QVERIFY(coreg.have_live_pen_position());
    QVERIFY((toQt(coreg.pen_position()) - QVector3D(-0.05f, 0.04f, 0.07f)).length() < 1e-6f);
    QCOMPARE(events.pen_poses, 1);

    // Near gimbal lock (pitch 85 deg about Y) the pose is frozen, but the signal still fires
    feed(coreg, 1, QVector3D(0.5f, 0.5f, 0.5f), QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 85.0f));
    QVERIFY((toQt(coreg.pen_position()) - QVector3D(-0.05f, 0.04f, 0.07f)).length() < 1e-6f);
    QCOMPARE(events.pen_poses, 2);

    // Probe (station 3) and an unknown station
    feed(coreg, 3, QVector3D(0.2f, 0.1f, 0.0f), penOri);
    QVERIFY(coreg.have_live_probe_position());
    QCOMPARE(toQt(coreg.probe_position()), QVector3D(-0.2f, -0.1f, 0.0f));
    QCOMPARE(toQt(coreg.probe_orientation()), penOri);
    feed(coreg, 4, QVector3D(), QQuaternion());
    QCOMPARE(events.device_poses + events.pen_poses + events.probe_poses, 4);
}

//=============================================================================================================

void TestPolhemusCoregistration::liveCapture_fiducialsHeadShapeVertexAndButton()
{
    PolhemusCoregistration coreg;
    Events events(coreg);

    const QVector3D lpa(-0.075f, 0.0f, 0.0f), nas(0.0f, 0.095f, 0.0f), rpa(0.075f, 0.0f, 0.0f);
    const QVector<std::pair<FiducialId, QVector3D>> fids = {
        {FiducialId::LPA, lpa + QVector3D(0, 0, 0.01f)}, {FiducialId::LPA, lpa}, {FiducialId::NAS, nas}, {FiducialId::RPA, rpa}};
    for (const auto& [id, pos] : fids) {
        feed(coreg, 1, pos, QQuaternion());
        QVERIFY(coreg.capture_current_pen_position_as_fiducial(id));
    }
    // Re-capturing LPA replaced the first one
    QCOMPARE(coreg.acquired_points().count_of(PointKind::Fiducial), 3);
    QCOMPARE(toQt(coreg.acquired_points().fiducial(FiducialId::LPA)), lpa);
    QVERIFY(coreg.has_all_pen_fiducials());

    for (int i = 0; i < 2; ++i) {
        feed(coreg, 1, QVector3D(0.01f * i, 0.02f, 0.09f), QQuaternion());
        QVERIFY(coreg.capture_current_pen_position_as_head_shape());
    }
    QCOMPARE(coreg.acquired_points().count_of(PointKind::HeadShape), 2);
    QCOMPARE(coreg.acquired_points().points().back().label, std::string("HSP-2"));

    feed(coreg, 1, QVector3D(0.0f, 0.02f, 0.1f), QQuaternion());
    QVERIFY(coreg.capture_current_pen_position_as_vertex());
    QVERIFY(coreg.has_pen_vertex());
    QCOMPARE(toQt(coreg.pen_vertex()), QVector3D(0.0f, 0.02f, 0.1f));

    // The pen button is forwarded only for the pen station, with the tip-adjusted position
    coreg.on_button(3, toEigen(QVector3D(1, 1, 1)), Eigen::Quaternionf::Identity());
    QCOMPARE(events.button_presses.size(), std::size_t(0));
    coreg.on_button(1, toEigen(QVector3D(0.03f, 0.0f, 0.0f)), Eigen::Quaternionf::Identity());
    QCOMPARE(events.button_presses.size(), std::size_t(1));
    QCOMPARE(toQt(events.button_presses.front()), QVector3D(0.03f, 0.0f, 0.0f));
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
        QVERIFY(coreg.capture_current_pen_position_as_fiducial(id));
    }
    feed(coreg, 2, QVector3D(0.2f, 0.0f, 0.0f), QQuaternion());

    QVERIFY(coreg.compute_registration());
    // Head frame: origin midway between the ears, x towards NAS, y towards LPA (orthogonalised), z = x cross y
    const QMatrix4x4 h = toQt(coreg.head_to_world());
    QVERIFY((h.map(QVector3D()) - o).length() < 1e-6f);
    const QVector3D ex = QVector3D(0.01f, 0.1f, 0).normalized();
    QVERIFY((h.mapVector(QVector3D(1, 0, 0)) - ex).length() < 1e-5f);
    const QVector3D ey = h.mapVector(QVector3D(0, 1, 0));
    QVERIFY(std::fabs(QVector3D::dotProduct(ey, ex)) < 1e-5f);
    QVERIFY(ey.x() < 0.0f);
    QVERIFY((h.mapVector(QVector3D(0, 0, 1)) - QVector3D(0, 0, 1)).length() < 1e-5f);
    QVERIFY(toQt(coreg.world_to_model()).isIdentity());
    QVERIFY((toQt(coreg.head_to_device()).map(QVector3D()) - (o - QVector3D(0.2f, 0, 0))).length() < 1e-6f);

    // NAS on the ear line: no frame can be built
    PolhemusCoregistration flat;
    const QVector<std::pair<FiducialId, QVector3D>> line = {
        {FiducialId::LPA, QVector3D(-0.07f, 0, 0)}, {FiducialId::NAS, QVector3D(0.03f, 0, 0)}, {FiducialId::RPA, QVector3D(0.07f, 0, 0)}};
    for (const auto& [id, pos] : line) {
        feed(flat, 1, pos, QQuaternion());
        QVERIFY(flat.capture_current_pen_position_as_fiducial(id));
    }
    QVERIFY(!flat.compute_registration());
}

//=============================================================================================================

void TestPolhemusCoregistration::pivotCalibration_recoversTipOffset()
{
    PolhemusCoregistration coreg;
    Events events(coreg);

    // The pen pivots about a fixed tip T; the sensor sits at T - R_i * offset
    const QVector3D tip(0.1f, 0.2f, 0.05f);
    const QVector3D offset(0.01f, -0.02f, -0.12f);
    coreg.start_pivot_calibration();
    coreg.on_button(1, toEigen(tip), Eigen::Quaternionf::Identity());
    QCOMPARE(coreg.pivot_state(), PolhemusCoregistration::PivotState::Collecting);

    for (int i = 0; i < 14; ++i) {
        const float phi = qDegreesToRadians(25.0f * i);
        const QQuaternion r = QQuaternion::fromAxisAndAngle(QVector3D(std::cos(phi), std::sin(phi), 0.3f), 30.0f);
        feed(coreg, 1, tip - r.rotatedVector(offset), r);
        // A repeat of the same orientation and a gimbal-locked pose are not collected
        feed(coreg, 1, tip - r.rotatedVector(offset), r);
        feed(coreg, 1, tip, QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), 85.0f));
    }
    QCOMPARE(coreg.pivot_sample_count(), 14);
    QCOMPARE(events.pivot_spans.size(), std::size_t(14));
    QVERIFY(events.pivot_spans.back() > 30.0f);

    coreg.on_button(1, toEigen(tip), Eigen::Quaternionf::Identity());
    QCOMPARE(coreg.pivot_state(), PolhemusCoregistration::PivotState::Done);
    QCOMPARE(events.pivot_done, 1);
    QVERIFY2((toQt(coreg.pen_tip_offset()) - offset).length() < 1e-5f,
             qPrintable(QString("offset %1 %2 %3").arg(toQt(coreg.pen_tip_offset()).x()).arg(toQt(coreg.pen_tip_offset()).y()).arg(toQt(coreg.pen_tip_offset()).z())));
    QVERIFY(coreg.pivot_residual_mm() < 0.01f);
}

//=============================================================================================================

void TestPolhemusCoregistration::pivotCalibration_rejectsTooFewOrSingleAxisSamples()
{
    PolhemusCoregistration coreg;
    const QVector3D offset(0.0f, 0.0f, -0.12f);

    // Fewer than 10 samples
    coreg.start_pivot_calibration();
    coreg.on_button(1, toEigen(QVector3D()), Eigen::Quaternionf::Identity());
    for (int i = 0; i < 5; ++i) {
        const QQuaternion r = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 10.0f * i);
        feed(coreg, 1, -r.rotatedVector(offset), r);
    }
    coreg.on_button(1, toEigen(QVector3D()), Eigen::Quaternionf::Identity());
    QCOMPARE(coreg.pivot_state(), PolhemusCoregistration::PivotState::Idle);

    // Rotations about the pen's own axis leave tip depth and offset length indistinguishable
    coreg.start_pivot_calibration();
    coreg.on_button(1, toEigen(QVector3D()), Eigen::Quaternionf::Identity());
    for (int i = 0; i < 12; ++i) {
        const QQuaternion r = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 10.0f * i);
        feed(coreg, 1, -r.rotatedVector(offset), r);
    }
    QCOMPARE(coreg.pivot_sample_count(), 12);
    coreg.on_button(1, toEigen(QVector3D()), Eigen::Quaternionf::Identity());
    QCOMPARE(coreg.pivot_state(), PolhemusCoregistration::PivotState::Idle);
    QCOMPARE(toQt(coreg.pen_tip_offset()), QVector3D());
}

//=============================================================================================================

void TestPolhemusCoregistration::calibrateOptics(PolhemusCoregistration& coreg, const QVector3D& center, const QVector3D& axis)
{
    // Focus points on the optical axis, seen from five different tracker poses
    for (int k = 0; k < 5; ++k) {
        const QQuaternion tracker_ori = QQuaternion::fromAxisAndAngle(QVector3D(0.2f * k, 1.0f, 0.5f), 15.0f + 7.0f * k);
        const QVector3D tracker_pos(0.3f + 0.01f * k, -0.1f, 0.4f - 0.02f * k);
        feed(coreg, 2, tracker_pos, tracker_ori);
        feed(coreg, 1, tracker_pos + tracker_ori.rotatedVector(center + (0.10f + 0.06f * k) * axis), QQuaternion());
        QVERIFY(coreg.capture_optical_calib_sample());
    }
    QCOMPARE(coreg.optical_calib_sample_count(), 5);
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
    Events events(coreg);

    const QVector3D center(0.05f, 0.18f, -0.03f);
    const QVector3D axis = QVector3D(0.1f, 1.0f, -0.3f).normalized();
    coreg.set_known_tracker_to_objective_distance(mode == 1 ? center.length() : 0.0f);

    calibrateOptics(coreg, center, axis);
    if (mode == 0) {
        const QQuaternion tracker_ori = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 40.0f);
        feed(coreg, 2, QVector3D(0.1f, 0.1f, 0.1f), tracker_ori);
        feed(coreg, 1, QVector3D(0.1f, 0.1f, 0.1f) + tracker_ori.rotatedVector(center), QQuaternion());
        QVERIFY(coreg.capture_objective_center());
        QVERIFY((toQt(coreg.objective_center_local()) - center).length() < 1e-5f);
    }
    QVERIFY(coreg.solve_optical_calibration());
    QVERIFY(coreg.optical_calibration_valid());
    QCOMPARE(events.optical_changes, 1);

    QVERIFY2(QVector3D::dotProduct(toQt(coreg.optical_axis_local()), axis) > 1.0f - 1e-5f,
             qPrintable(QString("axis %1 %2 %3").arg(toQt(coreg.optical_axis_local()).x()).arg(toQt(coreg.optical_axis_local()).y()).arg(toQt(coreg.optical_axis_local()).z())));
    QVERIFY(coreg.optical_calib_residual_mm() < 0.05f);
    QVERIFY(std::fabs(coreg.optical_calib_depth_spread_mm() - 240.0f) < 0.1f);
    // The centre lies on the axis; with a captured centre or a known distance it is the true one
    const QVector3D c = toQt(coreg.optical_center_local());
    const QVector3D rel = c - center;
    QVERIFY((rel - QVector3D::dotProduct(rel, axis) * axis).length() < 1e-5f);
    if (mode != 2) {
        QVERIFY2(rel.length() < 1e-4f, qPrintable(QString("centre off by %1 m").arg(rel.length())));
    }

    // World ray and up vector for the current tracker pose
    const QQuaternion tracker_ori = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 20.0f);
    const QVector3D tracker_pos(0.2f, 0.0f, 0.1f);
    feed(coreg, 2, tracker_pos, tracker_ori);
    QVector3D origin, direction, up;
    QVERIFY(optical_ray_in_world(coreg, origin, direction));
    QVERIFY((origin - (tracker_pos + tracker_ori.rotatedVector(c))).length() < 1e-5f);
    QVERIFY((direction - tracker_ori.rotatedVector(axis)).length() < 1e-4f);
    QVERIFY(optical_up_in_world(coreg, up));
    QVERIFY(std::fabs(QVector3D::dotProduct(up, direction)) < 1e-5f);
    QVERIFY(QVector3D::dotProduct(up, tracker_ori.rotatedVector(QVector3D(0, 0, 1))) > 0.9f);

    // Fine adjustment towards a target 2 degrees off the axis, then undo
    const QVector3D tilted = QQuaternion::fromAxisAndAngle(up, 2.0f).rotatedVector(direction);
    float correction = 0.0f;
    QVERIFY(coreg.apply_optical_axis_fine_adjust(toEigen(origin + 0.3f * tilted), correction));
    QVERIFY(std::fabs(correction - 2.0f) < 0.01f);
    QVERIFY(coreg.optical_fine_adjust_applied());
    QCOMPARE(coreg.optical_fine_adjust_deg(), correction);
    QVERIFY(optical_ray_in_world(coreg, origin, direction));
    QVERIFY((direction - tilted).length() < 1e-4f);
    QVERIFY(!coreg.apply_optical_axis_fine_adjust(toEigen(origin + 0.3f * QQuaternion::fromAxisAndAngle(up, 20.0f).rotatedVector(direction)), correction));
    QVERIFY(correction > 10.0f);
    QVERIFY(!coreg.apply_optical_axis_fine_adjust(toEigen(origin), correction));
    coreg.clear_optical_fine_adjust();
    QVERIFY(!coreg.optical_fine_adjust_applied());
    QVERIFY((toQt(coreg.optical_axis_local()) - axis).length() < 1e-4f);

    coreg.clear_optical_calib_samples();
    QVERIFY(!coreg.optical_calibration_valid());
    QVERIFY(!coreg.solve_optical_calibration());
}

//=============================================================================================================

void TestPolhemusCoregistration::opticalSession_roundTripAndRestoreReplacesFiducials()
{
    PolhemusCoregistration first;
    const QVector3D center(0.0f, 0.2f, 0.0f);
    const QVector3D axis = QVector3D(0.0f, 1.0f, 0.2f).normalized();
    calibrateOptics(first, center, axis);
    feed(first, 1, QVector3D(0.3f, 0.0f, 0.4f) + center, QQuaternion());
    feed(first, 2, QVector3D(0.3f, 0.0f, 0.4f), QQuaternion());
    QVERIFY(first.capture_objective_center());
    QVERIFY(first.solve_optical_calibration());
    QVector3D origin, direction;
    QVERIFY(optical_ray_in_world(first, origin, direction));
    float correction = 0.0f;
    QVERIFY(first.apply_optical_axis_fine_adjust(toEigen(origin + 0.3f * QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), 1.0f).rotatedVector(direction)), correction));
    first.set_model_vertex(toEigen(QVector3D(0.0f, 0.0f, 0.12f)));
    feed(first, 1, QVector3D(0.0f, 0.01f, 0.11f), QQuaternion());
    QVERIFY(first.capture_current_pen_position_as_vertex());
    const QVector<std::pair<FiducialId, QVector3D>> fids = {
        {FiducialId::LPA, QVector3D(-0.07f, 0, 0)}, {FiducialId::NAS, QVector3D(0, 0.1f, 0)}, {FiducialId::RPA, QVector3D(0.07f, 0, 0)}};
    for (const auto& [id, pos] : fids) {
        feed(first, 1, pos, QQuaternion());
        QVERIFY(first.capture_current_pen_position_as_fiducial(id));
    }
    QVERIFY(first.compute_registration());

    const JsonObject saved = first.session_state();

    // The restoring side already holds different live fiducials; the restored ones must replace them
    PolhemusCoregistration second;
    for (const auto& [id, pos] : fids) {
        feed(second, 1, pos + QVector3D(0.0f, 0.0f, 0.05f), QQuaternion());
        QVERIFY(second.capture_current_pen_position_as_fiducial(id));
    }
    QVERIFY(second.restore_session_state(saved));
    QVERIFY(second.restore_session_state(saved));
    QCOMPARE(second.acquired_points().count_of(PointKind::Fiducial), 3);
    for (const auto& [id, pos] : fids) {
        QCOMPARE(toQt(second.acquired_points().fiducial(id)), pos);
    }

    QCOMPARE(second.optical_calibration_valid(), true);
    QCOMPARE(second.optical_calib_sample_count(), 5);
    QCOMPARE(second.has_objective_center(), true);
    QVERIFY((toQt(second.objective_center_local()) - toQt(first.objective_center_local())).length() < 1e-6f);
    QVERIFY((toQt(second.optical_axis_local()) - toQt(first.optical_axis_local())).length() < 1e-6f);
    QVERIFY((toQt(second.optical_center_local()) - toQt(first.optical_center_local())).length() < 1e-6f);
    QCOMPARE(second.optical_fine_adjust_applied(), true);
    QVERIFY(std::fabs(second.optical_fine_adjust_deg() - first.optical_fine_adjust_deg()) < 1e-6f);
    QVERIFY(std::fabs(second.optical_calib_residual_mm() - first.optical_calib_residual_mm()) < 1e-4f);
    QVERIFY(std::fabs(second.optical_calib_depth_spread_mm() - first.optical_calib_depth_spread_mm()) < 1e-3f);
    QCOMPARE(second.has_pen_vertex(), true);
    QCOMPARE(second.has_model_vertex(), true);
    QCOMPARE(toQt(second.pen_vertex()), toQt(first.pen_vertex()));
    QCOMPARE(toQt(second.model_vertex()), toQt(first.model_vertex()));
    QCOMPARE(toQt(second.head_to_world()), toQt(first.head_to_world()));
    second.clear_optical_fine_adjust();
    QVERIFY((toQt(second.optical_axis_local()) - axis).length() < 1e-4f);
    QVERIFY(second.solve_optical_calibration());

    QVERIFY(!second.restore_session_state(JsonObject()));
}

//=============================================================================================================

void TestPolhemusCoregistration::digitizer_feedsTheSimulatedStylusAndTracker()
{
    MNEALIGN::Digitizer digitizer;
    PolhemusCoregistration coreg;
    connect(&digitizer, &MNEALIGN::Digitizer::pointReceived, &digitizer, [&coreg](int station, const Eigen::Vector3f& position, const Eigen::Quaternionf& orientation) {
        coreg.on_pose(station, position, orientation);
    });
    QSignalSpy connectedSpy(&digitizer, &MNEALIGN::Digitizer::connectedChanged);
    QSignalSpy pointSpy(&digitizer, &MNEALIGN::Digitizer::pointReceived);
    Events events(coreg);

    QVERIFY(digitizer.open(QString()));
    QVERIFY(digitizer.isConnected());
    QVERIFY(digitizer.description().contains("simulated"));
    QVERIFY(digitizer.open(QString())); // already open
    QCOMPARE(connectedSpy.count(), 1);

    QVERIFY(QTest::qWaitFor([&] { return coreg.have_live_pen_position() && events.device_poses > 0; }, 5000));
    // The simulated stylus sweeps a head of 10 cm radius
    QVERIFY(std::abs(coreg.pen_position().head<2>().norm() - 0.10f) < 1e-3f);
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
