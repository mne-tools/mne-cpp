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

EDFInfo::EDFInfo()
{

}


//*************************************************************************************************************

EDFInfo::EDFInfo(QIODevice* pDev)
{
    // simply parse header and fill datafields
    if(!pDev->open(QIODevice::ReadOnly)) {
        qDebug() << "[EDFInfo::EDFInfo] Fatal: could not open device !";
        return;
    }

    // check if we are really at the start of the file
    if(pDev->pos() != 0) {
        qDebug() << "[EDFInfo::EDFInfo] Warning: device not at position zero, this will most probably crash...";
    }

    // general info, which is not independent of individual signals
    m_sEDFVersionNo = QString::fromLatin1(pDev->read(8)).trimmed();
    m_sLocalPatientIdentification = QString::fromLatin1(pDev->read(80)).trimmed();
    m_sLocalRecordingIdentification = QString::fromLatin1(pDev->read(80)).trimmed();
    m_startDateTime.setDate(QDate::fromString(QString::fromLatin1(pDev->read(8)), "dd.MM.yy"));
    // the format only contains two digits for the year, so we need to do a bit of timetraveling in order not to end up in the 1910's
    m_startDateTime = m_startDateTime.addYears(100);  // NOTE: this code might only be futureproof for the next 81 years.
    m_startDateTime.setTime(QTime::fromString(QString::fromLatin1(pDev->read(8)), "hh.mm.ss"));
    m_iNumBytesInHeader = QString::fromLatin1(pDev->read(8)).toInt();
    pDev->read(44);  // next 44 bytes are unused
    m_iNumDataRecords = QString::fromLatin1(pDev->read(8)).toInt();
    m_fDataRecordsDuration = QString::fromLatin1(pDev->read(8)).toFloat();
    m_iNumSignals = QString::fromLatin1(pDev->read(4)).toInt();

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

    for(int i = 0; i < m_iNumSignals; ++i)
        labels.push_back(QString::fromLatin1(pDev->read(16)).trimmed());
    for(int i = 0; i < m_iNumSignals; ++i)
        transducers.push_back(QString::fromLatin1(pDev->read(80)).trimmed());
    for(int i = 0; i < m_iNumSignals; ++i)
        physicalDims.push_back(QString::fromLatin1(pDev->read(8)).trimmed());
    for(int i = 0; i < m_iNumSignals; ++i)
        physicalMins.push_back(QString::fromLatin1(pDev->read(8)).toFloat());
    for(int i = 0; i < m_iNumSignals; ++i)
        physicalMaxs.push_back(QString::fromLatin1(pDev->read(8)).toFloat());
    for(int i = 0; i < m_iNumSignals; ++i)
        digitalMins.push_back(QString::fromLatin1(pDev->read(8)).toLong());
    for(int i = 0; i < m_iNumSignals; ++i)
        digitalMaxs.push_back(QString::fromLatin1(pDev->read(8)).toLong());
    for(int i = 0; i < m_iNumSignals; ++i)
        prefilterings.push_back(QString::fromLatin1(pDev->read(80)).trimmed());
    for(int i = 0; i < m_iNumSignals; ++i)
        numbersOfSamplesPerRecords.push_back(QString::fromLatin1(pDev->read(8)).toLong());
    for(int i = 0; i < m_iNumSignals; ++i)
        pDev->read(32);  // next 32 bytes are unused

    // create temporary vector of EDFSignalInfos
    QVector<EDFSignalInfo> vTempSignals;
    for(int i = 0; i < m_iNumSignals; ++i) {
        vTempSignals.push_back(EDFSignalInfo(labels[i],
                                             transducers[i],
                                             physicalDims[i],
                                             prefilterings[i],
                                             physicalMins[i],
                                             physicalMaxs[i],
                                             digitalMins[i],
                                             digitalMaxs[i],
                                             numbersOfSamplesPerRecords[i],
                                             numbersOfSamplesPerRecords[i] * m_iNumDataRecords,
                                             numbersOfSamplesPerRecords[i] / m_fDataRecordsDuration));
    }

    // we should have reached the end of the header
    if(pDev->pos() != m_iNumBytesInHeader) {
        qDebug() << "[EDFInfo::EDFInfo] Warning: Number of bytes read is not equal to number of bytes in header!"
                 << " This most certainly means that a misalignment occured and that all data fields contain unusable values.";
    }

    // do post-processing: variable channel frequencies are not supported, take highest available frequency as main frequency.
    // use 'NumberOfSamplesPerRecord' field in order to avoid float comparisons
    long iMaxNumberOfSamplesPerRecord = -1;
    for(const auto& s : vTempSignals) {
        if(s.getNumberOfSamplesPerRecord() > iMaxNumberOfSamplesPerRecord) {
            iMaxNumberOfSamplesPerRecord = s.getNumberOfSamplesPerRecord();
        }
    }

    // sort vector of temporary EDFSignalInfos into two groups: measurementSignals and extraSignals
    for(const auto& signal : vTempSignals) {
        if(signal.getNumberOfSamplesPerRecord() == iMaxNumberOfSamplesPerRecord) {
            // this is (probably) a measurement signal
            m_vMeasurementSignals.push_back(signal);
        } else {
            // this is an extra signal, e.g. stimulus
            m_vExtraSignals.push_back(signal);
        }
    }

    // tell user about extra signals
    if(m_vExtraSignals.size()) {
        qDebug() << "[EDFInfo::EDFInfo] Found " << m_vExtraSignals.size() << " extra channels, which are: ";
        for(const auto& signal : m_vExtraSignals) {
            qDebug() << signal.getLabel();
        }
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
    result += "\nNumber of Signals in EDF File: " + QString::number(m_iNumSignals);

    result += "\n== MEASUREMENT SIGNALS ==";
    for(const auto& signal : m_vMeasurementSignals) {
        result += signal.getAsString();
    }

    result += "\n== EXTRA SIGNALS ==";
    for(const auto& signal : m_vExtraSignals) {
        result += signal.getAsString();
    }

    result += "\n== EDF INFO END ==";

    return result;
}
