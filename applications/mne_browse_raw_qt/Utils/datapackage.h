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
// DEFINE NAMESPACE TMSIPlugin
//=============================================================================================================

namespace MNEBrowseRawQt
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
    * @param originalRawData the original data
    * @param cutFront the amount to be cutted from orignal data from the front
    * @param cutBack the amount to be cutted from orignal data from the back
    */
    DataPackage(MatrixXdR &originalRawData=MatrixXdR(0,0), int cutFront=0, int cutBack=0);

    //=========================================================================================================
    /**
    * Sets the original long raw data matrix read from the file. Optional: Also cuts the original raw data to a specific size.
    *
    * @param originalRawData the original raw data
    */
    void setOrigRawData(MatrixXdR &originalRawData);

    //=========================================================================================================
    /**
    * Sets a row of the original long raw data matrix read from the file. Optional: Also cuts the original raw data to a specific size.
    *
    * @param originalRawData the original raw data in form of a row
    * @param row the row number
    */
    void setOrigRawData(RowVectorXd &originalRawData, int row);

    //=========================================================================================================
    /**
    * Sets the original long processed data matrix calculated. Optional: Also cuts the original processed data to a specific size.
    *
    * @param originalProcData the original processed data
    */
    void setOrigProcData(MatrixXdR &originalProcData);

    //=========================================================================================================
    /**
    * Sets a row of the original long processed data matrix read from the file. Optional: Also cuts the original processed data to a specific size.
    *
    * @param originalProcData the original raw data in form of a row
    * @param row the row number
    */
    void setOrigProcData(RowVectorXd &originalProcData, int row);

    //=========================================================================================================
    /**
    * Cuts the original data to a specific size and therfore sets the mapped raw data.
    *
    * @param cutFront the amount to be cutted from orignal data from the front
    * @param cutBack the amount to be cutted from orignal data from the back
    */
    void cutOrigRawData(int cutFront=0, int cutBack=0);

    //=========================================================================================================
    /**
    * Cuts the original processed data to a specific size and therfore sets the mapped processed data.
    *
    * @param cutFront the amount to be cutted from orignal data from the front
    * @param cutBack the amount to be cutted from orignal data from the back
    */
    void cutOrigProcData(int cutFront=0, int cutBack=0);

    //=========================================================================================================
    /**
    * Returns the original full raw data.
    *
    * @return the mapped data
    */
    const MatrixXdR & dataRawOrig();

    //=========================================================================================================
    /**
    * Returns the raw data mapped to i.e. fit the current window size.
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
    * FilterOperator::FilterOperator
    *
    * @param channelNumber the channel to be filtered
    * @param filter operator to use
    * @param useRawData flag if filter should be used on already processed data or on raw data
    * @return a row vector truncated by numberFilterTaps/2 at front and end
    */
    void applyFFTFilter(int channelNumber, QSharedPointer<FilterOperator> filter, bool useRawData = true);

private:
    MatrixXdR m_dataRawMapped;
    MatrixXdR m_dataRawOriginal;

    MatrixXdR m_dataProcOriginal;
    MatrixXdR m_dataProcMapped;

    int m_iCutFront;
    int m_iCutBack;
};

} // NAMESPACE

#endif // DATAPACKAGE_H
