//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March, 2016
 * @brief    Histogram of the source amplitudes of a clustered MNE, dSPM or sLORETA estimate of the sample evoked response.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/mne/mne.h>
#include <mne/math/numerics.h>
#include <mne/base/log.h>
#include <mne/disp/plots/bar.h>
#include <mne/disp/plots/helpers/figureexporter.h>
#include <mne/disp/plots/spline.h>
//includes for source localization data
#include <mne/fs/fs_label.h>
#include <mne/fs/fs_surface.h>
#include <mne/fs/fs_annotationset.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/inv/inv_source_estimate.h>
#include <mne/inv/minimum_norm/inv_minimum_norm.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QTime>
#include <QElapsedTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
//includes for source localization data
#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QSet>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <string>
#include <numbers>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace std;
using namespace mne::disp;
using namespace Eigen;

//=============================================================================================================
// MAIN
//=============================================================================================================

//sineWaveGenerator function - used to create synthetic data to test histogram functionality
Eigen::VectorXd sineWaveGenerator(double amplitude, double xStep, int xNow, int xEnd)
{
    unsigned int iterateAmount = ceil((xEnd - xNow) / xStep);
    Eigen::VectorXd sineWaveResultOriginal;
    sineWaveResultOriginal.resize(iterateAmount + 1);
    Eigen::VectorXd sineWaveResult = sineWaveResultOriginal.transpose();
    double sineResult;
    double omega = 2.0 * std::numbers::pi;
    int iterateCount = 0;
    for (double step = xNow; step < xEnd; step += xStep) {
        sineResult = amplitude * (sin(omega * step));
        sineWaveResult(iterateCount) = sineResult;
        iterateCount++;
    }
    return sineWaveResult;
}

