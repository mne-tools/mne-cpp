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

std::vector<Eigen::MatrixXd> TimeFrequencyData::computeTimeFrequency(const FIFFLIB::FiffEvokedSet& evokedSet)
{
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

std::vector<Eigen::MatrixXcd> TimeFrequencyData::computeComplexTimeFrequency(const FIFFLIB::FiffEvokedSet& evokedSet)
{
    qDebug() << "[RTPROCESSINGLIB::computeTimeFreqency]";

    auto& evoked = evokedSet.evoked.first();
    float fSampFreq = evoked.info.sfreq;

    std::vector<Eigen::MatrixXcd> tfvector;

    for (int i = 0; i < evoked.data.rows(); i++){
        Eigen::VectorXd dataCol = evoked.data.row(i).transpose();
        Eigen::MatrixXcd Spectrum = UTILSLIB::Spectrogram::makeSpectrogram(dataCol, fSampFreq * 0.2);
        tfvector.push_back(Spectrum);
    }

    return tfvector;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TimeFrequencyData::TimeFrequencyData()
{

}

