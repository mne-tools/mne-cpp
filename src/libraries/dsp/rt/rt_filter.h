//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_filter.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Real-time FIR / IIR filtering of streaming MEG / EEG data blocks.
 *
 * The functions in this header wrap @ref UTILSLIB::FilterKernel and the
 * design back-ends so an incoming raw data file or live data block can be
 * low-pass, high-pass, band-pass or notch-filtered in place with a single
 * call. @ref filterFile streams a @ref FIFFLIB::FiffRawData source through
 * a user-designed kernel and writes the filtered output to an arbitrary
 * @c QIODevice, optionally parallelising the per-channel overlap-add work
 * with Qt Concurrent.
 *
 * The companion @ref FilterObject struct bundles the FIR coefficients with
 * the per-channel @c iRow and a contiguous @c vecData buffer so independent
 * channels can be filtered in parallel without sharing mutable state. All
 * cutoff and transition values are specified in Hz — normalisation against
 * the sampling rate happens inside the design call.
 */

#ifndef RT_FILTER_RT_H
#define RT_FILTER_RT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"

#include "../filterkernel.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_evoked.h>

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

/**
 * @brief Lightweight filter configuration holding kernel coefficients and overlap-add state for one channel.
 */
struct FilterObject {
    UTILSLIB::FilterKernel filterKernel;
    int iRow;
    Eigen::RowVectorXd vecData;
};

//=========================================================================================================
/**
 * Creates a user designed filter kernel, filters data from an input file and writes the filtered data to a pIODevice.
 *
 * @param[in] pIODevice            The IO device to write to.
 * @param[in] pFiffRawData         The fiff raw data object to read from.
 * @param[in] type                 The type of the filter: LPF, HPF, BPF, NOTCH (from enum FilterType).
 * @param[in] dCenterfreq          The center of the frequency.
 * @param[in] dBandwidth           The filter bandwidth. Ignored if FilterType is set to LPF,HPF. If NOTCH/BPF: bandwidth of stop-/passband.
 * @param[in] dTransition          The transition band determines the width of the filter slopes (steepness).
 * @param[in] dSFreq               The input data sampling frequency.
 * @param[in] iOrder               Represents the order of the filter, the higher the higher is the stopband attenuation. Default is 4096 taps.
 * @param[in] designMethod         The design method to use. Choose between Cosine and Tschebyscheff. Default is set to Cosine.
 * @param[in] vecPicks             Channel indexes to filter. Default is filter all channels.
 * @param[in] bUseThreads          Whether to use multiple threads. Default is set to true.
 *
 * @return Returns true if successful, false otherwise.
 */
DSPSHARED_EXPORT bool filterFile(QIODevice& pIODevice,
                                         QSharedPointer<FIFFLIB::FiffRawData> pFiffRawData,
                                         int type,
                                         double dCenterfreq,
                                         double dBandwidth,
                                         double dTransition,
                                         double dSFreq,
                                         int iOrder = 4096,
                                         int designMethod = UTILSLIB::FilterKernel::m_designMethods.indexOf(UTILSLIB::FilterParameter("Cosine")),
                                         const Eigen::RowVectorXi &vecPicks = Eigen::RowVectorXi(),
                                         bool bUseThreads = true);

//=========================================================================================================
/**
 * Filters data from an input file based on an existing filter kernel and writes the filtered data to a
 * pIODevice.
 *
 * @param[in] pIODevice            The IO device to write to.
 * @param[in] pFiffRawData         The fiff raw data object to read from.
 * @param[in] filterKernel         The list of filter kernels to use.
 * @param[in] vecPicks             Channel indexes to filter. Default is filter all channels.
 * @param[in] bUseThreads          Whether to use multiple threads. Default is set to true.
 *
 * @return Returns true if successful, false otherwise.
 */
DSPSHARED_EXPORT bool filterFile(QIODevice& pIODevice,
                                         QSharedPointer<FIFFLIB::FiffRawData> pFiffRawData,
                                         const UTILSLIB::FilterKernel& filterKernel,
                                         const Eigen::RowVectorXi &vecPicks = Eigen::RowVectorXi(),
                                         bool bUseThreads = false);

