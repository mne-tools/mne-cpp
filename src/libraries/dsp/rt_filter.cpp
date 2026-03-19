//=============================================================================================================
/**
 * @file     filter.cpp
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
 * @brief     Filter definitions.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rt_filter.h"

#include <math/mnemath.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_file.h>
#include <mne/mne_epoch_data.h>
#include <mne/mne_epoch_data_list.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>
#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace Eigen;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace MNELIB;

//=============================================================================================================
// DEFINE GLOBAL RTPROCESSINGLIB METHODS
//=============================================================================================================

bool RTPROCESSINGLIB::filterFile(QIODevice &pIODevice,
                                 QSharedPointer<FiffRawData> pFiffRawData,
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
    dCenterfreq = dCenterfreq/(dSFreq/2.0);
    bandwidth = bandwidth/(dSFreq/2.0);
    dTransition = dTransition/(dSFreq/2.0);

    // create filter
    FilterKernel filter = FilterKernel("filter_kernel",
                                       type,
                                       iOrder,
                                       dCenterfreq,
                                       bandwidth,
                                       dTransition,
                                       dSFreq,
                                       designMethod);

    return filterFile(pIODevice,
                      pFiffRawData,
                      filter,
                      vecPicks,
                      bUseThreads);
}

//=============================================================================================================

bool RTPROCESSINGLIB::filterFile(QIODevice &pIODevice,
                                 QSharedPointer<FiffRawData> pFiffRawData,
                                 const FilterKernel& filterKernel,
                                 const RowVectorXi& vecPicks,
                                 bool bUseThreads)
{
    int iOrder = filterKernel.getFilterOrder();

    RowVectorXd cals;
    SparseMatrix<double> mult;
    RowVectorXi sel;
    FiffStream::SPtr outfid = FiffStream::start_writing_raw(pIODevice, pFiffRawData->info, cals);

    //Setup reading parameters
    fiff_int_t from = pFiffRawData->first_samp;
    fiff_int_t to = pFiffRawData->last_samp;

    // slice input data into data junks with proper length so that the slices are always >= the filter order
    float fFactor = 2.0f;
    int iSize = fFactor * iOrder;
    int residual = (to - from) % iSize;
    while(residual < iOrder) {
        fFactor = fFactor - 0.1f;
        iSize = fFactor * iOrder;
        residual = (to - from) % iSize;

        if((iSize < iOrder)) {
            qInfo() << "[Filter::filterData] Sliced data block size is too small. Filtering whole block at once.";
            iSize = to - from;
            break;
        }
    }

    float quantum_sec = iSize/pFiffRawData->info.sfreq;
    fiff_int_t quantum = ceil(quantum_sec*pFiffRawData->info.sfreq);

    // Read, filter and write the data
    bool first_buffer = true;

    fiff_int_t first, last;
    MatrixXd matData, matDataOverlap;
    MatrixXd times;

    for(first = from; first < to; first+=quantum) {
        last = first+quantum-1;
        if (last > to) {
            last = to;
        }

        if (!pFiffRawData->read_raw_segment(matData, times, mult, first, last, sel)) {
            qWarning("[Filter::filterData] Error during read_raw_segment\n");
            return false;
        }

        qInfo() << "Filtering and writing block" << first << "to" << last;

        if (first_buffer) {
           if (first > 0) {
               outfid->write_int(FIFF_FIRST_SAMPLE,&first);
           }
           first_buffer = false;
        }

        matData = filterDataBlock(matData,
                                  vecPicks,
                                  filterKernel,
                                  bUseThreads);

        if(first == from) {
            outfid->write_raw_buffer(matData.block(0,iOrder/2,matData.rows(),matData.cols()-iOrder), cals);
        } else if(first + quantum >= to) {
            matData.block(0,0,matData.rows(),iOrder) += matDataOverlap;
            outfid->write_raw_buffer(matData.block(0,0,matData.rows(),matData.cols()-iOrder), cals);
        } else {
            matData.block(0,0,matData.rows(),iOrder) += matDataOverlap;
            outfid->write_raw_buffer(matData.block(0,0,matData.rows(),matData.cols()-iOrder), cals);
        }

        matDataOverlap = matData.block(0,matData.cols()-iOrder,matData.rows(),iOrder);
    }

    outfid->finish_writing_raw();

    return true;
}

//=============================================================================================================

MatrixXd RTPROCESSINGLIB::filterData(const MatrixXd& matData,
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
    if(matData.cols() < iOrder){
        qWarning() << QString("[Filter::filterData] Filter length/order is bigger than data length. Returning.");
        return matData;
    }

    // Normalize cut off frequencies to nyquist
    dCenterfreq = dCenterfreq/(dSFreq/2.0);
    bandwidth = bandwidth/(dSFreq/2.0);
    dTransition = dTransition/(dSFreq/2.0);

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

MatrixXd RTPROCESSINGLIB::filterData(const MatrixXd& matData,
                                     const FilterKernel& filterKernel,
                                     const RowVectorXi& vecPicks,
                                     bool bUseThreads,
                                     bool bKeepOverhead)
{
    int iOrder = filterKernel.getFilterOrder();

    // Check for size of data
    if(matData.cols() < iOrder){
        qWarning() << "[Filter::filterData] Filter length/order is bigger than data length. Returning.";
        return matData;
    }

    // Create output matrix with size of input matrix
    MatrixXd matDataOut(matData.rows(), matData.cols()+iOrder);
    matDataOut.setZero();
    MatrixXd sliceFiltered;

    // slice input data into data junks with proper length so that the slices are always >= the filter order
    float fFactor = 2.0f;
    int iSize = fFactor * iOrder;
    int residual = matData.cols() % iSize;
    while(residual < iOrder) {
        fFactor = fFactor - 0.1f;
        iSize = fFactor * iOrder;
        residual = matData.cols() % iSize;

        if(iSize < iOrder) {
            iSize = matData.cols();
            break;
        }
    }

    if(matData.cols() > iSize) {
        int from = 0;
        int numSlices = ceil(float(matData.cols())/float(iSize)); //calculate number of data slices

        for (int i = 0; i < numSlices; i++) {
            if(i == numSlices-1) {
                //catch the last one that might be shorter than the other blocks
                iSize = matData.cols() - (iSize * (numSlices -1));
            }

            // Filter the data block. This will return data with a fitler delay of iOrder/2 in front and back
            sliceFiltered = filterDataBlock(matData.block(0,from,matData.rows(),iSize),
                                            vecPicks,
                                            filterKernel,
                                            bUseThreads);

            // Perform overlap add
            if(i == 0) {
                matDataOut.block(0,0,matData.rows(),sliceFiltered.cols()) += sliceFiltered;
            } else {
                matDataOut.block(0,from,matData.rows(),sliceFiltered.cols()) += sliceFiltered;
            }

            from += iSize;
        }
    } else {
        matDataOut = filterDataBlock(matData,
                                     vecPicks,
                                     filterKernel,
                                     bUseThreads);
    }

    if(bKeepOverhead) {
        return matDataOut;
    } else {
        return matDataOut.block(0,iOrder/2,matDataOut.rows(),matData.cols());
    }
}

//=============================================================================================================

MatrixXd RTPROCESSINGLIB::filterDataBlock(const MatrixXd& matData,
                                          const RowVectorXi& vecPicks,
                                          const FilterKernel& filterKernel,
                                          bool bUseThreads)
{
    int iOrder = filterKernel.getFilterOrder();

    // Check for size of data
    if(matData.cols() < iOrder){
        qWarning() << QString("[Filter::filterDataBlock] Filter length/order is bigger than data length. Returning.");
        return matData;
    }

    // Setup filters to the correct length, so we do not have to do this everytime we call the FFT filter function
    FilterKernel filterKernelSetup = filterKernel;
    filterKernelSetup.prepareFilter(matData.cols());

    // Do the concurrent filtering
    RowVectorXi vecPicksNew = vecPicks;
    if(vecPicksNew.cols() == 0) {
        vecPicksNew = RowVectorXi::LinSpaced(matData.rows(), 0, matData.rows());
    }

    // Generate QList structure which can be handled by the QConcurrent framework
    QList<FilterObject> timeData;

    // Only select channels specified in vecPicksNew
    FilterObject data;
    for(qint32 i = 0; i < vecPicksNew.cols(); ++i) {
        data.filterKernel = filterKernelSetup;
        data.iRow = vecPicksNew[i];
        data.vecData = matData.row(vecPicksNew[i]);
        timeData.append(data);
    }

    // Copy in data from last data block. This is necessary in order to also delay channels which are not filtered
    MatrixXd matDataOut(matData.rows(), matData.cols()+iOrder);
    matDataOut.setZero();
    matDataOut.block(0, iOrder/2, matData.rows(), matData.cols()) = matData;

    if(bUseThreads) {
        QFuture<void> future = QtConcurrent::map(timeData,
                                                 filterChannel);
        future.waitForFinished();
    } else {
        for(int i = 0; i < timeData.size(); ++i) {
            filterChannel(timeData[i]);
        }
    }

    // Do the overlap add method and store in matDataOut
    RowVectorXd tempData;

    for(int r = 0; r < timeData.size(); r++) {
        // Write the newly calculated filtered data to the filter data matrix. This data has a delay of iOrder/2 in front and back
        matDataOut.row(timeData.at(r).iRow) = timeData.at(r).vecData;
    }

    return matDataOut;
}

//=============================================================================================================

void RTPROCESSINGLIB::filterChannel(RTPROCESSINGLIB::FilterObject& channelDataTime)
{
    //channelDataTime.vecData = channelDataTime.first.at(i).applyConvFilter(channelDataTime.vecData, true);
    channelDataTime.filterKernel.applyFftFilter(channelDataTime.vecData, true); //FFT Convolution for rt is not suitable. FFT make the signal filtering non causal.
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
    if(matData.cols() < iOrder){
        qWarning() << QString("[Filter::filterData] Filter length/order is bigger than data length. Returning.");
        return matData;
    }

    // Normalize cut off frequencies to nyquist
    dCenterfreq = dCenterfreq/(dSFreq/2.0);
    bandwidth = bandwidth/(dSFreq/2.0);
    dTransition = dTransition/(dSFreq/2.0);

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
    int iOrder = filterKernel.getFilterOrder();

    // Check for size of data
    if(matData.cols() < iOrder){
        qWarning() << "[Filter::filterData] Filter length/order is bigger than data length. Returning.";
        return matData;
    }

    // Init overlaps from last block
    if(m_matOverlapBack.cols() != iOrder || m_matOverlapBack.rows() < matData.rows()) {
        m_matOverlapBack.resize(matData.rows(), iOrder);
        m_matOverlapBack.setZero();
    }

    if(m_matOverlapFront.cols() != iOrder || m_matOverlapFront.rows() < matData.rows()) {
        m_matOverlapFront.resize(matData.rows(), iOrder);
        m_matOverlapFront.setZero();
    }

    // Create output matrix with size of input matrix
    MatrixXd matDataOut(matData.rows(), matData.cols()+iOrder);
    matDataOut.setZero();
    MatrixXd sliceFiltered;

    // slice input data into data junks with proper length so that the slices are always >= the filter order
    float fFactor = 2.0f;
    int iSize = fFactor * iOrder;
    int residual = matData.cols() % iSize;
    while(residual < iOrder) {
        fFactor = fFactor - 0.1f;
        iSize = fFactor * iOrder;
        residual = matData.cols() % iSize;

        if(iSize < iOrder) {
            iSize = matData.cols();
            break;
        }
    }

    if(matData.cols() > iSize) {
        int from = 0;
        int numSlices = ceil(float(matData.cols())/float(iSize)); //calculate number of data slices

        for (int i = 0; i < numSlices; i++) {
            if(i == numSlices-1) {
                //catch the last one that might be shorter than the other blocks
                iSize = matData.cols() - (iSize * (numSlices -1));
            }

            // Filter the data block. This will return data with a fitler delay of iOrder/2 in front and back
            sliceFiltered = filterDataBlock(matData.block(0,from,matData.rows(),iSize),
                                            vecPicks,
                                            filterKernel,
                                            bUseThreads);

            if(i == 0) {
                matDataOut.block(0,0,matData.rows(),sliceFiltered.cols()) += sliceFiltered;
            } else {
                matDataOut.block(0,from,matData.rows(),sliceFiltered.cols()) += sliceFiltered;
            }

            if(bFilterEnd && (i == 0)) {
                matDataOut.block(0,0,matDataOut.rows(),iOrder) += m_matOverlapBack;
            } else if (!bFilterEnd && (i == numSlices-1)) {
                matDataOut.block(0,matDataOut.cols()-iOrder,matDataOut.rows(),iOrder) += m_matOverlapFront;
            }

            from += iSize;
        }
    } else {
        matDataOut = filterDataBlock(matData,
                                     vecPicks,
                                     filterKernel,
                                     bUseThreads);

        if(bFilterEnd) {
            matDataOut.block(0,0,matDataOut.rows(),iOrder) += m_matOverlapBack;
        } else {
            matDataOut.block(0,matDataOut.cols()-iOrder,matDataOut.rows(),iOrder) += m_matOverlapFront;
        }
    }

    // Refresh the overlap matrix with the new calculated filtered data
    m_matOverlapBack = matDataOut.block(0,matDataOut.cols()-iOrder,matDataOut.rows(),iOrder);
    m_matOverlapFront = matDataOut.block(0,0,matDataOut.rows(),iOrder);

    if(bKeepOverhead) {
        return matDataOut;
    } else {
        return matDataOut.block(0,0,matDataOut.rows(),matData.cols());
    }
}

//=============================================================================================================

void FilterOverlapAdd::reset()
{
    m_matOverlapBack.resize(0,0);
    m_matOverlapFront.resize(0,0);
}

//=============================================================================================================

FiffEvoked RTPROCESSINGLIB::computeFilteredAverage(const FiffRawData& raw,
                                                   const MatrixXi& matEvents,
                                                   float fTMinS,
                                                   float fTMaxS,
                                                   qint32 eventType,
                                                   bool bApplyBaseline,
                                                   float fTBaselineFromS,
                                                   float fTBaselineToS,
                                                   const QMap<QString,double>& mapReject,
                                                   const FilterKernel& filterKernel,
                                                   const QStringList& lExcludeChs,
                                                   const RowVectorXi& picks)
{
    MNEEpochDataList lstEpochDataList;

    // Select the desired events
    qint32 count = 0;
    qint32 p;
    MatrixXi selected = MatrixXi::Zero(1, matEvents.rows());
    for (p = 0; p < matEvents.rows(); ++p)
    {
        if (matEvents(p,1) == 0 && matEvents(p,2) == eventType)
        {
            selected(0,count) = p;
            ++count;
        }
    }
    selected.conservativeResize(1, count);
    if (count > 0) {
        qInfo("[RTPROCESSINGLIB::computeFilteredAverage] %d matching events found",count);
    }

    // If picks are empty, pick all
    RowVectorXi picksNew = picks;
    if(picks.cols() <= 0) {
        picksNew.resize(raw.info.chs.size());
        for(int i = 0; i < raw.info.chs.size(); ++i) {
            picksNew(i) = i;
        }
    }

    fiff_int_t event_samp, from, to;
    fiff_int_t dropCount = 0;
    MatrixXd timesDummy;
    MatrixXd times;

    std::unique_ptr<MNEEpochData> epoch;
    int iFilterDelay = filterKernel.getFilterOrder()/2;

    for (p = 0; p < count; ++p) {
        // Read a data segment
        event_samp = matEvents(selected(p),0);
        from = event_samp + fTMinS*raw.info.sfreq;
        to   = event_samp + floor(fTMaxS*raw.info.sfreq + 0.5);

        epoch = std::make_unique<MNEEpochData>();

        if(raw.read_raw_segment(epoch->epoch, timesDummy, from - iFilterDelay, to + iFilterDelay, picksNew)) {
            // Filter the data
            epoch->epoch = RTPROCESSINGLIB::filterData(epoch->epoch,filterKernel).block(0, iFilterDelay, epoch->epoch.rows(), to-from);

            if (p == 0) {
                times.resize(1, to-from+1);
                for (qint32 i = 0; i < times.cols(); ++i)
                    times(0, i) = ((float)(from-event_samp+i)) / raw.info.sfreq;
            }

            epoch->event = eventType;
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
            if(!lstEpochDataList.isEmpty()) {
                if(epoch->epoch.size() == lstEpochDataList.last()->epoch.size()) {
                    lstEpochDataList.append(MNEEpochData::SPtr(epoch.release()));
                }
            } else {
                lstEpochDataList.append(MNEEpochData::SPtr(epoch.release()));
            }
        } else {
            qWarning("[MNEEpochDataList::readEpochs] Can't read the event data segments.");
        }
    }

    qInfo().noquote() << "[MNEEpochDataList::readEpochs] Read a total of"<< lstEpochDataList.size() <<"epochs of type" << eventType << "and marked"<< dropCount <<"for rejection.";

    if(bApplyBaseline) {
        QPair<float, float> baselinePair(fTBaselineFromS, fTBaselineToS);
        lstEpochDataList.applyBaselineCorrection(baselinePair);
    }

    if(!mapReject.isEmpty()) {
        lstEpochDataList.dropRejected();
    }

    return lstEpochDataList.average(raw.info,
                                    0,
                                    lstEpochDataList.first()->epoch.cols());
}
