//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     rt_filter.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.3
 * @date     April, 2016
 * @brief    Filter definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_filter.h"

#include <mne/base/log.h>
#include <mne/base/thread_pool.h>
#include <mne/fiff/fiff_raw_data.h>
#include <mne/fiff/fiff_file.h>
#include <mne/mne/mne_epoch_data.h>
#include <mne/mne/mne_epoch_data_list.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>
#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <algorithm>
#include <cmath>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL mne_dsp METHODS
//=============================================================================================================

bool mne::filterFile(const std::filesystem::path& path,
                     const std::shared_ptr<FiffRawData>& pFiffRawData,
                     int type,
                     double dCenterfreq,
                     double bandwidth,
                     double dTransition,
                     double dSFreq,
                     int iOrder,
                     int designMethod,
                     const RowVectorXi& vecPicks,
                     bool bUseThreads)
{
    // Normalize cut off frequencies to nyquist
    dCenterfreq = dCenterfreq / (dSFreq / 2.0);
    bandwidth = bandwidth / (dSFreq / 2.0);
    dTransition = dTransition / (dSFreq / 2.0);

    // create filter
    FilterKernel filter = FilterKernel("filter_kernel",
                                       type,
                                       iOrder,
                                       dCenterfreq,
                                       bandwidth,
                                       dTransition,
                                       dSFreq,
                                       designMethod);

    return filterFile(path,
                      pFiffRawData,
                      filter,
                      vecPicks,
                      bUseThreads);
}

//=============================================================================================================

bool mne::filterFile(const std::filesystem::path& path,
                     const std::shared_ptr<FiffRawData>& pFiffRawData,
                     const FilterKernel& filterKernel,
                     const RowVectorXi& vecPicks,
                     bool bUseThreads)
{
    int iOrder = filterKernel.filter_order();

    RowVectorXd cals;
    SparseMatrix<double> mult;
    RowVectorXi sel;
    auto outfid = FiffStream::start_writing_raw(path, pFiffRawData->info, cals);
    if (!outfid) {
        return false;
    }

    //Setup reading parameters
    fiff_int_t from = pFiffRawData->first_samp;
    fiff_int_t to = pFiffRawData->last_samp;

    // slice input data into data junks with proper length so that the slices are always >= the filter order
    float fFactor = 2.0f;
    int iSize = fFactor * iOrder;
    int residual = (to - from) % iSize;
    while (residual < iOrder) {
        fFactor = fFactor - 0.1f;
        iSize = fFactor * iOrder;
        residual = (to - from) % iSize;

        if ((iSize < iOrder)) {
            Log::info("[Filter::filterData] Sliced data block size is too small. Filtering whole block at once.");
            iSize = to - from;
            break;
        }
    }

    float quantum_sec = iSize / pFiffRawData->info.sfreq;
    fiff_int_t quantum = ceil(static_cast<double>(quantum_sec) * pFiffRawData->info.sfreq);

    // Read, filter and write the data
    bool first_buffer = true;

    fiff_int_t first, last;
    MatrixXd matData, matDataOverlap;
    MatrixXd times;

    for (first = from; first < to; first += quantum) {
        last = first + quantum - 1;
        if (last > to) {
            last = to;
        }

        if (!pFiffRawData->read_raw_segment(matData, times, mult, first, last, sel)) {
            Log::warning("[Filter::filterData] Error during read_raw_segment\n");
            return false;
        }

        Log::info("Filtering and writing block {} to {}", first, last);

        if (first_buffer) {
            if (first > 0) {
                outfid->write_int(FIFF_FIRST_SAMPLE, &first);
            }
            first_buffer = false;
        }

        matData = filterDataBlock(matData,
                                  vecPicks,
                                  filterKernel,
                                  bUseThreads);

        // Output column c holds sample first + c - iOrder / 2; the last iOrder columns still get the next block's head
        const bool lastBlock = last == to;
        const int writeFrom = first == from ? iOrder / 2 : 0;
        const int writeTo = lastBlock ? matData.cols() - iOrder / 2 : matData.cols() - iOrder;
        if (first != from) {
            matData.block(0, 0, matData.rows(), iOrder) += matDataOverlap;
        }
        outfid->write_raw_buffer(matData.block(0, writeFrom, matData.rows(), writeTo - writeFrom), cals);

        matDataOverlap = matData.block(0, matData.cols() - iOrder, matData.rows(), iOrder);
    }

    outfid->finish_writing_raw();

    return true;
}

