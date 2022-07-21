//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Lorenz Esch. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Example application of reading raw data and presenting the result in histogram form
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <ctime>
#include <cstdlib>
#include <Eigen/Dense>
#include <string>

#include <fiff/fiff.h>
#include <mne/mne.h>
#include <utils/mnemath.h>
#include <utils/generics/applicationlogger.h>

#include <disp/plots/bar.h>
#include <disp/plots/spline.h>

//includes for source localization data
#include <fs/label.h>
#include <fs/surface.h>
#include <fs/annotationset.h>
#include <fiff/fiff_evoked.h>
#include <mne/mne_sourceestimate.h>
#include <inverse/minimumNorm/minimumnorm.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QTime>
#include <QElapsedTimer>
#include <QDebug>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

//includes for source localization data
#include <QApplication>
#include <QCommandLineParser>
#include <QSet>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace std;
using namespace DISPLIB;
using namespace FSLIB;
using namespace INVERSELIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// MAIN
//=============================================================================================================

//sineWaveGenerator function - used to create synthetic data to test histogram functionality
Eigen::VectorXd sineWaveGenerator(double amplitude, double xStep, int xNow, int xEnd)
{
    unsigned int iterateAmount = ceil((xEnd-xNow)/xStep);
    Eigen::VectorXd sineWaveResultOriginal;
    sineWaveResultOriginal.resize(iterateAmount+1);
    Eigen::VectorXd sineWaveResult = sineWaveResultOriginal.transpose();
    double sineResult;
    double omega = 2.0*M_PI;
    int iterateCount = 0;
    for (double step = xNow; step < xEnd; step +=xStep)
    {
        sineResult = amplitude* (sin(omega * step));
        sineWaveResult(iterateCount) = sineResult;
        iterateCount++;
    }
    return sineWaveResult;
}

int main(int argc, char *argv[])
{
    //code to generate source localization data
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Histogram Example");
    parser.addHelpOption();
    QCommandLineOption sampleFwdFileOption("fwd", "Path to forward solution <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption sampleCovFileOption("cov", "Path to covariance <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QCommandLineOption sampleEvokedFileOption("ave", "Path to evoked <file>.", "file", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption methodOption("method", "Inverse estimation <method>, i.e., 'MNE', 'dSPM' or 'sLORETA'.", "method", "dSPM");//"MNE" | "dSPM" | "sLORETA"
    QCommandLineOption snrOption("snr", "The SNR value used for computation <snr>.", "snr", "1.0");//3.0;//0.1;//3.0;
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
    QFile t_fileFwd(parser.value(sampleFwdFileOption));
    QFile t_fileCov(parser.value(sampleCovFileOption));
    QFile t_fileEvoked(parser.value(sampleEvokedFileOption));

    double snr = parser.value(snrOption).toDouble();
    QString method(parser.value(methodOption));

    QString t_sFileNameClusteredInv(parser.value(invFileOption));
    QString t_sFileNameStc(parser.value(stcFileOption));

    double lambda2 = 1.0 / pow(snr, 2);
    qDebug() << "Start calculation with: SNR" << snr << "; Lambda" << lambda2 << "; Method" << method << "; stc:" << t_sFileNameStc;

    // Load data
    fiff_int_t setno = 0;
    QPair<float, float> baseline(-1.0f, -1.0f);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
        return 1;

    std::cout << "evoked first " << evoked.first << "; last " << evoked.last << std::endl;

    MNEForwardSolution t_Fwd(t_fileFwd);
    if(t_Fwd.isEmpty())
        return 1;

    AnnotationSet t_annotationSet("sample", 2, "aparc.a2009s", QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects");

    FiffCov noise_cov(t_fileCov);

    // regularize noise covariance
    noise_cov = noise_cov.regularize(evoked.info, 0.05, 0.05, 0.1, true);

    //
    // Cluster forward solution;
    //
    MNEForwardSolution t_clusteredFwd = t_Fwd.cluster_forward_solution(t_annotationSet, 20);//40);

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
    if(!t_sFileNameClusteredInv.isEmpty())
    {
        QFile t_fileClusteredInverse(t_sFileNameClusteredInv);
        inverse_operator.write(t_fileClusteredInverse);
    }

    //
    // Compute inverse solution
    //
    MinimumNorm minimumNorm(inverse_operator, lambda2, method);
    MNESourceEstimate sourceEstimate = minimumNorm.calculateInverse(evoked);
    VectorXd sourceEstimateData = sourceEstimate.data.col(10);

    bool bMakeSymmetrical;
    bMakeSymmetrical = false;       //bMakeSymmetrical option: false means data is unchanged, true means histogram x axis is symmetrical to the right and left
    int classAmount = 10;           //initialize the amount of classes and class frequencies
    double inputGlobalMin = 0.0,
           inputGlobalMax = 0.0;
    Eigen::VectorXd resultClassLimit;
    Eigen::VectorXi resultFrequency;

    //start of the histogram calculation, similar to matlab function of the same name
    QElapsedTimer myTimerHistCounts;
    myTimerHistCounts.start();
    Eigen::VectorXd dataSine;
    dataSine = sineWaveGenerator(1.0e-6,(1.0/1.0e6), 0.0, 1.0);  //creates synthetic data using sineWaveGenerator function
    qDebug()<< "sourceEstimateData.rows = " << sourceEstimateData.rows();
    qDebug()<< "sourceEstaimateData.cols = " << sourceEstimateData.cols();
    MNEMath::histcounts(sourceEstimateData, bMakeSymmetrical, classAmount, resultClassLimit, resultFrequency, inputGlobalMin, inputGlobalMax);   //user input to normalize and sort the data matrix
    std::cout << "resultClassLimits = " << resultClassLimit << std::endl;
    std::cout << "resultFrequency = " << resultFrequency << std::endl;
    qDebug()<<"HistCounts timer:"<<myTimerHistCounts.elapsed();

    //displayObj can be in either Bar or Spline form; uncomment the preferred one and comment the other
    Spline* displayObj = new Spline(0, "MNE-CPP Histogram Example (Spline)");
    //Bar* displayObj = new Bar("MNE-CPP Histogram Example (Bar)");

    QElapsedTimer myTimerHistogram;
    myTimerHistogram.start();
    displayObj->setData(resultClassLimit, resultFrequency);
    QVector3D thresholdLines1(2.1e-10f, 5.0e-8f, 6.0e-7f);
    displayObj->setThreshold(thresholdLines1);

    qDebug()<<"Histogram timer:"<<myTimerHistogram.elapsed();

    displayObj->resize(800,600);
    displayObj->show();

    //std::cout << data.block(0,0,10,10);
    return a.exec();
}

//=============================================================================================================
