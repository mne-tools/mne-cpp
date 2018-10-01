//=============================================================================================================
/**
* @file     connectivitysettingsmanager.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the DraggableFramelessWidget Class.
*
*/

#ifndef CONNECTIVITYSETTINGSMANAGER_H
#define CONNECTIVITYSETTINGSMANAGER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <realtime/rtProcessing/rtconnectivity.h>

#include <connectivity/connectivitysettings.h>
#include <connectivity/network/network.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
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
using namespace REALTIMELIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS ConnectivitySettingsManager
*
* @brief The ConnectivitySettingsManager class provides a manager to handle connectivty computation for the ex_connectivity example.
*/
class ConnectivitySettingsManager : public QObject
{
    Q_OBJECT

public:

    ConnectivitySettingsManager(float sFreq = 1000.0f, QObject *parent = 0)
    : QObject(parent)
    , m_pRtConnectivity(RtConnectivity::SPtr::create())
    , m_iFreqBandLow(1)
    , m_iFreqBandHigh(50)
    , m_fSFreq(sFreq)
    {
        QObject::connect(m_pRtConnectivity.data(), &RtConnectivity::newConnectivityResultAvailable,
                         this, &ConnectivitySettingsManager::onNewConnectivityResultAvailable);
    }

    ConnectivitySettings    m_settings;
    RtConnectivity::SPtr    m_pRtConnectivity;
    QList<Eigen::MatrixXd>  m_matDataListOriginal;
    Network                 m_networkData;

    int                     m_iFreqBandLow;
    int                     m_iFreqBandHigh;

    float                   m_fSFreq;

    void onConnectivityMetricChanged(const QString& sMetric)
    {
        m_pRtConnectivity->restart();

        m_settings.m_sConnectivityMethods = QStringList() << sMetric;

        m_pRtConnectivity->append(m_settings);
    }

    void onNumberTrialsChanged(int iNumberTrials)
    {
        if(iNumberTrials > m_matDataListOriginal.size()) {
            iNumberTrials = m_matDataListOriginal.size();
        }

        m_settings.m_matDataList.clear();

        for(int i = 0; i < iNumberTrials; i++) {
            m_settings.m_matDataList << m_matDataListOriginal.at(i);
        }

        m_pRtConnectivity->append(m_settings);
    }

    void onFreqBandChanged(int iFreqLow, int iFreqHigh)
    {
        if(m_settings.m_matDataList.isEmpty()) {
            return;
        }

        // By default the number of frequency bins is half the signal since we only use the half spectrum
        double dScaleFactor = m_settings.m_matDataList.first().cols()/m_fSFreq;

        // Convert to frequency bins
        m_iFreqBandLow = iFreqLow * dScaleFactor;
        m_iFreqBandHigh = iFreqHigh * dScaleFactor;

        onNewConnectivityResultAvailable(m_networkData);
    }

    void onNewConnectivityResultAvailable(const Network& tNetworkData)
    {
        m_networkData = tNetworkData;
        m_networkData.setFrequencyBins(m_iFreqBandLow, m_iFreqBandHigh);

        emit newConnectivityResultAvailable(m_networkData);
    }

signals:
    void newConnectivityResultAvailable(const Network& tNetworkData);

};

#endif // CONNECTIVITYSETTINGSMANAGER_H
