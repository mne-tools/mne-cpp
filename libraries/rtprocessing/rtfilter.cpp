//=============================================================================================================
/**
 * @file     rtfilter.cpp
 * @author   Ruben Doerfel <Ruben.Doerfel@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     April, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Ruben Doerfel, Lorenz Esch. All rights reserved.
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
 * @brief     RtFilter class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtfilter.h"
#include <Eigen/Dense>
#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace Eigen;
using namespace UTILSLIB;
using namespace FIFFLIB;

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

void doFilterPerChannelRTMSA(RtFilter::RtFilterData &channelDataTime)
{
    for(int i = 0; i < channelDataTime.lFilterData.size(); ++i) {
        //channelDataTime.vecData = channelDataTime.first.at(i).applyConvFilter(channelDataTime.vecData, true, FilterData::ZeroPad);
        channelDataTime.vecData = channelDataTime.lFilterData.at(i).applyFFTFilter(channelDataTime.vecData, true, FilterData::ZeroPad); //FFT Convolution for rt is not suitable. FFT make the signal filtering non causal.
    }
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtFilter::RtFilter()
{
}

//=============================================================================================================

RtFilter::~RtFilter()
{
}

//=============================================================================================================

MatrixXd RtFilter::filterDataBlock(const MatrixXd& matDataIn,
                                   int iOrder,
                                   const RowVectorXi &vecPicks,
                                   const QList<FilterData>& lFilterData)
{
    //Initialise the overlay matrix
    if(m_matOverlap.cols() != iOrder || m_matOverlap.rows() < matDataIn.rows()) {
        m_matOverlap.resize(matDataIn.rows(), iOrder);
        m_matOverlap.setZero();
    }

    if(m_matDelay.cols() != iOrder/2 || m_matOverlap.rows() < matDataIn.rows()) {
        m_matDelay.resize(matDataIn.rows(), iOrder/2);
        m_matDelay.setZero();
    }

    //Resize output matrix to match input matrix
    MatrixXd matDataOut = matDataIn;

    //Generate QList structure which can be handled by the QConcurrent framework
    QList<RtFilterData> timeData;

    //Only select channels specified in vecPicks
    for(qint32 i = 0; i < vecPicks.cols(); ++i) {
        RtFilterData data;
        data.lFilterData = lFilterData;
        data.iRow = vecPicks[i];
        data.vecData = matDataIn.row(vecPicks[i]);
        timeData.append(data);
    }

    //Do the concurrent filtering
    if(!timeData.isEmpty()) {
        QFuture<void> future = QtConcurrent::map(timeData,
                                                 doFilterPerChannelRTMSA);

        future.waitForFinished();

        //Do the overlap add method and store in matDataOut
        int iFilteredNumberCols = timeData.at(0).vecData.cols();

        for(int r = 0; r < timeData.size(); r++) {
            //Get the currently filtered data. This data has a delay of filterLength/2 in front and back.
            RowVectorXd tempData = timeData.at(r).vecData;

            //Perform the actual overlap add by adding the last filterlength data to the newly filtered one
            tempData.head(iOrder) += m_matOverlap.row(timeData.at(r).iRow);

            //Write the newly calulated filtered data to the filter data matrix. Keep in mind that the current block also effect last part of the last block (begin at dataIndex-iFilterDelay).
            int start = 0;
            matDataOut.row(timeData.at(r).iRow).segment(start,iFilteredNumberCols-iOrder) = tempData.head(iFilteredNumberCols-iOrder);

            //Refresh the m_matOverlap with the new calculated filtered data.
            m_matOverlap.row(timeData.at(r).iRow) = timeData.at(r).vecData.tail(iOrder);
        }
    }

    if(matDataIn.cols() >= iOrder/2) {
        m_matDelay = matDataIn.block(0, matDataIn.cols()-iOrder/2, matDataIn.rows(), iOrder/2);
    } else {
        qWarning() << "[RtFilter::filterDataBlock] Half of filter length is larger than data size. Not filling m_matDelay for next step.";
    }

    return matDataOut;
}

//=============================================================================================================

MatrixXd RtFilter::filterData(const MatrixXd& matDataIn,
                              FilterData::FilterType type,
                              double dCenterfreq,
                              double bandwidth,
                              double dTransition,
                              double dSFreq,
                              const RowVectorXi& vecPicks,
                              int iOrder,
                              qint32 iFftLength,
                              FilterData::DesignMethod designMethod)
{
    // Check for size of data
    if (matDataIn.cols()<iOrder){
        qDebug() << QString("[RtFilter::filterData] Filter length bigger then data length.");
    }

    // Normalize cut off frequencies to nyquist
    dCenterfreq = dCenterfreq/(dSFreq/2.0);
    bandwidth = bandwidth/(dSFreq/2.0);
    dTransition = dTransition/(dSFreq/2.0);

    // create output matrix with size of inputmatrix and temporal input matrix with size of pick
    MatrixXd matDataOut = matDataIn;
    MatrixXd sliceFiltered;
    // create filter
    FilterData filter = FilterData("rt_filter",
                                   type,
                                   iOrder,
                                   dCenterfreq,
                                   bandwidth,
                                   dTransition,
                                   dSFreq,
                                   iFftLength,
                                   designMethod);

    QList<FilterData> filterList;
    filterList << filter;

    // slice input data into data junks with proper length for fft
    int iSize = iFftLength-iOrder;

    if(matDataIn.cols() > iSize) {
        int from = 0;
        int numSlices = ceil(float(matDataIn.cols())/float(iSize)); //calculate number of data slices

        for (int i = 0; i<numSlices; i++) {
            if(i == numSlices-1) {
                //catch the last one that might be shorter then original size
                iSize = matDataIn.cols() - (iSize * (numSlices -1));
            }
            sliceFiltered = filterDataBlock(matDataIn.block(0,from,matDataIn.rows(),iSize),
                                            iOrder,
                                            vecPicks,
                                            filterList);
            matDataOut.block(0,from,matDataIn.rows(),iSize) = sliceFiltered;
            from += iSize;
        }
    } else {
        matDataOut = filterDataBlock(matDataIn,
                                    iOrder,
                                    vecPicks,
                                    filterList);
    }
    return matDataOut;
}
