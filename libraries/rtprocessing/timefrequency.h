//=============================================================================================================
/**
 * @file     timefrequency.h
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
 * @brief    Declaration of TimeFrequency functions
 *
 */

#ifndef TIMEFREQUENCY_RTPROCESSING_H
#define TIMEFREQUENCY_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

#include <fiff/fiff_evoked.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/StdVector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffRawData;
    class FiffEvokedSet;
}

namespace MNELIB {
    class MNEEpochData;
}

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

class RTPROCESINGSHARED_EXPORT TimeFrequencyData
{
public:
    TimeFrequencyData();

    TimeFrequencyData(Eigen::MatrixXcd mat);

    static std::vector<Eigen::MatrixXd> computeEpochListTimeFrequency(const FIFFLIB::FiffEvokedSet& evokedSet);

    static std::vector<Eigen::MatrixXcd> computeComplexTimeFrequency(const FIFFLIB::FiffEvokedSet& evokedSet);

    static std::vector<std::vector<Eigen::MatrixXcd> > computeEpochListTimeFrequency(const FIFFLIB::FiffRawData& raw,
                                                               const Eigen::MatrixXi& matEvents,
                                                               float fTMinS,
                                                               float fTMaxS);

    static std::vector<Eigen::MatrixXcd> computeEpochTimeFrequency(const QSharedPointer<MNELIB::MNEEpochData>& epoch,
                                                      float sampleFrequency);

    static Eigen::MatrixXcd averageEpochTimeFrequency(const std::vector<Eigen::MatrixXcd>& epochTimeFrequency);

    static std::vector<Eigen::MatrixXcd> averageEpochListTimeFrequency(const std::vector<std::vector<Eigen::MatrixXcd> >& epochListTimeFrequency);



    Eigen::MatrixXcd getData();

    TimeFrequencyData& operator=(const Eigen::MatrixXcd& mat)
    {
        this->m_TFData = mat;
        return *this;
    }

    TimeFrequencyData& operator+=(const Eigen::MatrixXcd& mat)
    {
        this->m_TFData += mat;
        return *this;
    }

    TimeFrequencyData& operator/=(int i)
    {
        this->m_TFData /= i;
        return *this;
    }

private:

    void setTFData(Eigen::MatrixXcd matData);

    Eigen::MatrixXcd        m_TFData;
};


}//namespace
#endif // TIMEFREQUENCY_RTPROCESSING_H
