//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_sensor_field_mapper.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     August, 2026
 * @brief    Data-free tests for SensorFieldMapper utilities and state.
 */

#include <disp3D/scene/sensorfieldmapper.h>

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_info.h>

#include <Eigen/Core>

#include <QtTest>

#include <cmath>

using namespace Eigen;
using namespace FIFFLIB;

namespace {

FiffDigPoint makeDigPoint(int kind, const Vector3f& position, int coordFrame = FIFFV_COORD_HEAD) {
    FiffDigPoint point;
    point.kind = kind;
    point.coord_frame = coordFrame;
    point.r[0] = position.x();
    point.r[1] = position.y();
    point.r[2] = position.z();
    return point;
}

} // namespace

class TestSensorFieldMapper : public QObject {
    Q_OBJECT

    private slots:
    void defaultsAndBaseline();
    void unloadedAndSurfaceLookup();
    void contourStep_data();
    void contourStep();
    void sphereFit();
};

void TestSensorFieldMapper::defaultsAndBaseline() {
    SensorFieldMapper mapper;
    QVERIFY(!mapper.isLoaded());
    QCOMPARE(mapper.timePoint(), 0);
    QVERIFY(!mapper.megFieldMapOnHead());
    QCOMPARE(mapper.colormap(), QStringLiteral("MNE"));
    QVERIFY(!mapper.hasMappingFor(FiffEvoked()));

    mapper.setTimePoint(2);
    mapper.setMegFieldMapOnHead(true);
    mapper.setColormap(QStringLiteral("RdBu_r"));
    QCOMPARE(mapper.timePoint(), 2);
    QVERIFY(mapper.megFieldMapOnHead());
    QCOMPARE(mapper.colormap(), QStringLiteral("RdBu_r"));

    FiffEvoked evoked;
    evoked.nave = 1;
    evoked.data.resize(1, 3);
    evoked.data << 1.0, 3.0, 8.0;
    evoked.times.resize(3);
    evoked.times << -0.1f, 0.0f, 0.1f;
    evoked.baseline = qMakePair(0.0f, 0.0f);

    mapper.setEvoked(evoked);
    QVERIFY(mapper.isLoaded());
    QVERIFY(std::abs(mapper.evoked().data(0, 0) + 1.0) < 1.0e-12);
    QVERIFY(std::abs(mapper.evoked().data(0, 1) - 1.0) < 1.0e-12);
    QVERIFY(std::abs(mapper.evoked().data(0, 2) - 6.0) < 1.0e-12);

    QMap<QString, std::shared_ptr<BrainSurface>> surfaces;
    QVERIFY(!mapper.buildMapping(surfaces, FiffCoordTrans(), false));
}

void TestSensorFieldMapper::unloadedAndSurfaceLookup() {
    SensorFieldMapper mapper;
    FiffEvoked emptyEvoked;
    mapper.setEvoked(emptyEvoked);
    QVERIFY(!mapper.isLoaded());
    QVERIFY(!mapper.buildMapping({}, FiffCoordTrans(), false));
    mapper.computeNormRange();

    QMap<QString, std::shared_ptr<BrainSurface>> surfaces;
    QCOMPARE(SensorFieldMapper::findHeadSurfaceKey(surfaces), QString());
    QCOMPARE(SensorFieldMapper::findHelmetSurfaceKey(surfaces), QString());

    surfaces.insert(QStringLiteral("bem_outer_skin"), nullptr);
    surfaces.insert(QStringLiteral("bem_head"), nullptr);
    surfaces.insert(QStringLiteral("sens_surface_meg"), nullptr);
    QCOMPARE(SensorFieldMapper::findHeadSurfaceKey(surfaces), QStringLiteral("bem_head"));
    QCOMPARE(SensorFieldMapper::findHelmetSurfaceKey(surfaces),
             QStringLiteral("sens_surface_meg"));

    surfaces.remove(QStringLiteral("bem_head"));
    QCOMPARE(SensorFieldMapper::findHeadSurfaceKey(surfaces),
             QStringLiteral("bem_outer_skin"));

    FiffEvoked loadedEvoked;
    loadedEvoked.nave = 1;
    loadedEvoked.data = MatrixXd::Ones(1, 1);
    loadedEvoked.times = RowVectorXf::Zero(1);
    mapper.setEvoked(loadedEvoked);
    QVERIFY(mapper.isLoaded());
    mapper.apply(surfaces, SubView(), {});
}

void TestSensorFieldMapper::contourStep_data() {
    QTest::addColumn<float>("minimum");
    QTest::addColumn<float>("maximum");
    QTest::addColumn<int>("ticks");
    QTest::addColumn<float>("expected");

    QTest::newRow("no ticks") << 0.0f << 1.0f << 0 << 0.0f;
    QTest::newRow("reversed") << 2.0f << 1.0f << 5 << 0.0f;
    QTest::newRow("unit") << 0.0f << 10.0f << 10 << 1.0f;
    QTest::newRow("two") << 0.0f << 20.0f << 10 << 2.0f;
    QTest::newRow("five") << 0.0f << 50.0f << 10 << 5.0f;
    QTest::newRow("ten") << 0.0f << 100.0f << 10 << 10.0f;
    QTest::newRow("milliscale") << 0.0f << 0.05f << 10 << 0.01f;
}

void TestSensorFieldMapper::contourStep() {
    QFETCH(float, minimum);
    QFETCH(float, maximum);
    QFETCH(int, ticks);
    QFETCH(float, expected);

    QCOMPARE(SensorFieldMapper::contourStep(minimum, maximum, ticks), expected);
}

void TestSensorFieldMapper::sphereFit() {
    FiffInfo emptyInfo;
    float radius = -1.0f;
    const Vector3f fallback = SensorFieldMapper::fitSphereOrigin(emptyInfo, &radius);
    QVERIFY((fallback - Vector3f(0.0f, 0.0f, 0.04f)).norm() < 1.0e-7f);
    QCOMPARE(radius, 0.0f);

    const Vector3f center(0.01f, -0.02f, 0.03f);
    constexpr float expectedRadius = 0.09f;
    const Vector3f offsets[] = {
    Vector3f(expectedRadius, 0.0f, 0.0f),
    Vector3f(-expectedRadius, 0.0f, 0.0f),
    Vector3f(0.0f, expectedRadius, 0.0f),
    Vector3f(0.0f, -expectedRadius, 0.0f),
    Vector3f(0.0f, 0.0f, expectedRadius),
    Vector3f(0.0f, 0.0f, -expectedRadius)};

    FiffInfo info;
    for (int i = 0; i < 6; ++i) {
        const int kind = i < 3 ? FIFFV_POINT_EXTRA : FIFFV_POINT_EEG;
        info.dig.append(makeDigPoint(kind, center + offsets[i]));
    }
    info.dig.append(makeDigPoint(FIFFV_POINT_EXTRA, Vector3f(1.0f, 1.0f, -1.0f)));
    info.dig.append(makeDigPoint(FIFFV_POINT_EXTRA, Vector3f(2.0f, 2.0f, 2.0f), FIFFV_COORD_MRI));

    const Vector3f fittedCenter = SensorFieldMapper::fitSphereOrigin(info, &radius);
    QVERIFY((fittedCenter - center).norm() < 1.0e-5f);
    QVERIFY(std::abs(radius - expectedRadius) < 1.0e-5f);
}

QTEST_GUILESS_MAIN(TestSensorFieldMapper)
#include "test_sensor_field_mapper.moc"