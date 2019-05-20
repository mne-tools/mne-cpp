//=============================================================================================================
/**
* @file     edf_signal_info.cpp
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
* @brief    Definition of the EDFSignalInfo class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "edf_signal_info.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


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
                             const float mfrequency)
: m_sLabel(label),
  m_sTransducerType(transducer),
  m_sPhysicalDimension(physicalDimension),
  m_sPrefiltering(prefiltering),
  m_fPhysicalMinimum(physicalMin),
  m_fPhysicalMaximum(physicalMax),
  m_iDigitalMinimum(digitalMin),
  m_iDigitalMaximum(digitalMax),
  m_iNumberOfSamplesPerRecord(numberOfSamplesPerRecord),
  m_iNumberOfSamplesTotal(numberOfSamplesTotal),
  m_frequency(mfrequency)
{

}


//*************************************************************************************************************

EDFSignalInfo::EDFSignalInfo(const EDFSignalInfo& other)
: m_sLabel(other.m_sLabel),
  m_sTransducerType(other.m_sTransducerType),
  m_sPhysicalDimension(other.m_sPhysicalDimension),
  m_sPrefiltering(other.m_sPrefiltering),
  m_fPhysicalMinimum(other.m_fPhysicalMinimum),
  m_fPhysicalMaximum(other.m_fPhysicalMaximum),
  m_iDigitalMinimum(other.m_iDigitalMinimum),
  m_iDigitalMaximum(other.m_iDigitalMaximum),
  m_iNumberOfSamplesPerRecord(other.m_iNumberOfSamplesPerRecord),
  m_iNumberOfSamplesTotal(other.m_iNumberOfSamplesTotal),
  m_frequency(other.m_frequency)
{

}


//*************************************************************************************************************

QString EDFSignalInfo::getAsString() const
{
    QString result;

    result += "\n== EDF SIGNAL INFO START ==";
    result += "\nChannel Label: " + m_sLabel;
    result += "\nTransducer Type: " + m_sTransducerType;
    result += "\nPhysical Dimension: " + m_sPhysicalDimension;
    result += "\nPrefiltering: " + m_sPrefiltering;
    result += "\nPhysical Minimum: " + QString::number(m_fPhysicalMinimum);
    result += "\nPhysical Maximum: " + QString::number(m_fPhysicalMaximum);
    result += "\nDigital Minimum: " + QString::number(m_iDigitalMinimum);
    result += "\nDigital Maximum: " + QString::number(m_iDigitalMaximum);
    result += "\nNumber of Samples per Record: " + QString::number(m_iNumberOfSamplesPerRecord);
    result += "\nTotal Number of Samples: " + QString::number(m_iNumberOfSamplesTotal);
    result += "\nChannel Frequency: " + QString::number(m_frequency);

    return result;
}
