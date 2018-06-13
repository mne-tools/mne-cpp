//=============================================================================================================
/**
* @file     test_spectral_connectivity.cpp
* @author   Daniel Strohmeier <daniel.strohmeier@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Daniel Strohmeier and Matti Hamalainen. All rights reserved.
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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utils/ioutils.h>
#include "connectivity/metrics/coherence.h"
#include "connectivity/metrics/imagcoherence.h"
#include "connectivity/metrics/phaselockingvalue.h"
#include "connectivity/metrics/phaselagindex.h"
#include "connectivity/metrics/unbiasedsquaredphaselagindex.h"
#include "connectivity/metrics/weightedphaselagindex.h"
#include "connectivity/metrics/debiasedsquaredweightedphaselagindex.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
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
    void spectralConnectivityCoherence();
    void spectralConnectivityImagCoherence();
    void spectralConnectivityPLV();
    void spectralConnectivityPLI();
    void spectralConnectivityPLI2();
    void spectralConnectivityWPLI();
    void spectralConnectivityWPLI2();
    void cleanupTestCase();

private:
    void compareConnectivity();
    QList<MatrixXd> readConnectivityData();
    double epsilon;
    RowVectorXd m_ConnectivityOutput;
    RowVectorXd m_RefConnectivityOutput;
};


//*************************************************************************************************************

TestSpectralConnectivity::TestSpectralConnectivity()
: epsilon(0.000001)
{
}


//*************************************************************************************************************

void TestSpectralConnectivity::initTestCase()
{

}


//*************************************************************************************************************

void TestSpectralConnectivity::spectralConnectivityCoherence()
{
    //*********************************************************************************************************
    // Load Data
    //*********************************************************************************************************

    QList<MatrixXd> matDataList = readConnectivityData();
    int iNfft = matDataList.at(0).cols();
    QString sWindowType = "hanning";

    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    QVector<MatrixXd> Coh = Coherence::computeCoherence(matDataList, iNfft, sWindowType);
    m_ConnectivityOutput = Coh.at(0).row(1);

    //*********************************************************************************************************
    // Load MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QDir::currentPath()+"/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_coh.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_RefConnectivityOutput = refConnectivity.col(0).transpose();

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************
    compareConnectivity();
}


//*************************************************************************************************************

void TestSpectralConnectivity::spectralConnectivityImagCoherence()
{
    //*********************************************************************************************************
    // Load Data
    //*********************************************************************************************************

    QList<MatrixXd> matDataList = readConnectivityData();
    int iNfft = matDataList.at(0).cols();
    QString sWindowType = "hanning";

    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    QVector<MatrixXd> ImagCoh = ImagCoherence::computeImagCoherence(matDataList, iNfft, sWindowType);
    m_ConnectivityOutput = ImagCoh.at(0).row(1);

    //*********************************************************************************************************
    // Load MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QDir::currentPath()+"/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_imagcoh.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_RefConnectivityOutput = refConnectivity.col(0).transpose();

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************

    compareConnectivity();
}


//*************************************************************************************************************

void TestSpectralConnectivity::spectralConnectivityPLV()
{
    //*********************************************************************************************************
    // Load Data
    //*********************************************************************************************************

    QList<MatrixXd> matDataList = readConnectivityData();
    int iNfft = matDataList.at(0).cols();
    QString sWindowType = "hanning";

    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    QVector<MatrixXd> PLV = PhaseLockingValue::computePLV(matDataList, iNfft, sWindowType);
    m_ConnectivityOutput = PLV.at(0).row(1);

    //*********************************************************************************************************
    // Load MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QDir::currentPath()+"/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_plv.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_RefConnectivityOutput = refConnectivity.col(0).transpose();

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************

    compareConnectivity();
}


//*************************************************************************************************************

void TestSpectralConnectivity::spectralConnectivityPLI()
{
    //*********************************************************************************************************
    // Load Data
    //*********************************************************************************************************

    QList<MatrixXd> matDataList = readConnectivityData();
    int iNfft = matDataList.at(0).cols();
    QString sWindowType = "hanning";

    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    QVector<MatrixXd> PLI = PhaseLagIndex::computePLI(matDataList, iNfft, sWindowType);
    m_ConnectivityOutput = PLI.at(0).row(1);

    //*********************************************************************************************************
    // Load MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QDir::currentPath()+"/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_pli.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_RefConnectivityOutput = refConnectivity.col(0).transpose();

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************

    compareConnectivity();
}


//*************************************************************************************************************

void TestSpectralConnectivity::spectralConnectivityPLI2()
{
    //*********************************************************************************************************
    // Load Data
    //*********************************************************************************************************

    QList<MatrixXd> matDataList = readConnectivityData();
    int iNfft = matDataList.at(0).cols();
    QString sWindowType = "hanning";

    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    QVector<MatrixXd> PLI2 = UnbiasedSquaredPhaseLagIndex::computeUnbiasedSquaredPLI(matDataList, iNfft, sWindowType);
    m_ConnectivityOutput = PLI2.at(0).row(1);

    //*********************************************************************************************************
    // Load MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QDir::currentPath()+"/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_pli2.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_RefConnectivityOutput = refConnectivity.col(0).transpose();

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************

    compareConnectivity();
}


//*************************************************************************************************************

void TestSpectralConnectivity::spectralConnectivityWPLI()
{
    //*********************************************************************************************************
    // Load Data
    //*********************************************************************************************************

    QList<MatrixXd> matDataList = readConnectivityData();
    int iNfft = matDataList.at(0).cols();
    QString sWindowType = "hanning";

    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    QVector<MatrixXd> WPLI = WeightedPhaseLagIndex::computeWPLI(matDataList, iNfft, sWindowType);
    m_ConnectivityOutput = WPLI.at(0).row(1);

    //*********************************************************************************************************
    // Load MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QDir::currentPath()+"/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_wpli.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_RefConnectivityOutput = refConnectivity.col(0).transpose();

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************

    compareConnectivity();
}


//*************************************************************************************************************

void TestSpectralConnectivity::spectralConnectivityWPLI2()
{
    //*********************************************************************************************************
    // Load Data
    //*********************************************************************************************************

    QList<MatrixXd> matDataList = readConnectivityData();
    int iNfft = matDataList.at(0).cols();
    QString sWindowType = "hanning";

    //*********************************************************************************************************
    // Compute Connectivity
    //*********************************************************************************************************

    QVector<MatrixXd> WPLI2 = DebiasedSquaredWeightedPhaseLagIndex::computeDebiasedSquaredWPLI(matDataList, iNfft, sWindowType);
    m_ConnectivityOutput = WPLI2.at(0).row(1);

    //*********************************************************************************************************
    // Load MNE-PYTHON Results As Reference
    //*********************************************************************************************************

    MatrixXd refConnectivity;
    QString refFileName(QDir::currentPath()+"/mne-cpp-test-data/Result/Connectivity/ref_spectral_connectivity_wpli2.txt");
    IOUtils::read_eigen_matrix(refConnectivity, refFileName);
    m_RefConnectivityOutput = refConnectivity.col(0).transpose();

    //*********************************************************************************************************
    // Compare Connectivity
    //*********************************************************************************************************

    compareConnectivity();
}


//*************************************************************************************************************

QList<MatrixXd> TestSpectralConnectivity::readConnectivityData()
{
    MatrixXd inputTrials;
    QString dataFileName(QDir::currentPath()+"/mne-cpp-test-data/MEG/sample/data_spectral_connectivity.txt");
    IOUtils::read_eigen_matrix(inputTrials, dataFileName);
    int iNTrials = inputTrials.rows() / 2;

    QList<MatrixXd> matDataList;
    for (int i = 0; i < iNTrials; ++i)
    {
        matDataList.append(inputTrials.middleRows(i * 2, 2));
    }

    return matDataList;
}


//*************************************************************************************************************

void TestSpectralConnectivity::compareConnectivity()
{
    //*********************************************************************************************************
    // Compare Spectral Connectvitity Result to MNE-PYTHON
    //*********************************************************************************************************

    printf(">>>>>>>>>>>>>>>>>>>>>>>>> Compare Spectral Connectivities >>>>>>>>>>>>>>>>>>>>>>>>>\n");

    printf ("1\n");
    QCOMPARE( m_ConnectivityOutput.rows(), m_RefConnectivityOutput.rows() );

    printf ("2\n");
    QCOMPARE( m_ConnectivityOutput.cols(), m_RefConnectivityOutput.cols() );

    printf ("3 sizes: %d %d %d %d\n", int(m_ConnectivityOutput.rows()), int(m_RefConnectivityOutput.rows()), int(m_ConnectivityOutput.cols()), int(m_RefConnectivityOutput.cols()));

    //for (int i = 0; i < m_ConnectivityOutput.cols(); ++i)
    for (int i = m_ConnectivityOutput.cols() - 1; i >= 0; --i)
    {
        if (m_RefConnectivityOutput(i) == 0.0){
            printf ("4 sample: %d \n", i);
            QCOMPARE( m_ConnectivityOutput(i), m_RefConnectivityOutput(i) );
        } else {
            printf ("5 sample: %d \n", i);
            if ((fabs(m_ConnectivityOutput(i) - m_RefConnectivityOutput(i)) / fabs(m_RefConnectivityOutput(i))) >= epsilon){
                printf ("actual: %.10f  --  reference: %.10f  -- difference: %.10f  --  rel. difference: %.10f\n", m_ConnectivityOutput(i), m_RefConnectivityOutput(i), fabs(m_ConnectivityOutput(i) - m_RefConnectivityOutput(i)), (fabs(m_ConnectivityOutput(i) - m_RefConnectivityOutput(i)) / fabs(m_RefConnectivityOutput(i))));
            }
            printf ("6 sample: %d \n", i);
            QVERIFY( (fabs(m_ConnectivityOutput(i) - m_RefConnectivityOutput(i)) / fabs(m_RefConnectivityOutput(i))) < epsilon );
        }
    }

    printf("<<<<<<<<<<<<<<<<<<<<<<<<< Compare Spectral Connectivities Finished <<<<<<<<<<<<<<<<<<<<<<<<<\n");
}


//*************************************************************************************************************

void TestSpectralConnectivity::cleanupTestCase()
{
}


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_APPLESS_MAIN(TestSpectralConnectivity)
#include "test_spectral_connectivity.moc"
