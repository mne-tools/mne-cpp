//=============================================================================================================
/**
* @file     edf_raw_data.h
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
* @brief    Contains the declaration of the EDFRawData class.
*
*/

#ifndef EDF_RAW_DATA_H
#define EDF_RAW_DATA_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "edf_info.h"

#include <fiff/fiff_raw_data.h>

//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QVector>

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QIODevice;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE EDF2FIFF
//=============================================================================================================

namespace EDF2FIFF
{

//=============================================================================================================
/**
* DECLARE CLASS EDFRawData
*
* @brief The EDFRawData is the top level container class for EDF data.
*/
class EDFRawData : public QObject
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
    * @brief EDFRawData Constructor for EDFRawData
    * @param[in] pDev Pointer to a QIODevice.
    * @param[in] fScaleFactor Raw value scaling factor.
    * @param[in] parent Parent object.
    */
    EDFRawData(QIODevice* pDev, float fScaleFactor = 1e6, QObject *parent = nullptr);

    //=========================================================================================================
    /**
    * @brief getInfo Returns an EDFInfo object that holds most of the metadata.
    *
    * @return EDFInfo object.
    */
    EDFInfo getInfo() const;

    //=========================================================================================================
    /**
    * @brief read_raw_segment Reads a timeslice of data.
    * @param[in] startSampleIdx First sample index of timeslice.
    * @param[in] endSampleIdx Last sample index of timeslice (exclusive).
    *
    * @return An Eigen matrix that holds the timeslice.
    */
    Eigen::MatrixXf read_raw_segment(int iStartSampleIdx, int iEndSampleIdx) const;

    //=========================================================================================================
    /**
    * @brief read_raw_segment Reads a timeslice of data. This function simply converts the passed timepoints
    *        into sample indices by multiplying them with the sampling frequency.
    * @param[in] startTimePoint Start of timeslice in seconds.
    * @param[in] endTimePoint End of timeslice in seconds.
    *
    * @return An Eigen matrix that holds the timeslice.
    */
    Eigen::MatrixXf read_raw_segment(float fStartTimePoint, float fEndTimePoint) const;

    //=========================================================================================================
    /**
    * @brief toFiffRawData Converts the EDFRawData into a FiffRawData.
    *
    * @return A FiffRawData that represents the EDFRawData in the best possible way.
    */
    FIFFLIB::FiffRawData toFiffRawData() const;

signals:

public slots:

private:
    QIODevice* m_pDev;      /** The device that is reflected by this EDFRawData object. */
    float m_fScaleFactor;   /** Raw value scaling factor. */
    EDFInfo m_edfInfo;      /** EDF info that holds all the relevant information. */
};

} // NAMESPACE

#endif // EDF_RAW_DATA_H
