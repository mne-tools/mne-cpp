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
 * @brief     Filter class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filter.h"

#include <utils/mnemath.h>

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
// DEFINE MEMBER METHODS
//=============================================================================================================

Filter::Filter()
{
}

//=============================================================================================================

Filter::~Filter()
{
}

//=============================================================================================================

MatrixXd Filter::filterData(const MatrixXd& mataData,
                            FilterKernel::FilterType type,
                            double dCenterfreq,
                            double bandwidth,
                            double dTransition,
                            double dSFreq,
                            int iOrder,
                            FilterKernel::DesignMethod designMethod,
                            const RowVectorXi& vecPicks,
                            bool bFilterEnd,
                            bool bUseThreads)
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
                      QList<FilterKernel>() << filter,
                      vecPicks,
                      bFilterEnd,
                      bUseThreads);

}

//=============================================================================================================

MatrixXd Filter::filterData(const MatrixXd& mataData,
                            const QList<FilterKernel>& lFilterKernel,
                            const RowVectorXi& vecPicks,
                            bool bFilterEnd,
                            bool bUseThreads)
{
    if(lFilterKernel.isEmpty()) {
        qWarning() << "[Filter::filterData] Filter kernel list is empty. Returning.";
        return mataData;
    }

    int iOrder = lFilterKernel.first().getFilterOrder();
    for(int i = 0; i < lFilterKernel.size(); ++i) {
        if(lFilterKernel[i].getFilterOrder() > iOrder) {
            iOrder = lFilterKernel[i].getFilterOrder();
        }
    }

    // Check for size of data
    if(mataData.cols() < iOrder){
        qWarning() << "[Filter::filterData] Filter length/order is bigger than data length. Returning.";
        return mataData;
    }

    // Create output matrix with size of input matrix
    MatrixXd matDataOut(mataData.rows(), mataData.cols());
    MatrixXd sliceFiltered;

    // slice input data into data junks with proper length so that the slices are always >= the filter order
    int iGcd = MNEMath::gcd(mataData.cols(), iOrder);
    int iFactorOrder = (iOrder/iGcd);
    int iFactorData = (mataData.cols()/iGcd);
    int iSize = iGcd * ceil((iFactorData + iFactorOrder) / 2);

    if((iSize < iOrder) || (mataData.cols() % iSize != 0) || (iSize < 4096 - iOrder)) {
        //qWarning() << "[Filter::filterData] Data block is too small. Filtering whole block at once.";
        iSize = mataData.cols();
    }

    if(mataData.cols() > iSize) {
        int from = 0;
        int numSlices = ceil(float(mataData.cols())/float(iSize)); //calculate number of data slices

        for (int i = 0; i<numSlices; i++) {
            if(i == numSlices-1) {
                //catch the last one that might be shorter then original size
                iSize = mataData.cols() - (iSize * (numSlices -1));
            }
            sliceFiltered = filterDataBlock(mataData.block(0,from,mataData.rows(),iSize),
                                            vecPicks,
                                            lFilterKernel,
                                            bUseThreads);
            matDataOut.block(0,from,mataData.rows(),iSize) = sliceFiltered;
            from += iSize;
        }
    } else {
        matDataOut = filterDataBlock(mataData,
                                     vecPicks,
                                     lFilterKernel,
                                     bFilterEnd,
                                     bUseThreads);
    }

    return matDataOut;
}

//=============================================================================================================

void Filter::filterChannel(Filter::FilterObject& channelDataTime)
{
    for(int i = 0; i < channelDataTime.lFilterKernel.size(); ++i) {
        //channelDataTime.vecData = channelDataTime.first.at(i).applyConvFilter(channelDataTime.vecData, true, FilterKernel::ZeroPad);
        channelDataTime.vecData = channelDataTime.lFilterKernel[i].applyFftFilter(channelDataTime.vecData,
                                                                                  true); //FFT Convolution for rt is not suitable. FFT make the signal filtering non causal.
    }
}

//=============================================================================================================

