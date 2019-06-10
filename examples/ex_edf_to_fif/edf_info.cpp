//=============================================================================================================
/**
* @file     edf_info.cpp
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
* @brief    Definition of the EDFInfo class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "edf_info.h"

#include <fiff/fiff_ch_info.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QIODevice>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EDFINFOEXAMPLE;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EDFInfo::EDFInfo()
{

}


//*************************************************************************************************************

EDFInfo::EDFInfo(QIODevice* pDev)
{
    // simply parse header and fill datafields
    if(pDev->open(QIODevice::ReadOnly) == false) {
        qDebug() << "[EDFInfo::EDFInfo] Fatal: could not open device !";
        return;
    }

    // check if we are really at the start of the file
    if(pDev->pos() != 0) {
        qDebug() << "[EDFInfo::EDFInfo] Warning: device not at position zero, this will most probably crash...";
    }

    // general info, which is not dependent on individual signals
    m_sEDFVersionNo = QString::fromLatin1(pDev->read(EDF_VERSION)).trimmed();
    m_sLocalPatientIdentification = QString::fromLatin1(pDev->read(LOCAL_PATIENT_INFO)).trimmed();
    m_sLocalRecordingIdentification = QString::fromLatin1(pDev->read(LOCAL_RECORD_INFO)).trimmed();
    m_startDateTime.setDate(QDate::fromString(QString::fromLatin1(pDev->read(STARTDATE)), "dd.MM.yy"));
    // the format only contains two digits for the year, so we need to do a bit of timetraveling in order not to end up in the 1910's
    m_startDateTime = m_startDateTime.addYears(100);  // NOTE: this code might only be futureproof for the next 81 years.
    m_startDateTime.setTime(QTime::fromString(QString::fromLatin1(pDev->read(STARTTIME)), "hh.mm.ss"));
    m_iNumBytesInHeader = QString::fromLatin1(pDev->read(NUM_BYTES_USED_FOR_HEADER)).toInt();
    pDev->read(HEADER_RESERVED);  // next 44 bytes are unused
    m_iNumDataRecords = QString::fromLatin1(pDev->read(NUM_DATA_RECORDS)).toInt();
    m_fDataRecordsDuration = QString::fromLatin1(pDev->read(DURATION_DATA_RECORDS)).toFloat();
    m_iNumChannels = QString::fromLatin1(pDev->read(NUM_SIGNALS)).toInt();

    // use the order specified by EDF
    QVector<QString> labels;
    QVector<QString> transducers;
    QVector<QString> physicalDims;
    QVector<float> physicalMins;
    QVector<float> physicalMaxs;
    QVector<long> digitalMins;
    QVector<long> digitalMaxs;
    QVector<QString> prefilterings;
    QVector<long> numbersOfSamplesPerRecords;

    for(int i = 0; i < m_iNumChannels; ++i)
        labels.push_back(QString::fromLatin1(pDev->read(SIG_LABEL)).trimmed());
    for(int i = 0; i < m_iNumChannels; ++i)
        transducers.push_back(QString::fromLatin1(pDev->read(SIG_TRANSDUCER_TYPE)).trimmed());
    for(int i = 0; i < m_iNumChannels; ++i)
        physicalDims.push_back(QString::fromLatin1(pDev->read(SIG_PHYSICAL_DIMENSION)).trimmed());
    for(int i = 0; i < m_iNumChannels; ++i)
        physicalMins.push_back(QString::fromLatin1(pDev->read(SIG_PHYSICAL_MIN)).toFloat());
    for(int i = 0; i < m_iNumChannels; ++i)
        physicalMaxs.push_back(QString::fromLatin1(pDev->read(SIG_PHYSICAL_MAX)).toFloat());
    for(int i = 0; i < m_iNumChannels; ++i)
        digitalMins.push_back(QString::fromLatin1(pDev->read(SIG_DIGITAL_MIN)).toLong());
    for(int i = 0; i < m_iNumChannels; ++i)
        digitalMaxs.push_back(QString::fromLatin1(pDev->read(SIG_DIGITAL_MAX)).toLong());
    for(int i = 0; i < m_iNumChannels; ++i)
        prefilterings.push_back(QString::fromLatin1(pDev->read(SIG_PREFILTERING)).trimmed());
    for(int i = 0; i < m_iNumChannels; ++i)
        numbersOfSamplesPerRecords.push_back(QString::fromLatin1(pDev->read(SIG_NUM_SAMPLES_PER_DATA_RECORD)).toLong());
    for(int i = 0; i < m_iNumChannels; ++i)
        pDev->read(SIG_RESERVED);  // next 32 bytes are unused

    // store full list of signals in original order
    for(int i = 0; i < m_iNumChannels; ++i) {
        m_vAllChannels.push_back(EDFChannelInfo(i,
                                                labels[i],
                                                transducers[i],
                                                physicalDims[i],
                                                prefilterings[i],
                                                physicalMins[i],
                                                physicalMaxs[i],
                                                digitalMins[i],
                                                digitalMaxs[i],
                                                numbersOfSamplesPerRecords[i],
                                                numbersOfSamplesPerRecords[i] * m_iNumDataRecords,
                                                numbersOfSamplesPerRecords[i] / m_fDataRecordsDuration,
                                                false));
    }

    // we should have reached the end of the header
    if(pDev->pos() != m_iNumBytesInHeader) {
        qDebug() << "[EDFInfo::EDFInfo] Warning: Number of bytes read is not equal to number of bytes in header!"
                 << " This most certainly means that a misalignment occured and that all data fields contain unusable values.";
    }

    // calculate number of bytes per data record for later usage in read_raw function
    m_iNumBytesPerDataRecord = 0;
    for(const auto& chan : m_vAllChannels) {
        m_iNumBytesPerDataRecord += chan.getNumberOfSamplesPerRecord() * 2;  // 2 integer representation, this might be different for bdf files
    }

    // do post-processing: variable channel frequencies are not supported, take highest available frequency as main frequency.
    // use 'NumberOfSamplesPerRecord' field in order to avoid float comparisons (duration of data records is fixed)
    long iMaxNumberOfSamplesPerRecord = -1;
    for(const auto& chan : m_vAllChannels) {
        if(chan.getNumberOfSamplesPerRecord() > iMaxNumberOfSamplesPerRecord) {
            iMaxNumberOfSamplesPerRecord = chan.getNumberOfSamplesPerRecord();
        }
    }

    // remember which channels are measurement channels and which are not
    for(int i = 0; i < m_vAllChannels.size(); ++i) {
        if(m_vAllChannels[i].getNumberOfSamplesPerRecord() == iMaxNumberOfSamplesPerRecord) {
            // this is (probably) a measurement signal
            m_vAllChannels[i].setAsMeasurementChannel();
            m_vMeasChannels.push_back(m_vAllChannels[i]);
        } else {
            // this is an extra signal, e.g. stimulus. Dont have to do anything, default value for measurement flag is false.
        }
    }

    // tell user about extra signals
    if(m_vAllChannels.size() - m_vMeasChannels.size() > 0) {
        qDebug() << "[EDFInfo::EDFInfo] Found " << m_vAllChannels.size() - m_vMeasChannels.size() << " extra channels, which are: ";
        for(const auto& chan : m_vAllChannels) {
            if(chan.isMeasurementChannel() == false) {
                qDebug() << chan.getLabel();
            }
        }
    }

    // basic sanity checks for measurement signals:
    if(m_vMeasChannels.size()) {
        long numSamplesPerRecord = m_vMeasChannels[0].getNumberOfSamplesPerRecord();
        long numSamplesTotal = m_vMeasChannels[0].getSampleCount();
        for(const auto& measChan : m_vMeasChannels) {
            if(measChan.getNumberOfSamplesPerRecord() != numSamplesPerRecord || measChan.getSampleCount() != numSamplesTotal) {
                qDebug() << "[EDFInfo::EDFInfo] Warning, major inconsistency in measurement sample counts !";
            }
        }
    } else {
        qDebug() << "[EDFInfo::EDFInfo] No measurement signals listed. Something probably went wrong.";
    }
}


//*************************************************************************************************************

QString EDFInfo::getAsString() const
{
    QString result;
    result += "== EDF INFO START ==";
    result += "\nEDF Version Number: " + m_sEDFVersionNo;
    result += "\nLocal Patient Identification: " + m_sLocalPatientIdentification;
    result += "\nLocal Recording Identification: " + m_sLocalRecordingIdentification;
    result += "\nDate of Recording: " + m_startDateTime.date().toString("dd.MM.yyyy");
    result += "\nTime of Recording: " + m_startDateTime.time().toString("hh:mm:ss");
    result += "\nNumber of Bytes in the EDF Header: " + QString::number(m_iNumBytesInHeader);
    result += "\nNumber of Data Records: " + QString::number(m_iNumDataRecords);
    result += "\nDuration of each Data Record (in Seconds): " + QString::number(m_fDataRecordsDuration);
    result += "\nNumber of Signals in EDF File: " + QString::number(m_iNumChannels);

    result += "\n== MEASUREMENT SIGNALS ==";
    for(const auto& sig : m_vAllChannels) {
        if(sig.isMeasurementChannel()) {
            result += sig.getAsString();
        }
    }

    result += "\n== EXTRA SIGNALS ==";
    for(const auto& sig : m_vAllChannels) {
        if(sig.isMeasurementChannel() == false) {
            result += sig.getAsString();
        }
    }

    result += "\n== EDF INFO END ==";

    return result;
}


//*************************************************************************************************************

FiffInfo EDFInfo::toFiffInfo() const {
    FiffInfo result;

    // fiff_info_base members
    result.nchan = m_vMeasChannels.size();

    for(const auto& edfChan : m_vMeasChannels) {
        const FiffChInfo temp = edfChan.toFiffChInfo();
        result.chs.append(temp);
        result.ch_names.append(temp.ch_name);
    }

    // fiff_info members
    result.sfreq = getFrequency();


    return result;
}
