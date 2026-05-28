//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file rt_cov.cpp
 * @since March 2026
 * @brief Definition of the RtCov Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_cov.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QtConcurrent>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtCov::RtCov(QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo)
: m_fiffInfo(*pFiffInfo)
, m_iSamples(0)
, m_bPicksReady(false)
{
}

//=============================================================================================================

FiffCov RtCov::estimateCovariance(const Eigen::MatrixXd& matData,
                                  int iNewMaxSamples)
{
    if(m_fiffInfo.chs.isEmpty()) {
        qWarning() << "[RtCov::estimateCovariance] FiffInfo was not set. Returning empty covariance estimation.";
        return FiffCov();
    }

    // Compute picks once: select only MEG and EEG channels
    if(!m_bPicksReady) {
        m_picks.clear();
        for(int i = 0; i < m_fiffInfo.chs.size(); i++) {
            if(m_fiffInfo.chs.at(i).kind == FIFFV_MEG_CH ||
               m_fiffInfo.chs.at(i).kind == FIFFV_EEG_CH) {
                m_picks.append(i);
            }
        }
        m_bPicksReady = true;
    }

    // Apply picks: extract only MEG/EEG channel rows
    if(!m_picks.isEmpty() && m_picks.size() < matData.rows()) {
        MatrixXd matPicked(m_picks.size(), matData.cols());
        for(int i = 0; i < m_picks.size(); i++) {
            matPicked.row(i) = matData.row(m_picks[i]);
        }
        m_lData.append(matPicked);
    } else {
        m_lData.append(matData);
    }
    m_iSamples += matData.cols();

    if(m_iSamples < iNewMaxSamples) {
        return FiffCov();
    }

    QFuture<RtCovComputeResult> result = QtConcurrent::mappedReduced(m_lData,
                                                                     compute,
                                                                     reduce);

    result.waitForFinished();

    RtCovComputeResult finalResult = result.result();

    //Final computation
    FiffCov computedCov;
    computedCov.data = finalResult.matData;

    bool doProj = true;

    if(m_iSamples > 0) {
        finalResult.mu /= (float)m_iSamples;
        computedCov.data.array() -= m_iSamples * (finalResult.mu * finalResult.mu.transpose()).array();
        computedCov.data.array() /= (m_iSamples - 1);

        computedCov.kind = FIFFV_MNE_NOISE_COV;
        computedCov.diag = false;
        computedCov.dim = computedCov.data.rows();

        // Set names to picked channels only
        QStringList pickedNames;
        if(!m_picks.isEmpty() && m_picks.size() < m_fiffInfo.ch_names.size()) {
            for(int i = 0; i < m_picks.size(); i++) {
                pickedNames << m_fiffInfo.ch_names.at(m_picks[i]);
            }
        } else {
            pickedNames = m_fiffInfo.ch_names;
        }
        computedCov.names = pickedNames;
        computedCov.projs = m_fiffInfo.projs;
        computedCov.bads = m_fiffInfo.bads;
        computedCov.nfree = m_iSamples;

        // regularize noise covariance
        computedCov = computedCov.regularize(m_fiffInfo, 0.05, 0.05, 0.1, doProj, QStringList());

//            qint32 samples = rawSegment.cols();
//            VectorXf mu = rawSegment.rowwise().sum().array() / (float)samples;

//            MatrixXf noise_covariance = rawSegment * rawSegment.transpose();// noise_covariance == raw_covariance
//            noise_covariance.array() -= samples * (mu * mu.transpose()).array();
//            noise_covariance.array() /= (samples - 1);

//            std::cout << "Noise Covariance:\n" << noise_covariance.block(0,0,10,10) << std::endl;

//            printf("%d raw buffer (%d x %d) generated\r\n", count, tmp.rows(), tmp.cols());

        m_lData.clear();
        m_iSamples = 0;

        return computedCov;
    } else {
        qWarning() << "[RtCov::estimateCovariance] Number of samples equals zero. Regularization not possible. Returning empty covariance estimation.";
        return FiffCov();
    }

}

//=============================================================================================================

RtCovComputeResult RtCov::compute(const MatrixXd &matData)
{
    RtCovComputeResult result;
    result.mu = matData.rowwise().sum();
    result.matData = matData * matData.transpose();
    return result;
}

//=============================================================================================================

void RtCov::reduce(RtCovComputeResult& finalResult, const RtCovComputeResult &tempResult)
{
    if(finalResult.matData.size() == 0 || finalResult.mu.size() == 0) {
        finalResult.mu = tempResult.mu;
        finalResult.matData = tempResult.matData;
    } else {
        finalResult.mu += tempResult.mu;
        finalResult.matData += tempResult.matData;
    }
}
