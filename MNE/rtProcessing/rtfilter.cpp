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
* @brief     RtFilter class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtfilter.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtFilter::RtFilter()
{
}


//*************************************************************************************************************

RtFilter::~RtFilter()
{
}


//*************************************************************************************************************

void RtFilter::filterChannelsConcurrently(const MatrixXd &matDataIn, const MatrixXd &matDataOut, int dataIndex)
{
    //std::cout<<"START RealTimeMultiSampleArrayModel::filterChannelsConcurrently"<<std::endl;

    if(dataIndex >= m_matDataFiltered.cols() || data.cols()<m_iMaxFilterLength)
        return;

    if(data.rows() != m_matDataFiltered.rows()) {
        m_matDataFiltered = m_matDataRaw;
        return;
    }

    //Generate QList structure which can be handled by the QConcurrent framework
    QList<QPair<QList<FilterData>,QPair<int,RowVectorXd> > > timeData;
    QList<int> notFilterChannelIndex;

    for(qint32 i=0; i<data.rows(); ++i) {
        if(m_filterChannelList.contains(m_pFiffInfo->chs.at(i).ch_name))
            timeData.append(QPair<QList<FilterData>,QPair<int,RowVectorXd> >(m_filterData,QPair<int,RowVectorXd>(i,data.row(i))));
        else
            notFilterChannelIndex.append(i);
    }

    //Do the concurrent filtering
    if(!timeData.isEmpty()) {
        QFuture<void> future = QtConcurrent::map(timeData,
                                             doFilterPerChannelRTMSA);

        future.waitForFinished();

        //Do the overlap add method and store in m_matDataFiltered
        int iFilterDelay = m_iMaxFilterLength/2;
        int iFilteredNumberCols = timeData.at(0).second.second.cols();

        for(int r = 0; r<timeData.size(); r++) {
            if(dataIndex+2*data.cols() > m_matDataRaw.cols()) {
                //Handle last data block
                //std::cout<<"Handle last data block"<<std::endl;

                if(m_bDrawFilterFront) {
                    //Get the currently filtered data. This data has a delay of filterLength/2 in front and back.
                    RowVectorXd tempData = timeData.at(r).second.second;

                    //Perform the actual overlap add by adding the last filterlength data to the newly filtered one
                    tempData.head(m_iMaxFilterLength) += m_matOverlap.row(timeData.at(r).second.first);

                    //Write the newly calulated filtered data to the filter data matrix. Keep in mind that the current block also effect last part of the last block (begin at dataIndex-iFilterDelay).
                    int start = dataIndex-iFilterDelay < 0 ? 0 : dataIndex-iFilterDelay;
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(start,iFilteredNumberCols-m_iMaxFilterLength) = tempData.head(iFilteredNumberCols-m_iMaxFilterLength);
                } else {
                    //Perform this else case everytime the filter was changed. Do not begin to plot from dataIndex-iFilterDelay because the impsulse response and m_matOverlap do not match with the new filter anymore.
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(dataIndex-iFilterDelay,m_iMaxFilterLength) = timeData.at(r).second.second.segment(m_iMaxFilterLength,m_iMaxFilterLength);
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(dataIndex+iFilterDelay,iFilteredNumberCols-2*m_iMaxFilterLength) = timeData.at(r).second.second.segment(m_iMaxFilterLength,iFilteredNumberCols-2*m_iMaxFilterLength);
                }

                //Refresh the m_matOverlap with the new calculated filtered data.
                m_matOverlap.row(timeData.at(r).second.first) = timeData.at(r).second.second.tail(m_iMaxFilterLength);
            } else if(dataIndex == 0) {
                //Handle first data block
                //std::cout<<"Handle first data block"<<std::endl;

                if(m_bDrawFilterFront) {
                    //Get the currently filtered data. This data has a delay of filterLength/2 in front and back.
                    RowVectorXd tempData = timeData.at(r).second.second;

                    //Add newly calculate data to the tail of the current filter data matrix
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(m_matDataFiltered.cols()-iFilterDelay-m_iResidual, iFilterDelay) = tempData.head(iFilterDelay) + m_matOverlap.row(timeData.at(r).second.first).head(iFilterDelay);

                    //Perform the actual overlap add by adding the last filterlength data to the newly filtered one
                    tempData.head(m_iMaxFilterLength) += m_matOverlap.row(timeData.at(r).second.first);
                    m_matDataFiltered.row(timeData.at(r).second.first).head(iFilteredNumberCols-m_iMaxFilterLength-iFilterDelay) = tempData.segment(iFilterDelay,iFilteredNumberCols-m_iMaxFilterLength-iFilterDelay);

                    //Copy residual data from the front to the back. The residual is != 0 if the chosen block size cannot be evenly fit into the matrix size
                    m_matDataFiltered.row(timeData.at(r).second.first).tail(m_iResidual) = m_matDataFiltered.row(timeData.at(r).second.first).head(m_iResidual);
                } else {
                    //Perform this else case everytime the filter was changed. Do not begin to plot from dataIndex-iFilterDelay because the impsulse response and m_matOverlap do not match with the new filter anymore.
                    m_matDataFiltered.row(timeData.at(r).second.first).head(m_iMaxFilterLength) = timeData.at(r).second.second.segment(m_iMaxFilterLength,m_iMaxFilterLength);
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(iFilterDelay,iFilteredNumberCols-2*m_iMaxFilterLength) = timeData.at(r).second.second.segment(m_iMaxFilterLength,iFilteredNumberCols-2*m_iMaxFilterLength);
                }

                //Refresh the m_matOverlap with the new calculated filtered data.
                m_matOverlap.row(timeData.at(r).second.first) = timeData.at(r).second.second.tail(m_iMaxFilterLength);
            } else {
                //Handle middle data blocks
                //std::cout<<"Handle middle data block"<<std::endl;

                if(m_bDrawFilterFront) {
                    //Get the currently filtered data. This data has a delay of filterLength/2 in front and back.
                    RowVectorXd tempData = timeData.at(r).second.second;

                    //Perform the actual overlap add by adding the last filterlength data to the newly filtered one
                    tempData.head(m_iMaxFilterLength) += m_matOverlap.row(timeData.at(r).second.first);

                    //Write the newly calulated filtered data to the filter data matrix. Keep in mind that the current block also effect last part of the last block (begin at dataIndex-iFilterDelay).
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(dataIndex-iFilterDelay,iFilteredNumberCols-m_iMaxFilterLength) = tempData.head(iFilteredNumberCols-m_iMaxFilterLength);
                } else {
                    //Perform this else case everytime the filter was changed. Do not begin to plot from dataIndex-iFilterDelay because the impsulse response and m_matOverlap do not match with the new filter anymore.
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(dataIndex-iFilterDelay,m_iMaxFilterLength).setZero();// = timeData.at(r).second.second.segment(m_iMaxFilterLength,m_iMaxFilterLength);
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(dataIndex+iFilterDelay,iFilteredNumberCols-2*m_iMaxFilterLength) = timeData.at(r).second.second.segment(m_iMaxFilterLength,iFilteredNumberCols-2*m_iMaxFilterLength);
                }

                //Refresh the m_matOverlap with the new calculated filtered data.
                m_matOverlap.row(timeData.at(r).second.first) = timeData.at(r).second.second.tail(m_iMaxFilterLength);
            }
        }
    }

    m_bDrawFilterFront = true;

    //Fill filtered data with raw data if the channel was not filtered
    for(int i = 0; i<notFilterChannelIndex.size(); i++)
        m_matDataFiltered.row(notFilterChannelIndex.at(i)).segment(dataIndex,data.row(notFilterChannelIndex.at(i)).cols()) = data.row(notFilterChannelIndex.at(i));

    //std::cout<<"END RealTimeMultiSampleArrayModel::filterChannelsConcurrently"<<std::endl;
}
