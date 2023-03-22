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

#include "filter.h"

#include <utils/mnemath.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_file.h>

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

MatrixXd RTPROCESSINGLIB::filterData(const MatrixXd& mataData,
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
    if(mataData.cols() < iOrder){
        qWarning() << QString("[Filter::filterData] Filter length/order is bigger than data length. Returning.");
        return mataData;
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

    return filterData(mataData,
                      filter,
                      vecPicks,
                      bUseThreads,
                      bKeepOverhead);
}

//=============================================================================================================

MatrixXd RTPROCESSINGLIB::filterData(const MatrixXd& mataData,
                                     const FilterKernel& filterKernel,
                                     const RowVectorXi& vecPicks,
                                     bool bUseThreads,
                                     bool bKeepOverhead)
{
    int iOrder = filterKernel.getFilterOrder();

    // Check for size of data
    if(mataData.cols() < iOrder){
        qWarning() << "[Filter::filterData] Filter length/order is bigger than data length. Returning.";
        return mataData;
    }

    // Create output matrix with size of input matrix
    MatrixXd matDataOut(mataData.rows(), mataData.cols()+iOrder);
    matDataOut.setZero();
    MatrixXd sliceFiltered;

    // slice input data into data junks with proper length so that the slices are always >= the filter order
    float fFactor = 2.0f;
    int iSize = fFactor * iOrder;
    int residual = mataData.cols() % iSize;
    while(residual < iOrder) {
        fFactor = fFactor - 0.1f;
        iSize = fFactor * iOrder;
        residual = mataData.cols() % iSize;

        if(iSize < iOrder) {
            iSize = mataData.cols();
            break;
        }
    }

    if(mataData.cols() > iSize) {
        int from = 0;
        int numSlices = ceil(float(mataData.cols())/float(iSize)); //calculate number of data slices

        for (int i = 0; i < numSlices; i++) {
            if(i == numSlices-1) {
                //catch the last one that might be shorter than the other blocks
                iSize = mataData.cols() - (iSize * (numSlices -1));
            }

            // Filter the data block. This will return data with a fitler delay of iOrder/2 in front and back
            sliceFiltered = filterDataBlock(mataData.block(0,from,mataData.rows(),iSize),
                                            vecPicks,
                                            filterKernel,
                                            bUseThreads);

            // Perform overlap add
            if(i == 0) {
                matDataOut.block(0,0,mataData.rows(),sliceFiltered.cols()) += sliceFiltered;
            } else {
                matDataOut.block(0,from,mataData.rows(),sliceFiltered.cols()) += sliceFiltered;
            }

            from += iSize;
        }
    } else {
        matDataOut = filterDataBlock(mataData,
                                     vecPicks,
                                     filterKernel,
                                     bUseThreads);
    }

    if(bKeepOverhead) {
        return matDataOut;
    } else {
        return matDataOut.block(0,iOrder/2,matDataOut.rows(),mataData.cols());
    }
}

//=============================================================================================================

MatrixXd RTPROCESSINGLIB::filterDataBlock(const MatrixXd& mataData,
                                          const RowVectorXi& vecPicks,
                                          const FilterKernel& filterKernel,
                                          bool bUseThreads)
{
    int iOrder = filterKernel.getFilterOrder();

    // Check for size of data
    if(mataData.cols() < iOrder){
        qWarning() << QString("[Filter::filterDataBlock] Filter length/order is bigger than data length. Returning.");
        return mataData;
    }

    // Setup filters to the correct length, so we do not have to do this everytime we call the FFT filter function
    FilterKernel filterKernelSetup = filterKernel;
    filterKernelSetup.prepareFilter(mataData.cols());

    // Do the concurrent filtering
    RowVectorXi vecPicksNew = vecPicks;
    if(vecPicksNew.cols() == 0) {
        vecPicksNew = RowVectorXi::LinSpaced(mataData.rows(), 0, mataData.rows());
    }

    // Generate QList structure which can be handled by the QConcurrent framework
    QList<FilterObject> timeData;

    // Only select channels specified in vecPicksNew
    FilterObject data;
    for(qint32 i = 0; i < vecPicksNew.cols(); ++i) {
        data.filterKernel = filterKernelSetup;
        data.iRow = vecPicksNew[i];
        data.vecData = mataData.row(vecPicksNew[i]);
        timeData.append(data);
    }

    // Copy in data from last data block. This is necessary in order to also delay channels which are not filtered
    MatrixXd matDataOut(mataData.rows(), mataData.cols()+iOrder);
    matDataOut.setZero();
    matDataOut.block(0, iOrder/2, mataData.rows(), mataData.cols()) = mataData;

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

MatrixXd FilterOverlapAdd::calculate(const MatrixXd& mataData,
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
    if(mataData.cols() < iOrder){
        qWarning() << QString("[Filter::filterData] Filter length/order is bigger than data length. Returning.");
        return mataData;
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

    return calculate(mataData,
                                filter,
                                vecPicks,
                                bFilterEnd,
                                bUseThreads,
                                bKeepOverhead);
}

//=============================================================================================================

MatrixXd FilterOverlapAdd::calculate(const MatrixXd& mataData,
                                     const FilterKernel& filterKernel,
                                     const RowVectorXi& vecPicks,
                                     bool bFilterEnd,
                                     bool bUseThreads,
                                     bool bKeepOverhead)
{
    int iOrder = filterKernel.getFilterOrder();

    // Check for size of data
    if(mataData.cols() < iOrder){
        qWarning() << "[Filter::filterData] Filter length/order is bigger than data length. Returning.";
        return mataData;
    }

    // Init overlaps from last block
    if(m_matOverlapBack.cols() != iOrder || m_matOverlapBack.rows() < mataData.rows()) {
        m_matOverlapBack.resize(mataData.rows(), iOrder);
        m_matOverlapBack.setZero();
    }

    if(m_matOverlapFront.cols() != iOrder || m_matOverlapFront.rows() < mataData.rows()) {
        m_matOverlapFront.resize(mataData.rows(), iOrder);
        m_matOverlapFront.setZero();
    }

    // Create output matrix with size of input matrix
    MatrixXd matDataOut(mataData.rows(), mataData.cols()+iOrder);
    matDataOut.setZero();
    MatrixXd sliceFiltered;

    // slice input data into data junks with proper length so that the slices are always >= the filter order
    float fFactor = 2.0f;
    int iSize = fFactor * iOrder;
    int residual = mataData.cols() % iSize;
    while(residual < iOrder) {
        fFactor = fFactor - 0.1f;
        iSize = fFactor * iOrder;
        residual = mataData.cols() % iSize;

        if(iSize < iOrder) {
            iSize = mataData.cols();
            break;
        }
    }

    if(mataData.cols() > iSize) {
        int from = 0;
        int numSlices = ceil(float(mataData.cols())/float(iSize)); //calculate number of data slices

        for (int i = 0; i < numSlices; i++) {
            if(i == numSlices-1) {
                //catch the last one that might be shorter than the other blocks
                iSize = mataData.cols() - (iSize * (numSlices -1));
            }

            // Filter the data block. This will return data with a fitler delay of iOrder/2 in front and back
            sliceFiltered = filterDataBlock(mataData.block(0,from,mataData.rows(),iSize),
                                            vecPicks,
                                            filterKernel,
                                            bUseThreads);

            if(i == 0) {
                matDataOut.block(0,0,mataData.rows(),sliceFiltered.cols()) += sliceFiltered;
            } else {
                matDataOut.block(0,from,mataData.rows(),sliceFiltered.cols()) += sliceFiltered;
            }

            if(bFilterEnd && (i == 0)) {
                matDataOut.block(0,0,matDataOut.rows(),iOrder) += m_matOverlapBack;
            } else if (!bFilterEnd && (i == numSlices-1)) {
                matDataOut.block(0,matDataOut.cols()-iOrder,matDataOut.rows(),iOrder) += m_matOverlapFront;
            }

            from += iSize;
        }
    } else {
        matDataOut = filterDataBlock(mataData,
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
        return matDataOut.block(0,0,matDataOut.rows(),mataData.cols());
    }
}

//=============================================================================================================

void FilterOverlapAdd::reset()
{
    m_matOverlapBack.resize(0,0);
    m_matOverlapFront.resize(0,0);
}
