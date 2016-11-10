//=============================================================================================================
/**
* @file     histogram.cpp
* @author   Ricky Tjen <ricky270@student.sgu.ac.id>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     March, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Ricky Tjen and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
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
#include <dispCharts/bar.h>
#include <dispCharts/spline.h>

//includes for source localization data
#include <fs/label.h>
#include <fs/surface.h>
#include <fs/annotationset.h>
#include <fiff/fiff_evoked.h>
#include <mne/mne_sourceestimate.h>
#include <inverse/minimumNorm/minimumnorm.h>
#include <disp3D/view3D.h>
#include <disp3D/control/control3dwidget.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QTime>
#include <QDebug>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

//includes for source localization data
#include <QApplication>
#include <QCommandLineParser>
#include <QSet>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace std;
using namespace DISPCHARTSLIB;
using namespace FSLIB;
using namespace INVERSELIB;
using namespace DISP3DLIB;
using namespace UTILSLIB;


//*************************************************************************************************************
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
    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Clustered Inverse Example");
    parser.addHelpOption();
    QCommandLineOption sampleFwdFileOption("f", "Path to forward solution <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    QCommandLineOption sampleCovFileOption("c", "Path to covariance <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-cov.fif");
    QCommandLineOption sampleEvokedFileOption("e", "Path to evoked <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    parser.addOption(sampleFwdFileOption);
    parser.addOption(sampleCovFileOption);
    parser.addOption(sampleEvokedFileOption);
    parser.process(a);

    //########################################################################################
    // Source Estimate
    QFile t_fileFwd(parser.value(sampleFwdFileOption));
    QFile t_fileCov(parser.value(sampleCovFileOption));
    QFile t_fileEvoked(parser.value(sampleEvokedFileOption));

    double snr = 1.0f;//3.0f;//0.1f;//3.0f;
    QString method("MNE"); //"MNE" | "dSPM" | "sLORETA"

    QString t_sFileNameClusteredInv("");
    QString t_sFileNameStc("");

    // Parse command line parameters
    for(qint32 i = 0; i < argc; ++i)
    {
        if(strcmp(argv[i], "-snr") == 0 || strcmp(argv[i], "--snr") == 0)
        {
            if(i + 1 < argc)
                snr = atof(argv[i+1]);
        }
        else if(strcmp(argv[i], "-method") == 0 || strcmp(argv[i], "--method") == 0)
        {
            if(i + 1 < argc)
                method = QString::fromUtf8(argv[i+1]);
        }
        else if(strcmp(argv[i], "-inv") == 0 || strcmp(argv[i], "--inv") == 0)
        {
            if(i + 1 < argc)
                t_sFileNameClusteredInv = QString::fromUtf8(argv[i+1]);
        }
        else if(strcmp(argv[i], "-stc") == 0 || strcmp(argv[i], "--stc") == 0)
        {
            if(i + 1 < argc)
                t_sFileNameStc = QString::fromUtf8(argv[i+1]);
        }
    }

    double lambda2 = 1.0 / pow(snr, 2);
    qDebug() << "Start calculation with: SNR" << snr << "; Lambda" << lambda2 << "; Method" << method << "; stc:" << t_sFileNameStc;

    // Load data
    fiff_int_t setno = 0;
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(t_fileEvoked, setno, baseline);
    if(evoked.isEmpty())
        return 1;

    std::cout << "evoked first " << evoked.first << "; last " << evoked.last << std::endl;

    MNEForwardSolution t_Fwd(t_fileFwd);
    if(t_Fwd.isEmpty())
        return 1;

    AnnotationSet t_annotationSet("sample", 2, "aparc.a2009s", "./MNE-sample-data/subjects");

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
    QTime myTimerHistCounts;
    myTimerHistCounts.start();
    Eigen::VectorXd dataSine;
    dataSine = sineWaveGenerator(1.0e-6,(1.0/1.0e6), 0.0, 1.0);  //creates synthetic data using sineWaveGenerator function
    qDebug()<< "sourceEstimateData.rows = " << sourceEstimateData.rows();
    qDebug()<< "sourceEstaimateData.cols = " << sourceEstimateData.cols();
    MNEMath::histcounts(sourceEstimateData, bMakeSymmetrical, classAmount, resultClassLimit, resultFrequency, inputGlobalMin, inputGlobalMax);   //user input to normalize and sort the data matrix
    std::cout << "resultClassLimits = " << resultClassLimit << std::endl;
    std::cout << "resultFrequency = " << resultFrequency << std::endl;
    qDebug()<<"HistCounts timer:"<<myTimerHistCounts.elapsed();

    int precision = 2;             //format for the amount digits of coefficient shown in the Bar Histogram (does not affect Spline)

    //displayObj can be in either Bar or Spline form; uncomment the preferred one and comment the other
    Spline* displayObj = new Spline("MNE-CPP Histogram Example (Spline)");
    //Bar* displayObj = new Bar("MNE-CPP Histogram Example (Bar)");

    QTime myTimerHistogram;
    myTimerHistogram.start();
    displayObj->setData(resultClassLimit, resultFrequency, precision);
    QVector3D thresholdLines1(2.1e-10, 5.0e-8, 6.0e-7);
    displayObj->setThreshold(thresholdLines1);

    qDebug()<<"Histogram timer:"<<myTimerHistogram.elapsed();

    displayObj->resize(800,600);
    displayObj->show();

    //std::cout << data.block(0,0,10,10);
    return a.exec();
}


//*************************************************************************************************************
