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

using namespace EDF2FIFF;
using namespace FIFFLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EDFRawData::EDFRawData(QIODevice* pDev,
                       float fScaleFactor,
                       QObject *parent)
: QObject(parent),
  m_pDev(pDev),
  m_fScaleFactor(fScaleFactor),
  m_edfInfo(m_pDev)
{

}


//*************************************************************************************************************

EDFInfo EDFRawData::getInfo() const
{
    // return copy of own info
    return m_edfInfo;
}


//*************************************************************************************************************

MatrixXf EDFRawData::read_raw_segment(int iStartSampleIdx, int iEndSampleIdx) const
{
    // basic sanity checks for indices:
    if(iStartSampleIdx < 0 || iStartSampleIdx >= m_edfInfo.getSampleCount() || iEndSampleIdx < 0 || iEndSampleIdx > m_edfInfo.getSampleCount()) {
        qDebug() << "[EDFRawData::read_raw_segment] An index seems to be out of bounds:";
        qDebug() << "Start: " << iStartSampleIdx << " End: " << iEndSampleIdx;
        return MatrixXf();  // return empty matrix
    }

    int iNumSamples = iEndSampleIdx - iStartSampleIdx;
    if(iNumSamples <= 0) {
        qDebug() << "[EDFRawData::read_raw_segment] Timeslice is empty or negative";
        qDebug() << "Start: " << iStartSampleIdx << " End: " << iEndSampleIdx;
        return MatrixXf();  // return empty matrix
    }

    // print what segment is being read
    qDebug() << "Reading " << iStartSampleIdx << " ... " << iEndSampleIdx << "  =   " << iStartSampleIdx / m_edfInfo.getFrequency() << " ... " << iEndSampleIdx / m_edfInfo.getFrequency() << " secs...";

    // calculate which is the first needed data record, the relative first sample number and the number of data records we need to read
    int iFirstDataRecordIdx = (iStartSampleIdx - (iStartSampleIdx % m_edfInfo.getNumSamplesPerRecord())) / m_edfInfo.getNumSamplesPerRecord();
    int iRelativeFirstSampleIdx = iStartSampleIdx % m_edfInfo.getNumSamplesPerRecord();
    int iNumDataRecords = static_cast<int>(std::ceil(static_cast<float>(iNumSamples + iRelativeFirstSampleIdx) / m_edfInfo.getNumSamplesPerRecord()));

    // because of the data-record-wise organisation of EDF, we need to patch together the channels later
    QVector<QVector<int>> vRawPatches(m_edfInfo.getNumberOfAllChannels(), QVector<int>());

    // finally start reading the file, put file pointer to beginning of first needed data record
    m_pDev->seek(m_edfInfo.getNumberOfBytesInHeader() + iFirstDataRecordIdx * m_edfInfo.getNumberOfBytesPerDataRecord());

    // since measurement channels make up the bulk part of most edf files, we can simply read the data records as a whole
    QVector<QByteArray> vRecords;
    for(int i = 0; i < iNumDataRecords; ++i) {
        vRecords.push_back(m_pDev->read(m_edfInfo.getNumberOfBytesPerDataRecord()));
    }

    // again, extra channels are mostly insignificant compared to measurement channels, so we just filter them out later
    for(int iRecIdx = 0; iRecIdx < vRecords.size(); ++iRecIdx) {  // go through each record
        int iRecordSampIdx = 0;  // this is the sample idx counter for the records
        for(int iChanIdx = 0; iChanIdx < m_edfInfo.getNumberOfAllChannels(); ++iChanIdx) {  // go through all channels
            const EDFChannelInfo sig = m_edfInfo.getAllChannelInfos()[iChanIdx];
            QVector<int> rawPatch(sig.getNumberOfSamplesPerRecord());
            for(int iTemporarySampIdx = iRecordSampIdx; iTemporarySampIdx < iRecordSampIdx + sig.getNumberOfSamplesPerRecord(); ++iTemporarySampIdx) {  // we need a temporary sample index in order to handle the channel-dependent offsets
                // factor 2 because of 2 byte integer representation, this might be different for bdf files
                // we need the unary AND operation with '0x00ff' on the right side in order to prevent sign flipping through unintential interpretation as 2's complement integer.
                rawPatch[iTemporarySampIdx - iRecordSampIdx] = (vRecords[iRecIdx].at(iTemporarySampIdx * 2 + 1) << 8) | (vRecords[iRecIdx].at(iTemporarySampIdx * 2) & 0x00ff);
            }
            iRecordSampIdx += sig.getNumberOfSamplesPerRecord();
            vRawPatches[iChanIdx] += rawPatch;  // append raw patch
        }
    }

    // post-processing: filter out extra channels, start in the back to avoid ugly index offsets
    QVector<EDFChannelInfo> vAllChannels = m_edfInfo.getAllChannelInfos();
    for(int iChanIdx = vRawPatches.size() - 1; iChanIdx >= 0; --iChanIdx) {
        if(vAllChannels[iChanIdx].isMeasurementChannel() == false) {
            vRawPatches.remove(iChanIdx);
        }
    }

    // quick sanity check
    QVector<EDFChannelInfo> vMeasChannels = m_edfInfo.getMeasurementChannelInfos();
    if(vRawPatches.size() != vMeasChannels.size()) {
       qDebug() << "[EDFRawData::read_raw_segment] Dimension mismatch for filtered raw patches";
    }

    // prepare result matrix
    MatrixXf result(vRawPatches.size(), iNumSamples);

    // scale raw values according to digital und physical min/max and copy scaled values into result matrix while omitting unwanted samples in the beginning and end
    for(int iMeasChanIdx = 0; iMeasChanIdx < vRawPatches.size(); ++iMeasChanIdx) {
        vRawPatches[iMeasChanIdx] = vRawPatches[iMeasChanIdx].mid(iRelativeFirstSampleIdx);  // cut away unwanted samples in the beginning
        const EDFChannelInfo chan = vMeasChannels[iMeasChanIdx];
        for(int iSampIdx = 0; iSampIdx < iNumSamples; ++iSampIdx) {  // by only letting sampIdx go so far, we automatically exclude unwanted samples in the end
            result(iMeasChanIdx, iSampIdx) = static_cast<float>(vRawPatches[iMeasChanIdx][iSampIdx] - chan.digitalMin()) / (chan.digitalMax() - chan.digitalMin()) * (chan.physicalMax() - chan.physicalMin()) + chan.physicalMin();
            if(chan.isMeasurementChannel()) {
                // probably uV values, need to scale them with raw value scaling factor
                result(iMeasChanIdx, iSampIdx) = result(iMeasChanIdx, iSampIdx) / m_fScaleFactor;
            }
        }
    }

    /* Mattis version: do scaling by adjusting range and cal in the conversion from EDF to Fiff. */
    /*
    // copy scaled values into result matrix while omitting unwanted samples in the beginning and end
    for(int iMeasChanIdx = 0; iMeasChanIdx < vRawPatches.size(); ++iMeasChanIdx) {
        vRawPatches[iMeasChanIdx] = vRawPatches[iMeasChanIdx].mid(iRelativeFirstSampleIdx);  // cut away unwanted samples in the beginning
        const EDFChannelInfo chan = vMeasChannels[iMeasChanIdx];
        for(int iSampIdx = 0; iSampIdx < iNumSamples; ++iSampIdx) {  // by only letting sampIdx go so far, we automatically exclude unwanted samples in the end
            result(iMeasChanIdx, iSampIdx) = vRawPatches[iMeasChanIdx][iSampIdx];
        }
    }
    */

    return result;
}


//*************************************************************************************************************

MatrixXf EDFRawData::read_raw_segment(float fStartTimePoint, float fEndTimePoint) const
{
    float fFrequency = m_edfInfo.getFrequency();
    int iStartSampleIdx = static_cast<int>(std::floor(fStartTimePoint * fFrequency));
    int iEndSampleIdx = static_cast<int>(std::ceil(fEndTimePoint * fFrequency));

    return read_raw_segment(iStartSampleIdx, iEndSampleIdx);
}


//*************************************************************************************************************

FiffRawData EDFRawData::toFiffRawData() const
{
    FiffRawData fiffRawData;

    fiffRawData.info = m_edfInfo.toFiffInfo();
    fiffRawData.first_samp = 0;  // EDF files always start at zero
    fiffRawData.last_samp = m_edfInfo.getSampleCount();

    // copy calibrations:
    RowVectorXd cals(fiffRawData.info.nchan);
    for(int i = 0; i < fiffRawData.info.chs.size(); ++i) {
        cals[i] = static_cast<double>(fiffRawData.info.chs[i].cal);
    }
    fiffRawData.cals = cals;

    return fiffRawData;
}