//=============================================================================================================

MatrixXd mne::filterData(const MatrixXd& matData,
                         int type,
                         double dCenterfreq,
                         double bandwidth,
                         double dTransition,
                         double dSFreq,
                         int iOrder,
                         int designMethod,
                         const RowVectorXi& vecPicks,
                         bool bUseThreads,
                         bool bKeepOverhead)
{
    // Check for size of data
    if (matData.cols() < iOrder) {
        Log::warning("[Filter::filterData] Filter length/order is bigger than data length. Returning.");
        return matData;
    }

    // Normalize cut off frequencies to nyquist
    dCenterfreq = dCenterfreq / (dSFreq / 2.0);
    bandwidth = bandwidth / (dSFreq / 2.0);
    dTransition = dTransition / (dSFreq / 2.0);

    // create filter
    FilterKernel filter = FilterKernel("filter_kernel",
                                       type,
                                       iOrder,
                                       dCenterfreq,
                                       bandwidth,
                                       dTransition,
                                       dSFreq,
                                       designMethod);

    return filterData(matData,
                      filter,
                      vecPicks,
                      bUseThreads,
                      bKeepOverhead);
}

//=============================================================================================================

MatrixXd mne::filterData(const MatrixXd& matData,
                         const FilterKernel& filterKernel,
                         const RowVectorXi& vecPicks,
                         bool bUseThreads,
                         bool bKeepOverhead)
{
    int iOrder = filterKernel.filter_order();

    // Check for size of data
    if (matData.cols() < iOrder) {
        Log::warning("[Filter::filterData] Filter length/order is bigger than data length. Returning.");
        return matData;
    }

    // Create output matrix with size of input matrix
    MatrixXd matDataOut(matData.rows(), matData.cols() + iOrder);
    matDataOut.setZero();
    MatrixXd sliceFiltered;

    // slice input data into data junks with proper length so that the slices are always >= the filter order
    float fFactor = 2.0f;
    int iSize = fFactor * iOrder;
    int residual = matData.cols() % iSize;
    while (residual < iOrder) {
        fFactor = fFactor - 0.1f;
        iSize = fFactor * iOrder;
        residual = matData.cols() % iSize;

        if (iSize < iOrder) {
            iSize = matData.cols();
            break;
        }
    }

    if (matData.cols() > iSize) {
        int from = 0;
        const int numSlices = static_cast<int>((matData.cols() + iSize - 1) / iSize);

        for (int i = 0; i < numSlices; i++) {
            if (i == numSlices - 1) {
                //catch the last one that might be shorter than the other blocks
                iSize = matData.cols() - (iSize * (numSlices - 1));
            }

            // Filter the data block. This will return data with a fitler delay of iOrder/2 in front and back
            sliceFiltered = filterDataBlock(matData.block(0, from, matData.rows(), iSize),
                                            vecPicks,
                                            filterKernel,
                                            bUseThreads);

            // Perform overlap add
            if (i == 0) {
                matDataOut.block(0, 0, matData.rows(), sliceFiltered.cols()) += sliceFiltered;
            } else {
                matDataOut.block(0, from, matData.rows(), sliceFiltered.cols()) += sliceFiltered;
            }

            from += iSize;
        }
    } else {
        matDataOut = filterDataBlock(matData,
                                     vecPicks,
                                     filterKernel,
                                     bUseThreads);
    }

    if (bKeepOverhead) {
        return matDataOut;
    } else {
        return matDataOut.block(0, iOrder / 2, matDataOut.rows(), matData.cols());
    }
}