//=========================================================================================================
/**
 * Creates a user designed filter kernel and filters the raw input data.
 * The data needs to be present all at once. For continuous filtering via overlap add use the FilterOverlapAdd class.
 *
 * @param[in] matData          The data which is to be filtered.
 * @param[in] type             The type of the filter: LPF, HPF, BPF, NOTCH (from enum FilterType).
 * @param[in] dCenterfreq      The center of the frequency.
 * @param[in] dBandwidth       The filter bandwidth. Ignored if FilterType is set to LPF,HPF. If NOTCH/BPF: bandwidth of stop-/passband.
 * @param[in] dTransition      The transition band determines the width of the filter slopes (steepness).
 * @param[in] dSFreq           The input data sampling frequency.
 * @param[in] iOrder           Represents the order of the filter, the higher the higher is the stopband attenuation. Default is 1024 taps.
 * @param[in] designMethod     The design method to use. Choose between Cosine and Tschebyscheff. Default is set to Cosine.
 * @param[in] vecPicks         Channel indexes to filter. Default is filter all channels.
 * @param[in] bUseThreads      Whether to use multiple threads. Default is set to true.
 * @param[in] bKeepOverhead    Whether to keep the delayed part of the data after filtering. Default is set to false .
 *
 * @return The filtered data in form of a matrix.
 */
DSPSHARED_EXPORT Eigen::MatrixXd filterData(const Eigen::MatrixXd& matData,
                                                    int type,
                                                    double dCenterfreq,
                                                    double dBandwidth,
                                                    double dTransition,
                                                    double dSFreq,
                                                    int iOrder = 1024,
                                                    int designMethod = UTILSLIB::FilterKernel::m_designMethods.indexOf(UTILSLIB::FilterParameter("Cosine")),
                                                    const Eigen::RowVectorXi &vecPicks = Eigen::RowVectorXi(),
                                                    bool bUseThreads = true,
                                                    bool bKeepOverhead = false);

//=========================================================================================================
/**
 * Calculates the filtered version of the raw input data based on a given list filters.
 * The data needs to be present all at once. For continuous filtering via overlap add use the FilterOverlapAdd class.
 *
 * @param[in] matData          The data which is to be filtered.
 * @param[in] filterKernel     The list of filter kernels to use.
 * @param[in] vecPicks         Channel indexes to filter. Default is filter all channels.
 * @param[in] bUseThreads      Whether to use multiple threads. Default is set to true.
 * @param[in] bKeepOverhead    Whether to keep the delayed part of the data after filtering. Default is set to false .
 *
 * @return The filtered data in form of a matrix.
 */
DSPSHARED_EXPORT Eigen::MatrixXd filterData(const Eigen::MatrixXd& matData,
                                                    const UTILSLIB::FilterKernel& filterKernel,
                                                    const Eigen::RowVectorXi& vecPicks = Eigen::RowVectorXi(),
                                                    bool bUseThreads = true,
                                                    bool bKeepOverhead = false);

//=========================================================================================================
/**
 * Calculates the filtered version of the raw input data block.
 * Always returns the data with half the filter length delay in the front and back.
 *
 * @param[in] matData          The data which is to be filtered.
 * @param[in] vecPicks         The used channel as index in RowVector.
 * @param[in] filterKernel     The FilterKernel to filter the data with.
 * @param[in] bUseThreads      Whether to use multiple threads.
 *
 * @return The filtered data in form of a matrix with half the filter length delay in the front and back.
 */
DSPSHARED_EXPORT Eigen::MatrixXd filterDataBlock(const Eigen::MatrixXd& matData,
                                                         const Eigen::RowVectorXi& vecPicks,
                                                         const UTILSLIB::FilterKernel& filterKernel,
                                                         bool bUseThreads = true);