MatrixXd Filter::filterDataBlock(const MatrixXd& mataData,
                                 const RowVectorXi& vecPicks,
                                 const QList<FilterKernel>& lFilterKernel,
                                 bool bFilterEnd,
                                 bool bUseThreads)
{
    if(lFilterKernel.isEmpty()) {
        return mataData;
    }

    // Find highest order
    int iOrder = lFilterKernel.first().getFilterOrder();
    for(int i = 0; i < lFilterKernel.size(); ++i) {
        if(lFilterKernel[i].getFilterOrder() > iOrder) {
            iOrder = lFilterKernel[i].getFilterOrder();
        }
    }

    // Check for size of data
    if(mataData.cols() < iOrder){
        qWarning() << QString("[Filter::filterDataBlock] Filter length/order is bigger than data length. Returning.");
        return mataData;
    }

    //Initialise the overlap add matrices
    if(m_matOverlapBack.cols() != iOrder || m_matOverlapBack.rows() < mataData.rows()) {
        m_matOverlapBack.resize(mataData.rows(), iOrder);
        m_matOverlapBack.setZero();
    }

    if(m_matDelayBack.cols() != iOrder/2 || m_matOverlapBack.rows() < mataData.rows()) {
        m_matDelayBack.resize(mataData.rows(), iOrder/2);
        m_matDelayBack.setZero();
    }

    if(m_matOverlapFront.cols() != iOrder || m_matOverlapFront.rows() < mataData.rows()) {
        m_matOverlapFront.resize(mataData.rows(), iOrder);
        m_matOverlapFront.setZero();
    }

    if(m_matDelayFront.cols() != iOrder/2 || m_matDelayFront.rows() < mataData.rows()) {
        m_matDelayFront.resize(mataData.rows(), iOrder/2);
        m_matDelayFront.setZero();
    }

    // Setup filters to the correct length, so we do not have to do this everytime we call the FFT filter function
    QList<FilterKernel> lFilterKernelSetup = lFilterKernel;
    FilterKernel::prepareFilters(lFilterKernelSetup,
                                 mataData.cols());

    //Do the concurrent filtering
    RowVectorXi vecPicksNew = vecPicks;
    if(vecPicksNew.cols() == 0) {
        vecPicksNew = RowVectorXi::LinSpaced(mataData.rows(), 0, mataData.rows());
    }

    //Generate QList structure which can be handled by the QConcurrent framework
    QList<FilterObject> timeData;

    //Only select channels specified in vecPicksNew
    FilterObject data;
    for(qint32 i = 0; i < vecPicksNew.cols(); ++i) {
        data.lFilterKernel = lFilterKernelSetup;
        data.iRow = vecPicksNew[i];
        data.vecData = mataData.row(vecPicksNew[i]);
        timeData.append(data);
    }

    // Copy in data from last data block. This is necessary in order to also delay channels which are not filtered
    MatrixXd matDataOut(mataData.rows(), mataData.cols());
    matDataOut.block(0, iOrder/2, mataData.rows(), matDataOut.cols()-iOrder/2) = mataData.block(0, 0, mataData.rows(), matDataOut.cols()-iOrder/2);
    matDataOut.block(0, 0, mataData.rows(), iOrder/2) = m_matDelayBack;

    if(bUseThreads) {
        QFuture<void> future = QtConcurrent::map(timeData,
                                                 filterChannel);

        future.waitForFinished();
    } else {
        for(int i = 0; i < timeData.size(); ++i) {
            filterChannel(timeData[i]);
        }
    }

    //Do the overlap add method and store in matDataOut
    int iFilteredNumberCols = timeData.at(0).vecData.cols();
    RowVectorXd tempData;

    for(int r = 0; r < timeData.size(); r++) {
        //Get the currently filtered data. This data has a delay of filterLength/2 in front and back
        tempData = timeData.at(r).vecData;

        //Perform the actual overlap add by adding the last filter length data to the newly filtered one
        if(bFilterEnd) {
            tempData.head(iOrder) += m_matOverlapBack.row(timeData.at(r).iRow);
        } else {
            tempData.tail(iOrder) += m_matOverlapFront.row(timeData.at(r).iRow);
        }

        //Write the newly calculated filtered data to the filter data matrix
        int start = 0;
        if(bFilterEnd) {
            matDataOut.row(timeData.at(r).iRow).segment(start,iFilteredNumberCols-iOrder) = tempData.head(iFilteredNumberCols-iOrder);
        } else {
            matDataOut.row(timeData.at(r).iRow).segment(start,iFilteredNumberCols-iOrder) = tempData.tail(iFilteredNumberCols-iOrder);
        }

        //Refresh the overlap matrix with the new calculated filtered data
        m_matOverlapBack.row(timeData.at(r).iRow) = timeData.at(r).vecData.tail(iOrder);
        m_matOverlapFront.row(timeData.at(r).iRow) = timeData.at(r).vecData.head(iOrder);
    }

    if(mataData.cols() >= iOrder/2) {
        m_matDelayBack = mataData.block(0, mataData.cols()-iOrder/2, mataData.rows(), iOrder/2);
        m_matDelayFront = mataData.block(0, 0, mataData.rows(), iOrder/2);
    } else {
        qWarning() << "[Filter::filterDataBlock] Half of filter length is larger than data size. Not filling m_matDelayBack for next step.";
    }

    return matDataOut;
}
