//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file phaselockingvalue.cpp
 * @since 2026
 * @date  March 2026
 * @brief Implementation of @ref CONNLIB::PhaseLockingValue - resultant length of the unit-modulus cross-spectrum (Lachaux et al. HBM 1999).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "phaselockingvalue.h"
#include "../network/networknode.h"
#include "../network/networkedge.h"
#include "../network/network.h"

#include <math/spectral.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QtConcurrent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNLIB;
using namespace Eigen;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PhaseLockingValue::PhaseLockingValue()
{
}

//*******************************************************************************************************

Network PhaseLockingValue::calculate(ConnectivitySettings& connectivitySettings)
{
//    QElapsedTimer timer;
//    qint64 iTime = 0;
//    timer.start();

    Network finalNetwork("PLV");

    if(connectivitySettings.isEmpty()) {
        qDebug() << "PhaseLockingValue::calculate - Input data is empty";
        return finalNetwork;
    }

    if(AbstractMetric::m_bStorageModeIsActive == false) {
        connectivitySettings.clearIntermediateData();
    }

    finalNetwork.setSamplingFrequency(connectivitySettings.getSamplingFrequency());

    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

    //Create nodes
    int iNRows = connectivitySettings.at(0).matData.rows();
    RowVectorXf rowVert = RowVectorXf::Zero(3);

    for(int i = 0; i < iNRows; ++i) {
        rowVert = RowVectorXf::Zero(3);

        if(connectivitySettings.getNodePositions().rows() != 0 && i < connectivitySettings.getNodePositions().rows()) {
            rowVert(0) = connectivitySettings.getNodePositions().row(i)(0);
            rowVert(1) = connectivitySettings.getNodePositions().row(i)(1);
            rowVert(2) = connectivitySettings.getNodePositions().row(i)(2);
        }

        finalNetwork.append(NetworkNode::SPtr(new NetworkNode(i, rowVert)));
    }

    // Check that iNfft >= signal length
    int iSignalLength = connectivitySettings.at(0).matData.cols();
    int iNfft = connectivitySettings.getFFTSize();

    // Generate tapers
    QPair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iSignalLength, connectivitySettings.getWindowType());

    // Initialize
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;

    // Check if start and bin amount need to be reset to full spectrum
    if(m_iNumberBinStart == -1 ||
       m_iNumberBinAmount == -1 ||
       m_iNumberBinStart > iNFreqs ||
       m_iNumberBinAmount > iNFreqs ||
       m_iNumberBinAmount + m_iNumberBinStart > iNFreqs) {
        qDebug() << "PhaseLockingValue::calculate - Resetting to full spectrum";
        AbstractMetric::m_iNumberBinStart = 0;
        AbstractMetric::m_iNumberBinAmount = iNFreqs;
    }

    // Pass information about the FFT length. Use iNFreqs because we only use the half spectrum
    finalNetwork.setFFTSize(iNFreqs);
    finalNetwork.setUsedFreqBins(AbstractMetric::m_iNumberBinAmount);

    QMutex mutex;

    std::function<void(ConnectivitySettings::IntermediateTrialData&)> computeLambda = [&](ConnectivitySettings::IntermediateTrialData& inputData) {
        compute(inputData,
                connectivitySettings.getIntermediateSumData().vecPairCsdSum,
                connectivitySettings.getIntermediateSumData().vecPairCsdNormalizedSum,
                mutex,
                iNRows,
                iNFreqs,
                iNfft,
                tapers);
    };

//    iTime = timer.elapsed();
//    qWarning() << "Preparation" << iTime;
//    timer.restart();

    // Compute PLV in parallel for all trials
    QFuture<void> result = QtConcurrent::map(connectivitySettings.getTrialData(),
                                             computeLambda);
    result.waitForFinished();

//    iTime = timer.elapsed();
//    qWarning() << "ComputeSpectraPSDCSD" << iTime;
//    timer.restart();

    // Compute PLV
    computePLV(connectivitySettings,
               finalNetwork);

//    iTime = timer.elapsed();
//    qWarning() << "Compute" << iTime;
//    timer.restart();

    return finalNetwork;
}

//=============================================================================================================

