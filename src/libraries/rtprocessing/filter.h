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
 * @brief     Filter declarations.
 *
 */

#ifndef FILTER_RTPROCESSING_H
#define FILTER_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

#include "helpers/filterkernel.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QtConcurrent/QtConcurrent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <unsupported/Eigen/FFT>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffRawData;
}

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

typedef struct {
    FilterKernel filterKernel;
    int iRow;
    Eigen::RowVectorXd vecData;
} FilterObject;

//=========================================================================================================
/**
 * Creates a user designed filter kernel, filters data from an input file and writes the filtered data to a pIODevice.
 *
 * @param[in] pIODevice            The IO device to write to.
 * @param[in] pFiffRawData         The fiff raw data object to read from.
 * @param[in] type                 The type of the filter: LPF, HPF, BPF, NOTCH (from enum FilterType).
 * @param[in] dCenterfreq          The center of the frequency.
 * @param[in] dBandwidth           The filter bandwidth. Ignored if FilterType is set to LPF,HPF. If NOTCH/BPF: bandwidth of stop-/passband.
 * @param[in] dTransition          The transistion band determines the width of the filter slopes (steepness).
 * @param[in] dSFreq               The input data sampling frequency.
 * @param[in] iOrder               Represents the order of the filter, the higher the higher is the stopband attenuation. Default is 4096 taps.
 * @param[in] designMethod         The design method to use. Choose between Cosine and Tschebyscheff. Defaul is set to Cosine.
 * @param[in] vecPicks             Channel indexes to filter. Default is filter all channels.
 * @param[in] bUseThreads          hether to use multiple threads. Default is set to true.
 *
 * @return Returns true if successfull, false otherwise.
 */
RTPROCESINGSHARED_EXPORT bool filterFile(QIODevice& pIODevice,
                                         QSharedPointer<FIFFLIB::FiffRawData> pFiffRawData,
                                         int type,
                                         double dCenterfreq,
                                         double dBandwidth,
                                         double dTransition,
                                         double dSFreq,
                                         int iOrder = 4096,
                                         int designMethod = FilterKernel::m_designMethods.indexOf(FilterParameter("Cosine")),
                                         const Eigen::RowVectorXi &vecPicks = Eigen::RowVectorXi(),
                                         bool bUseThreads = true);

//=========================================================================================================
/**
 * Filters data from an input file based on an exisiting filter kernel and writes the filtered data to a
 * pIODevice.
 *
 * @param[in] pIODevice            The IO device to write to.
 * @param[in] pFiffRawData         The fiff raw data object to read from.
 * @param[in] filterKernel         The list of filter kernels to use.
 * @param[in] vecPicks             Channel indexes to filter. Default is filter all channels.
 * @param[in] bUseThreads          hether to use multiple threads. Default is set to true.
 *
 * @return Returns true if successfull, false otherwise.
 */
RTPROCESINGSHARED_EXPORT bool filterFile(QIODevice& pIODevice,
                                         QSharedPointer<FIFFLIB::FiffRawData> pFiffRawData,
                                         const RTPROCESSINGLIB::FilterKernel& filterKernel,
                                         const Eigen::RowVectorXi &vecPicks = Eigen::RowVectorXi(),
                                         bool bUseThreads = false);

//=========================================================================================================
/**
 * Creates a user designed filter kernel and filters the raw input data.
 * The data needs to be present all at once. For continous filtering via overlap add use the FilterOverlapAdd class.
 *
 * @param[in] matData          The data which is to be filtered.
 * @param[in] type             The type of the filter: LPF, HPF, BPF, NOTCH (from enum FilterType).
 * @param[in] dCenterfreq      The center of the frequency.
 * @param[in] dBandwidth       The filter bandwidth. Ignored if FilterType is set to LPF,HPF. If NOTCH/BPF: bandwidth of stop-/passband.
 * @param[in] dTransition      The transistion band determines the width of the filter slopes (steepness).
 * @param[in] dSFreq           The input data sampling frequency.
 * @param[in] iOrder           Represents the order of the filter, the higher the higher is the stopband attenuation. Default is 1024 taps.
 * @param[in] designMethod     The design method to use. Choose between Cosine and Tschebyscheff. Defaul is set to Cosine.
 * @param[in] vecPicks         Channel indexes to filter. Default is filter all channels.
 * @param[in] bUseThreads      Whether to use multiple threads. Default is set to true.
 * @param[in] bKeepOverhead    Whether to keep the delayed part of the data after filtering. Default is set to false .
 *
 * @return The filtered data in form of a matrix.
 */
RTPROCESINGSHARED_EXPORT Eigen::MatrixXd filterData(const Eigen::MatrixXd& matData,
                                                    int type,
                                                    double dCenterfreq,
                                                    double dBandwidth,
                                                    double dTransition,
                                                    double dSFreq,
                                                    int iOrder = 1024,
                                                    int designMethod = FilterKernel::m_designMethods.indexOf(FilterParameter("Cosine")),
                                                    const Eigen::RowVectorXi &vecPicks = Eigen::RowVectorXi(),
                                                    bool bUseThreads = true,
                                                    bool bKeepOverhead = false);

//=========================================================================================================
/**
 * Calculates the filtered version of the raw input data based on a given list filters
 * The data needs to be present all at once. For continous filtering via overlap add use the FilterOverlapAdd class.
 *
 * @param[in] mataData         The data which is to be filtered.
 * @param[in] filterKernel     The list of filter kernels to use.
 * @param[in] vecPicks         Channel indexes to filter. Default is filter all channels.
 * @param[in] bUseThreads      Whether to use multiple threads. Default is set to true.
 * @param[in] bKeepOverhead    Whether to keep the delayed part of the data after filtering. Default is set to false .
 *
 * @return The filtered data in form of a matrix.
 */
