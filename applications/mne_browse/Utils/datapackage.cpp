//=============================================================================================================
/**
 * @file     datapackage.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     November, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the DataPackage class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "datapackage.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DataPackage::DataPackage(const MatrixXdR &originalRawData, const MatrixXdR &originalRawTime, int cutFront, int cutBack)
: m_iCutFrontRaw(cutFront)
, m_iCutBackRaw(cutBack)
, m_iCutFrontProc(cutFront)
, m_iCutBackProc(cutBack)
{
    if(originalRawData.rows() != 0 && originalRawData.cols() != 0) {
        setOrigRawData(originalRawData, m_iCutFrontRaw, m_iCutBackRaw);

        m_timeRawOriginal = originalRawTime;
        m_timeRawMapped = cutData(m_timeRawOriginal, m_iCutFrontRaw, m_iCutBackRaw);
    }

    //Init processed data with zero and double the multiple integer of 2 (because of zero padding)
    int exp = ceil(MNEMath::log2(originalRawTime.cols()));
    int length = pow(2, exp+1);

    m_dataProcOriginal = MatrixXdR::Zero(m_dataRawOriginal.rows(), length);
    m_dataProcMapped = MatrixXdR::Zero(m_dataRawMapped.rows(), m_dataRawMapped.cols());

    //Init mean data
    m_dataRawMean = calculateMatMean(m_dataRawMapped);
    m_dataProcMean = calculateMatMean(m_dataProcMapped);
}


//*************************************************************************************************************

void DataPackage::setOrigRawData(const MatrixXdR &originalRawData, int cutFront, int cutBack)
{
    //set orignal data
    m_dataRawOriginal = originalRawData;

    //Cut data
    m_dataRawMapped = cutData(m_dataRawOriginal, cutFront, cutBack);

    if(cutFront != m_iCutFrontRaw)
        m_iCutFrontRaw = cutFront;

    if(cutBack != m_iCutBackRaw)
        m_iCutBackRaw = cutBack;

    //Calculate mean
    m_dataRawMean = calculateMatMean(m_dataRawMapped);
}


//*************************************************************************************************************

void DataPackage::setOrigRawData(const RowVectorXd &originalRawData, int row, int cutFront, int cutBack)
{
    if(originalRawData.cols() != m_dataRawOriginal.cols() || row >= m_dataRawOriginal.rows()){
        qDebug()<<"DataPackage::setOrigRawData - cannot set row data to m_dataRawOriginal";
        return;
    }

    //set orignal data at row row
    m_dataRawOriginal.row(row) = originalRawData;

    //Cut data
    m_dataRawMapped.row(row) = cutData(m_dataRawOriginal, cutFront, cutBack);

    if(cutFront != m_iCutFrontRaw)
        m_iCutFrontRaw = cutFront;

    if(cutBack != m_iCutBackRaw)
        m_iCutBackRaw = cutBack;

    //Calculate mean
    m_dataRawMean(row) = calculateRowMean(m_dataRawMapped.row(row));
}


//*************************************************************************************************************

void DataPackage::setOrigProcData(const MatrixXdR &originalProcData, int cutFront, int cutBack)
{
    //set orignal processed data
    m_dataProcOriginal = originalProcData;

    //Cut data
    m_dataProcMapped = cutData(m_dataProcOriginal, cutFront, cutBack);

    if(cutFront != m_iCutFrontProc)
        m_iCutFrontProc = cutFront;

    if(cutBack != m_iCutBackProc)
        m_iCutBackProc = cutBack;

    //Calculate mean
    m_dataProcMean = calculateMatMean(m_dataProcMapped);
}


//*************************************************************************************************************

void DataPackage::setMappedProcData(const MatrixXdR &originalProcData, int cutFront, int cutBack)
{
    //Cut data
    m_dataProcMapped = cutData(originalProcData, cutFront, cutBack);

    if(cutFront != m_iCutFrontProc)
        m_iCutFrontProc = cutFront;

    if(cutBack != m_iCutBackProc)
        m_iCutBackProc = cutBack;

    //Calculate mean
    m_dataProcMean = calculateMatMean(m_dataProcMapped);
}


//*************************************************************************************************************

void DataPackage::setOrigProcData(const RowVectorXd &originalProcData, int row, int cutFront, int cutBack)
{
    if(originalProcData.cols() != m_dataProcOriginal.cols() || row >= m_dataProcOriginal.rows()){
        qDebug()<<"DataPackage::setOrigProcData - cannot set row data to m_dataProcOriginal";
        return;
    }

    //set orignal data at row row
    m_dataProcOriginal.row(row) = originalProcData;

    //Cut data
    m_dataProcMapped.row(row) = cutData(originalProcData, cutFront, cutBack);

    if(cutFront != m_iCutFrontProc)
        m_iCutFrontProc = cutFront;

    if(cutBack != m_iCutBackProc)
        m_iCutBackProc = cutBack;

    //Calculate mean
    m_dataProcMean(row) = calculateRowMean(m_dataProcMapped.row(row));
}


//*************************************************************************************************************

void DataPackage::setMappedProcData(const RowVectorXd &originalProcData, int row, int cutFront, int cutBack)
{
    if(originalProcData.cols()-cutFront-cutBack != m_dataProcMapped.cols() || row >= m_dataProcMapped.rows()){
        qDebug()<<"DataPackage::setMappedProcData - cannot set row data to m_dataProcOriginal";
        return;
    }

    //Cut data
    m_dataProcMapped.row(row) = cutData(originalProcData, cutFront, cutBack);

    if(cutFront != m_iCutFrontProc)
        m_iCutFrontProc = cutFront;

    if(cutBack != m_iCutBackProc)
        m_iCutBackProc = cutBack;

    //Calculate mean
    m_dataProcMean(row) = calculateRowMean(m_dataProcMapped.row(row));
}

//*************************************************************************************************************

const MatrixXdR & DataPackage::dataRawOrig()
{
    return m_dataRawOriginal;
}


//*************************************************************************************************************

const MatrixXdR & DataPackage::dataProcOrig()
{
    return m_dataProcOriginal;
}


//*************************************************************************************************************

const MatrixXdR & DataPackage::dataRaw()
{
    return m_dataRawMapped;
}


//*************************************************************************************************************

const MatrixXdR & DataPackage::dataProc()
{
    return m_dataProcMapped;
}


//*************************************************************************************************************

double DataPackage::dataProcMean(int row)
{
    if(row>=m_dataProcMean.rows())
        return 0;

    return m_dataProcMean(row);
}


//*************************************************************************************************************

double DataPackage::dataRawMean(int row)
{
    if(row>=m_dataRawMean.rows())
        return 0;

    return m_dataRawMean(row);
}


//*************************************************************************************************************

void DataPackage::applyFFTFilter(int channelNumber, QSharedPointer<FilterOperator> filter, bool useRawData)
{
    if(channelNumber >= m_dataRawOriginal.rows()){
        qDebug()<<"DataPackage::applyFFTFilter - channel number out of range.";
        return;
    }

    if(useRawData)
        m_dataProcOriginal.row(channelNumber) = filter->applyFFTFilter(m_dataRawOriginal.row(channelNumber)).eval();
    else
        m_dataProcOriginal.row(channelNumber) = filter->applyFFTFilter(m_dataProcOriginal.row(channelNumber)).eval();

    //Cut filtered m_dataProcOriginal
    m_dataProcMapped = cutData(m_dataProcOriginal, m_iCutFrontProc, m_iCutBackProc);

    //Calculate mean
    m_dataProcMean(channelNumber) = calculateRowMean(m_dataProcMapped);
}


//*************************************************************************************************************

MatrixXdR DataPackage::cutData(const MatrixXdR &originalData, int cutFront, int cutBack)
{
    if(originalData.cols()-cutFront-cutBack < 0 || cutFront>originalData.cols()) {
        qDebug()<<"DataPackage::cutData - cutFront or cutBack do not fit. Aborting mapping and returning original data.";
        MatrixXdR returnMat = originalData;
        return returnMat;
    }

    //Cut original data using block
    return (MatrixXdR)originalData.block(0, cutFront, originalData.rows(), originalData.cols()-cutFront-cutBack);
}


//*************************************************************************************************************

RowVectorXd DataPackage::cutData(const RowVectorXd &originalData, int cutFront, int cutBack)
{
    if(originalData.cols()-cutFront-cutBack < 0 || cutFront>originalData.cols()) {
        qDebug()<<"DataPackage::cutData - cutFront or cutBack do not fit. Aborting mapping and returning original data.";
        RowVectorXd returnVec = originalData;
        return returnVec;
    }

    //Cut original data using segment
    return (RowVectorXd)originalData.segment(cutFront, originalData.cols()-cutFront-cutBack);
}


//*************************************************************************************************************

VectorXd DataPackage::calculateMatMean(const MatrixXd &dataMat)
{
    VectorXd channelMeans(dataMat.rows());

    for(int i = 0; i<channelMeans.rows(); i++)
        channelMeans[i] = dataMat.row(i).mean();

    return channelMeans;
}


//*************************************************************************************************************

double DataPackage::calculateRowMean(const VectorXd &dataRow)
{
    return dataRow.mean();
}





