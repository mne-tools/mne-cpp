//=============================================================================================================
/**
 * @file     connectivitysettings.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch. All rights reserved.
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
 * @brief    ConnectivitySettings class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivitysettings.h"

#include <mne/mne_forwardsolution.h>
#include <fs/surfaceset.h>
#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;
using namespace MNELIB;
using namespace Eigen;
using namespace FIFFLIB;
using namespace FSLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ConnectivitySettings::ConnectivitySettings()
: m_fFreqResolution(1.0f)
, m_fSFreq(1000.0f)
, m_sWindowType("hanning")
{
    m_iNfft = int(m_fSFreq/m_fFreqResolution);
    qRegisterMetaType<CONNECTIVITYLIB::ConnectivitySettings>("CONNECTIVITYLIB::ConnectivitySettings");
}

//*******************************************************************************************************

void ConnectivitySettings::clearAllData() 
{
    m_trialData.clear();

    clearIntermediateData();
}

//*******************************************************************************************************

void ConnectivitySettings::clearIntermediateData() 
{
    for (int i = 0; i < m_trialData.size(); ++i) {
        m_trialData[i].matPsd.resize(0,0);
        m_trialData[i].vecPairCsd.clear();
        m_trialData[i].vecTapSpectra.clear();
        m_trialData[i].vecPairCsdNormalized.clear();
        m_trialData[i].vecPairCsdImagSign.clear();
        m_trialData[i].vecPairCsdImagAbs.clear();
        m_trialData[i].vecPairCsdImagSqrd.clear();
    }

    m_intermediateSumData.matPsdSum.resize(0,0);
    m_intermediateSumData.vecPairCsdSum.clear();
    m_intermediateSumData.vecPairCsdNormalizedSum.clear();
    m_intermediateSumData.vecPairCsdImagSignSum.clear();
    m_intermediateSumData.vecPairCsdImagAbsSum.clear();
    m_intermediateSumData.vecPairCsdImagSqrdSum.clear();
}

//*******************************************************************************************************

void ConnectivitySettings::append(const QList<MatrixXd>& matInputData)
{
    for(int i = 0; i < matInputData.size(); ++i) {
        this->append(matInputData.at(i));
    }
}

//*******************************************************************************************************

void ConnectivitySettings::append(const MatrixXd& matInputData)
{
    ConnectivitySettings::IntermediateTrialData tempData;
    tempData.matData = matInputData;

    m_trialData.append(tempData);
}

//*******************************************************************************************************

void ConnectivitySettings::append(const ConnectivitySettings::IntermediateTrialData& inputData)
{
    m_trialData.append(inputData);
}

//*******************************************************************************************************

const ConnectivitySettings::IntermediateTrialData& ConnectivitySettings::at(int i) const
{
    return m_trialData.at(i);
}

//*******************************************************************************************************

int ConnectivitySettings::size() const
{
    return m_trialData.size();
}

//*******************************************************************************************************

bool ConnectivitySettings::isEmpty() const
{
    return m_trialData.isEmpty();
}

//*******************************************************************************************************

void ConnectivitySettings::removeFirst(int iAmount)
{
//    QElapsedTimer timer;
//    qint64 iTime = 0;
//    timer.start();

    if(m_trialData.isEmpty()) {
        qDebug() << "ConnectivitySettings::removeFirst - No elements to delete. Returning.";
        return;
    }

    if(m_trialData.size() < iAmount) {
        qDebug() << "ConnectivitySettings::removeFirst - Not enough elements stored in list in order to delete them. Returning.";
        return;
    }

    // Substract influence of trials from overall summed up intermediate data and remove from data list
    for (int j = 0; j < iAmount; ++j) {
        for (int i = 0; i < m_trialData.first().matData.rows(); ++i) {
            if(i < m_intermediateSumData.vecPairCsdSum.size() && (m_intermediateSumData.vecPairCsdSum.size() == m_trialData.first().vecPairCsd.size())) {
                m_intermediateSumData.vecPairCsdSum[i].second -= m_trialData.first().vecPairCsd.at(i).second;
            }
            if(i < m_intermediateSumData.vecPairCsdNormalizedSum.size() && (m_intermediateSumData.vecPairCsdNormalizedSum.size() == m_trialData.first().vecPairCsdNormalized.size())) {
                m_intermediateSumData.vecPairCsdNormalizedSum[i].second -= m_trialData.first().vecPairCsdNormalized.at(i).second;
            }
            if(i < m_intermediateSumData.vecPairCsdImagSignSum.size() && (m_intermediateSumData.vecPairCsdImagSignSum.size() == m_trialData.first().vecPairCsdImagSign.size())) {
                m_intermediateSumData.vecPairCsdImagSignSum[i].second -= m_trialData.first().vecPairCsdImagSign.at(i).second;
            }
            if(i < m_intermediateSumData.vecPairCsdImagAbsSum.size() && (m_intermediateSumData.vecPairCsdImagAbsSum.size() == m_trialData.first().vecPairCsdImagAbs.size())) {
                m_intermediateSumData.vecPairCsdImagAbsSum[i].second -= m_trialData.first().vecPairCsdImagAbs.at(i).second;
            }
            if(i < m_intermediateSumData.vecPairCsdImagSqrdSum.size() && (m_intermediateSumData.vecPairCsdImagSqrdSum.size() == m_trialData.first().vecPairCsdImagSqrd.size())) {
                m_intermediateSumData.vecPairCsdImagSqrdSum[i].second -= m_trialData.first().vecPairCsdImagSqrd.at(i).second;
            }
        }

        if(m_intermediateSumData.matPsdSum.rows() == m_trialData.first().matPsd.rows() &&
           m_intermediateSumData.matPsdSum.cols() == m_trialData.first().matPsd.cols() ) {
            m_intermediateSumData.matPsdSum -= m_trialData.first().matPsd;
        }

        m_trialData.removeFirst();
    }

//    iTime = timer.elapsed();
//    qDebug() << "ConnectivitySettings::removeFirst" << iTime;
//    timer.restart();
}

//*******************************************************************************************************

void ConnectivitySettings::removeLast(int iAmount)
{
//    QElapsedTimer timer;
//    qint64 iTime = 0;
//    timer.start();

    if(m_trialData.isEmpty()) {
        qDebug() << "ConnectivitySettings::removeLast - No elements to delete. Returning.";
        return;
    }

    if(m_trialData.size() < iAmount) {
        qDebug() << "ConnectivitySettings::removeLast - Not enough elements stored in list in order to delete them. Returning.";
        return;
    }

    // Substract influence of trials from overall summed up intermediate data and remove from data list
    for (int j = 0; j < iAmount; ++j) {
        for (int i = 0; i < m_trialData.last().matData.rows(); ++i) {
            if(i < m_intermediateSumData.vecPairCsdSum.size() && (m_intermediateSumData.vecPairCsdSum.size() == m_trialData.last().vecPairCsd.size())) {
                m_intermediateSumData.vecPairCsdSum[i].second -= m_trialData.last().vecPairCsd.at(i).second;
            }
            if(i < m_intermediateSumData.vecPairCsdNormalizedSum.size() && (m_intermediateSumData.vecPairCsdNormalizedSum.size() == m_trialData.last().vecPairCsdNormalized.size())) {
                m_intermediateSumData.vecPairCsdNormalizedSum[i].second -= m_trialData.last().vecPairCsdNormalized.at(i).second;
            }
            if(i < m_intermediateSumData.vecPairCsdImagSignSum.size() && (m_intermediateSumData.vecPairCsdImagSignSum.size() == m_trialData.last().vecPairCsdImagSign.size())) {
                m_intermediateSumData.vecPairCsdImagSignSum[i].second -= m_trialData.last().vecPairCsdImagSign.at(i).second;
            }
            if(i < m_intermediateSumData.vecPairCsdImagAbsSum.size() && (m_intermediateSumData.vecPairCsdImagAbsSum.size() == m_trialData.last().vecPairCsdImagAbs.size())) {
                m_intermediateSumData.vecPairCsdImagAbsSum[i].second -= m_trialData.last().vecPairCsdImagAbs.at(i).second;
            }
            if(i < m_intermediateSumData.vecPairCsdImagSqrdSum.size() && (m_intermediateSumData.vecPairCsdImagSqrdSum.size() == m_trialData.last().vecPairCsdImagSqrd.size())) {
                m_intermediateSumData.vecPairCsdImagSqrdSum[i].second -= m_trialData.last().vecPairCsdImagSqrd.at(i).second;
            }
        }

        if(m_intermediateSumData.matPsdSum.rows() == m_trialData.last().matPsd.rows() &&
           m_intermediateSumData.matPsdSum.cols() == m_trialData.last().matPsd.cols() ) {
            m_intermediateSumData.matPsdSum -= m_trialData.last().matPsd;
        }

        m_trialData.removeLast();
    }

//    iTime = timer.elapsed();
//    qDebug() << "ConnectivitySettings::removeLast" << iTime;
//    timer.restart();
}

//*******************************************************************************************************

void ConnectivitySettings::setConnectivityMethods(const QStringList& sConnectivityMethods)
{
    m_sConnectivityMethods = sConnectivityMethods;
}

//*******************************************************************************************************

const QStringList& ConnectivitySettings::getConnectivityMethods() const
{
    return m_sConnectivityMethods;
}

//*******************************************************************************************************

void ConnectivitySettings::setSamplingFrequency(int iSFreq)
{
    if(m_fSFreq == iSFreq) {
        return;
    }

    clearIntermediateData();

    m_fSFreq = iSFreq;

    if(m_fFreqResolution != 0.0f) {
        m_iNfft = int(m_fSFreq/m_fFreqResolution);
    }
}

//*******************************************************************************************************

int ConnectivitySettings::getSamplingFrequency() const
{
    return m_fSFreq;
}

//*******************************************************************************************************

void ConnectivitySettings::setFFTSize(int iNfft)
{
    if(iNfft == 0) {
        return;
    }

    clearIntermediateData();

    m_iNfft = iNfft;
    m_fFreqResolution = m_fSFreq/m_iNfft;
}

//*******************************************************************************************************

int ConnectivitySettings::getFFTSize() const
{
    return m_iNfft;
}

//*******************************************************************************************************

void ConnectivitySettings::setWindowType(const QString& sWindowType)
{
    // Clear all intermediate data since this will have an effect on the frequency calculation
    clearIntermediateData();

    m_sWindowType = sWindowType;
}

//*******************************************************************************************************

const QString& ConnectivitySettings::getWindowType() const
{
    return m_sWindowType;
}

//*******************************************************************************************************

void ConnectivitySettings::setNodePositions(const FiffInfo& fiffInfo,
                                            const RowVectorXi& picks)
{
    m_matNodePositions.resize(picks.cols(),3);

    qint32 kind;
    for(int i = 0; i < picks.cols(); ++i) {
        kind = fiffInfo.chs.at(i).kind;
        if(kind == FIFFV_EEG_CH ||
           kind == FIFFV_MEG_CH) {
            m_matNodePositions(i,0) = fiffInfo.chs.at(picks(i)).chpos.r0(0);
            m_matNodePositions(i,1) = fiffInfo.chs.at(picks(i)).chpos.r0(1);
            m_matNodePositions(i,2) = fiffInfo.chs.at(picks(i)).chpos.r0(2);
        }
    }
}

//*******************************************************************************************************

void ConnectivitySettings::setNodePositions(const MNEForwardSolution& forwardSolution, const SurfaceSet& surfSet)
{
    //Generate node vertices
    MatrixX3f matNodeVertLeft, matNodeVertRight;

    if(forwardSolution.isClustered()) {
        matNodeVertLeft.resize(forwardSolution.src[0].cluster_info.centroidVertno.size(),3);
        for(int j = 0; j < matNodeVertLeft.rows(); ++j) {
            matNodeVertLeft.row(j) = surfSet[0].rr().row(forwardSolution.src[0].cluster_info.centroidVertno.at(j)) - surfSet[0].offset().transpose();
        }

        matNodeVertRight.resize(forwardSolution.src[1].cluster_info.centroidVertno.size(),3);
        for(int j = 0; j < matNodeVertRight.rows(); ++j) {
            matNodeVertRight.row(j) = surfSet[1].rr().row(forwardSolution.src[1].cluster_info.centroidVertno.at(j)) - surfSet[1].offset().transpose();
        }
    } else {
        matNodeVertLeft.resize(forwardSolution.src[0].vertno.rows(),3);
        for(int j = 0; j < matNodeVertLeft.rows(); ++j) {
            matNodeVertLeft.row(j) = surfSet[0].rr().row(forwardSolution.src[0].vertno(j)) - surfSet[0].offset().transpose();
        }

        matNodeVertRight.resize(forwardSolution.src[1].vertno.rows(),3);
        for(int j = 0; j < matNodeVertRight.rows(); ++j) {
            matNodeVertRight.row(j) = surfSet[1].rr().row(forwardSolution.src[1].vertno(j)) - surfSet[1].offset().transpose();
        }
    }

    m_matNodePositions.resize(matNodeVertLeft.rows()+matNodeVertRight.rows(),3);
    m_matNodePositions << matNodeVertLeft, matNodeVertRight;
}

//*******************************************************************************************************

void ConnectivitySettings::setNodePositions(const MatrixX3f& matNodePositions)
{
    m_matNodePositions = matNodePositions;
}

//*******************************************************************************************************

const MatrixX3f& ConnectivitySettings::getNodePositions() const
{
    return m_matNodePositions;
}

//*******************************************************************************************************

QList<ConnectivitySettings::IntermediateTrialData>& ConnectivitySettings::getTrialData()
{
    return m_trialData;
}

//*******************************************************************************************************

ConnectivitySettings::IntermediateSumData& ConnectivitySettings::getIntermediateSumData()
{
    return m_intermediateSumData;
}
