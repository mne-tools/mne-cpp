//=============================================================================================================
/**
* @file     edf_raw_data.cpp
* @author   Simon Heinke <simon.heinke@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2019
*
* @section  LICENSE
*
* Copyright (C) 2019, Simon Heinke and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the EDFRawData class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "edf_raw_data.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QIODevice>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EDFINFOEXAMPLE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EDFRawData::EDFRawData(QIODevice* pDev, QObject *parent)
: QObject(parent),
  m_pDev(pDev),
  m_info(m_pDev)
{

}


//*************************************************************************************************************

EDFInfo EDFRawData::getInfo() const
{
    // return copy of own info
    return m_info;
}


//*************************************************************************************************************

QVector<QVector<float>> EDFRawData::readRawData() const
{
    // read whole file in data-record-sized portions (this is probably quite ineffective, better read bigger chunks)
    int sizeOfDataRecordInBytes = 0;
    for(const auto& signal : m_info.vSignals) {
        sizeOfDataRecordInBytes += signal.iNumberOfSamplesPerRecord * 2;  // 2 bytes per integer value, this might be different for bdf files
    }
    QVector<QByteArray> vRecords;
    for(int i = 0; i < m_info.iNumDataRecords; ++i) {
        vRecords.push_back(m_pDev->read(sizeOfDataRecordInBytes));
    }

    // we should have reached the end of the file
    if(m_pDev->pos() != m_pDev->size()) {
        qDebug() << "[EDFInfo::readRawData] Fatal: Number of bytes read is not equal to number of bytes in file!";
    }

    // translate data records into signals, start with empty vectors
    QVector<QVector<int>> signalValues;
    for(int i = 0; i < m_info.iNumSignals; ++i) {
        signalValues.append(QVector<int>());
    }
    // go through each record
    for(int recIdx = 0; recIdx < vRecords.size(); ++recIdx) {
        for(int sigIdx = 0; sigIdx < m_info.iNumSignals; ++sigIdx) {
            for(int sampIdx = 0; sampIdx < m_info.vSignals[sigIdx].iNumberOfSamplesPerRecord; ++sampIdx) {
                // factor 2 because of 2 byte representation, this is different for bdf files
                // we need the unary AND operation with '0x00ff' on the right side in order to prevent sign flipping through unintential interpretation as 2's complement integer.
                signalValues[sigIdx].append((vRecords[recIdx].at(sampIdx * 2 + 1) << 8) | (vRecords[recIdx].at(sampIdx * 2) & 0x00ff));
            }
        }
    }

    // basic sanity check: number of signal values read should be number of bytes divided by 2
    int numBytes = 0;
    for(const auto& r : vRecords) {
        numBytes += r.size();
    }
    int numSignalValues = 0;
    for(const auto& s : signalValues) {
        numSignalValues += s.size();
    }
    if(numSignalValues * 2 != numBytes) {
        qDebug() << "[EDFInfo::readRawData] Fatal: Divergence between total number of samples and read bytes";
    }

    // @TODO add scaling according to digitalMin, digitalMax, physicalMin, physicalMax ?
    QVector<QVector<float>> result;

    for(const auto& sv : signalValues) {
        QVector<float> temp;
        for(const int v : sv) {
            temp.append(static_cast<float>(v));
        }
        result.append(temp);
    }

    return result;
}
