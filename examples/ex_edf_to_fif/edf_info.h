//=============================================================================================================
/**
* @file     edf_info.h
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
* @brief    Contains the declaration of the EDFInfo class. When making EDF data compatible with fiff data, one
*           encounters a significant problem: fiff data does not support variable frequencies across channels.
*           To solve this problem, we differentiate between "Measurement Signals" and "Extra Signals".
*           Measurement signals are meant to represent the continous EEG data, extra signals are meant to represent
*           additional information, e.g. stimulus channels etc. Because the EEG data usually has the highest sampling
*           frequency, all channels that have the highest sampling frequency are interpreted as measurement signals.
*
*/

#ifndef EDF_INFO_H
#define EDF_INFO_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "edf_signal_info.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDateTime>
#include <QDebug>
#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QIODevice;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE EDFINFOEXAMPLE
//=============================================================================================================

namespace EDFINFOEXAMPLE
{

/**
* Enum that contains the LENGTHS of the header fields. Note that e.g. NUM_DATA_RECORDS = 8 does NOT mean that
* that there are 8 data records in the file, but rather that you need to read 8 bytes to obtain the number of records.
*/
enum EDFHEADERFIELDS {  // general fields for all signals
                        EDF_VERSION = 8,
                        LOCAL_PATIENT_INFO = 80,
                        LOCAL_RECORD_INFO = 80,
                        STARTDATE = 8,
                        STARTTIME = 8,
                        NUM_BYTES_USED_FOR_HEADER = 8,
                        HEADER_RESERVED = 44,
                        NUM_DATA_RECORDS = 8,
                        DURATION_DATA_RECORDS = 8,
                        NUM_SIGNALS = 4,
                        // fields for individual signals
                        SIG_LABEL = 16,
                        SIG_TRANSDUCER_TYPE = 80,
                        SIG_PHYSICAL_DIMENSION = 8,
                        SIG_PHYSICAL_MIN = 8,
                        SIG_PHYSICAL_MAX = 8,
                        SIG_DIGITAL_MIN = 8,
                        SIG_DIGITAL_MAX = 8,
                        SIG_PREFILTERING = 80,
                        SIG_NUM_SAMPLES_PER_DATA_RECORD = 8,
                        SIG_RESERVED = 32
                    };

//=============================================================================================================
/**
* DECLARE CLASS EDFInfo
*
* @brief The EDFInfo holds all relevant information for EDF files.
*/
class EDFInfo
{

public:
    //=========================================================================================================
    /**
    * Constructs an EDFInfo by parsing the header of the passed edf file pDev.
    */
    EDFInfo();

    //=========================================================================================================
    /**
    * Constructs an EDFInfo by parsing the passed IO device.
    */
    EDFInfo(QIODevice* pDev);

    //=========================================================================================================
    /**
    * Obtain textual representation of signal.
    *
    * @return Textual representation of signal.
    */
    QString getAsString() const;

    inline QVector<EDFSignalInfo> getAllSignalInfos() const;

    inline QVector<EDFSignalInfo> getMeasurementSignalInfos() const;

    inline int getNumberOfDataRecords() const;

    inline int getNumberOfSignals() const;

    inline int getSampleCount() const;

    inline int getNumSamplesPerRecord() const;

    inline int getNumberOfBytesInHeader() const;

    inline int getNumberOfBytesPerDataRecord() const;

private:
    // data fields for EDF header. The member order does NOT correlate with the position in the header.
    QString     m_sEDFVersionNo;
    QString     m_sLocalPatientIdentification;
    QString     m_sLocalRecordingIdentification;
    QDateTime   m_startDateTime;
    int         m_iNumBytesInHeader;
    int         m_iNumDataRecords;
    float       m_fDataRecordsDuration;
    int         m_iNumSignals;

    // convenience field, calculated by using the EDF fields
    int         m_iNumBytesPerDataRecord;


    // vector of all signals contained in file. We need to know the original order of signals when reading
    // raw data from data records, otherwise misalignments are inevitable.
    QVector<EDFSignalInfo> m_vAllSignals;
    // vector of all signals that contain continuous measurement data (i.e. signals that have the
    // maximum frequency). This is redundant, but very convenient.
    QVector<EDFSignalInfo>  m_vMeasSignals;
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QVector<EDFSignalInfo> EDFInfo::getAllSignalInfos() const {
    return m_vAllSignals;
}


//*************************************************************************************************************

inline QVector<EDFSignalInfo> EDFInfo::getMeasurementSignalInfos() const {
    return m_vMeasSignals;
}


//*************************************************************************************************************

inline int EDFInfo::getNumberOfDataRecords() const {
    return m_iNumDataRecords;
}


//*************************************************************************************************************

inline int EDFInfo::getNumberOfSignals() const {
    return m_iNumSignals;
}


//*************************************************************************************************************

inline int EDFInfo::getSampleCount() const {
    if(m_vMeasSignals.size()) {
        return m_vMeasSignals[0].getSampleCount();
    } else {
        qDebug() << "[EDFInfo::getSampleCount] Warning, no measurement signals, return -1 for sample count...";
        return -1;
    }
}


//*************************************************************************************************************

inline int EDFInfo::getNumSamplesPerRecord() const {
    if(m_vMeasSignals.size()) {
        return m_vMeasSignals[0].getNumberOfSamplesPerRecord();
    } else {
        qDebug() << "[EDFInfo::getNumSamplesPerRecord] Warning, no measurement signals, return -1 for record sample count...";
        return -1;
    }
}


//*************************************************************************************************************

inline int EDFInfo::getNumberOfBytesInHeader() const {
    return m_iNumBytesInHeader;
}


//*************************************************************************************************************

inline int EDFInfo::getNumberOfBytesPerDataRecord() const {
    return m_iNumBytesPerDataRecord;
}

} // NAMESPACE

#endif // EDF_INFO_H
