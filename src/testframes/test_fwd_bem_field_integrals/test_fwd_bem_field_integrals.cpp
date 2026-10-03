//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fwd_bem_field_integrals.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Checks the analytic BEM magnetic-field triangle integrals against numerical quadrature.
 *
 * The BEM field coefficients integrate the Biot-Savart kernel
 *
 *     K(r') = ((dest - r') x n) . dir / |dest - r'|^3
 *
 * over a surface triangle, either alone (constant collocation, one_field_coeff)
 * or weighted with the linear basis function of each vertex (linear
 * collocation: Ferguson and Urankar closed forms, and the "simple" one-point
 * rule). The oracle is a 160 000-point centroid rule on a uniform
 * subdivision of each triangle, done in double precision.
 *
 * The Ferguson and Urankar forms integrate by parts: per triangle they differ
 * from the direct integral by edge terms that only cancel on a closed surface.
 * The linear coefficients are therefore compared per vertex, summed over a
 * closed irregular octahedron.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fwd/fwd_bem_model.h>
#include <mne/mne_triangle.h>

#include <cmath>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FWDLIB;
using namespace MNELIB;
using namespace Eigen;

namespace
{

//=============================================================================================================
/**
 * Integrals of K over the triangle: (constant, vertex 1, vertex 2, vertex 3).
 */
Vector4d quadrature(const MNETriangle& tri, const Vector3d& dest, const Vector3d& dir)
{
    constexpr int kN = 400;
    const Vector3d r1 = tri.r1.cast<double>();
    const Vector3d e1 = (tri.r2 - tri.r1).cast<double>() / kN;
    const Vector3d e2 = (tri.r3 - tri.r1).cast<double>() / kN;
    const Vector3d nn = tri.nn.cast<double>();
    const double dA = static_cast<double>(tri.area) / (kN * kN);

    Vector4d sum = Vector4d::Zero();
    auto add = [&](double u, double v) {
        const Vector3d r = r1 + u * e1 + v * e2;
        const Vector3d d = dest - r;
        const double k = d.cross(nn).dot(dir) / std::pow(d.norm(), 3) * dA;
        const double b = u / kN, c = v / kN;
        sum += k * Vector4d(1.0, 1.0 - b - c, b, c);
    };
    for (int i = 0; i < kN; ++i) {
        for (int j = 0; i + j < kN; ++j) {
            add(i + 1.0 / 3.0, j + 1.0 / 3.0);
            if (i + j + 1 < kN)
                add(i + 2.0 / 3.0, j + 2.0 / 3.0);
        }
    }
    return sum;
}

//=============================================================================================================
/**
 * Closed octahedron with uneven vertex radii and outward normals, centred at the origin.
 */
struct Mesh
{
    Mesh()
    {
        const Vector3f dirs[6] = {Vector3f::UnitX(), -Vector3f::UnitX(), Vector3f::UnitY(), -Vector3f::UnitY(), Vector3f::UnitZ(), -Vector3f::UnitZ()};
        const float radius[6] = {0.010f, 0.012f, 0.009f, 0.011f, 0.013f, 0.008f};
        for (int v = 0; v < 6; ++v)
            rr[v] = radius[v] * dirs[v];
        const int faces[8][3] = {{0, 2, 4}, {2, 1, 4}, {1, 3, 4}, {3, 0, 4}, {2, 0, 5}, {1, 2, 5}, {3, 1, 5}, {0, 3, 5}};
        for (int t = 0; t < 8; ++t) {
            for (int k = 0; k < 3; ++k)
                vert[t][k] = faces[t][k];
            tris[t].vert = vert[t];
            tris[t].r1 = rr[faces[t][0]];
            tris[t].r2 = rr[faces[t][1]];
            tris[t].r3 = rr[faces[t][2]];
            tris[t].compute_data();
        }
    }

    Vector3f rr[6];
    int vert[8][3];
    MNETriangle tris[8];
};

} // namespace

//=============================================================================================================
/**
 * Checks the BEM field integrals against quadrature.
 */
class TestFwdBemFieldIntegrals : public QObject
{
    Q_OBJECT

private slots:
    void integrals_data();
    void integrals();
};

//=============================================================================================================

