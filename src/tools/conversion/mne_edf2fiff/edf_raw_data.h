//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     edf_raw_data.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Simon Heinke <simon.heinke@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @date     April, 2019
* @version  1.0
* @brief    Contains the declaration of the EDFRawData class.
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
