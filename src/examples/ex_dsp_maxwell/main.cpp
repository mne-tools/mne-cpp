//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    DSP Maxwell filtering: SSS, bad-channel detection, movement compensation, fine calibration and cHPI removal.
 *
 * Uses the 306-channel sensor geometry of the MNE-CPP test data. The field of
 * a dipole inside the head and a uniform field from outside are computed in
 * closed form, so SSS must keep the first and remove the second. Exits
 * non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/bad_channels_maxwell.h>
#include <dsp/filter_chpi.h>
#include <dsp/fine_calibration.h>
#include <dsp/maxwell_movement_comp.h>
#include <dsp/sss.h>

#include <fiff/fiff_constants.h>
#include <fiff/fiff_raw_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

constexpr double kPi = 3.14159265358979323846;

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

/** Field component along each MEG coil normal: a magnetic dipole @p moment at @p pos plus a uniform field @p uniform. */
VectorXd coilField(const FiffInfo& info, const QVector<int>& meg, const Vector3d& pos, const Vector3d& moment, const Vector3d& uniform)
{
    VectorXd b(meg.size());
    for (int k = 0; k < meg.size(); ++k) {
        const FiffChInfo& ch = info.chs[meg[k]];
        const Vector3d r = ch.chpos.r0.cast<double>() - pos;
        const double d = r.norm();
        const Vector3d dipole = (3.0 * r * r.dot(moment) / (d * d) - moment) / (d * d * d);
        b(k) = ch.chpos.ez.cast<double>().normalized().dot(dipole + uniform);
    }
    return b;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    QCommandLineOption rawOption("raw", "Raw <file>.", "file",
                                 QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
    parser.addOption(rawOption);
    parser.process(app);
    QFile rawFile(parser.value(rawOption));
    const FiffRawData raw(rawFile);
    FiffInfo info = raw.info;
    bool ok = info.nchan > 0;

    // Gradiometers measure field differences in T/m; this example models every MEG coil as a point magnetometer.
    for (FiffChInfo& ch : info.chs) {
        if (ch.kind == FIFFV_MEG_CH) {
            ch.unit = FIFF_UNIT_T;
        }
    }

    //! [sss_usage]
    SSS::Params params; // internal order 8, external order 3
    params.origin = Vector3d(0.0, 0.0, 0.04);
    const SSS::Basis basis = SSS::computeBasis(info, params); // geometry only, reusable for every buffer
    //! [sss_usage]
    const QVector<int>& meg = basis.megChannelIdx;

    // Device-frame fields: a dipole 3 cm above the expansion origin, and a uniform field from far away
    const Vector3d inside = params.origin + Vector3d(0.01, -0.01, 0.03);
    const VectorXd brain = coilField(info, meg, inside, Vector3d(1e-9, 2e-9, 0.0), Vector3d::Zero());
    const VectorXd noise = coilField(info, meg, inside, Vector3d::Zero(), Vector3d(3e-9, -1e-9, 2e-9));
    MatrixXd data = MatrixXd::Zero(info.nchan, 2);
    for (int k = 0; k < meg.size(); ++k) {
        data(meg[k], 0) = brain(k);
        data(meg[k], 1) = noise(k);
    }

    //! [sss_apply]
    const MatrixXd cleaned = SSS::apply(data, basis); // MEG rows replaced by their internal-space reconstruction
    //! [sss_apply]
    MatrixXd megIn(meg.size(), 2);
    MatrixXd megOut(meg.size(), 2);
    for (int k = 0; k < meg.size(); ++k) {
        megIn.row(k) = data.row(meg[k]);
        megOut.row(k) = cleaned.row(meg[k]);
    }
    const double keptBrain = (megOut.col(0) - megIn.col(0)).norm() / megIn.col(0).norm();
    const double keptNoise = megOut.col(1).norm() / megIn.col(1).norm();
    ok &= expect(meg.size() == 306 && keptBrain < 0.05 && keptNoise < 1e-3,
                 QString("SSS keeps the inside dipole (error %1) and removes the uniform field (residual %2)").arg(keptBrain, 0, 'g', 3).arg(keptNoise, 0, 'g', 3));

    //! [bad_channels_maxwell_usage]
    MatrixXd broken = data;
    broken.row(meg[40]).array() += 2e-7; // a sensor with a constant offset the multipole model cannot explain
    BadChannelsMaxwellParams badParams;
    badParams.origin = params.origin;
    badParams.dZThreshold = 50.0; // robust z-score of the reconstruction residual
    const BadChannelsMaxwellResult bads = findBadChannelsMaxwell(broken, info, badParams);
    //! [bad_channels_maxwell_usage]
    int worst = 0;
    bads.zScores.maxCoeff(&worst);
    ok &= expect(worst == 40 && bads.badIndices.contains(meg[40]) && bads.badChannels.contains(info.ch_names[meg[40]]),
                 QString("findBadChannelsMaxwell ranks %1 worst (z = %2), flagged: %3").arg(info.ch_names[meg[worst]]).arg(bads.zScores(worst), 0, 'f', 0).arg(bads.badChannels.join(", ")));

    //! [maxwell_movement_comp_usage]
    QList<HeadPosEntry> headPos(1); // one position from t = 0: identity rotation, no translation
    MaxwellMoveCompParams moveParams;
    moveParams.origin = params.origin;
    const MatrixXd compensated = MaxwellMovementComp::apply(data, info, headPos, 1.0, moveParams);

    QTemporaryDir dir;
    const QString posPath = dir.filePath("head.pos"); // same columns as mne.chpi.write_head_pos
    const bool posWritten = MaxwellMovementComp::writeHeadPos(posPath, headPos);
    const QList<HeadPosEntry> posBack = MaxwellMovementComp::readHeadPos(posPath);
    //! [maxwell_movement_comp_usage]
    double moveError = 0.0;
    for (int k = 0; k < meg.size(); ++k) {
        moveError = std::max(moveError, (compensated.row(meg[k]) - cleaned.row(meg[k])).cwiseAbs().maxCoeff());
    }
    ok &= expect(posWritten && posBack.size() == 1 && moveError < 1e-3 * megIn.cwiseAbs().maxCoeff(),
                 "MaxwellMovementComp without movement equals SSS; head positions round-trip");

    //! [fine_calibration_usage]
    FineCalibration calibration;
    FineCalEntry magnetometer;
    magnetometer.chNumber = 111; // numbers ending in 1 are magnetometers
    magnetometer.position = Vector3d(-0.1066, 0.0464, -0.0604);
    magnetometer.imbalance = VectorXd::Constant(1, 0.996645); // calibration factor
    calibration.addEntry(magnetometer);
    const QString calPath = dir.filePath("sss_cal.dat"); // the format of mne.preprocessing.write_fine_calibration
    const bool calWritten = calibration.write(calPath);
    const FineCalibration calBack = FineCalibration::read(calPath);
    //! [fine_calibration_usage]
    ok &= expect(calWritten && calBack.size() == 1 && calBack.gainVector()(0) == 0.996645 && calBack.entries().first().position.isApprox(magnetometer.position),
                 "FineCalibration round-trips a magnetometer line");

    //! [filter_chpi_usage]
    const double sFreq = 1000.0;
    MatrixXd chpi = MatrixXd::Zero(info.nchan, 4000);
    for (int t = 0; t < chpi.cols(); ++t) {
        chpi(meg[0], t) = std::sin(2.0 * kPi * 10.0 * t / sFreq) + std::sin(2.0 * kPi * 83.0 * t / sFreq);
    }
    const QVector<double> coilFreqs{83.0};
    FilterChpiParams chpiParams; // +-2 Hz order-4 notches on the MEG channels only
    filterChpi(chpi, info, sFreq, coilFreqs, chpiParams);
    //! [filter_chpi_usage]
    double at10 = 0.0;
    double at83 = 0.0;
    for (int t = 1000; t < 3000; ++t) {
        at10 += chpi(meg[0], t) * std::sin(2.0 * kPi * 10.0 * t / sFreq);
        at83 += chpi(meg[0], t) * std::sin(2.0 * kPi * 83.0 * t / sFreq);
    }
    ok &= expect(std::fabs(at10 / 1000.0 - 1.0) < 0.05 && std::fabs(at83 / 1000.0) < 0.05,
                 QString("filterChpi keeps 10 Hz (%1) and removes the 83 Hz coil (%2)").arg(at10 / 1000.0, 0, 'f', 3).arg(at83 / 1000.0, 0, 'f', 3));

    qInfo().noquote() << (ok ? "All dsp Maxwell checks passed." : "dsp Maxwell checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
