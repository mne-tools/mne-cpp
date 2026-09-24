//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     edf_ch_info.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Simon Heinke <simon.heinke@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @date     May, 2019
* @version  1.0
* @brief    Definition of the EDFChannelInfo class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "edf_ch_info.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EDF2FIFF;
using namespace FIFFLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EDFChannelInfo::EDFChannelInfo(const int channelNumber,
                               const QString label,
                               const QString transducer,
                               const QString physicalDimension,
                               const QString prefiltering,
                               const float physicalMin,
                               const float physicalMax,
                               const long digitalMin,
                               const long digitalMax,
                               const long numberOfSamplesPerRecord,
                               const long numberOfSamplesTotal,
                               const float frequency,
                               const bool isMeasurement)
: m_iChanNo(channelNumber),
  m_sLabel(label),
  m_sTransducerType(transducer),
  m_sPhysicalDimension(physicalDimension),
  m_sPrefiltering(prefiltering),
  m_fPhysicalMinimum(physicalMin),
  m_fPhysicalMaximum(physicalMax),
  m_iDigitalMinimum(digitalMin),
  m_iDigitalMaximum(digitalMax),
  m_iNumberOfSamplesPerRecord(numberOfSamplesPerRecord),
  m_iNumberOfSamplesTotal(numberOfSamplesTotal),
  m_frequency(frequency),
  m_bIsMeas(isMeasurement)
{

}

//*************************************************************************************************************

QString EDFChannelInfo::getAsString() const
{
    QString sDescription;
    sDescription += "\n== EDF CHANNEL INFO START ==";
    sDescription += "\nChannel FsLabel: " + m_sLabel;
    sDescription += "\nTransducer Type: " + m_sTransducerType;
    sDescription += "\nPhysical Dimension: " + m_sPhysicalDimension;
    sDescription += "\nPrefiltering: " + m_sPrefiltering;
    sDescription += "\nPhysical Minimum: " + QString::number(static_cast<double>(m_fPhysicalMinimum));
    sDescription += "\nPhysical Maximum: " + QString::number(static_cast<double>(m_fPhysicalMaximum));
    sDescription += "\nDigital Minimum: " + QString::number(m_iDigitalMinimum);
    sDescription += "\nDigital Maximum: " + QString::number(m_iDigitalMaximum);
    sDescription += "\nNumber of Samples per Record: " + QString::number(m_iNumberOfSamplesPerRecord);
    sDescription += "\nTotal Number of Samples: " + QString::number(m_iNumberOfSamplesTotal);
    sDescription += "\nChannel Frequency: " + QString::number(static_cast<double>(m_frequency));

    return sDescription;
}

//*************************************************************************************************************

void EDFChannelInfo::setAsMeasurementChannel()
{
    m_bIsMeas = true;
}

//*************************************************************************************************************

FiffChInfo EDFChannelInfo::toFiffChInfo() const
{
    FiffChInfo fiffChInfo;

    fiffChInfo.scanNo = m_iChanNo;
    fiffChInfo.logNo = m_iChanNo;  // simply take index from file organisation as logical channel number, this guarantees uniqueness.
    // check a few basic cases for channel kind:
    QString sLabelUpper = m_sLabel.toUpper();
    if(m_bIsMeas == false) {
        if(sLabelUpper.contains("STIM"))
            fiffChInfo.kind = FIFFV_STIM_CH;
        else
            fiffChInfo.kind = FIFFV_MISC_CH;  // declare as miscellaneous, cannot be wrong.
    }
    else {
        if(sLabelUpper.contains("EEG"))
            fiffChInfo.kind = FIFFV_EEG_CH;
        else if(sLabelUpper.contains("MEG"))
            fiffChInfo.kind = FIFFV_MEG_CH;
        else if(sLabelUpper.contains("ECG"))
            fiffChInfo.kind = FIFFV_ECG_CH;
        else if(sLabelUpper.contains("EOG"))
            fiffChInfo.kind = FIFFV_EOG_CH;
        else
            fiffChInfo.kind = FIFFV_MISC_CH;  // declare as miscellaneous, cannot be wrong.
    }

    // check a few basic cases for physical dimension / unit:
    QString sUnitUpper = m_sPhysicalDimension.toUpper();
    if(sUnitUpper.endsWith("V") || sUnitUpper.endsWith("VOLT")) {
        fiffChInfo.unit = FIFF_UNIT_V;
        if(sUnitUpper.startsWith("U") || sUnitUpper.startsWith("MICRO"))
            fiffChInfo.unit_mul = FIFF_UNITM_MU;
        else
            fiffChInfo.unit_mul = FIFF_UNITM_NONE;  // seems to be the best solution
    }
    else {
        fiffChInfo.unit = FIFF_UNIT_NONE;
        fiffChInfo.unit_mul = FIFF_UNITM_NONE;
    }

    // EDF has a different scaling, which is encapsulated by the interface
    fiffChInfo.cal = 1.0f;
    fiffChInfo.range = 1.0f;

    /* Mattis version: do scaling by adjusting range and cal */
    /*
    fiffChInfo.unit = FIFF_UNIT_V;
    fiffChInfo.unit_mul = FIFF_UNITM_NONE;

    if(m_sPhysicalDimension.toUpper().contains("UV"))
        fiffChInfo.unit_mul = FIFF_UNITM_MU;  // This will be changed by adjusting the calibration instead, see below

    fiffChInfo.range = 0.0f;
    fiffChInfo.cal = 0.0f;

    fiffChInfo.cal = -m_fPhysicalMinimum;
    fiffChInfo.cal += m_fPhysicalMaximum;

    fiffChInfo.range = -m_iDigitalMinimum;
    fiffChInfo.range += m_iDigitalMaximum;

    // Final adjustment to the calibration:
    if(fiffChInfo.unit_mul == FIFF_UNITM_MU) {
        fiffChInfo.cal = 1e-6 * fiffChInfo.cal / fiffChInfo.range;
        fiffChInfo.unit_mul = FIFF_UNITM_NONE;
    }
    else {
        fiffChInfo.cal = fiffChInfo.cal / fiffChInfo.range;
    }
    fiffChInfo.range = 1.0f;
    */

    // simply take signal label as channel name
    fiffChInfo.ch_name = m_sLabel;

    return fiffChInfo;
}