//=========================================================================================================
/**
 * This function is used to filter row-wise in parallel threads
 *
 * @param[in] channelDataTime  The channel data to perform the filtering on.
 */
DSPSHARED_EXPORT void filterChannel(FilterObject &channelDataTime);

//=========================================================================================================
/**
 * Computes the filtered average for given fiff raw data.
 * Reads epochs, filters each one, applies baseline correction and artifact rejection,
 * then returns the averaged evoked response.
 *
 * @param[in] raw               The raw data.
 * @param[in] matEvents         The events provided in samples and event kinds.
 * @param[in] fTMinS            The start time relative to the event in seconds.
 * @param[in] fTMaxS            The end time relative to the event in seconds.
 * @param[in] eventType         The event type.
 * @param[in] bApplyBaseline    Whether to use baseline correction (mode=mean).
 * @param[in] fTBaselineFromS   The start baseline correction time relative to the event in seconds.
 * @param[in] fTBaselineToS     The end baseline correction time relative to the event in seconds.
 * @param[in] mapReject         The thresholds per channel type to reject epochs.
 * @param[in] filterKernel      The filter kernel to use when reading the fiff raw data.
 * @param[in] lExcludeChs       List of channel names to exclude.
 * @param[in] vecPicks          Which channels to pick.
 *
 * @return The filtered, averaged evoked data.
 */
DSPSHARED_EXPORT FIFFLIB::FiffEvoked computeFilteredAverage(const FIFFLIB::FiffRawData& raw,
                                                                    const Eigen::MatrixXi& matEvents,
                                                                    float fTMinS,
                                                                    float fTMaxS,
                                                                    qint32 eventType,
                                                                    bool bApplyBaseline,
                                                                    float fTBaselineFromS,
                                                                    float fTBaselineToS,
                                                                    const QMap<QString,double>& mapReject,
                                                                    const UTILSLIB::FilterKernel& filterKernel,
                                                                    const QStringList &lExcludeChs = QStringList(),
                                                                    const Eigen::RowVectorXi& vecPicks = Eigen::RowVectorXi());

//=============================================================================================================
/**
 * Filtering with FFT convolution and the overlap add method for continuous data streams. This class will hold
 * all needed information about the last block in order to overlap it with the current one.
 *
 * @brief Applies FIR filtering via FFT-based overlap-add convolution for continuous data streams.
 */
class DSPSHARED_EXPORT FilterOverlapAdd
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
     * @param[in] dTransition      The transition band determines the width of the filter slopes (steepness).
     * @param[in] dSFreq           The input data sampling frequency.
     * @param[in] iOrder           Represents the order of the filter, the higher the higher is the stopband attenuation. Default is 1024 taps.
     * @param[in] designMethod     The design method to use. Choose between Cosine and Tschebyscheff. Default is set to Cosine.
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
                              int designMethod = UTILSLIB::FilterKernel::m_designMethods.indexOf(UTILSLIB::FilterParameter("Cosine")),
                              const Eigen::RowVectorXi &vecPicks = Eigen::RowVectorXi(),
                              bool bFilterEnd = true,
                              bool bUseThreads = true,
                              bool bKeepOverhead = false);

    //=========================================================================================================
    /**
     * Calculates the filtered version of the raw input data based on a given list filters
     *
     * @param[in] matData           The data which is to be filtered.
     * @param[in] filterKernel     The list of filter kernels to use.
     * @param[in] vecPicks         Channel indexes to filter. Default is filter all channels.
     * @param[in] bFilterEnd       Whether to perform the overlap add in the beginning or end of the data. Default is set to true (end of data).
     * @param[in] bUseThreads      Whether to use multiple threads. Default is set to true.
     * @param[in] bKeepOverhead    Whether to keep the delayed part of the data after filtering. Default is set to false .
     *
     * @return The filtered data in form of a matrix.
     */
    Eigen::MatrixXd calculate(const Eigen::MatrixXd& matData,
                              const UTILSLIB::FilterKernel& filterKernel,
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

#endif // RT_FILTER_RT_H