int main(int argc, char* argv[])
{
    //code to generate source localization data
    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Histogram Example");
    parser.addHelpOption();
    QCommandLineOption sampleFwdFileOption("fwd", "Path to forward solution <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption sampleCovFileOption("cov", "Path to covariance <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QCommandLineOption sampleEvokedFileOption("ave", "Path to evoked <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption methodOption("method", "Inverse estimation <method>, i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM"); //"MNE" | "dSPM" | "sLORETA"
    QCommandLineOption snrOption("snr", "The SNR value used for computation <snr>.", "snr", "1.0");                                //3.0;//0.1;//3.0;
    QCommandLineOption stcFileOption("stcOut", "Path to stc <file>, which is to be written.", "file", "");
    QCommandLineOption invFileOption("invOut", "Path to inverse <file>, which is to be written.", "file", "");

    parser.addOption(sampleFwdFileOption);
    parser.addOption(sampleCovFileOption);
    parser.addOption(sampleEvokedFileOption);
    parser.addOption(methodOption);
    parser.addOption(snrOption);
    parser.addOption(stcFileOption);
    parser.addOption(invFileOption);

    parser.process(a);

    // Source Estimate

    double snr = parser.value(snrOption).toDouble();
    QString method(parser.value(methodOption));

    QString t_sFileNameClusteredInv(parser.value(invFileOption));
    QString t_sFileNameStc(parser.value(stcFileOption));

    double lambda2 = 1.0 / pow(snr, 2);
    Log::debug("Start calculation with: SNR {} ; Lambda {} ; Method {} ; stc: {}", snr, lambda2, method.toStdString(), t_sFileNameStc.toStdString());

    // Load data
    // Without a baseline the DC offset of the average dominates the estimate
    FiffEvoked evoked(parser.value(sampleEvokedFileOption).toStdU16String(), 0, {{-0.2f, 0.0f}});

    std::cout << "evoked first " << evoked.first << "; last " << evoked.last << std::endl;

    MNEForwardSolution t_Fwd(parser.value(sampleFwdFileOption).toStdU16String());
    if (t_Fwd.empty())
        return 1;

    FsAnnotationSet t_annotationSet("sample", 2, "aparc.a2009s", (QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects").toStdU16String());

    FiffCov noise_cov(parser.value(sampleCovFileOption).toStdString());

    // regularize noise covariance
    noise_cov = noise_cov.regularize(evoked.info, 0.05, 0.05, 0.1, true);

    //
    // Cluster forward solution;
    //
    MNEForwardSolution t_clusteredFwd = t_Fwd.cluster_forward_solution(t_annotationSet, 20); //40);

    //    std::cout << "Size " << t_clusteredFwd.sol->data.rows() << " x " << t_clusteredFwd.sol->data.cols() << std::endl;
    //    std::cout << "Clustered Fwd:\n" << t_clusteredFwd.sol->data.row(0) << std::endl;

    //
    // make an inverse operators
    //
    FiffInfo info = evoked.info;

    MNEInverseOperator inverse_operator(info, t_clusteredFwd, noise_cov, 0.2f, 0.8f);

    //
    // save clustered inverse
    //
    if (!t_sFileNameClusteredInv.isEmpty()) {
        if (auto written = inverse_operator.write(t_sFileNameClusteredInv.toStdU16String()); !written) {
            Log::warning("{}", written.error().what());
        }
    }

    //
    // Compute inverse solution
    //
    InvMinimumNorm minimumNorm(inverse_operator, lambda2, method.toStdString());
    InvSourceEstimate sourceEstimate = minimumNorm.calculateInverse(evoked);
    // The source amplitudes at 100 ms, near the peak of the auditory response
    const int peakSample = static_cast<int>(std::lround((0.1f - sourceEstimate.tmin) / sourceEstimate.tstep));
    VectorXd sourceEstimateData = sourceEstimate.data.col(peakSample);

    bool bMakeSymmetrical;
    bMakeSymmetrical = false; //bMakeSymmetrical option: false means data is unchanged, true means histogram x axis is symmetrical to the right and left
    int classAmount = 10;     //initialize the amount of classes and class frequencies
    double inputGlobalMin = 0.0,
           inputGlobalMax = 0.0;
    Eigen::VectorXd resultClassLimit;
    Eigen::VectorXi resultFrequency;

    //start of the histogram calculation, similar to matlab function of the same name
    QElapsedTimer myTimerHistCounts;
    myTimerHistCounts.start();
    Eigen::VectorXd dataSine;
    dataSine = sineWaveGenerator(1.0e-6, (1.0 / 1.0e6), 0.0, 1.0); //creates synthetic data using sineWaveGenerator function
    Log::debug("sourceEstimateData.rows = {}", sourceEstimateData.rows());
    Log::debug("sourceEstaimateData.cols = {}", sourceEstimateData.cols());
    Numerics::histcounts(sourceEstimateData, bMakeSymmetrical, classAmount, resultClassLimit, resultFrequency, inputGlobalMin, inputGlobalMax); //user input to normalize and sort the data matrix
    std::cout << "resultClassLimits = " << resultClassLimit << std::endl;
    std::cout << "resultFrequency = " << resultFrequency << std::endl;
    Log::debug("HistCounts timer: {}", myTimerHistCounts.elapsed());

    //displayObj can be in either Bar or Spline form; uncomment the preferred one and comment the other
    Spline* displayObj = new Spline(0, QString("%1 of %2 clustered sources at 100 ms").arg(method).arg(sourceEstimateData.size()));
    //Bar* displayObj = new Bar("MNE-CPP Histogram Example (Bar)");

    QElapsedTimer myTimerHistogram;
    myTimerHistogram.start();
    displayObj->setData(resultClassLimit, resultFrequency);
    displayObj->setXLabel(method);
    displayObj->setYLabel("Sources");
    // Markers at the 50th, 90th and 99th percentile, e.g. as the limits of a colour map
    VectorXd sorted = sourceEstimateData;
    std::sort(sorted.data(), sorted.data() + sorted.size());
    const auto percentile = [&](double q) {
        return static_cast<float>(sorted(static_cast<Index>(q * (sorted.size() - 1))));
    };
    displayObj->setThreshold(QVector3D(percentile(0.5), percentile(0.9), percentile(0.99)));

    Log::debug("Histogram timer: {}", myTimerHistogram.elapsed());

    FigureExporter::save(*displayObj, "histogram", QString("Distribution of the %1 values of the %2 clustered sources at 100 ms in %3 classes, with markers at the 50th, 90th and 99th percentile.").arg(method).arg(sourceEstimateData.size()).arg(classAmount));
    if (!FigureExporter::directory().isEmpty()) {
        delete displayObj;
        return 0;
    }
    displayObj->resize(800, 600);
    displayObj->show();

    //std::cout << data.block(0,0,10,10);
    return a.exec();
}

//=============================================================================================================
