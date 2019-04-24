//=============================================================================================================
/**
* @file     edfinfo.cpp
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

#include "edfinfo.h"


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

EDFSignalInfo::EDFSignalInfo()
{

}


//*************************************************************************************************************

EDFSignalInfo::EDFSignalInfo(const QString label,
                             const QString transducer,
                             const QString physicalDimension,
                             const QString prefiltering,
                             const float physicalMin,
                             const float physicalMax,
                             const long digitalMin,
                             const long digitalMax,
                             const long numberOfSamplesPerRecord,
                             const long numberOfSamplesTotal,
                             const float mfrequency,
                             QObject *parent)
: QObject(parent),
  sLabel(label),
  sTransducerType(transducer),
  sPhysicalDimension(physicalDimension),
  sPrefiltering(prefiltering),
  physicalMinimum(physicalMin),
  physicalMaximum(physicalMax),
  digitalMinimum(digitalMin),
  digitalMaximum(digitalMax),
  iNumberOfSamplesPerRecord(numberOfSamplesPerRecord),
  iNumberOfSamplesTotal(numberOfSamplesTotal),
  frequency(mfrequency)
{

}


//*************************************************************************************************************

EDFSignalInfo::EDFSignalInfo(const EDFSignalInfo& other)
: sLabel(other.sLabel),
  sTransducerType(other.sTransducerType),
  sPhysicalDimension(other.sPhysicalDimension),
  sPrefiltering(other.sPrefiltering),
  physicalMinimum(other.physicalMinimum),
  physicalMaximum(other.physicalMaximum),
  digitalMinimum(other.digitalMinimum),
  digitalMaximum(other.digitalMaximum),
  iNumberOfSamplesPerRecord(other.iNumberOfSamplesPerRecord),
  iNumberOfSamplesTotal(other.iNumberOfSamplesTotal),
  frequency(other.frequency)
{

}


//*************************************************************************************************************

QString EDFSignalInfo::getAsString() const
{
    QString result;

    result += "\n======================";
    result += "\nChannel Label: " + sLabel;
    result += "\nTransducer Type: " + sTransducerType;
    result += "\nPhysical Dimension: " + sPhysicalDimension;
    result += "\nPrefiltering: " + sPrefiltering;
    result += "\nPhysical Minimum: " + QString::number(physicalMinimum);
    result += "\nPhysical Maximum: " + QString::number(physicalMaximum);
    result += "\nDigital Minimum: " + QString::number(digitalMinimum);
    result += "\nDigital Maximum: " + QString::number(digitalMaximum);
    result += "\nNumber of Samples per Record: " + QString::number(iNumberOfSamplesPerRecord);
    result += "\nTotal Number of Samples: " + QString::number(iNumberOfSamplesTotal);
    result += "\nChannel Frequency: " + QString::number(frequency);

    return result;
}


//*************************************************************************************************************

EDFInfo::EDFInfo(QIODevice* pDev, QObject *parent)
: QObject(parent)
{
    // simply parse header and fill datafields
    if (!pDev->open(QIODevice::ReadOnly)) {
        qDebug() << "[EDFInfo::EDFInfo] Fatal: could not open device !";
        return;
    }

    // general info, which is not independent of the signals
    sEDFVersionNo = QString::fromLatin1(pDev->read(8)).trimmed();
    sLocalPatientIdentification = QString::fromLatin1(pDev->read(80)).trimmed();
    sLocalRecordingIdentification = QString::fromLatin1(pDev->read(80)).trimmed();
    startDateTime.setDate(QDate::fromString(QString::fromLatin1(pDev->read(8)), "dd.MM.yy"));
    // original paper on EDF was first published in 1992 (this code is future-proof only for the next 73 years)
    if(startDateTime.date().year() <= 1990) {
        // timetravel into the 2000's
        startDateTime = startDateTime.addYears(100);
    }
    startDateTime.setTime(QTime::fromString(QString::fromLatin1(pDev->read(8)), "hh.mm.ss"));
    iNumBytesInHeader = QString::fromLatin1(pDev->read(8)).toInt();
    pDev->read(44);  // next 44 bytes are unused
    iNumDataRecords = QString::fromLatin1(pDev->read(8)).toInt();
    fDataRecordsDuration = QString::fromLatin1(pDev->read(8)).toFloat();
    iNumSignals = QString::fromLatin1(pDev->read(4)).toInt();

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

    for(int i = 0; i < iNumSignals; ++i)
        labels.push_back(QString::fromLatin1(pDev->read(16)).trimmed());
    for(int i = 0; i < iNumSignals; ++i)
        transducers.push_back(QString::fromLatin1(pDev->read(80)).trimmed());
    for(int i = 0; i < iNumSignals; ++i)
        physicalDims.push_back(QString::fromLatin1(pDev->read(8)).trimmed());
    for(int i = 0; i < iNumSignals; ++i)
        physicalMins.push_back(QString::fromLatin1(pDev->read(8)).toFloat());
    for(int i = 0; i < iNumSignals; ++i)
        physicalMaxs.push_back(QString::fromLatin1(pDev->read(8)).toFloat());
    for(int i = 0; i < iNumSignals; ++i)
        digitalMins.push_back(QString::fromLatin1(pDev->read(8)).toLong());
    for(int i = 0; i < iNumSignals; ++i)
        digitalMaxs.push_back(QString::fromLatin1(pDev->read(8)).toLong());
    for(int i = 0; i < iNumSignals; ++i)
        prefilterings.push_back(QString::fromLatin1(pDev->read(80)).trimmed());
    for(int i = 0; i < iNumSignals; ++i)
        numbersOfSamplesPerRecords.push_back(QString::fromLatin1(pDev->read(8)).toLong());
    for(int i = 0; i < iNumSignals; ++i)
        pDev->read(32);  // next 32 bytes are unused

    for(int i = 0; i < iNumSignals; ++i) {
        vSignals.push_back(EDFSignalInfo(labels[i],
                                         transducers[i],
                                         physicalDims[i],
                                         prefilterings[i],
                                         physicalMins[i],
                                         physicalMaxs[i],
                                         digitalMins[i],
                                         digitalMaxs[i],
                                         numbersOfSamplesPerRecords[i],
                                         numbersOfSamplesPerRecords[i] * iNumDataRecords,
                                         numbersOfSamplesPerRecords[i] / fDataRecordsDuration));
    }

    // we should have reached the end of the header
    if(pDev->pos() != iNumBytesInHeader) {
        qDebug() << "FATAL: Number of bytes read is not equal to number of bytes in header!";
    }

    this->pDev = pDev;
}


//*************************************************************************************************************

QString EDFInfo::getAsString() const
{
    QString result;
    result += "======================";
    result += "\nEDF Version Number: " + sEDFVersionNo;
    result += "\nLocal Patient Identification: " + sLocalPatientIdentification;
    result += "\nLocal Recording Identification: " + sLocalRecordingIdentification;
    result += "\nDate of Recording: " + startDateTime.date().toString("dd.MM.yyyy");
    result += "\nTime of Recording: " + startDateTime.time().toString("hh:mm:ss");
    result += "\nNumber of Bytes in the EDF Header: " + QString::number(iNumBytesInHeader);
    result += "\nNumber of Data Records: " + QString::number(iNumDataRecords);
    result += "\nDuration of each Data Record (in Seconds): " + QString::number(fDataRecordsDuration);
    result += "\nNumber of Signals in EDF File: " + QString::number(iNumSignals);

    for(const auto& signal : vSignals) {
        result += signal.getAsString();
    }

    result += "\n======================";

    return result;
}


QVector<QVector<float>> EDFInfo::readRawData()
{
    // read whole file data-record-sized portions (this is probably quite ineffective, better read bigger chunks)
    int sizeOfDataRecordInBytes = 0;
    for(const auto& signal : vSignals) {
        sizeOfDataRecordInBytes += signal.iNumberOfSamplesPerRecord * 2;  // 2 bytes per integer value, this might be different for bdf files
    }
    QVector<QByteArray> vRecords;
    for(int i = 0; i < iNumDataRecords; ++i) {
        vRecords.push_back(pDev->read(sizeOfDataRecordInBytes));
    }

    // we should have reached the end of the file
    if(pDev->pos() != pDev->size()) {
        qDebug() << "FATAL: Number of bytes read is not equal to number of bytes in file!";
    }

    // translate data records into signals, start with empty series
    QVector<QVector<int>> signalValues;
    for(int i = 0; i < iNumSignals; ++i) {
        signalValues.append(QVector<int>());
    }
    // go through each record
    for(int recIdx = 0; recIdx < vRecords.size(); ++recIdx) {
        for(int sigIdx = 0; sigIdx < iNumSignals; ++sigIdx) {
            for(int sampIdx = 0; sampIdx < vSignals[sigIdx].iNumberOfSamplesPerRecord; ++sampIdx) {
                // factor 2 because of 2 byte representation, this is different for bdf files
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
        qDebug() << "FATAL Divergence between total number of samples and read bytes";
    }

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