void PhaseLockingValue::compute(ConnectivitySettings::IntermediateTrialData& inputData,
                                QVector<QPair<int,Eigen::MatrixXcd> >& vecPairCsdSum,
                                QVector<QPair<int,MatrixXcd> >& vecPairCsdNormalizedSum,
                                QMutex& mutex,
                                int iNRows,
                                int iNFreqs,
                                int iNfft,
                                const QPair<MatrixXd, VectorXd>& tapers)
{
    if(inputData.vecPairCsdNormalized.size() == iNRows) {
        //qDebug() << "PhaseLockingValue::compute - vecPairCsdNormalized was already computed for this trial.";
        return;
    }

    int i,j;

    // Calculate tapered spectra if not available already
    // This code was copied and changed modified Utils/Spectra since we do not want to call the function due to time loss.
    if(inputData.vecTapSpectra.isEmpty()) {
        RowVectorXd vecInputFFT, rowData;
        RowVectorXcd vecTmpFreq;

        MatrixXcd matTapSpectrum(tapers.first.rows(), iNFreqs);

        FFT<double> fft;
        fft.SetFlag(fft.HalfSpectrum);

        for (i = 0; i < iNRows; ++i) {
            // Substract mean
            rowData.array() = inputData.matData.row(i).array() - inputData.matData.row(i).mean();

            // Calculate tapered spectra if not available already
            for(j = 0; j < tapers.first.rows(); j++) {
                // Zero padd if necessary. The zero padding in Eigen's FFT is only working for column vectors.
                if (rowData.cols() < iNfft) {
                    vecInputFFT.setZero(iNfft);
                    vecInputFFT.block(0,0,1,rowData.cols()) = rowData.cwiseProduct(tapers.first.row(j));;
                } else {
                    vecInputFFT = rowData.cwiseProduct(tapers.first.row(j));
                }

                // FFT for freq domain returning the half spectrum and multiply taper weights
                fft.fwd(vecTmpFreq, vecInputFFT, iNfft);
                matTapSpectrum.row(j) = vecTmpFreq * tapers.second(j);
            }

            inputData.vecTapSpectra.append(matTapSpectrum);
        }
    }

    // Compute CSD
    if(inputData.vecPairCsd.isEmpty()) {
        MatrixXcd matCsd = MatrixXcd(iNRows, m_iNumberBinAmount);

        bool bNfftEven = false;
        if (iNfft % 2 == 0){
            bNfftEven = true;
        }

        double denomCSD = sqrt(tapers.second.cwiseAbs2().sum()) * sqrt(tapers.second.cwiseAbs2().sum()) / 2.0;

        for (i = 0; i < iNRows; ++i) {
            for (j = i; j < iNRows; ++j) {
                // Compute CSD (average over tapers if necessary)
                matCsd.row(j) = inputData.vecTapSpectra.at(i).block(0,m_iNumberBinStart,inputData.vecTapSpectra.at(i).rows(),m_iNumberBinAmount).cwiseProduct(inputData.vecTapSpectra.at(j).block(0,m_iNumberBinStart,inputData.vecTapSpectra.at(j).rows(),m_iNumberBinAmount).conjugate()).colwise().sum() / denomCSD;

                // Divide first and last element by 2 due to half spectrum
                if(m_iNumberBinStart == 0) {
                    matCsd.row(j)(0) /= 2.0;
                }

                if(bNfftEven && m_iNumberBinStart + m_iNumberBinAmount >= iNFreqs) {
                    matCsd.row(j).tail(1) /= 2.0;
                }
            }

            inputData.vecPairCsd.append(QPair<int,MatrixXcd>(i,matCsd));
            inputData.vecPairCsdNormalized.append(QPair<int,MatrixXcd>(i,matCsd.cwiseQuotient(matCsd.cwiseAbs())));
        }

        mutex.lock();

        if(vecPairCsdSum.isEmpty()) {
            vecPairCsdSum = inputData.vecPairCsd;
            vecPairCsdNormalizedSum = inputData.vecPairCsdNormalized;
        } else {
            for (int j = 0; j < vecPairCsdSum.size(); ++j) {
                vecPairCsdSum[j].second += inputData.vecPairCsd.at(j).second;
                vecPairCsdNormalizedSum[j].second += inputData.vecPairCsdNormalized.at(j).second;
            }
        }

        mutex.unlock();
    } else {
        if(inputData.vecPairCsdNormalized.isEmpty()) {
            for (i = 0; i < iNRows; ++i) {
                inputData.vecPairCsdNormalized.append(QPair<int,MatrixXcd>(i,inputData.vecPairCsd.at(i).second.cwiseQuotient(inputData.vecPairCsd.at(i).second.cwiseAbs())));
            }

            mutex.lock();

            if(vecPairCsdNormalizedSum.isEmpty()) {
                vecPairCsdNormalizedSum = inputData.vecPairCsdNormalized;
            } else {
                for (int j = 0; j < vecPairCsdNormalizedSum.size(); ++j) {
                    vecPairCsdNormalizedSum[j].second += inputData.vecPairCsdNormalized.at(j).second;
                }
            }

            mutex.unlock();
        }
    }

    if(!m_bStorageModeIsActive) {
        inputData.vecPairCsd.clear();
        inputData.vecTapSpectra.clear();
        inputData.vecPairCsdNormalized.clear();
    }
}

//=============================================================================================================

void PhaseLockingValue::computePLV(ConnectivitySettings &connectivitySettings,
                                   Network& finalNetwork)
{
    // Compute final PLV and create Network
    MatrixXd matNom;
    MatrixXd matWeight;
    QSharedPointer<NetworkEdge> pEdge;
    int j;

    for (int i = 0; i < connectivitySettings.at(0).matData.rows(); ++i) {
        matNom = connectivitySettings.getIntermediateSumData().vecPairCsdNormalizedSum.at(i).second.cwiseAbs() / connectivitySettings.size();

        for(j = i; j < connectivitySettings.at(0).matData.rows(); ++j) {
            matWeight = matNom.row(j).transpose();

            pEdge = QSharedPointer<NetworkEdge>(new NetworkEdge(i, j, matWeight));

            finalNetwork.getNodeAt(i)->append(pEdge);
            finalNetwork.getNodeAt(j)->append(pEdge);
            finalNetwork.append(pEdge);
        }
    }
}
