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

#include <Eigen/Dense>
#include <Eigen/Core>

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace Eigen;
using namespace FIFFLIB;

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

MatrixXd Filter::filterData(const MatrixXd& matDataIn,
                            FilterKernel::FilterType type,
                            double dCenterfreq,
                            double bandwidth,
                            double dTransition,
                            double dSFreq,
                            int iOrder,
                            FilterKernel::DesignMethod designMethod,
                            const RowVectorXi& vecPicks,
                            bool bUseThreads)
{
    // Check for size of data
    if(matDataIn.cols() < iOrder){
        qWarning() << QString("[Filter::filterData] Filter length/order is bigger than data length. Returning.");
        return matDataIn;
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

    return filterData(matDataIn,
                      QList<FilterKernel>() << filter,
                      vecPicks,
                      bUseThreads);

}

//=============================================================================================================

MatrixXd Filter::filterData(const MatrixXd& matDataIn,
                            const QList<FilterKernel>& lFilterKernel,
                            const RowVectorXi& vecPicks,
                            bool bUseThreads)
{
    if(lFilterKernel.isEmpty()) {
        qWarning() << "[Filter::filterData] Filter kernel list is empty. Returning.";
        return matDataIn;
    }

    int iOrder = lFilterKernel.first().getFilterOrder();
    for(int i = 0; i < lFilterKernel.size(); ++i) {
        if(lFilterKernel[i].getFilterOrder() > iOrder) {
            iOrder = lFilterKernel[i].getFilterOrder();
        }
    }

    // Check for size of data
    if(matDataIn.cols() < iOrder){
        qWarning() << QString("[Filter::filterData] Filter length/order is bigger than data length. Returning.");
        return matDataIn;
    }

    // create output matrix with size of inputmatrix and temporal input matrix with size of pick
    MatrixXd matDataOut = matDataIn;
    MatrixXd sliceFiltered;

    // slice input data into data junks with proper length for fft
    int iSize = 4096 - iOrder;

    if(matDataIn.cols() > iSize) {
        int from = 0;
        int numSlices = ceil(float(matDataIn.cols())/float(iSize)); //calculate number of data slices

        for (int i = 0; i<numSlices; i++) {
            if(i == numSlices-1) {
                //catch the last one that might be shorter then original size
                iSize = matDataIn.cols() - (iSize * (numSlices -1));
            }
            sliceFiltered = filterDataBlock(matDataIn.block(0,from,matDataIn.rows(),iSize),
                                            vecPicks,
                                            lFilterKernel,
                                            bUseThreads);
            matDataOut.block(0,from,matDataIn.rows(),iSize) = sliceFiltered;
            from += iSize;
        }
    } else {
        matDataOut = filterDataBlock(matDataIn,
                                     vecPicks,
                                     lFilterKernel,
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

MatrixXd Filter::filterDataBlock(const MatrixXd& matDataIn,
                                 const RowVectorXi& vecPicks,
                                 const QList<FilterKernel>& lFilterKernel,
                                 bool bUseThreads)
{
    //Copy input data
    MatrixXd matDataOut = matDataIn;

    if(lFilterKernel.isEmpty()) {
        return matDataOut;
    }

    int iOrder = lFilterKernel.first().getFilterOrder();
    for(int i = 0; i < lFilterKernel.size(); ++i) {
        if(lFilterKernel[i].getFilterOrder() > iOrder) {
            iOrder = lFilterKernel[i].getFilterOrder();
        }
    }

    // Check for size of data
    if(matDataIn.cols() < iOrder){
        qWarning() << QString("[Filter::filterDataBlock] Filter length/order is bigger than data length. Returning.");
        return matDataIn;
    }

    //Initialise the overlap matrix
    if(m_matOverlap.cols() != iOrder || m_matOverlap.rows() < matDataIn.rows()) {
        m_matOverlap.resize(matDataIn.rows(), iOrder);
        m_matOverlap.setZero();
    }

    if(m_matDelay.cols() != iOrder/2 || m_matOverlap.rows() < matDataIn.rows()) {
        m_matDelay.resize(matDataIn.rows(), iOrder/2);
        m_matDelay.setZero();
    }

    //Do the concurrent filtering
    if(vecPicks.cols() > 0) {
        //Generate QList structure which can be handled by the QConcurrent framework
        QList<FilterObject> timeData;

        //Only select channels specified in vecPicks
        FilterObject data;
        for(qint32 i = 0; i < vecPicks.cols(); ++i) {
            data.lFilterKernel = lFilterKernel;
            data.iRow = vecPicks[i];
            data.vecData = matDataIn.row(vecPicks[i]);
            timeData.append(data);
        }

        // Copy in data from last data block. This is necessary in order to also delay channels which are not filtered.
        matDataOut.block(0, iOrder/2, matDataIn.rows(), matDataOut.cols()-iOrder/2) = matDataIn.block(0, 0, matDataIn.rows(), matDataOut.cols()-iOrder/2);
        matDataOut.block(0, 0, matDataIn.rows(), iOrder/2) = m_matDelay;

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
            //Get the currently filtered data. This data has a delay of filterLength/2 in front and back.
            tempData = timeData.at(r).vecData;

            //Perform the actual overlap add by adding the last filter length data to the newly filtered one
            tempData.head(iOrder) += m_matOverlap.row(timeData.at(r).iRow);

            //Write the newly calculated filtered data to the filter data matrix.
            int start = 0;
            matDataOut.row(timeData.at(r).iRow).segment(start,iFilteredNumberCols-iOrder) = tempData.head(iFilteredNumberCols-iOrder);

            //Refresh the m_matOverlap with the new calculated filtered data.
            m_matOverlap.row(timeData.at(r).iRow) = timeData.at(r).vecData.tail(iOrder);
        }

        if(matDataIn.cols() >= iOrder/2) {
            m_matDelay = matDataIn.block(0, matDataIn.cols()-iOrder/2, matDataIn.rows(), iOrder/2);
        } else {
            qWarning() << "[Filter::filterDataBlock] Half of filter length is larger than data size. Not filling m_matDelay for next step.";
        }
    } else {
        qWarning() << "[Filter::filterDataBlock] Nubmer of picked channels is zero.";
    }

    return matDataOut;
}
