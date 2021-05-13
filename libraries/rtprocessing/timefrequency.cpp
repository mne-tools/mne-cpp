//=============================================================================================================
/**
 * @file     timefrequency.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of TimeFrequency functions
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "timefrequency.h"

#include <iostream>

#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_evoked.h>

#include <utils/spectrogram.h>

#include <mne/mne_epoch_data_list.h>
#include <utils/tracer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE STATIC RTPROCESSINGLIB METHODS
//=============================================================================================================

std::vector<Eigen::MatrixXd> TimeFrequencyData::computeEpochListTimeFrequency(const FIFFLIB::FiffEvokedSet& evokedSet)
{
    __TRACE_FUNC();
    qDebug() << "[RTPROCESSINGLIB::computeTimeFreqency]";

    int a = 0;
    qDebug() << a;

    std::cout << "SIZE:" << evokedSet.evoked.size();

    auto& evoked = evokedSet.evoked.first();
    float fSampFreq = evoked.info.sfreq;

    std::cout << "ROWS: " << evoked.data.rows();

    std::vector<Eigen::MatrixXd> tfvector;

    for (int i = 0; i < evoked.data.rows(); i++){
        Eigen::VectorXd dataCol = evoked.data.row(i).transpose();
        //std::cout << "First data sample from evoked: " << dataCol(0);
        Eigen::MatrixXd Spectrum = UTILSLIB::Spectrogram::makeSpectrogram(dataCol, fSampFreq * 0.2);
        tfvector.push_back(Spectrum);
    }

    return tfvector;
}

//=============================================================================================================

std::vector<Eigen::MatrixXcd> TimeFrequencyData::computeComplexTimeFrequency(const FIFFLIB::FiffEvokedSet& evokedSet)
{
    __TRACE_FUNC();
    qDebug() << "[RTPROCESSINGLIB::computeTimeFreqency]";

    auto& evoked = evokedSet.evoked.first();
    float fSampFreq = evoked.info.sfreq;

    std::vector<Eigen::MatrixXcd> tfvector;

    for (int i = 0; i < evoked.data.rows(); i++){
        Eigen::VectorXd dataCol = evoked.data.row(i).transpose();
        Eigen::MatrixXcd Spectrum = UTILSLIB::Spectrogram::makeComplexSpectrogram(dataCol, fSampFreq * 0.2);
        tfvector.push_back(Spectrum);
    }

    return tfvector;
}

//=============================================================================================================

std::vector<std::vector<Eigen::MatrixXcd>> TimeFrequencyData::computeEpochListTimeFrequency(const FIFFLIB::FiffRawData &raw,
                                                                               const Eigen::MatrixXi &matEvents,
                                                                               float fTMinS,
                                                                               float fTMaxS)
{
    __TRACE_FUNC();
    QMap<QString,double> mapReject;
    mapReject.insert("eog", 300e-06);
    int iType = 1;

    std::vector<std::vector<Eigen::MatrixXcd> > epochTimeFrequencyList; //list of epochs x channels x tf

    MNELIB::MNEEpochDataList epochDataList = MNELIB::MNEEpochDataList::readEpochs(raw,
                                                                                     matEvents,
                                                                                     fTMinS,
                                                                                     fTMaxS,
                                                                                     iType,
                                                                                     mapReject);

    for(QSharedPointer<MNELIB::MNEEpochData>& epoch : epochDataList){
        epochTimeFrequencyList.emplace_back(computeEpochTimeFrequency(epoch,
                                                    raw.info.sfreq));
    }

    return epochTimeFrequencyList;
}

//=============================================================================================================


std::vector<Eigen::MatrixXcd> TimeFrequencyData::computeEpochTimeFrequency(const QSharedPointer<MNELIB::MNEEpochData>& epoch,
                                                              float sampleFrequency)
{
    __TRACE_FUNC();
    int numChannels(epoch->epoch.rows());

    std::vector<Eigen::MatrixXcd> channelTimeFrequencyList;

    for(int channeli = 0; channeli < numChannels; channeli++){
        channelTimeFrequencyList.push_back(Eigen::MatrixXcd::Zero(10,10));
//        channelTimeFrequencyList.emplace_back(UTILSLIB::Spectrogram::makeComplexSpectrogram(epoch->epoch.row(channeli).transpose(), 200));
//        channelTimeFrequencyList.emplace_back(UTILSLIB::Spectrogram::makeSpectrogram(epoch->epoch.row(channeli).transpose(),0));
    }

    return channelTimeFrequencyList;
}



//=============================================================================================================

Eigen::MatrixXcd TimeFrequencyData::averageEpochTimeFrequency(const std::vector<Eigen::MatrixXcd>& epochTimeFrequency)
{
    __TRACE_FUNC();
    if (epochTimeFrequency.empty()){
        return Eigen::MatrixXcd();
    }

    Eigen::MatrixXcd averageTimeFrequency(epochTimeFrequency.front().rows(), epochTimeFrequency.front().cols());

    for(auto channelTimeFreq : epochTimeFrequency){
        averageTimeFrequency += channelTimeFreq;
    }

    return averageTimeFrequency / epochTimeFrequency.size();
}

//=============================================================================================================

std::vector<Eigen::MatrixXcd> TimeFrequencyData::averageEpochListTimeFrequency(const std::vector<std::vector<Eigen::MatrixXcd> >& epochListTimeFrequency)
{
    __TRACE_FUNC();
    int numFreqs(epochListTimeFrequency.front().front().rows());
    int numSamples(epochListTimeFrequency.front().front().cols());

    int numEpochs(epochListTimeFrequency.size());

    if (numSamples ==0 || numFreqs == 0 || numEpochs == 0){
        return std::vector<Eigen::MatrixXcd>();
    }

    int numChannelsInEpoch(epochListTimeFrequency.front().size());

    for (int epochindex(1) ; epochindex < epochListTimeFrequency.size(); epochindex++){
        if (static_cast<int>(epochListTimeFrequency[epochindex].size()) != numChannelsInEpoch){
            qDebug() << "Channel number does not match across epoch" << epochindex;
            return std::vector<Eigen::MatrixXcd>();
        }
    }

    std::vector<Eigen::MatrixXcd> averagedEpochListTimeFrequency;
    Eigen::MatrixXcd auxMatrix;

    for (int iChannel = 0; iChannel < numChannelsInEpoch; iChannel++){
        auxMatrix = Eigen::MatrixXcd::Zero(numFreqs, numSamples);
        for (auto epoch : epochListTimeFrequency){
            auxMatrix += epoch[iChannel];
        }
        auxMatrix /= numEpochs;
        averagedEpochListTimeFrequency.push_back(auxMatrix);
    }

    return averagedEpochListTimeFrequency;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TimeFrequencyData::TimeFrequencyData()
{

}

//=============================================================================================================

TimeFrequencyData::TimeFrequencyData(Eigen::MatrixXcd mat)
{
    this->m_TFData = mat;
}
