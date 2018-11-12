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

#include <rtprocessing/rtconnectivity.h>

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
using namespace RTPROCESSINGLIB;


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

    ConnectivitySettingsManager(int iBlockSize, float sFreq = 1000.0f, QObject *parent = 0)
    : QObject(parent)
    , m_pRtConnectivity(RtConnectivity::SPtr::create())
    , m_fSFreq(sFreq)
    {
        QObject::connect(m_pRtConnectivity.data(), &RtConnectivity::newConnectivityResultAvailable,
                         this, &ConnectivitySettingsManager::onNewConnectivityResultAvailable);

        // By default the number of frequency bins is half the signal since we only use the half spectrum
        double dScaleFactor = iBlockSize/m_fSFreq;

        // Convert to frequency bins
        m_iFreqBandLow = 1 * dScaleFactor;
        m_iFreqBandHigh = 50 * dScaleFactor;
    }

    ConnectivitySettings    m_settings;
    RtConnectivity::SPtr    m_pRtConnectivity;
    Network                 m_networkData;

    int                     m_iFreqBandLow;
    int                     m_iFreqBandHigh;

    float                   m_fSFreq;

    QVector<int>            m_indexList;

    QList<ConnectivitySettings::IntermediateTrialData>    m_dataListOriginal;

    void onConnectivityMetricChanged(const QString& sMetric)
    {
        if(m_settings.getConnectivityMethods().contains(sMetric)) {
            return;
        }

        m_pRtConnectivity->restart();

        m_settings.setConnectivityMethods(QStringList() << sMetric);

        m_pRtConnectivity->append(m_settings);
    }

    void onNumberTrialsChanged(int iNumberTrials)
    {
        if(iNumberTrials > m_dataListOriginal.size()) {
            iNumberTrials = m_dataListOriginal.size();
        }

        if(iNumberTrials == m_settings.size()) {
            return;
        }

        //Pop data from connectivity settings
        int size = m_settings.size();

        if(size > iNumberTrials) {
            m_pRtConnectivity->restart();

            for(int i = 0; i < size-iNumberTrials; ++i) {
                m_settings.removeFirst();

                if(!m_indexList.isEmpty()) {
                    m_indexList.pop_front();
                }
            }
        }

        while(m_settings.size() < iNumberTrials) {
            bool finish = false;
            int index = 0;

            while(!finish) {
                index = rand() % iNumberTrials;

                if(!m_indexList.contains(index)) {
                    m_indexList.append(index);
                    finish = true;
                }
            }

            m_settings.append(m_dataListOriginal.at(index));
        }

        //qDebug() << "ConnectivitySettingsManager::onNumberTrialsChanged - m_indexList" << m_indexList;

        m_pRtConnectivity->append(m_settings);
    }

    void onFreqBandChanged(int iFreqLow, int iFreqHigh)
    {
        if(m_settings.isEmpty()) {
            return;
        }

        // By default the number of frequency bins is half the signal since we only use the half spectrum
        double dScaleFactor = m_settings.at(0).matData.cols()/m_fSFreq;

        // Convert to frequency bins
        m_iFreqBandLow = iFreqLow * dScaleFactor;
        m_iFreqBandHigh = iFreqHigh * dScaleFactor;

        onNewConnectivityResultAvailable(m_networkData, m_settings);
    }

    void onNewConnectivityResultAvailable(const Network& tNetworkData,
                                          const ConnectivitySettings& connectivitySettings)
    {
        m_settings = connectivitySettings;
        m_networkData = tNetworkData;
        m_networkData.setFrequencyBins(m_iFreqBandLow, m_iFreqBandHigh);
        m_networkData.normalize();

        if(!m_networkData.isEmpty()) {
            emit newConnectivityResultAvailable(m_networkData);
        }
    }

signals:
    void newConnectivityResultAvailable(const Network& tNetworkData);

};

#endif // CONNECTIVITYSETTINGSMANAGER_H
