//=============================================================================================================
/**
* @file     datapackage.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the DataPackage class.
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

using namespace MNEBrowseRawQt;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DataPackage::DataPackage(MatrixXdR &originalRawData, int cutFront, int cutBack)
: m_iCutFront(cutFront)
, m_iCutBack(cutBack)
{
    if(originalRawData.rows() != 0 && originalRawData.cols() != 0) {
        setOrigRawData(originalRawData);
        cutOrigRawData(cutFront, cutBack);
    }

    //Init processed data with zero
    m_dataProcOriginal = MatrixXdR::Zero(m_dataRawOriginal.rows(), m_dataRawOriginal.cols());
    m_dataProcMapped = MatrixXdR::Zero(m_dataRawMapped.rows(), m_dataRawMapped.cols());
}


//*************************************************************************************************************

void DataPackage::setOrigRawData(MatrixXdR &originalRawData)
{
    //set orignal data
    m_dataRawOriginal = originalRawData;
}


//*************************************************************************************************************

void DataPackage::setOrigRawData(RowVectorXd &originalRawData, int row)
{
    if(originalRawData.cols() != m_dataRawOriginal.cols() || row >= m_dataRawOriginal.rows()){
        qDebug()<<"DataPackage::setOrigRawData - cannot set row data to m_dataRawOriginal";
        return;
    }

    //set orignal data at row row
    m_dataRawOriginal.row(row) = originalRawData;
}


//*************************************************************************************************************

void DataPackage::setOrigProcData(MatrixXdR &originalProcData)
{
    //set orignal processed data
    m_dataProcOriginal = originalProcData;
}


//*************************************************************************************************************

void DataPackage::setOrigProcData(RowVectorXd &originalProcData, int row)
{
    if(originalProcData.cols() != m_dataProcOriginal.cols() || row >= m_dataProcOriginal.rows()){
        qDebug()<<"DataPackage::setOrigProcData - cannot set row data to m_dataProcOriginal";
        return;
    }

    //set orignal data at row row
    m_dataProcOriginal.row(row) = originalProcData;
}


//*************************************************************************************************************

void DataPackage::cutOrigRawData(int cutFront, int cutBack)
{
    if(m_dataRawOriginal.cols()-cutFront-cutBack < 0 || cutFront>m_dataRawOriginal.cols()) {
        qDebug()<<"DataPackage::cutOrigRawData - cutFront or cutBack do not fit. Aborting mapping of m_dataRawOriginal.";
        return;
    }

    //Cut original data using block
    m_dataRawMapped = m_dataRawOriginal.block(0, cutFront, m_dataRawOriginal.rows(), m_dataRawOriginal.cols()-cutFront-cutBack);

    if(cutFront != m_iCutFront)
        m_iCutFront = cutFront;

    if(cutBack != m_iCutBack)
        m_iCutBack = cutBack;
}


//*************************************************************************************************************

void DataPackage::cutOrigProcData(int cutFront, int cutBack)
{
    if(m_dataProcOriginal.cols()-cutFront-cutBack < 0 || cutFront>m_dataProcOriginal.cols()) {
        qDebug()<<"DataPackage::cutOrigProcData - cutFront or cutBack do not fit. Aborting mapping of m_dataProcOriginal.";
        return;
    }

    //Cut original data using block
    m_dataProcMapped = m_dataProcOriginal.block(0, cutFront, m_dataProcOriginal.rows(), m_dataProcOriginal.cols()-cutFront-cutBack);

    if(cutFront != m_iCutFront)
        m_iCutFront = cutFront;

    if(cutBack != m_iCutBack)
        m_iCutBack = cutBack;
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
    cutOrigProcData(m_iCutFront, m_iCutBack);
}




