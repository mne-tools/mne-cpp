//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     connectivitysettingsmanager.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           johaenns <j.vorw01@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     September, 2018
 * @brief    Definition of the DraggableFramelessWidget Class.
 */

#ifndef CONNECTIVITYSETTINGSMANAGER_H
#define CONNECTIVITYSETTINGSMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/connectivity/compute_connectivity.h>
#include <mne/connectivity/connectivitysettings.h>
#include <mne/connectivity/network/network.h>
#include <mne/disp_widgets/plots/plot.h>
#include <mne/disp_widgets/plots/tfplot.h>
#include <mne/disp_widgets/plots/imagesc.h>
#include <mne/dsp/spectrogram.h>
#include <mne/mne/mne_epoch_data_list.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QDebug>
#include <QElapsedTimer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <map>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;
using namespace mne::disp;

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
    ConnectivitySettingsManager(QObject* parent = 0)
    : QObject(parent)
    {
        // Changing the number of trials computes only the spectra of added trials
        m_settings.set_keep_trial_data(true);
        m_settings.set_frequency_bins(0, 50);

        // Default frequency range
        m_fFreqBandLow = 7.0f;
        m_fFreqBandHigh = 13.0f;

        //set coloring
        mColor["COR"] = Vector4i(90, 26, 100, 255);
        mColor["XCOR"] = Vector4i(255, 20, 80, 255);
        mColor["PLI"] = Vector4i(255, 255, 0, 255);
        mColor["COH"] = Vector4i(2, 89, 100, 255);
        mColor["IMAGCOH"] = Vector4i(50, 255, 48, 255);
        mColor["PLV"] = Vector4i(0, 255, 255, 255);
        mColor["WPLI"] = Vector4i(255, 0, 100, 255);
        mColor["USPLI"] = Vector4i(255, 89, 200, 255);
        mColor["DSWPLI"] = Vector4i(25, 10, 255, 255);
    }

    ConnectivitySettings m_settings;
    std::vector<Network> m_rawNetworkData; /**< Results as computed. */
    std::vector<Network> m_networkData;    /**< Results averaged over the selected band and normalized. */
    std::map<std::string, Eigen::Vector4i> mColor;

    float m_fFreqBandLow;
    float m_fFreqBandHigh;

    QVector<int> m_indexList;

    QList<ConnectivitySettings::IntermediateTrialData> m_dataListOriginal;

    mne::disp::Plot* m_pSignalCoursePlot = nullptr;
    mne::disp::Plot* m_pSpectrumPlot = nullptr;
    mne::disp::Plot* m_pEvokedSignalCoursePlot = nullptr;
    mne::disp::Plot* m_pEpochSignalCoursePlot = nullptr;
    mne::disp::Plot* m_pEvokedSourceSignalCoursePlot = nullptr;
    mne::disp::ImageSc* m_pImageConnWeights = nullptr;

    std::shared_ptr<TFplot> m_pTfPlot;
    Eigen::MatrixXd m_matEvoked;
    Eigen::MatrixXd m_matEvokedSource;
    mne::MNEEpochDataList epochs;

    void onConnectivityMetricChanged(const QString& sMetric)
    {
        if (std::ranges::contains(m_settings.getConnectivityMethods(), sMetric.toStdString())) {
            return;
        }

        m_settings.setConnectivityMethods({sMetric.toStdString()});
        compute();
    }

    void onNumberTrialsChanged(int iNumberTrials)
    {
        //        QElapsedTimer timer;
        //        qint64 iTime = 0;
        //        timer.start();

        //The maximum number of trials will always be the number of orginal trials stored
        if (iNumberTrials > m_dataListOriginal.size()) {
            iNumberTrials = m_dataListOriginal.size();
        }

        //Pop data from connectivity settings
        int size = m_settings.size();

        if (size > iNumberTrials) {
            m_settings.removeLast(size - iNumberTrials);
        } else {
            while (m_settings.size() < iNumberTrials) {
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

        compute();

        //        iTime = timer.elapsed();
        //        qDebug() << "Coherency::computeCoherencyImag timer - Preparation:" << iTime;
        //        timer.restart();
    }

    void onFreqBandChanged(float fFreqLow, float fFreqHigh)
    {
        if (m_settings.empty()) {
            return;
        }

        // Convert to frequency bins
        m_fFreqBandLow = fFreqLow;
        m_fFreqBandHigh = fFreqHigh;

        updateNetworks();
    }

    void compute()
    {
        m_rawNetworkData = compute_connectivity(m_settings);
        updateNetworks();
    }

    void updateNetworks()
    {
        m_networkData = m_rawNetworkData;

        for (int i = 0; i < std::ssize(m_networkData); ++i) {
            m_networkData[i].set_frequency_range(m_fFreqBandLow, m_fFreqBandHigh);
            m_networkData[i].normalize();

            VisualizationInfo visInfo = m_networkData[i].visualization_info();
            visInfo.coloring = VisualizationInfo::Coloring::fixed;
            visInfo.node_color = mColor[m_networkData[i].method()];
            visInfo.edge_color = visInfo.node_color;
            m_networkData[i].set_visualization_info(std::move(visInfo));
        }

        for (const Network& network : m_networkData) {
            emit newConnectivityResultAvailable("sample", "1", network);
        }
    }

    void plotTimeCourses(int iTrialNumber, int iRowNumber)
    {
        if (iTrialNumber < m_settings.size()) {
            if (iRowNumber < m_settings.at(iTrialNumber).matData.rows()) {
                Eigen::RowVectorXd plotVeca = m_settings.at(iTrialNumber).matData.row(iRowNumber).array() - m_settings.at(iTrialNumber).matData.row(iRowNumber).mean();
                Eigen::Map<Eigen::VectorXd> v1a(plotVeca.data(), plotVeca.size());
                Eigen::VectorXd tempa = v1a.array() - v1a.mean();
                if (!m_pSignalCoursePlot) {
                    m_pSignalCoursePlot = new mne::disp::Plot(tempa);
                } else {
                    m_pSignalCoursePlot->updateData(tempa);
                }

                m_pSignalCoursePlot->setTitle(QString("Conn used signal for trial %1 and source %2").arg(QString::number(iTrialNumber)).arg(QString::number(iRowNumber)));
                m_pSignalCoursePlot->show();

                //            Eigen::MatrixXd dataSpectrum = Spectrogram::makeSpectrogram(plotVeca, m_settings.getSamplingFrequency()*0.05);

                //            m_pTfPlot = std::make_shared<TFplot>(dataSpectrum, m_settings.getSamplingFrequency(), 2,50, ColorMaps::Jet);
                //            m_pTfPlot->show();
            }

            if (iRowNumber < epochs.at(iTrialNumber)->epoch.rows()) {
                Eigen::RowVectorXd plotVecc = epochs.at(iTrialNumber)->epoch.row(iRowNumber);
                Eigen::Map<Eigen::VectorXd> v1c(plotVecc.data(), plotVecc.size());
                Eigen::VectorXd tempc = v1c;
                if (!m_pEpochSignalCoursePlot) {
                    m_pEpochSignalCoursePlot = new mne::disp::Plot(tempc);
                } else {
                    m_pEpochSignalCoursePlot->updateData(tempc);
                }

                m_pEpochSignalCoursePlot->setTitle(QString("Sensor signal for trial %1 and row %2").arg(QString::number(iTrialNumber)).arg(QString::number(iRowNumber)));
                m_pEpochSignalCoursePlot->show();
            }

            if (iRowNumber < std::ssize(m_settings.at(iTrialNumber).vecTapSpectra)) {
                Eigen::RowVectorXd plotVec = m_settings.at(iTrialNumber).vecTapSpectra.at(iRowNumber).cwiseAbs().row(0);
                Eigen::Map<Eigen::VectorXd> v1(plotVec.data(), plotVec.size());
                Eigen::VectorXd temp = v1;
                if (!m_pSpectrumPlot) {
                    m_pSpectrumPlot = new mne::disp::Plot(temp);
                } else {
                    m_pSpectrumPlot->updateData(temp);
                }

                m_pSpectrumPlot->setTitle(QString("Spectrum of conn used signal for trial %1 and row %2").arg(QString::number(iTrialNumber)).arg(QString::number(iRowNumber)));
                m_pSpectrumPlot->show();
            }
        }

        if (iRowNumber < m_matEvoked.rows()) {
            Eigen::RowVectorXd plotVeca = m_matEvoked.row(iRowNumber);
            Eigen::Map<Eigen::VectorXd> v1a(plotVeca.data(), plotVeca.size());
            Eigen::VectorXd tempa = v1a;
            if (!m_pEvokedSignalCoursePlot) {
                m_pEvokedSignalCoursePlot = new mne::disp::Plot(tempa);
            } else {
                m_pEvokedSignalCoursePlot->updateData(tempa);
            }

            m_pEvokedSignalCoursePlot->setTitle(QString("Evoked signal for row %1").arg(QString::number(iRowNumber)));
            m_pEvokedSignalCoursePlot->show();
        }

        if (iRowNumber < m_matEvokedSource.rows()) {
            Eigen::RowVectorXd plotVeca = m_matEvokedSource.row(iRowNumber);
            Eigen::Map<Eigen::VectorXd> v1a(plotVeca.data(), plotVeca.size());
            Eigen::VectorXd tempa = v1a;
            if (!m_pEvokedSourceSignalCoursePlot) {
                m_pEvokedSourceSignalCoursePlot = new mne::disp::Plot(tempa);
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
