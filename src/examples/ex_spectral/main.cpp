//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March, 2018
 * @brief    Example of the computation of spectra, PSD and CSD
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <math.h>
#include <disp/plots/plot.h>
#include <math/spectral.h>
#include <utils/generics/mne_logger.h>
#include <mne/mne.h>

//=============================================================================================================
// Eigen
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QtMath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace DISPLIB;
using namespace UTILSLIB;

//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
 * The function main marks the entry point of the program.
 * By default, main has the storage class extern.
 *
 * @param[in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
 * @param[in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
 * @return the value that was set to exit() (which is 0 if exit() is called via quit()).
 */
int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QApplication a(argc, argv);

    // Generate input data
    int iNSamples = 500;
    double dSampFreq = 23.0;
    MatrixXd inputData = MatrixXd::Random(2, iNSamples);
    for (int n = 0; n < iNSamples; n++) {
        inputData(0, n) += 10.0 * sin(2.0 * M_PI *  10. * n / iNSamples );
        inputData(0, n) += 10.0 * sin(2.0 * M_PI * 100. * n / iNSamples);
        inputData(0, n) += 10.0 * sin(2.0 * M_PI * 200. * n / iNSamples);

        inputData(1, n) += 10.0 * sin(2.0 * M_PI *  50. * n / iNSamples);
        inputData(1, n) += 10.0 * sin(2.0 * M_PI * 100. * n / iNSamples);
        inputData(1, n) += 10.0 * sin(2.0 * M_PI * 150. * n / iNSamples);
    }

    //Generate hanning window
    QPair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iNSamples, QString("hanning"));
    MatrixXd matTaps = tapers.first;
    VectorXd vecTapWeights = tapers.second;

    //Plot hanning window
    VectorXd hann = matTaps.row(0).transpose();
    Plot plotWindow(hann);
    plotWindow.setTitle("Hanning window");
    plotWindow.setXLabel("X Axes");
    plotWindow.setYLabel("Y Axes");
    plotWindow.setWindowTitle("Corresponding function to MATLABs plot");
    plotWindow.show();

    //Compute Spectrum of first row of input data
    int iNfft = iNSamples;
    MatrixXcd matTapSpectrumSeed = Spectral::computeTaperedSpectraRow(inputData.row(0), matTaps, iNfft);

    //Compute PSD
    RowVectorXd vecPsd = Spectral::psdFromTaperedSpectra(matTapSpectrumSeed, vecTapWeights, iNfft, dSampFreq);

    //Plot PSD
    VectorXd psd = vecPsd.transpose();
    Plot plotPsd(psd);
    plotPsd.setTitle("PSD of Row 0");
    plotPsd.setXLabel("X Axes");
    plotPsd.setYLabel("Y Axes");
    plotPsd.setWindowTitle("Corresponding function to MATLABs plot");
    plotPsd.show();

    //Check PSD
    VectorXd psdTest = matTapSpectrumSeed.row(0).cwiseAbs2().transpose();
    psdTest *= 2.0;
    psdTest(0) /= 2.0;
    if (iNfft % 2 == 0){
        psdTest.tail(1) /= 2.0;
    }
    //Normalization
    psdTest /= dSampFreq;

    //Plot PSDTest
    Plot plotPsdTest(psdTest);
    plotPsdTest.setTitle("PSD of Row 0");
    plotPsdTest.setXLabel("X Axes");
    plotPsdTest.setYLabel("Y Axes");
    plotPsdTest.setWindowTitle("Corresponding function to MATLABs plot");
    plotPsdTest.show();

    //Compute CSD of matTapSpectrumSeed and matTapSpectrumSeed
    //The real part should be equivalent to the PSD of matTapSpectrumSeed)
    RowVectorXcd vecCsdSeed = Spectral::csdFromTaperedSpectra(matTapSpectrumSeed, matTapSpectrumSeed, vecTapWeights, vecTapWeights, iNfft, dSampFreq);
    VectorXd psdTest2 = vecCsdSeed.real();

    //Plot PSDTest2
    Plot plotPsdTest2(psdTest2);
    plotPsdTest2.setTitle("PSD of Row 0");
    plotPsdTest2.setXLabel("X Axes");
    plotPsdTest2.setYLabel("Y Axes");
    plotPsdTest2.setWindowTitle("Corresponding function to MATLABs plot");
    plotPsdTest2.show();

    //Check that sums of different psds of the same signal are equal
    qDebug()<<psd.sum();
    qDebug()<<psdTest.sum();
    qDebug()<<psdTest2.sum();
    RowVectorXd data_hann = inputData.row(0).cwiseProduct(matTaps.row(0));
    qDebug()<<data_hann.row(0).cwiseAbs2().sum() * double(iNSamples) / dSampFreq;

    //Compute Spectrum of second row of input data
    MatrixXcd matTapSpectrumTarget = Spectral::computeTaperedSpectraRow(inputData.row(1), matTaps, iNfft);

    //Compute CSD between seed and target
    RowVectorXcd vecCsd = Spectral::csdFromTaperedSpectra(matTapSpectrumSeed, matTapSpectrumTarget, vecTapWeights, vecTapWeights, iNfft, dSampFreq);

    //Plot real part of CSD
    VectorXd csd = vecCsd.transpose().real();
    Plot plotCsd(csd);
    plotCsd.setTitle("Real part of the CSD of Row 0 and 1");
    plotCsd.setXLabel("X Axes");
    plotCsd.setYLabel("Y Axes");
    plotCsd.setWindowTitle("Corresponding function to MATLABs plot");
    plotCsd.show();

    return a.exec();
}
