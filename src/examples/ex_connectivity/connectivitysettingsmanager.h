//=============================================================================================================
/**
 * @file     connectivitysettingsmanager.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <rtprocessing/rtconnectivity.h>

#include <connectivity/metrics/abstractmetric.h>
#include <connectivity/connectivitysettings.h>
#include <connectivity/network/network.h>
#include <connectivity/network/networknode.h>
#include <connectivity/network/networkedge.h>

#include <disp/plots/plot.h>
#include <disp/plots/tfplot.h>
#include <disp/plots/imagesc.h>
#include <utils/spectrogram.h>
#include <mne/mne_epoch_data_list.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QDebug>
#include <QElapsedTimer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;
using namespace RTPROCESSINGLIB;
using namespace Eigen;
using namespace DISPLIB;
using namespace UTILSLIB;

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

    ConnectivitySettingsManager(QObject *parent = 0)
    : QObject(parent)
    , m_pRtConnectivity(RtConnectivity::SPtr::create())
    {
        QObject::connect(m_pRtConnectivity.data(), &RtConnectivity::newConnectivityResultAvailable,
                         this, &ConnectivitySettingsManager::onNewConnectivityResultAvailable);

        // Default frequency range
        m_fFreqBandLow = 7.0f;
        m_fFreqBandHigh = 13.0f;
    }

    ConnectivitySettings    m_settings;
    RtConnectivity::SPtr    m_pRtConnectivity;
    QList<Network>          m_networkData;

    float                   m_fFreqBandLow;
    float                   m_fFreqBandHigh;

    QVector<int>            m_indexList;

    QList<ConnectivitySettings::IntermediateTrialData>    m_dataListOriginal;

    DISPLIB::Plot *m_pSignalCoursePlot = Q_NULLPTR;
    DISPLIB::Plot *m_pSpectrumPlot = Q_NULLPTR;
    DISPLIB::Plot *m_pEvokedSignalCoursePlot = Q_NULLPTR;
    DISPLIB::Plot *m_pEpochSignalCoursePlot = Q_NULLPTR;
    DISPLIB::Plot *m_pEvokedSourceSignalCoursePlot = Q_NULLPTR;
    DISPLIB::ImageSc *m_pImageConnWeights = Q_NULLPTR;

    TFplot::SPtr m_pTfPlot;
    Eigen::MatrixXd m_matEvoked;
    Eigen::MatrixXd m_matEvokedSource;
    MNELIB::MNEEpochDataList epochs;

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
//        QElapsedTimer timer;
//        qint64 iTime = 0;
//        timer.start();

        //The maximum number of trials will always be the number of orginal trials stored
        if(iNumberTrials > m_dataListOriginal.size()) {
            iNumberTrials = m_dataListOriginal.size();
        }

        //Pop data from connectivity settings
        int size = m_settings.size();

        if(size > iNumberTrials) {
            m_settings.removeLast(size-iNumberTrials);
        } else {
            while(m_settings.size() < iNumberTrials) {
    //            bool finish = false;
    //            int index = 0;

    //            while(!finish) {
    //                index = rand() % iNumberTrials;

    //                if(!m_indexList.contains(index)) {
    //                    m_indexList.append(index);
    //                    finish = true;
    //                }
    //            }

                m_settings.append(m_dataListOriginal.at(m_settings.size()));
            }
        }

        //qDebug() << "ConnectivitySettingsManager::onNumberTrialsChanged - m_indexList" << m_indexList;

        m_pRtConnectivity->append(m_settings);

//        iTime = timer.elapsed();
//        qDebug() << "Coherency::computeCoherencyImag timer - Preparation:" << iTime;
//        timer.restart();
    }

    void onFreqBandChanged(float fFreqLow, float fFreqHigh)
    {
        if(m_settings.isEmpty()) {
            return;
        }

        // Convert to frequency bins
        m_fFreqBandLow = fFreqLow;
        m_fFreqBandHigh = fFreqHigh;

        onNewConnectivityResultAvailable(m_networkData, m_settings);
    }

    void onNewConnectivityResultAvailable(const QList<Network>& connectivityResults,
                                          const ConnectivitySettings& connectivitySettings)
    {
        m_settings = connectivitySettings;
        m_networkData = connectivityResults;

        for(int i = 0; i < connectivityResults.size(); ++i) {
            m_networkData[i].setFrequencyRange(m_fFreqBandLow, m_fFreqBandHigh);
            //m_networkData[i].normalize();

//            if(!m_networkData.isEmpty()) {
//                Network network = m_networkData.first();
//                Eigen::MatrixXd image;

//                for(int i = 0; i < network.getNodes().size(); i++) {
//                    for(int j = 0; j < network.getNodes().at(i)->getFullEdges().size(); j++) {
//                        NetworkEdge::SPtr edge = network.getNodes().at(i)->getFullEdges().at(j);

//                        if(edge->isActive()) {
//                            if(image.cols() == 0) {
//                                image = edge->getMatrixWeight();
//                            } else {
//                                image.conservativeResize(image.rows(),image.cols()+1);
//                                image.col(image.cols()-1) = edge->getMatrixWeight();
//                            }
//                        }
//                    }
//                }

//                image.conservativeResize(image.rows()-1,image.cols());

//                if(!m_pImageConnWeights) {
//                    m_pImageConnWeights = new DISPLIB::ImageSc(image);
//                } else {
//                    m_pImageConnWeights->updateData(image);
//                }
//                m_pImageConnWeights->show();
//            }
        }

        if(!m_networkData.isEmpty()) {
            for(int i = 0; i < m_networkData.size(); ++i) {
                emit newConnectivityResultAvailable("sample",
                                                    "1",
                                                    m_networkData.at(i));
            }
        }
    }

    void plotTimeCourses(int iTrialNumber, int iRowNumber)
    {
        if(iTrialNumber < m_settings.size()) {
            if(iRowNumber < m_settings.at(iTrialNumber).matData.rows()) {
                Eigen::RowVectorXd plotVeca = m_settings.at(iTrialNumber).matData.row(iRowNumber).array() - m_settings.at(iTrialNumber).matData.row(iRowNumber).mean();
                Eigen::Map<Eigen::VectorXd> v1a(plotVeca.data(), plotVeca.size());
                Eigen::VectorXd tempa = v1a.array() - v1a.mean();
                if(!m_pSignalCoursePlot) {
                    m_pSignalCoursePlot = new DISPLIB::Plot(tempa);
                } else {
                    m_pSignalCoursePlot->updateData(tempa);
                }

                m_pSignalCoursePlot->setTitle(QString("Conn used signal for trial %1 and source %2").arg(QString::number(iTrialNumber)).arg(QString::number(iRowNumber)));
                m_pSignalCoursePlot->show();

    //            Eigen::MatrixXd dataSpectrum = Spectrogram::makeSpectrogram(plotVeca, m_settings.getSamplingFrequency()*0.05);

    //            m_pTfPlot = TFplot::SPtr::create(dataSpectrum, m_settings.getSamplingFrequency(), 2,50, ColorMaps::Jet);
    //            m_pTfPlot->show();
            }

            if(iRowNumber < epochs.at(iTrialNumber)->epoch.rows()) {
                Eigen::RowVectorXd plotVecc = epochs.at(iTrialNumber)->epoch.row(iRowNumber);
                Eigen::Map<Eigen::VectorXd> v1c(plotVecc.data(), plotVecc.size());
                Eigen::VectorXd tempc =v1c;
                if(!m_pEpochSignalCoursePlot) {
                    m_pEpochSignalCoursePlot = new DISPLIB::Plot(tempc);
                } else {
                    m_pEpochSignalCoursePlot->updateData(tempc);
                }

                m_pEpochSignalCoursePlot->setTitle(QString("Sensor signal for trial %1 and row %2").arg(QString::number(iTrialNumber)).arg(QString::number(iRowNumber)));
                m_pEpochSignalCoursePlot->show();
            }

            if(iRowNumber < m_settings.at(iTrialNumber).vecTapSpectra.size()) {
                Eigen::RowVectorXd plotVec = m_settings.at(iTrialNumber).vecTapSpectra.at(iRowNumber).cwiseAbs().row(0);
                Eigen::Map<Eigen::VectorXd> v1(plotVec.data(), plotVec.size());
                Eigen::VectorXd temp =v1;
                if(!m_pSpectrumPlot) {
                    m_pSpectrumPlot = new DISPLIB::Plot(temp);
                } else {
                    m_pSpectrumPlot->updateData(temp);
                }

                m_pSpectrumPlot->setTitle(QString("Spectrum of conn used signal for trial %1 and row %2").arg(QString::number(iTrialNumber)).arg(QString::number(iRowNumber)));
                m_pSpectrumPlot->show();
            }
        }

        if(iRowNumber < m_matEvoked.rows()) {
            Eigen::RowVectorXd plotVeca = m_matEvoked.row(iRowNumber);
            Eigen::Map<Eigen::VectorXd> v1a(plotVeca.data(), plotVeca.size());
            Eigen::VectorXd tempa =v1a;
            if(!m_pEvokedSignalCoursePlot) {
                m_pEvokedSignalCoursePlot = new DISPLIB::Plot(tempa);
            } else {
                m_pEvokedSignalCoursePlot->updateData(tempa);
            }

            m_pEvokedSignalCoursePlot->setTitle(QString("Evoked signal for row %1").arg(QString::number(iRowNumber)));
            m_pEvokedSignalCoursePlot->show();
        }

        if(iRowNumber < m_matEvokedSource.rows()) {
            Eigen::RowVectorXd plotVeca = m_matEvokedSource.row(iRowNumber);
            Eigen::Map<Eigen::VectorXd> v1a(plotVeca.data(), plotVeca.size());
            Eigen::VectorXd tempa =v1a;
            if(!m_pEvokedSourceSignalCoursePlot) {
                m_pEvokedSourceSignalCoursePlot = new DISPLIB::Plot(tempa);
            } else {
                m_pEvokedSourceSignalCoursePlot->updateData(tempa);
            }

            m_pEvokedSourceSignalCoursePlot->setTitle(QString("Evoked source signal for row %1").arg(QString::number(iRowNumber)));
            m_pEvokedSourceSignalCoursePlot->show();
        }
    }

signals:
    void newConnectivityResultAvailable(const QString& sSubject,
                                        const QString& sMeasurement,
                                        const Network& tNetworkData);
};

#endif // CONNECTIVITYSETTINGSMANAGER_H
