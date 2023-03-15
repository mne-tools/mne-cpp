//=============================================================================================================
/**
 * @file     rtcov.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Gabriel B Motta, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief     Definition of the RtCov Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtcov.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QtConcurrent>

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

    m_lData.append(matData);
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

    QStringList exclude;
    for(int i = 0; i<m_fiffInfo.chs.size(); i++) {
        if(m_fiffInfo.chs.at(i).kind != FIFFV_MEG_CH &&
           m_fiffInfo.chs.at(i).kind != FIFFV_EEG_CH) {
            exclude << m_fiffInfo.chs.at(i).ch_name;
        }
    }
    bool doProj = true;

    if(m_iSamples > 0) {
        finalResult.mu /= (float)m_iSamples;
        computedCov.data.array() -= m_iSamples * (finalResult.mu * finalResult.mu.transpose()).array();
        computedCov.data.array() /= (m_iSamples - 1);

        computedCov.kind = FIFFV_MNE_NOISE_COV;
        computedCov.diag = false;
        computedCov.dim = computedCov.data.rows();

        //ToDo do picks
        computedCov.names = m_fiffInfo.ch_names;
        computedCov.projs = m_fiffInfo.projs;
        computedCov.bads = m_fiffInfo.bads;
        computedCov.nfree = m_iSamples;

        // regularize noise covariance
        computedCov = computedCov.regularize(m_fiffInfo, 0.05, 0.05, 0.1, doProj, exclude);

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
