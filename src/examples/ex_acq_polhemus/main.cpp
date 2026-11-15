//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Polhemus digitiser: stream stylus poses, register fiducials captured with the button, collect points.
 *
 * Streams a simulated Fastrak through polhemus:// (no hardware needed), captures three fiducials at stylus
 * button presses, registers them onto known model positions, and keeps a list of digitised points. Every step
 * checks its result and the example exits non-zero on a mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <acq/acq_convert.h>
#include <acq/acq_device.h>
#include <acq/devices/acq_devices.h>
#include <acq/devices/polhemus/acquired_points.h>
#include <acq/devices/polhemus/polhemus_coregistration.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include <Eigen/Geometry>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std::chrono_literals;

namespace
{

bool expect(bool condition, const char* what)
{
    std::printf("%s %s\n", condition ? "  ok  " : "  FAIL", what);
    return condition;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main()
{
    bool ok = true;

    //! [polhemus_stream_usage]
    register_polhemus_scheme();
    // Stylus on station 1, head tracker on station 2; the simulated stylus button is pressed every 0.4 s.
    // A Fastrak on a serial port: "polhemus:///dev/cu.usbserial-1?stations=1,2"
    auto device = AcqDevice::open("polhemus://?simulate=1&stations=1,2&button=0.4");
    if (!device) {
        std::fprintf(stderr, "Cannot open the digitizer: %s\n", device.error().text().c_str());
        return EXIT_FAILURE;
    }
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless); // 14 channels: per station X Y Z (m), QW QX QY QZ
    if (!reader || !device->start().ok()) {
        return EXIT_FAILURE;
    }
    //! [polhemus_stream_usage]

    //! [polhemus_coregistration_usage]
    PolhemusCoregistration coreg; // station 1 = pen, station 2 = head tracker
    // Model fiducials are the pen fiducials turned 90 deg about z and moved 5 cm in x; the registration must find that transform
    const Eigen::Affine3f truth = Eigen::Translation3f(0.05f, 0.0f, 0.0f) * Eigen::AngleAxisf(0.5f * static_cast<float>(EIGEN_PI), Eigen::Vector3f::UnitZ());
    const FiducialId fiducials[] = {FiducialId::NAS, FiducialId::LPA, FiducialId::RPA};
    std::vector<Eigen::Vector3f> penAt;
    constexpr std::size_t kWidth = 14;
    while (penAt.size() < 3) {
        auto view = reader->lease(1, 2s);
        if (!view) {
            break;
        }
        const AcqBlockView& block = **view;
        std::vector<float> frames(kWidth * block.sampleCount());
        if (!decode(block, AcqMatrixRef<float>::columnMajor(frames.data(), kWidth, block.sampleCount())).ok()) {
            break;
        }
        for (std::size_t i = 0; i < block.sampleCount(); ++i) {
            const float* f = &frames[i * kWidth];
            for (int station : {1, 2}) {
                const float* p = f + (station - 1) * 7;
                coreg.on_pose(station, Eigen::Vector3f(p[0], p[1], p[2]), Eigen::Quaternionf(p[3], p[4], p[5], p[6]));
            }
            for (std::size_t e = 0; e < block.events().size(); ++e) {
                if (block.events()[e].sample == block.firstSample() + i && penAt.size() < 3) { // a stylus button press
                    const FiducialId id = fiducials[penAt.size()];
                    coreg.captureCurrentPenPositionAsFiducial(id);
                    penAt.push_back(coreg.penPosition());
                    coreg.setModelFiducial(id, truth * coreg.penPosition());
                }
            }
        }
    }
    device->close();
    const bool registered = penAt.size() == 3 && coreg.computeRegistration();
    //! [polhemus_coregistration_usage]
    const Eigen::Vector3f mapped = registered ? Eigen::Vector3f((coreg.worldToModel() * penAt[1].homogeneous()).head<3>()) : Eigen::Vector3f::Zero();
    ok &= expect(registered && coreg.acquiredPoints()->hasAllFiducials() && coreg.acquiredPoints()->countOf(PointKind::Fiducial) == 3 &&
                     (mapped - truth * penAt[1]).norm() < 1e-5f,
                 "PolhemusCoregistration maps the LPA captured at a stylus button press onto its model position");

    //! [acquired_points_usage]
    AcquiredPoints points;
    points.append({PointKind::HeadShape, "HSP-1", 1, Eigen::Vector3f(0.0f, 0.08f, 0.05f)});
    points.append({PointKind::Eeg, "Cz", 1, Eigen::Vector3f(0.0f, 0.0f, 0.09f)});
    points.undoLast(PointKind::HeadShape);
    const DigitizedPoint& kept = points.points().first(); // kind, label, FIFF ident, position (m)
    //! [acquired_points_usage]
    ok &= expect(points.points().size() == 1 && kept.label == "Cz" && kept.position.z() == 0.09f && points.countOf(PointKind::Eeg) == 1 && !points.hasAllFiducials(),
                 "AcquiredPoints keeps Cz after undoing the head-shape point");

    std::printf("%s\n", ok ? "All Polhemus checks passed." : "Polhemus checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
