//=============================================================================================================
/**
* @file     edf_ch_info.h
* @author   Simon Heinke <simon.heinke@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2019
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
* @brief    Contains the declaration of the EDFChannelInfo class.
*           Note that "Channel" is synonymous to "Signal" in the context of EDF files.
*
*/

#ifndef EDF_CH_INFO_H
#define EDF_CH_INFO_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE EDFINFOEXAMPLE
//=============================================================================================================

namespace EDFINFOEXAMPLE
{


//=============================================================================================================
/**
* DECLARE CLASS EDFChannelInfo
*
* @brief The EDFChannelInfo is a simple container class that holds all relevant information for EDF signals / channels
*/
class EDFChannelInfo
{

public:

    //=========================================================================================================
    /**
    * Default constructor.
    */
    EDFChannelInfo();

    //=========================================================================================================
    /**
    * Constructor just copies values.
    */
    EDFChannelInfo(const QString label,
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
                  const bool bIsMeas);

    //=========================================================================================================
    /**
    * Copy constructor.
    */
    EDFChannelInfo(const EDFChannelInfo& other);

    //=========================================================================================================
    /**
    * Obtain textual representation of channel info.
    *
    * @return Textual representation of channel info.
    */
    QString getAsString() const;

    inline QString getLabel() const;

    inline int getNumberOfSamplesPerRecord() const;

    inline float getFrequency() const;

    inline long getSampleCount() const;

    inline bool isMeasurementChannel() const;

    void setAsMeasurementChannel();

    inline long digitalMin() const;

    inline long digitalMax() const;

    inline float physicalMin() const;

    inline float physicalMax() const;

private:
    // data fields for EDF channels. The member order does NOT correlate with the position in the header.
    QString m_sLabel;                   // e.g. EEG Fpz-Cz or Body temp
    QString m_sTransducerType;          // e.g. AgAgCl electrode
    QString m_sPhysicalDimension;       // e.g. uV or degreeC
    QString m_sPrefiltering;            // e.g. HP: 0.1Hz LP: 75Hz
    float m_fPhysicalMinimum;           // e.g. -500 or 34
    float m_fPhysicalMaximum;           // e.g. -500 or 34
    long m_iDigitalMinimum;             // e.g. -2048
    long m_iDigitalMaximum;             // e.g. 2047
    long m_iNumberOfSamplesPerRecord;   // e.g. 250 or 1

    // convenience fields, calculated using the above fields
    long m_iNumberOfSamplesTotal;
    float m_frequency;

    // flag for differentiating between measurement channels and extra channels
    bool m_bIsMeas;
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QString EDFChannelInfo::getLabel() const {
    return m_sLabel;
}


//*************************************************************************************************************

inline int EDFChannelInfo::getNumberOfSamplesPerRecord() const {
    return m_iNumberOfSamplesPerRecord;
}


//*************************************************************************************************************

inline float EDFChannelInfo::getFrequency() const {
    return m_frequency;
}


//*************************************************************************************************************

inline long EDFChannelInfo::getSampleCount() const {
    return m_iNumberOfSamplesTotal;
}


//*************************************************************************************************************

inline bool EDFChannelInfo::isMeasurementChannel() const {
    return m_bIsMeas;
}


//*************************************************************************************************************

inline long EDFChannelInfo::digitalMin() const {
    return m_iDigitalMinimum;
}


//*************************************************************************************************************

inline long EDFChannelInfo::digitalMax() const {
    return m_iDigitalMaximum;
}


//*************************************************************************************************************

inline float EDFChannelInfo::physicalMin() const {
    return m_fPhysicalMinimum;
}


//*************************************************************************************************************

inline float EDFChannelInfo::physicalMax() const {
    return m_fPhysicalMaximum;
}

} // NAMESPACE

#endif // EDF_CH_INFO_H
