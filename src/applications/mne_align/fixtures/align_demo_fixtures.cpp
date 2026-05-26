//=============================================================================================================
/**
 * @file     align_demo_fixtures.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Implementation of the mne_align synthetic demo fixtures.
 */

#include "align_demo_fixtures.h"

#include <QtMath>

namespace MNEALIGN
{

using UTILSLIB::AcquiredPoints;
using UTILSLIB::DigitizedPoint;
using UTILSLIB::FiducialId;
using UTILSLIB::PointKind;

QVector<DigitizedPoint> demoFiducials()
{
    QVector<DigitizedPoint> out;
    out.reserve(3);

    DigitizedPoint nas;
    nas.kind        = PointKind::Fiducial;
    nas.label       = QStringLiteral("NAS");
    nas.identNumber = static_cast<int>(FiducialId::NAS);
    nas.position    = QVector3D(0.000f, 0.095f, 0.005f);
    out.append(nas);

    DigitizedPoint lpa;
    lpa.kind        = PointKind::Fiducial;
    lpa.label       = QStringLiteral("LPA");
    lpa.identNumber = static_cast<int>(FiducialId::LPA);
    lpa.position    = QVector3D(-0.080f, 0.000f, 0.000f);
    out.append(lpa);

    DigitizedPoint rpa;
    rpa.kind        = PointKind::Fiducial;
    rpa.label       = QStringLiteral("RPA");
    rpa.identNumber = static_cast<int>(FiducialId::RPA);
    rpa.position    = QVector3D(0.080f, 0.000f, 0.000f);
    out.append(rpa);

    return out;
}

QVector<DigitizedPoint> demoEegCap(int count)
{
    // Eight cardinal 10-20 positions on a 95 mm sphere centred at the
    // head origin. Values are loosely realistic; exact geometry is not
    // important for documentation screenshots.
    struct Entry { const char* label; QVector3D pos; };
    static const Entry kEntries[] = {
        {"Fz", QVector3D( 0.000f,  0.070f,  0.060f)},
        {"Cz", QVector3D( 0.000f,  0.000f,  0.095f)},
        {"Pz", QVector3D( 0.000f, -0.070f,  0.060f)},
        {"Oz", QVector3D( 0.000f, -0.095f,  0.005f)},
        {"T7", QVector3D(-0.075f,  0.000f,  0.040f)},
        {"T8", QVector3D( 0.075f,  0.000f,  0.040f)},
        {"O1", QVector3D(-0.035f, -0.085f,  0.025f)},
        {"O2", QVector3D( 0.035f, -0.085f,  0.025f)},
    };

    const int n = qBound(0, count, static_cast<int>(sizeof(kEntries) / sizeof(kEntries[0])));
    QVector<DigitizedPoint> out;
    out.reserve(n);
    for (int i = 0; i < n; ++i) {
        DigitizedPoint p;
        p.kind        = PointKind::Eeg;
        p.label       = QString::fromLatin1(kEntries[i].label);
        p.identNumber = i + 1;
        p.position    = kEntries[i].pos;
        out.append(p);
    }
    return out;
}

QVector<DigitizedPoint> demoHeadShape(int count)
{
    QVector<DigitizedPoint> out;
    if (count <= 0) return out;
    out.reserve(count);

    constexpr float kRadius = 0.095f;  // 95 mm head sphere
    for (int i = 0; i < count; ++i) {
        // Spiral over the upper hemisphere using a Fibonacci-like layout.
        const float t   = (i + 0.5f) / static_cast<float>(count);
        const float phi = std::acos(1.0f - t);             // 0 .. π/2 (upper hemisphere)
        const float th  = static_cast<float>(i) * 2.39996f; // golden angle

        DigitizedPoint p;
        p.kind        = PointKind::HeadShape;
        p.label       = QStringLiteral("HSP-%1").arg(i + 1);
        p.identNumber = i + 1;
        p.position    = QVector3D(
            kRadius * std::sin(phi) * std::cos(th),
            kRadius * std::sin(phi) * std::sin(th),
            kRadius * std::cos(phi));
        out.append(p);
    }
    return out;
}

QVector<DigitizedPoint> demoFullDigitisation()
{
    QVector<DigitizedPoint> out;
    out += demoFiducials();
    out += demoEegCap(8);
    out += demoHeadShape(40);
    return out;
}

void applyTo(AcquiredPoints* store, const QVector<DigitizedPoint>& points)
{
    if (!store) return;
    store->clear();
    for (const DigitizedPoint& p : points) {
        store->append(p);
    }
}

} // namespace MNEALIGN
