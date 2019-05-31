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
using namespace Eigen;


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

MatrixXf EDFRawData::read_raw_segment(int startSampleIdx, int endSampleIdx) const
{
    // basic sanity checks for indices:
    if(startSampleIdx < 0 || startSampleIdx >= m_info.getSampleCount() || endSampleIdx < 0 || endSampleIdx >= m_info.getSampleCount()) {
        qDebug() << "[EDFRawData::read_raw_segment] An index seems to be out of bounds:";
        qDebug() << "Start: " << startSampleIdx << " End: " << endSampleIdx;
        return MatrixXf();  // return empty matrix
    }

    int numSamples = endSampleIdx - startSampleIdx;
    if(numSamples <= 0) {
        qDebug() << "[EDFRawData::read_raw_segment] Timeslice is empty or negative";
        qDebug() << "Start: " << startSampleIdx << " End: " << endSampleIdx;
        return MatrixXf();  // return empty matrix
    }

    // calculate which is the first needed data record, the relative first sample number and the number of data records we need to read
    int firstDataRecordIdx = (startSampleIdx - (startSampleIdx % m_info.getNumSamplesPerRecord())) / m_info.getNumSamplesPerRecord();
    int relativeFirstSampleIdx = startSampleIdx % m_info.getNumSamplesPerRecord();
    int numDataRecords = static_cast<int>(std::ceil(static_cast<float>(numSamples + relativeFirstSampleIdx) / m_info.getNumSamplesPerRecord()));

    // because of the data-record-wise organisation of EDF, we need to patch together the channels later
    QVector<QVector<int>> vRawPatches(m_info.getNumberOfAllChannels(), QVector<int>());

    // finally start reading the file, put file pointer to beginning of first needed data record
    m_pDev->seek(m_info.getNumberOfBytesInHeader() + firstDataRecordIdx * m_info.getNumberOfBytesPerDataRecord());

    // since measurement channels make up the bulk part of most edf files, we can simply read the data records as a whole
    QVector<QByteArray> vRecords;
    for(int i = 0; i < numDataRecords; ++i) {
        vRecords.push_back(m_pDev->read(m_info.getNumberOfBytesPerDataRecord()));
    }

    // again, extra channels are mostly insignificant compared to measurement channels, so we just filter them out later
    for(int recIdx = 0; recIdx < vRecords.size(); ++recIdx) {  // go through each record
        for(int chanIdx = 0; chanIdx < m_info.getNumberOfAllChannels(); ++chanIdx) {  // go through all channels
            const EDFChannelInfo sig = m_info.getAllChannelInfos()[chanIdx];
            QVector<int> rawPatch(sig.getNumberOfSamplesPerRecord());
            for(int sampIdx = 0; sampIdx < sig.getNumberOfSamplesPerRecord(); ++sampIdx) {
                // factor 2 because of 2 byte integer representation, this might be different for bdf files
                // we need the unary AND operation with '0x00ff' on the right side in order to prevent sign flipping through unintential interpretation as 2's complement integer.
                rawPatch[sampIdx] = (vRecords[recIdx].at(sampIdx * 2 + 1) << 8) | (vRecords[recIdx].at(sampIdx * 2) & 0x00ff);
            }
            vRawPatches[chanIdx] += rawPatch;  // append raw patch
        }
    }

    // post-processing: filter out extra channels, start in the back to avoid ugly index offsets
    QVector<EDFChannelInfo> vAllChannels = m_info.getAllChannelInfos();
    for(int chanIdx = vRawPatches.size() - 1; chanIdx >= 0; --chanIdx) {
        if(vAllChannels[chanIdx].isMeasurementChannel() == false) {
            vRawPatches.remove(chanIdx);
        }
    }

    // quick sanity check
    QVector<EDFChannelInfo> vMeasChannels = m_info.getMeasurementChannelInfos();
    if(vRawPatches.size() != vMeasChannels.size()) {
       qDebug() << "[EDFRawData::read_raw_segment] Dimension mismatch for filtered raw patches";
    }

    // prepare result matrix
    MatrixXf result(vRawPatches.size(), numSamples);

    // scale raw values according to digital und physical min/max and copy scaled values into result matrix while omitting unwanted samples in the beginning and end
    for(int measChanIdx = 0; measChanIdx < vRawPatches.size(); ++measChanIdx) {
        vRawPatches[measChanIdx] = vRawPatches[measChanIdx].mid(relativeFirstSampleIdx);  // cut away unwanted samples in the beginning
        const EDFChannelInfo chan = vMeasChannels[measChanIdx];
        for(int sampIdx = 0; sampIdx < numSamples; ++sampIdx) {  // by only letting sampIdx go so far, we automatically exclude unwanted samples in the end
            result(measChanIdx, sampIdx) = static_cast<float>(vRawPatches[measChanIdx][sampIdx] - chan.digitalMin()) / (chan.digitalMax() - chan.digitalMin()) * (chan.physicalMax() - chan.physicalMin()) + chan.physicalMin();
        }
    }

    return result;
}


//*************************************************************************************************************

MatrixXf EDFRawData::read_raw_segment(float startTimePoint, float endTimePoint) const {
    float frequency = m_info.getFrequency();
    int startSampleIdx = static_cast<int>(std::floor(startTimePoint * frequency));
    int endSampleIdx = static_cast<int>(std::ceil(endTimePoint * frequency));

    return read_raw_segment(startSampleIdx, endSampleIdx);
}