//=============================================================================================================

MatrixXd mne::filterDataBlock(const MatrixXd& matData,
                              const RowVectorXi& vecPicks,
                              const FilterKernel& filterKernel,
                              bool bUseThreads)
{
    int iOrder = filterKernel.filter_order();

    // Check for size of data
    if (matData.cols() < iOrder) {
        Log::warning("[Filter::filterDataBlock] Filter length/order is bigger than data length. Returning.");
        return matData;
    }

    // Setup filters to the correct length, so we do not have to do this everytime we call the FFT filter function
    FilterKernel filterKernelSetup = filterKernel;
    filterKernelSetup.prepare_filter(matData.cols());

    RowVectorXi vecPicksNew = vecPicks;
    if (vecPicksNew.cols() == 0) {
        vecPicksNew = RowVectorXi::LinSpaced(matData.rows(), 0, matData.rows() - 1);
    }

    // Channels that are not filtered are delayed by half the filter order like the filtered ones
    MatrixXd matDataOut = MatrixXd::Zero(matData.rows(), matData.cols() + iOrder);
    matDataOut.middleCols(iOrder / 2, matData.cols()) = matData;

    const Index channels = vecPicksNew.cols();
    ThreadPool& pool = ThreadPool::global();
    const Index chunks = bUseThreads ? std::min<Index>(channels, static_cast<Index>(pool.thread_count()) + 1) : 1;
    auto filterChunk = [&](std::size_t chunk) {
        // Each chunk filters with its own kernel, whose FFT buffers are not shared
        FilterKernel kernel = filterKernelSetup;
        RowVectorXd row;
        for (Index i = channels * static_cast<Index>(chunk) / chunks; i < channels * (static_cast<Index>(chunk) + 1) / chunks; ++i) {
            row = matData.row(vecPicksNew[i]);
            kernel.apply_fft_filter(row, true);
            matDataOut.row(vecPicksNew[i]) = row;
        }
    };
    if (chunks > 1) {
        pool.parallel_for(0, static_cast<std::size_t>(chunks), filterChunk);
    } else if (channels > 0) {
        filterChunk(0);
    }

    return matDataOut;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXd FilterOverlapAdd::calculate(const MatrixXd& matData,
                                     int type,
                                     double dCenterfreq,
                                     double bandwidth,
                                     double dTransition,
                                     double dSFreq,
                                     int iOrder,
                                     int designMethod,
                                     const RowVectorXi& vecPicks,
                                     bool bFilterEnd,
                                     bool bUseThreads,
                                     bool bKeepOverhead)
{
    // Check for size of data
    if (matData.cols() < iOrder) {
        Log::warning("[Filter::filterData] Filter length/order is bigger than data length. Returning.");
        return matData;
    }

    // Normalize cut off frequencies to nyquist
    dCenterfreq = dCenterfreq / (dSFreq / 2.0);
    bandwidth = bandwidth / (dSFreq / 2.0);
    dTransition = dTransition / (dSFreq / 2.0);

    // create filter
    FilterKernel filter = FilterKernel("filter_kernel",
                                       type,
                                       iOrder,
                                       dCenterfreq,
                                       bandwidth,
                                       dTransition,
                                       dSFreq,
                                       designMethod);

    return calculate(matData,
                     filter,
                     vecPicks,
                     bFilterEnd,
                     bUseThreads,
                     bKeepOverhead);
}

//=============================================================================================================

MatrixXd FilterOverlapAdd::calculate(const MatrixXd& matData,
                                     const FilterKernel& filterKernel,
                                     const RowVectorXi& vecPicks,
                                     bool bFilterEnd,
                                     bool bUseThreads,
                                     bool bKeepOverhead)
{
    int iOrder = filterKernel.filter_order();

    // Check for size of data
    if (matData.cols() < iOrder) {
        Log::warning("[Filter::filterData] Filter length/order is bigger than data length. Returning.");
        return matData;
    }

    // Init overlaps from last block
    if (m_matOverlapBack.cols() != iOrder || m_matOverlapBack.rows() < matData.rows()) {
        m_matOverlapBack.resize(matData.rows(), iOrder);
        m_matOverlapBack.setZero();
    }

    if (m_matOverlapFront.cols() != iOrder || m_matOverlapFront.rows() < matData.rows()) {
        m_matOverlapFront.resize(matData.rows(), iOrder);
        m_matOverlapFront.setZero();
    }

    // Create output matrix with size of input matrix
    MatrixXd matDataOut(matData.rows(), matData.cols() + iOrder);
    matDataOut.setZero();
    MatrixXd sliceFiltered;

    // slice input data into data junks with proper length so that the slices are always >= the filter order
    float fFactor = 2.0f;
    int iSize = fFactor * iOrder;
    int residual = matData.cols() % iSize;
    while (residual < iOrder) {
        fFactor = fFactor - 0.1f;
        iSize = fFactor * iOrder;
        residual = matData.cols() % iSize;

        if (iSize < iOrder) {
            iSize = matData.cols();
            break;
        }
    }

    if (matData.cols() > iSize) {
        int from = 0;
        const int numSlices = static_cast<int>((matData.cols() + iSize - 1) / iSize);

        for (int i = 0; i < numSlices; i++) {
            if (i == numSlices - 1) {
                //catch the last one that might be shorter than the other blocks
                iSize = matData.cols() - (iSize * (numSlices - 1));
            }

            // Filter the data block. This will return data with a fitler delay of iOrder/2 in front and back
            sliceFiltered = filterDataBlock(matData.block(0, from, matData.rows(), iSize),
                                            vecPicks,
                                            filterKernel,
                                            bUseThreads);

            if (i == 0) {
                matDataOut.block(0, 0, matData.rows(), sliceFiltered.cols()) += sliceFiltered;
            } else {
                matDataOut.block(0, from, matData.rows(), sliceFiltered.cols()) += sliceFiltered;
            }

            if (bFilterEnd && (i == 0)) {
                matDataOut.block(0, 0, matDataOut.rows(), iOrder) += m_matOverlapBack;
            } else if (!bFilterEnd && (i == numSlices - 1)) {
                matDataOut.block(0, matDataOut.cols() - iOrder, matDataOut.rows(), iOrder) += m_matOverlapFront;
            }

            from += iSize;
        }
    } else {
        matDataOut = filterDataBlock(matData,
                                     vecPicks,
                                     filterKernel,
                                     bUseThreads);

        if (bFilterEnd) {
            matDataOut.block(0, 0, matDataOut.rows(), iOrder) += m_matOverlapBack;
        } else {
            matDataOut.block(0, matDataOut.cols() - iOrder, matDataOut.rows(), iOrder) += m_matOverlapFront;
        }
    }

    // Refresh the overlap matrix with the new calculated filtered data
    m_matOverlapBack = matDataOut.block(0, matDataOut.cols() - iOrder, matDataOut.rows(), iOrder);
    m_matOverlapFront = matDataOut.block(0, 0, matDataOut.rows(), iOrder);

    if (bKeepOverhead) {
        return matDataOut;
    } else {
        return matDataOut.block(0, 0, matDataOut.rows(), matData.cols());
    }
}

//=============================================================================================================

void FilterOverlapAdd::reset()
{
    m_matOverlapBack.resize(0, 0);
    m_matOverlapFront.resize(0, 0);
}

//=============================================================================================================

FiffEvoked mne::computeFilteredAverage(const FiffRawData& raw,
                                       const MatrixXi& matEvents,
                                       float fTMinS,
                                       float fTMaxS,
                                       int eventType,
                                       bool bApplyBaseline,
                                       float fTBaselineFromS,
                                       float fTBaselineToS,
                                       const std::map<std::string, double>& mapReject,
                                       const FilterKernel& filterKernel,
                                       const StringList& lExcludeChs,
                                       const RowVectorXi& picks)
{
    MNEEpochDataList lstEpochDataList;

    // Select the desired events
    int count = 0;
    int p;
    MatrixXi selected = MatrixXi::Zero(1, matEvents.rows());
    for (p = 0; p < matEvents.rows(); ++p) {
        if (matEvents(p, 1) == 0 && matEvents(p, 2) == eventType) {
            selected(0, count) = p;
            ++count;
        }
    }
    selected.conservativeResize(1, count);
    if (count > 0) {
        Log::info("[mne::computeFilteredAverage] {} matching events found", count);
    }

    // If picks are empty, pick all
    RowVectorXi picksNew = picks;
    if (picks.cols() <= 0) {
        picksNew.resize(raw.info.chs.size());
        for (int i = 0; i < std::ssize(raw.info.chs); ++i) {
            picksNew(i) = i;
        }
    }

    fiff_int_t event_samp, from, to;
    fiff_int_t dropCount = 0;
    MatrixXd timesDummy;
    MatrixXd times;

    std::unique_ptr<MNEEpochData> epoch;
    int iFilterDelay = filterKernel.filter_order() / 2;

    for (p = 0; p < count; ++p) {
        // Read a data segment
        event_samp = matEvents(selected(p), 0);
        // Like mne.Epochs: both ends rounded to the nearest sample
        from = event_samp + static_cast<fiff_int_t>(std::lround(fTMinS * raw.info.sfreq));
        to = event_samp + static_cast<fiff_int_t>(std::lround(fTMaxS * raw.info.sfreq));

        epoch = std::make_unique<MNEEpochData>();

        if (raw.read_raw_segment(epoch->epoch, timesDummy, from - iFilterDelay, to + iFilterDelay, picksNew)) {
            // Filter the data
            epoch->epoch = mne::filterData(epoch->epoch, filterKernel).block(0, iFilterDelay, epoch->epoch.rows(), to - from + 1);

            if (p == 0) {
                times.resize(1, to - from + 1);
                for (int i = 0; i < times.cols(); ++i)
                    times(0, i) = (static_cast<float>(from - event_samp + i)) / raw.info.sfreq;
            }

            epoch->event = eventType;
            epoch->eventSample = event_samp;
            epoch->tmin = fTMinS;
            epoch->tmax = fTMaxS;

            epoch->bReject = MNEEpochDataList::checkForArtifact(epoch->epoch,
                                                                raw.info,
                                                                mapReject,
                                                                lExcludeChs);

            if (epoch->bReject) {
                dropCount++;
            }

            //Check if data block has the same size as the previous one
            if (!lstEpochDataList.empty()) {
                if (epoch->epoch.size() == lstEpochDataList.back()->epoch.size()) {
                    lstEpochDataList.push_back(std::shared_ptr<MNEEpochData>(epoch.release()));
                }
            } else {
                lstEpochDataList.push_back(std::shared_ptr<MNEEpochData>(epoch.release()));
            }
        } else {
            Log::warning("[MNEEpochDataList::readEpochs] Can't read the event data segments.");
        }
    }

    Log::info("[MNEEpochDataList::readEpochs] Read a total of {} epochs of type {} and marked {} for rejection.", lstEpochDataList.size(), eventType, dropCount);

    if (bApplyBaseline) {
        std::pair<float, float> baselinePair(fTBaselineFromS, fTBaselineToS);
        lstEpochDataList.applyBaselineCorrection(baselinePair);
    }

    if (!mapReject.empty()) {
        lstEpochDataList.dropRejected();
    }

    if (lstEpochDataList.empty())
        return FiffEvoked();

    const fiff_int_t first = static_cast<fiff_int_t>(std::lround(fTMinS * raw.info.sfreq));
    FiffEvoked evoked = lstEpochDataList.average(raw.info,
                                                 first,
                                                 first + static_cast<fiff_int_t>(lstEpochDataList.front()->epoch.cols()) - 1);
    evoked.baseline = bApplyBaseline ? std::pair(fTBaselineFromS, fTBaselineToS) : std::pair(0.0f, 0.0f);
    return evoked;
}