void TestFwdBemFieldIntegrals::integrals_data()
{
    QTest::addColumn<Vector3f>("dest");
    QTest::addColumn<Vector3f>("dir");
    QTest::addColumn<double>("simpleTol");
    QTest::addColumn<double>("closedTol");

    const Vector3f oblique = Vector3f(0.3f, -0.5f, 0.8f).normalized();
    // The one-point rule's error falls with distance / triangle size; numpy gives 0.30 at 7 cm, 0.025 at 63 cm.
    QTest::newRow("close outside, oblique") << Vector3f(0.008f, 0.007f, 0.009f) << oblique << -1.0 << 1e-4;
    QTest::newRow("close outside, tangential") << Vector3f(0.009f, -0.004f, -0.006f) << Vector3f(1.0f, 0.0f, 0.0f) << -1.0 << 1e-4;
    QTest::newRow("inside") << Vector3f(0.001f, -0.002f, 0.003f) << Vector3f(0.0f, 0.6f, 0.8f) << -1.0 << 1e-4;
    QTest::newRow("sensor distance") << Vector3f(0.03f, -0.02f, 0.06f) << oblique << 0.35 << 1e-4;
    // At 63 cm float32 cancellation costs Urankar 5 % (a float64 numpy port is exact), so only the simple rule is checked.
    QTest::newRow("far") << Vector3f(0.27f, -0.18f, 0.54f) << oblique << 0.03 << -1.0;
}

void TestFwdBemFieldIntegrals::integrals()
{
    QFETCH(Vector3f, dest);
    QFETCH(Vector3f, dir);
    QFETCH(double, simpleTol);
    QFETCH(double, closedTol);

    Mesh mesh;
    VectorXd lin = VectorXd::Zero(6), ferg = VectorXd::Zero(6), uran = VectorXd::Zero(6), simple = VectorXd::Zero(6);
    for (MNETriangle& tri : mesh.tris) {
        const Vector4d ref = quadrature(tri, dest.cast<double>(), dir.cast<double>());

        const double constant = FwdBemModel::one_field_coeff(dest, dir, tri);
        QVERIFY2(std::abs(constant - ref[0]) < 1e-4 * ref.head<1>().cwiseAbs().maxCoeff() + 1e-6 * ref.cwiseAbs().maxCoeff(),
                 qPrintable(QStringLiteral("constant %1 vs %2").arg(constant, 0, 'g', 10).arg(ref[0], 0, 'g', 10)));

        Vector3d one;
        for (int k = 0; k < 3; ++k)
            lin[tri.vert[k]] += ref[k + 1];
        FwdBemModel::fwd_bem_one_lin_field_coeff_ferg(dest, dir, tri, one);
        for (int k = 0; k < 3; ++k)
            ferg[tri.vert[k]] += one[k];
        FwdBemModel::fwd_bem_one_lin_field_coeff_uran(dest, dir, tri, one);
        for (int k = 0; k < 3; ++k)
            uran[tri.vert[k]] += one[k];
        FwdBemModel::fwd_bem_one_lin_field_coeff_simple(dest, dir, tri, one);
        for (int k = 0; k < 3; ++k)
            simple[tri.vert[k]] += one[k];
    }

    const double scale = lin.cwiseAbs().maxCoeff();
    QVERIFY(scale > 0.0);
    auto show = [](const VectorXd& v) {
        QString s;
        for (int i = 0; i < v.size(); ++i)
            s += QString::number(v[i], 'g', 6) + " ";
        return s;
    };
    if (closedTol > 0.0)
        QVERIFY2((ferg - lin).cwiseAbs().maxCoeff() < closedTol * scale, qPrintable("Ferguson " + show(ferg) + "vs " + show(lin)));
    if (closedTol > 0.0)
        QVERIFY2((uran - lin).cwiseAbs().maxCoeff() < closedTol * scale, qPrintable("Urankar " + show(uran) + "vs " + show(lin)));
    if (simpleTol > 0.0)
        QVERIFY2((simple - lin).cwiseAbs().maxCoeff() < simpleTol * scale, qPrintable("simple " + show(simple) + "vs " + show(lin)));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFwdBemFieldIntegrals)
#include "test_fwd_bem_field_integrals.moc"
