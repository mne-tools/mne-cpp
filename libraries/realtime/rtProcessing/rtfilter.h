//=============================================================================================================
/**
* @file     rtfilter.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
* @version  1.0
* @date     April, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
* @brief     RtFilter class declaration.
*
*/

#ifndef RTFILTER_H
#define RTFILTER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../realtime_global.h"

#include <utils/filterTools/filterdata.h>
#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <unsupported/Eigen/FFT>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE REALTIMELIB
//=============================================================================================================

namespace REALTIMELIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//=============================================================================================================
/**
* Real-time noise Spectrum estimation
*
* @brief Real-time Noise estimation
*/
class REALTIMESHARED_EXPORT RtFilter
{

public:
    typedef QSharedPointer<RtFilter> SPtr;             /**< Shared pointer type for RtFilter. */
    typedef QSharedPointer<const RtFilter> ConstSPtr;  /**< Const shared pointer type for RtFilter. */

    //=========================================================================================================
    /**
    * Creates the real-time covariance estimation object.
    */
    explicit RtFilter();

    //=========================================================================================================
    /**
    * Destroys the Real-time noise estimation object.
    */
    ~RtFilter();

    //=========================================================================================================
    /**
    * Calculates the filtered version of the raw input data
    *
    * @param [in] matDataIn     data which is to be filtered
    * @param [out] matDataOut    data which is to be filtered
    * @param [in] iDataIndex    current position in the global data matrix
    */
    Eigen::MatrixXd filterChannelsConcurrently(const Eigen::MatrixXd& matDataIn,
                                               int iMaxFilterLength,
                                               const QVector<int>& lFilterChannelList,
                                               const QList<UTILSLIB::FilterData> &lFilterData);

protected:
    Eigen::MatrixXd                 m_matOverlap;                   /**< Last overlap block */
    Eigen::MatrixXd                 m_matDelay;                     /**< Last delay block */

private:

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // RTFILTER_H
