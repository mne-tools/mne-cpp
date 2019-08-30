//=============================================================================================================
/**
* @file     datapackage.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the DataPackage class.
*
*/

#ifndef DATAPACKAGE_H
#define DATAPACKAGE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filteroperator.h"
#include "types.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TMSIPLUGIN
//=============================================================================================================

namespace MNEBROWSE
{


//=============================================================================================================
/**
* DataPackage...
*
* @brief The DataPackage class provides central place to hold all program relevant data.
*/
class DataPackage
{
public:
    //=========================================================================================================
    /**
    * Constructs a DataPackage.
    *
    * @param originalRawData the original raw data
    * @param originalRawTime the original raw time data
    * @param cutFront the amount to be cutted from orignal data from the front
    * @param cutBack the amount to be cutted from orignal data from the back
    */
    DataPackage(const MatrixXdR &originalRawData=MatrixXdR(0,0), const MatrixXdR &originalRawTime=MatrixXdR(0,0), int cutFront=0, int cutBack=0);

    //=========================================================================================================
    /**
    * Sets the original long raw data matrix read from the file. Optional: Also cuts the original raw data to a specific size.
    *
    * @param originalRawData the original raw data
    * @param cutFront the amount to be cutted from orignal data from the front
    * @param cutBack the amount to be cutted from orignal data from the back
    */
    void setOrigRawData(const MatrixXdR &originalRawData, int cutFront=0, int cutBack=0);

    //=========================================================================================================
    /**
    * Sets a row of the original long raw data matrix read from the file. Optional: Also cuts the original raw data to a specific size.
    *
    * @param originalRawData the original raw data in form of a row
    * @param row the row number
    * @param cutFront the amount to be cutted from orignal data from the front
    * @param cutBack the amount to be cutted from orignal data from the back
    */
    void setOrigRawData(const RowVectorXd &originalRawData, int row, int cutFront=0, int cutBack=0);

    //=========================================================================================================
    /**
    * Sets the original long processed data matrix calculated. Optional: Also cuts the original processed data to a specific size.
    *
    * @param originalProcData the original processed data
    * @param cutFront the amount to be cutted from orignal data from the front
    * @param cutBack the amount to be cutted from orignal data from the back
    */
    void setOrigProcData(const MatrixXdR &originalProcData, int cutFront=0, int cutBack=0);

    //=========================================================================================================
    /**
    * Sets the mapped processed data matrix calculated.
    *
    * @param originalProcData the original processed data
    * @param cutFront the amount to be cutted from orignal data from the front
    * @param cutBack the amount to be cutted from orignal data from the back
    */
    void setMappedProcData(const MatrixXdR &originalProcData, int cutFront, int cutBack);


    //=========================================================================================================
    /**
    * Sets a row of the original long processed data matrix read from the file. Optional: Also cuts the original processed data to a specific size.
    *
    * @param originalProcData the original processed data in form of a row
    * @param row the row number
    * @param cutFront the amount to be cutted from orignal data from the front
    * @param cutBack the amount to be cutted from orignal data from the back
    */
    void setOrigProcData(const RowVectorXd &originalProcData, int row, int cutFront=0, int cutBack=0);

    //=========================================================================================================
    /**
    * Sets a row of the mapped processed data matrix read from the file.
    *
    * @param originalProcData the original processed data in form of a row
    * @param row the row number
    * @param cutFront the amount to be cutted from orignal data from the front
    * @param cutBack the amount to be cutted from orignal data from the back
    */
    void setMappedProcData(const RowVectorXd &originalProcData, int row, int cutFront, int cutBack);

    //=========================================================================================================
    /**
    * Returns the original full raw data.
    *
    * @return the mapped data
    */
    const MatrixXdR & dataRawOrig();

    //=========================================================================================================
    /**
    * Returns the raw data mapped i.e. to fit the current window size.
    *
    * @return the mapped data
    */
    const MatrixXdR & dataRaw();

    //=========================================================================================================
    /**
    * Returns the processed original full data.
    *
    * @return the mapped data
    */
    const MatrixXdR & dataProcOrig();

    //=========================================================================================================
    /**
    * Returns the processed data mapped to i.e. fit the current window size.
    *
    * @return the mapped data
    */
    const MatrixXdR & dataProc();

    //=========================================================================================================
    /**
    * Returns the mean of the processed mapped data.
    *
    * @param row the row index
    * @return the mean value
    */
    double dataProcMean(int row);

    //=========================================================================================================
    /**
    * Returns the mean of the raw mapped data.
    *
    * @param row the row index
    * @return the mean value
    */
    double dataRawMean(int row);

    //=========================================================================================================
    /**
    * FilterOperator::FilterOperator
    *
    * @param channelNumber the channel to be filtered
    * @param filter operator to use
    * @param useRawData flag if filter should be used on already processed data or on raw data
    * @return a row vector truncated by numberFilterTaps/2 at front and end
    */
    void applyFFTFilter(int channelNumber, QSharedPointer<FilterOperator> filter, bool useRawData = true);

private:
    //=========================================================================================================
    /**
    * Cuts the original data to a specific size and returns the result.
    *
    * @param originalData input matrix data
    * @param cutFront the amount to be cutted from orignal data from the front
    * @param cutBack the amount to be cutted from orignal data from the back
    */
    MatrixXdR cutData(const MatrixXdR &originalData, int cutFront=0, int cutBack=0);

    //=========================================================================================================
    /**
    * Cuts the original data to a specific size and there+fore sets the mapped raw data.
    *
    * @param originalData input row data
    * @param cutFront the amount to be cutted from orignal data from the front
    * @param cutBack the amount to be cutted from orignal data from the back
    */
    RowVectorXd cutData(const RowVectorXd &originalData, int cutFront=0, int cutBack=0);

    //=========================================================================================================
    /**
    * calculateMean calculates the mean for all data stored in the data matrix
    *
    * @param data the data matrix
    * @return the means of each row of the input data matrix
    */
    VectorXd calculateMatMean(const MatrixXd &dataMat);

    //=========================================================================================================
    /**
    * calculateMean calculates the mean for all data stored in the data row
    *
    * @param data the data row
    * @return the means of each row of the input data matrix
    */
    double calculateRowMean(const VectorXd &dataRow);

    //Time data
    MatrixXdR   m_timeRawMapped;        /**< The mapped/cut time data */
    MatrixXdR   m_timeRawOriginal;      /**< the original time data */

    //Raw data
    MatrixXdR   m_dataRawMapped;        /**< The mapped/cut raw data */
    MatrixXdR   m_dataRawOriginal;      /**< The original raw data */
    VectorXd    m_dataRawMean;          /**< The mean of the mapped/cut raw data */

    //Processed data
    MatrixXdR   m_dataProcOriginal;     /**< The mapped/cut processed/filtered data */
    MatrixXdR   m_dataProcMapped;       /**< The original processed/filtered data */
    VectorXd    m_dataProcMean;         /**< The mean of the mapped/cut processed/filtered data */

    //Cutting parameters
    int m_iCutFrontRaw;                 /**< The last used cut front value of the raw data */
    int m_iCutBackRaw;                  /**< The last used cut back value of the raw data*/
    int m_iCutFrontProc;                /**< The last used cut front value of the raw data */
    int m_iCutBackProc;                 /**< The last used cut back value of the raw data*/
};

} // NAMESPACE

#endif // DATAPACKAGE_H
