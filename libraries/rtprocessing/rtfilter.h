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

#include "rtprocessing_global.h"

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
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//=============================================================================================================
/**
* Real-time filtering with fft overlap
*
* @brief Real-time filtering
*/
class RTPROCESINGSHARED_EXPORT RtFilter
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
    * @param [in] matDataIn The data which is to be filtered
    * @param [in] iOrder The maximum filterlength, sames as filter order(FIR)
    * @param [in] vecPicks The used channel as index in RowVector
    * @param [in] lFilterData The FilterData generated by filterobject from utilslib
    *
    * @return The filtered data in form of a matrix.
    */
    Eigen::MatrixXd filterDataBlock(const Eigen::MatrixXd& matDataIn,
                                               int iOrder,
                                               const Eigen::RowVectorXi& vecPicks,
                                               const QList<UTILSLIB::FilterData> &lFilterData);

    /**
    * Calculates the filtered version of the raw input data AND creates filter
    *
    * @param [in] matDataIn The data which is to be filtered
    * @param [in] type of the filter: LPF, HPF, BPF, NOTCH (from enum FilterType)
    * @param [in] iOrder
    * @param [in] dCenterfreq determines the center of the frequency
    * @param [in] dBandwidth ignored if FilterType is set to LPF,HPF. if NOTCH/BPF: bandwidth of stop-/passband
    * @param [in] dTransition determines the width of the filter slopes (steepness)
    * @param [in] dSFreq sampling frequency
    * @param [in] vecPicks - used channel as index in QVector
    * @param [in] iOrder represents the order of the filter, the higher the higher is the stopband attenuation
    * @param [in] iFftLength length of the fft (multiple integer of 2^x) - Default = 4096
    * @param [in] designMethod specifies the design method to use. Choose between Cosind and Tschebyscheff; Defaul = Cosine
    *
    * @return The filtered data in form of a matrix.
    */

    Eigen::MatrixXd filterData(const Eigen::MatrixXd& matDataIn,
                               UTILSLIB::FilterData::FilterType type,
                               double dCenterfreq,
                               double dBandwidth,
                               double dTransition,
                               double dSFreq,
                               const Eigen::RowVectorXi &vecPicks = Eigen::RowVectorXi(),
                               int iOrder = 1024,
                               qint32 iFftLength = 4096,
                               UTILSLIB::FilterData::DesignMethod designMethod = UTILSLIB::FilterData::Cosine);

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
