//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Inverse library: continuous head position (HPI) fitting.
 *
 * Fits the four HPI coils in the first 200 ms of the test recording and
 * compares the device-to-head transform with the head position MaxFilter
 * computed for the same recording (Result/ref_hpiFit_pos.txt, quaternion
 * format). Exits non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/hpi/inv_hpi_data_updater.h>
#include <inv/hpi/inv_hpi_fit.h>
#include <inv/hpi/inv_hpi_fit_data.h>
#include <inv/hpi/inv_hpi_model_parameters.h>
#include <inv/hpi/inv_sensor_set.h>
#include <inv/hpi/inv_signal_model.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_raw_data.h>
#include <utils/ioutils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    QCommandLineOption dataOption("data", "MNE-CPP test data <dir>.", "dir",
                                  QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data");
    parser.addOption(dataOption);
    parser.process(app);
    const QString data = parser.value(dataOption);
    bool ok = true;

    QFile rawFile(data + "/MEG/sample/test_hpiFit_raw.fif");
    FiffRawData raw(rawFile);
    const FiffInfo::SPtr info = FiffInfo::SPtr::create(raw.info);
    MatrixXd segment;
    MatrixXd times;
    ok &= raw.read_raw_segment(segment, times, raw.first_samp, raw.first_samp + static_cast<int>(std::ceil(0.2 * info->sfreq)));

    //! [inv_sensor_set_usage]
    InvSensorSetCreator creator; // reads resources/general/coilDefinitions/coil_def.dat
    QList<FiffChInfo> megChannels;
    for (const FiffChInfo& ch : info->chs) {
        if (ch.kind == FIFFV_MEG_CH)
            megChannels << ch;
    }
    const InvSensorSet sensors = creator.updateSensorSet(megChannels, Accuracy::high); // integration points per coil
    //! [inv_sensor_set_usage]
    ok &= expect(sensors.ncoils() == 306 && sensors.np() == 8 && sensors.rmag().rows() == 306 * 8, "InvSensorSetCreator/InvSensorSet: 306 MEG sensors with 8 integration points each");

    //! [inv_hpi_usage]
    InvHpiDataUpdater updater(info); // good MEG channels, their sensors and the digitised coil positions
    updater.prepareDataAndProjectors(segment, MatrixXd::Identity(info->nchan, info->nchan));
    // The coil frequencies are known but not their order; the first fit with bOrderFrequencies finds it.
    const InvHpiModelParameters model(QVector<int>({154, 158, 161, 166}), info->sfreq, info->linefreq, true);
    InvHpiFit hpi(updater.getSensors());
    HpiFitResult fit;
    hpi.fit(updater.getProjectedData(), updater.getProjectors(), model, updater.getHpiDigitizer(), true, fit);
    //! [inv_hpi_usage]
    // MaxFilter head position at t = 0: translation (-0.814, -11.046, 53.572) mm. One 200 ms window, so allow 1 mm
    // (test_hpiFit_integration holds the mean over the whole recording to 0.3 mm).
    MatrixXd maxfilter;
    IOUtils::read_eigen_matrix(maxfilter, data + "/Result/ref_hpiFit_pos.txt");
    const Vector3d translation = fit.devHeadTrans.trans.block<3, 1>(0, 3).cast<double>();
    const Vector3d reference = maxfilter.block<1, 3>(0, 4).transpose();
    ok &= expect(fit.hpiFreqs == QVector<int>({166, 154, 161, 158}) && fit.GoF.minCoeff() > 0.98 && (translation - reference).norm() < 0.001,
                 QString("InvHpiDataUpdater/InvHpiModelParameters/InvHpiFit: coils ordered 166/154/161/158 Hz, head position within %1 mm of MaxFilter")
                     .arg(1e3 * (translation - reference).norm(), 0, 'f', 3));

    //! [inv_signal_model_usage]
    // Sine and cosine amplitudes of every coil frequency in every channel, least-squares fitted.
    InvSignalModel signalModel;
    const InvHpiModelParameters ordered(fit.hpiFreqs, info->sfreq, info->linefreq, false);
    const MatrixXd amplitudes = signalModel.fitData(ordered, updater.getProjectedData()); // (sin, cos) x 4 coils, then channels
    //! [inv_signal_model_usage]
    ok &= expect(amplitudes.rows() == 8 && amplitudes.cols() == updater.getProjectedData().rows() && amplitudes.allFinite(),
                 "InvSignalModel: sine and cosine amplitudes of the 4 coils for every channel");

    //! [inv_hpi_fit_data_usage]
    // One coil's magnetic-dipole fit, as InvHpiFit runs it per coil: seeded 5 mm off the fitted position.
    const MatrixXd coilsDevice = fit.devHeadTrans.apply_inverse_trans(updater.getHpiDigitizer().cast<float>()).cast<double>();
    InvHpiFitData coilFit;
    coilFit.m_coilPos = coilsDevice.row(0) + RowVector3d(0.005, 0.0, 0.0);
    const int strongest = amplitudes.row(0).squaredNorm() >= amplitudes.row(4).squaredNorm() ? 0 : 4;
    coilFit.m_sensorData = amplitudes.row(strongest).transpose();
    coilFit.m_sensors = updater.getSensors();
    coilFit.m_matProjector = updater.getProjectors();
    coilFit.m_iMaxIterations = 500;
    coilFit.m_fAbortError = 1e-9f;
    coilFit.doDipfitConcurrent();
    //! [inv_hpi_fit_data_usage]
    ok &= expect((coilFit.m_coilPos - coilsDevice.row(0)).norm() < 0.002 && coilFit.m_errorInfo.error < 0.05,
                 QString("InvHpiFitData: the single-coil fit returns to within %1 mm of the full fit").arg(1e3 * (coilFit.m_coilPos - coilsDevice.row(0)).norm(), 0, 'f', 2));

    qInfo().noquote() << (ok ? "All HPI checks passed." : "HPI checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
