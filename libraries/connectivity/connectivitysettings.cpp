//=============================================================================================================
/**
* @file     connectivitysettings.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivitysettings.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
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

void ConnectivitySettings::clearAllData() {
    m_trialData.clear();

    clearIntermediateData();
}


//*******************************************************************************************************

void ConnectivitySettings::clearIntermediateData() {
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

void ConnectivitySettings::append(const QList<Eigen::MatrixXd>& matInputData)
{
    for(int i = 0; i < matInputData.size(); ++i) {
        this->append(matInputData.at(i));
    }
}


//*******************************************************************************************************

void ConnectivitySettings::append(const Eigen::MatrixXd& matInputData)
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

void ConnectivitySettings::removeFirst()
{
    QElapsedTimer timer;
    qint64 iTime = 0;
    timer.start();
    if(!m_trialData.isEmpty()) {
        // Substract the influence by the first item on all intermediate sum data
        // Substract PSD of first trial from overall summed up PSD
        if(m_intermediateSumData.matPsdSum.rows() == m_trialData.first().matPsd.rows() &&
           m_intermediateSumData.matPsdSum.cols() == m_trialData.first().matPsd.cols() ) {
            m_intermediateSumData.matPsdSum -= m_trialData.first().matPsd;
        }

        // Substract CSD of first trial from overall summed up CSD
        if(m_intermediateSumData.vecPairCsdSum.size() == m_trialData.first().vecPairCsd.size() &&
           m_intermediateSumData.vecPairCsdNormalizedSum.size() == m_trialData.first().vecPairCsdNormalized.size() &&
           m_intermediateSumData.vecPairCsdImagSignSum.size() == m_trialData.first().vecPairCsdImagSign.size() &&
           m_intermediateSumData.vecPairCsdImagAbsSum.size() == m_trialData.first().vecPairCsdImagAbs.size() &&
           m_intermediateSumData.vecPairCsdImagSqrdSum.size() == m_trialData.first().vecPairCsdImagSqrd.size()) {
            for (int i = 0; i < m_intermediateSumData.vecPairCsdSum.size(); ++i) {
                if(i < m_intermediateSumData.vecPairCsdSum.size()) {
                    m_intermediateSumData.vecPairCsdSum[i].second -= m_trialData.first().vecPairCsd.at(i).second;
                }
                if(i < m_intermediateSumData.vecPairCsdNormalizedSum.size()) {
                    m_intermediateSumData.vecPairCsdNormalizedSum[i].second -= m_trialData.first().vecPairCsdNormalized.at(i).second;
                }
                if(i < m_intermediateSumData.vecPairCsdImagSignSum.size()) {
                    m_intermediateSumData.vecPairCsdImagSignSum[i].second -= m_trialData.first().vecPairCsdImagSign.at(i).second;
                }
                if(i < m_intermediateSumData.vecPairCsdImagAbsSum.size()) {
                    m_intermediateSumData.vecPairCsdImagAbsSum[i].second -= m_trialData.first().vecPairCsdImagAbs.at(i).second;
                }
                if(i < m_intermediateSumData.vecPairCsdImagSqrdSum.size()) {
                    m_intermediateSumData.vecPairCsdImagSqrdSum[i].second -= m_trialData.first().vecPairCsdImagSqrd.at(i).second;
                }
            }
        }

        m_trialData.removeFirst();
    }
    iTime = timer.elapsed();
    qDebug() << "ConnectivitySettings::removeFirst" << iTime;
    timer.restart();
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

    if(m_fFreqResolution != 0) {
        m_iNfft = int(m_fSFreq/m_fFreqResolution);
    }
}


//*******************************************************************************************************

int ConnectivitySettings::getSamplingFrequency() const
{
    return m_fSFreq;
}


//*******************************************************************************************************

void ConnectivitySettings::setNumberFFT(int iNfft)
{
    if(m_fSFreq == 0) {
        return;
    }

    clearIntermediateData();

    m_iNfft = iNfft;
}


//*******************************************************************************************************

int ConnectivitySettings::getNumberFFT() const
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

void ConnectivitySettings::setNodePositions(const Eigen::MatrixX3f& matNodePositions)
{
    m_matNodePositions = matNodePositions;
}


//*******************************************************************************************************

const Eigen::MatrixX3f& ConnectivitySettings::getNodePositions() const
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