RTPROCESINGSHARED_EXPORT Eigen::MatrixXd filterData(const Eigen::MatrixXd& mataData,
                                                    const RTPROCESSINGLIB::FilterKernel& filterKernel,
                                                    const Eigen::RowVectorXi& vecPicks = Eigen::RowVectorXi(),
                                                    bool bUseThreads = true,
                                                    bool bKeepOverhead = false);

//=========================================================================================================
/**
 * Calculates the filtered version of the raw input data block.
 * Always returns the data with half the filter length delay in the front and back.
 *
 * @param[in] mataData         The data which is to be filtered.
 * @param[in] vecPicks         The used channel as index in RowVector.
 * @param[in] filterKernel     The FilterKernel to to filter the data with.
 * @param[in] bUseThreads      Whether to use multiple threads.
 *
 * @return The filtered data in form of a matrix with half the filter length delay in the front and back.
 */
RTPROCESINGSHARED_EXPORT Eigen::MatrixXd filterDataBlock(const Eigen::MatrixXd& mataData,
                                                         const Eigen::RowVectorXi& vecPicks,
                                                         const RTPROCESSINGLIB::FilterKernel& filterKernel,
                                                         bool bUseThreads = true);

//=========================================================================================================
/**
 * This function is used to filter row-wise in parallel threads
 *
 * @param[in] channelDataTime  The channel data to perform the filtering on.
 */
RTPROCESINGSHARED_EXPORT void filterChannel(FilterObject &channelDataTime);

//=============================================================================================================
/**
 * Filtering with FFT convolution and the overlap add method for continous data streams. This class will hold
 * all needed information about the last block in order to overlap it with the current one.
 *
 * @brief Filtering with FFT convolution and the overlap add method for continous data streams.
 */
class RTPROCESINGSHARED_EXPORT FilterOverlapAdd
{
public:
    typedef QSharedPointer<FilterOverlapAdd> SPtr;             /**< Shared pointer type for FilterOverlapAdd. */
    typedef QSharedPointer<const FilterOverlapAdd> ConstSPtr;  /**< Const shared pointer type for FilterOverlapAdd. */

    //=========================================================================================================
    /**
     * Creates a user designed filter kernel and filters the raw input data
     *
     * @param[in] matData          The data which is to be filtered.
     * @param[in] type             The type of the filter: LPF, HPF, BPF, NOTCH (from enum FilterType).
     * @param[in] dCenterfreq      The center of the frequency.
     * @param[in] dBandwidth       The filter bandwidth. Ignored if FilterType is set to LPF,HPF. If NOTCH/BPF: bandwidth of stop-/passband.
     * @param[in] dTransition      The transistion band determines the width of the filter slopes (steepness).
     * @param[in] dSFreq           The input data sampling frequency.
     * @param[in] iOrder           Represents the order of the filter, the higher the higher is the stopband attenuation. Default is 1024 taps.
     * @param[in] designMethod     The design method to use. Choose between Cosine and Tschebyscheff. Defaul is set to Cosine.
     * @param[in] vecPicks         Channel indexes to filter. Default is filter all channels.
     * @param[in] bFilterEnd       Whether to perform the overlap add in the beginning or end of the data. Default is set to true (end of data).
     * @param[in] bUseThreads      Whether to use multiple threads. Default is set to true.
     * @param[in] bKeepOverhead    Whether to keep the delayed part of the data after filtering. Default is set to false .
     *
     * @return The filtered data in form of a matrix.
     */
    Eigen::MatrixXd calculate(const Eigen::MatrixXd& matData,
                              int type,
                              double dCenterfreq,
                              double dBandwidth,
                              double dTransition,
                              double dSFreq,
                              int iOrder = 1024,
                              int designMethod = FilterKernel::m_designMethods.indexOf(FilterParameter("Cosine")),
                              const Eigen::RowVectorXi &vecPicks = Eigen::RowVectorXi(),
                              bool bFilterEnd = true,
                              bool bUseThreads = true,
                              bool bKeepOverhead = false);

    //=========================================================================================================
    /**
     * Calculates the filtered version of the raw input data based on a given list filters
     *
     * @param[in] mataData         The data which is to be filtered.
     * @param[in] filterKernel     The list of filter kernels to use.
     * @param[in] vecPicks         Channel indexes to filter. Default is filter all channels.
     * @param[in] bFilterEnd       Whether to perform the overlap add in the beginning or end of the data. Default is set to true (end of data).
     * @param[in] bUseThreads      Whether to use multiple threads. Default is set to true.
     * @param[in] bKeepOverhead    Whether to keep the delayed part of the data after filtering. Default is set to false .
     *
     * @return The filtered data in form of a matrix.
     */
    Eigen::MatrixXd calculate(const Eigen::MatrixXd& mataData,
                              const RTPROCESSINGLIB::FilterKernel& filterKernel,
                              const Eigen::RowVectorXi& vecPicks = Eigen::RowVectorXi(),
                              bool bFilterEnd = true,
                              bool bUseThreads = true,
                              bool bKeepOverhead = false);

    //=========================================================================================================
    /**
     * Reset the stored overlap matrices
     */
    void reset();

private:
    Eigen::MatrixXd                 m_matOverlapBack;                   /**< Overlap block for the end of the data block. */
    Eigen::MatrixXd                 m_matOverlapFront;                  /**< Overlap block for the beginning of the data block. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // FILTER_RTPROCESSING_H
