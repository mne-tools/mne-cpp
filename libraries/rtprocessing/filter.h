//=============================================================================================================
/**
 * @file     filter.h
 * @author   Ruben Doerfel <Ruben.Doerfel@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.3
 * @date     June, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Doerfel, Lorenz Esch. All rights reserved.
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
 * @brief     Filter class declaration.
 *
 */

#ifndef FILTER_H
#define FILTER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

#include "helpers/filterdata.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <unsupported/Eigen/FFT>

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//=============================================================================================================
/**
 * Filtering with FFT convolution and the overlap add method
 *
 * @brief Filtering with FFT convolution and the overlap add method
 */
class RTPROCESINGSHARED_EXPORT Filter
{

public:
    typedef QSharedPointer<Filter> SPtr;             /**< Shared pointer type for Filter. */
    typedef QSharedPointer<const Filter> ConstSPtr;  /**< Const shared pointer type for Filter. */

    typedef struct {
        QList<RTPROCESSINGLIB::FilterData> lFilterData;
        int iRow;
        Eigen::RowVectorXd vecData;
    } FilterObject;

    //=========================================================================================================
    /**
     * Creates the real-time covariance estimation object.
     */
    explicit Filter();

    //=========================================================================================================
    /**
     * Destroys the Real-time noise estimation object.
     */
    ~Filter();

    /**
     * Calculates a suer designed filter kernel and filteres the raw input data
     *
     * @param [in] matDataIn        The data which is to be filtered
     * @param [in] type             of the filter: LPF, HPF, BPF, NOTCH (from enum FilterType)
     * @param [in] dCenterfreq      determines the center of the frequency
     * @param [in] dBandwidth       ignored if FilterType is set to LPF,HPF. if NOTCH/BPF: bandwidth of stop-/passband
     * @param [in] dTransition      determines the width of the filter slopes (steepness)
     * @param [in] dSFreq           sampling frequency
     * @param [in] iOrder           represents the order of the filter, the higher the higher is the stopband attenuation
     * @param [in] iFftLength       length of the fft (multiple integer of 2^x) - Default = 4096
     * @param [in] designMethod     specifies the design method to use. Choose between Cosind and Tschebyscheff; Defaul = Cosine
     * @param [in] vecPicks         used channel as index in QVector
     * @param [in] bUseThreads      Whether to use multiple threads
     *
     * @return The filtered data in form of a matrix.
     */
    Eigen::MatrixXd filterData(const Eigen::MatrixXd& matDataIn,
                               RTPROCESSINGLIB::FilterData::FilterType type,
                               double dCenterfreq,
                               double dBandwidth,
                               double dTransition,
                               double dSFreq,
                               int iOrder = 1024,
                               int iFftLength = 4096,
                               RTPROCESSINGLIB::FilterData::DesignMethod designMethod = RTPROCESSINGLIB::FilterData::Cosine,
                               const Eigen::RowVectorXi &vecPicks = Eigen::RowVectorXi(),
                               bool bUseThreads = true);
    /**
     * Calculates the filtered version of the raw input data based on a given filter
     *
     * @param [in] matDataIn        The data which is to be filtered
     * @param [in] lFilterData      The list of filter kernels to use
     * @param [in] vecPicks         used channel as index in QVector
     * @param [in] bUseThreads      Whether to use multiple threads
     *
     * @return The filtered data in form of a matrix.
     */
    Eigen::MatrixXd filterData(const Eigen::MatrixXd& matDataIn,
                               const QList<RTPROCESSINGLIB::FilterData>& lFilterData,
                               const Eigen::RowVectorXi& vecPicks = Eigen::RowVectorXi(),
                               bool bUseThreads = true);

private:
    //=========================================================================================================
    /**
     * This static function is used to filter row-wise in parallel
     *
     * @param [in] channelDataTime  The channel data to perform the filtering on
     */
    static void filterChannel(Filter::FilterObject &channelDataTime);

    //=========================================================================================================
    /**
     * Calculates the filtered version of the raw input data
     *
     * @param [in] matDataIn    The data which is to be filtered
     * @param [in] vecPicks     The used channel as index in RowVector
     * @param [in] lFilterData  The FilterData generated by filterobject from utilslib
     * @param [in] bUseThreads  Whether to use multiple threads
     *
     * @return The filtered data in form of a matrix.
     */
    Eigen::MatrixXd filterDataBlock(const Eigen::MatrixXd& matDataIn,
                                    const Eigen::RowVectorXi& vecPicks,
                                    const QList<RTPROCESSINGLIB::FilterData> &lFilterData,
                                    bool bUseThreads = true);

    Eigen::MatrixXd                 m_matOverlap;                   /**< Last overlap block */
    Eigen::MatrixXd                 m_matDelay;                     /**< Last delay block */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // FILTER_H
