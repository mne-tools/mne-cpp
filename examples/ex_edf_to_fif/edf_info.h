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
* @brief    Contains the declaration of the EDFInfo class.
*
*/

#ifndef EDF_INFO_H
#define EDF_INFO_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDateTime>
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


//=============================================================================================================
/**
* DECLARE CLASS EDFSignalInfo
*
* @brief The EDFSignalInfo holds all relevant information for EDF signals.
*/
class EDFSignalInfo
{

public:

    //=========================================================================================================
    /**
    * Default constructor.
    */
    EDFSignalInfo();

    //=========================================================================================================
    /**
    * Constructor just copies values.
    */
    EDFSignalInfo(const QString label,
                  const QString transducer,
                  const QString physicalDimension,
                  const QString prefiltering,
                  const float physicalMin,
                  const float physicalMax,
                  const long digitalMin,
                  const long digitalMax,
                  const long numberOfSamplesPerRecord,
                  const long numberOfSamplesTotal,
                  const float mfrequency);

    //=========================================================================================================
    /**
    * Copy constructor.
    */
    EDFSignalInfo(const EDFSignalInfo& other);

    //=========================================================================================================
    /**
    * Obtain textual representation of signal.
    *
    * @return Textual representation of signal.
    */
    QString getAsString() const;

    // data fields for EDF signals. The member order does not correlate with the position in the header.
    QString sLabel;                 // e.g. EEG Fpz-Cz or Body temp
    QString sTransducerType;        // e.g. AgAgCl electrode
    QString sPhysicalDimension;     // e.g. uV or degreeC
    QString sPrefiltering;          // e.g. HP: 0.1Hz LP: 75Hz
    float physicalMinimum;          // e.g. -500 or 34
    float physicalMaximum;          // e.g. -500 or 34
    long digitalMinimum;            // e.g. -2048
    long digitalMaximum;            // e.g. 2047
    long iNumberOfSamplesPerRecord; // e.g. 250 or 1

    // convenience fields, calculated using EDF data fields
    long iNumberOfSamplesTotal;
    float frequency;
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

    // data fields for EDF header. The member order does not correlate with the position in the header.
    QString     sEDFVersionNo;
    QString     sLocalPatientIdentification;
    QString     sLocalRecordingIdentification;
    QDateTime   startDateTime;
    int         iNumBytesInHeader;
    int         iNumDataRecords;
    float       fDataRecordsDuration;
    int         iNumSignals;

    // vector of all signals contained in the EDF file
    QVector<EDFSignalInfo>  vSignals;
};

} // NAMESPACE

#endif // EDF_INFO_H
