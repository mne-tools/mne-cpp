//=============================================================================================================
/**
 * @file     test_spectral_connectivity.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Daniel Strohmeier <Daniel.Strohmeier@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Gabriel B Motta, Daniel Strohmeier, Lorenz Esch. All rights reserved.
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
 * @brief    The spectral connectivity test implementation
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/generics/applicationlogger.h>

#include <utils/ioutils.h>
#include <connectivity/metrics/coherency.h>
#include <connectivity/metrics/coherence.h>
#include <connectivity/metrics/imagcoherence.h>
#include <connectivity/metrics/phaselockingvalue.h>
#include <connectivity/metrics/phaselagindex.h>
#include <connectivity/metrics/unbiasedsquaredphaselagindex.h>
#include <connectivity/metrics/weightedphaselagindex.h>
#include <connectivity/metrics/debiasedsquaredweightedphaselagindex.h>
#include <connectivity/metrics/crosscorrelation.h>
#include <connectivity/connectivitysettings.h>
#include <connectivity/network/network.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace CONNECTIVITYLIB;
using namespace UTILSLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestSpectralConnectivity
 *
 * @brief The TestSpectralConnectivity class provides spectral connectivity tests
 *
 */
class TestSpectralConnectivity: public QObject
{
    Q_OBJECT

public:
    TestSpectralConnectivity();

private slots:
    void initTestCase();
    void spectralConnectivityPLV();
    void spectralConnectivityPLI();
    void spectralConnectivityPLI2();
    void spectralConnectivityWPLI();
    void spectralConnectivityWPLI2();
    void spectralConnectivityCoherence();
    void spectralConnectivityImagCoherence();
    void spectralConnectivityXCOR();
    void cleanupTestCase();

private:
    void compareConnectivity();
    QList<MatrixXd> readConnectivityData();
    double dEpsilon;
    double m_dConnectivityOutput;
    double m_dRefConnectivityOutput;
    ConnectivitySettings m_connectivitySettings;
};

//=============================================================================================================

TestSpectralConnectivity::TestSpectralConnectivity()
: dEpsilon(0.0000000001)
{
}

//=============================================================================================================

void TestSpectralConnectivity::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    //*********************************************************************************************************
    // Load and Setup Testing Data
    //*********************************************************************************************************

    QList<MatrixXd> matDataList = readConnectivityData();
    m_connectivitySettings.setFFTSize(matDataList.at(0).cols());
    m_connectivitySettings.setWindowType("hanning");
    m_connectivitySettings.append(matDataList);
}

//=============================================================================================================

void TestSpectralConnectivity::spectralConnectivityCoherence()
{
    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    Network network = Coherence::calculate(m_connectivitySettings);
    m_dConnectivityOutput = network.getFullConnectivityMatrix()(0,1);

    //*********************************************************************************************************
    // Load MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_coh.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_dRefConnectivityOutput = refConnectivity.col(0).mean();

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************
    compareConnectivity();
}

//=============================================================================================================

void TestSpectralConnectivity::spectralConnectivityImagCoherence()
{
    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    Network network = ImagCoherence::calculate(m_connectivitySettings);
    m_dConnectivityOutput = network.getFullConnectivityMatrix()(0,1);

    //*********************************************************************************************************
    // Load MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_imagcoh.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_dRefConnectivityOutput = refConnectivity.col(0).mean();

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************

    compareConnectivity();
}

//=============================================================================================================

void TestSpectralConnectivity::spectralConnectivityPLV()
{
    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    Network network = PhaseLockingValue::calculate(m_connectivitySettings);
    m_dConnectivityOutput = network.getFullConnectivityMatrix()(0,1);

    //*********************************************************************************************************
    // Load MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_plv.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_dRefConnectivityOutput = refConnectivity.col(0).mean();

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************

    compareConnectivity();
}

//=============================================================================================================

void TestSpectralConnectivity::spectralConnectivityPLI()
{
    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    Network network = PhaseLagIndex::calculate(m_connectivitySettings);
    m_dConnectivityOutput = network.getFullConnectivityMatrix()(0,1);

    //*********************************************************************************************************
    // Load MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_pli.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_dRefConnectivityOutput = refConnectivity.col(0).mean();

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************

    compareConnectivity();
}

//=============================================================================================================

void TestSpectralConnectivity::spectralConnectivityPLI2()
{
    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    Network network = UnbiasedSquaredPhaseLagIndex::calculate(m_connectivitySettings);
    m_dConnectivityOutput = network.getFullConnectivityMatrix()(0,1);

    //*********************************************************************************************************
    // Load MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_pli2.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_dRefConnectivityOutput = refConnectivity.col(0).mean();

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************

    compareConnectivity();
}

//=============================================================================================================

void TestSpectralConnectivity::spectralConnectivityWPLI()
{
    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    Network network = WeightedPhaseLagIndex::calculate(m_connectivitySettings);
    m_dConnectivityOutput = network.getFullConnectivityMatrix()(0,1);

    //*********************************************************************************************************
    // Load MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_wpli.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_dRefConnectivityOutput = refConnectivity.col(0).mean();

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************

    compareConnectivity();
}

//=============================================================================================================

void TestSpectralConnectivity::spectralConnectivityWPLI2()
{
    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    Network network = DebiasedSquaredWeightedPhaseLagIndex::calculate(m_connectivitySettings);
    m_dConnectivityOutput = network.getFullConnectivityMatrix()(0,1);

    //*********************************************************************************************************
    // Load MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_wpli2.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_dRefConnectivityOutput = refConnectivity.col(0).mean();

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************

    compareConnectivity();
}

//=============================================================================================================

void TestSpectralConnectivity::spectralConnectivityXCOR()
{
    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    QString outputResultFile("ref_spectral_connectivity_xcorr.txt");
    Network network = CrossCorrelation::calculate(m_connectivitySettings);
    m_dConnectivityOutput = network.getFullConnectivityMatrix()(0,1);
    IOUtils::write_eigen_matrix(network.getFullConnectivityMatrix(), outputResultFile);

    //*********************************************************************************************************
    // Load Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_xcor.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_dRefConnectivityOutput = refConnectivity(0,1);

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************

    compareConnectivity();

    //*********************************************************************************************************
    // Cleanup
    //*********************************************************************************************************

    if(QFile::exists(outputResultFile))
    {
        QFile::remove(outputResultFile);
    }
}

//=============================================================================================================

QList<MatrixXd> TestSpectralConnectivity::readConnectivityData()
{
    MatrixXd inputTrials;
    QString dataFileName(QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/data_spectral_connectivity.txt");
    IOUtils::read_eigen_matrix(inputTrials, dataFileName);
    int iNTrials = inputTrials.rows() / 2;

    QList<MatrixXd> matDataList;
    for (int i = 0; i < iNTrials; ++i)
    {
        matDataList.append(inputTrials.middleRows(i * 2, 2));
    }

    return matDataList;
}

//=============================================================================================================

void TestSpectralConnectivity::compareConnectivity()
{
    //*********************************************************************************************************
    // Compare Spectral Connectvitity Result to MNE-PYTHON
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compare Spectral Connectivities >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    //*********************************************************************************************************
    // Compare connectivity estimate
    //*********************************************************************************************************

    QVERIFY( (fabs(m_dConnectivityOutput - m_dRefConnectivityOutput)) < dEpsilon );

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compare Spectral Connectivities Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

//=============================================================================================================

void TestSpectralConnectivity::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestSpectralConnectivity)
#include "test_spectral_connectivity.moc"
