//=============================================================================================================
/**
* @file     calcmetric.h
* @author   Louis Eichhorst <louis.eichhorst@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Louis Eichhorst and Matti Hamalainen. All rights reserved.
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
* @brief     CalcMetric class declaration.
*
*/

#ifndef CALCMETRIC_H
#define CALCMETRIC_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QFuture>
#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>


//=============================================================================================================
/**
* DECLARE CLASS CalcMetric
*
* @brief Brief calculates all necessairy measurements for the detection algorithm
*/

class CalcMetric
{

public:
    typedef QSharedPointer<CalcMetric> SPtr;            /**< Shared pointer type for CalcMetric. */
    typedef QSharedPointer<const CalcMetric> ConstSPtr; /**< Const shared pointer type for CalcMetric. */

    //=========================================================================================================
    /**
    * Constructs a CalcMetric object.
    */
    CalcMetric();
    //=========================================================================================================
    /**
    * Handles new input data
    *
    * @param [in] input matrix containing the newest dataset.
    */
    void setData(Eigen::MatrixXd input);

    //=========================================================================================================
    /**
    * Handles calculation of measurements and multithreading.
    *
    * @param [in] input matrix containing the newest dataset.
    * @param [in] dim embedding dimension of fuzzy entropy.
    * @param [in] r width of fuzzy exponential function.
    * @param [in] n step of fuzzy exponential function.
    */
    void calcAll(Eigen::MatrixXd input, int dim, double r, double n);

    //=========================================================================================================
    /**
    * Calculates peak-to-peak-magnitude
    *
    */
    void calcP2P();

    //=========================================================================================================
    /**
    * Calculates kurtosis
    *
    * @param [in] start index of the channel where calculation starts.
    * @param [in] start index of the channel where calculation ends.
    */
    void calcKurtosis(int start, int end);

    //=========================================================================================================
    /**
    * Called whenever a seizure is suspected to take place. Calculates the Fuzzy Entropy for every channel in the list.
    *
    * @param [in] dim embedding dimension of fuzzy entropy.
    * @param [in] r width of fuzzy exponential function.
    * @param [in] n step of fuzzy exponential function.
    * @param [in] checkChs list of channels in which a seizure is suspected.
    * @param [out] returns a matrix with the Fuzzy Entropy for all channels.
    */
    Eigen::VectorXd onSeizureDetection(int dim, double r, double n, QList<int> checkChs);

    //=========================================================================================================
    /**
    * Returns the the value m_dvecKurtosis.
    *
    * @param [out] contains the current Kurtosis values.
    */
    Eigen::VectorXd getKurtosis();

    //=========================================================================================================
    /**
    * Returns the the value m_dvecP2P.
    *
    * @param [out] contains the current peak-to-peak-magnitude values.
    */
    Eigen::VectorXd getP2P();

    //=========================================================================================================
    /**
    * Returns the the value m_dvecFuzzyEn.
    *
    * @param [out] contains the current Fuzzy Entropy values.
    */
    Eigen::VectorXd getFuzzyEn();


    //=========================================================================================================
    /**
    * Returns the the value m_dvecKurtosisHistory.
    *
    * @param [out] contains Kurtosis value history.
    */
    Eigen::MatrixXd getKurtosisHistory();

    //=========================================================================================================
    /**
    * Returns the the value m_dvecP2PHistory.
    *
    * @param [out] contains the peak-to-peak-magnitude value history.
    */
    Eigen::MatrixXd getP2PHistory();

    //=========================================================================================================
    /**
    * Returns the the value m_dvecFuzzyEnHistory.
    *
    * @param [out] contains the Fuzzy Entropy value history.
    */
    Eigen::MatrixXd getFuzzyEnHistory();

    bool                                    m_bHistoryReady;            /**< True if m_dvecFuzzyEnHistory has no undefined values.*/
    int                                     m_iListLength;              /**< Number of values inside the history matrices for each channel.*/
    int                                     m_iFuzzyEnStep;             /**< Number of channels which are skipped after every calculation of FuzzyEn.*/

private:

    Eigen::MatrixXd                         m_dmatData;                 /**< The currently used data-set.*/
    Eigen::Matrix<bool, Eigen::Dynamic, 1>  m_bFuzzyEnCalc;             /**< Contains information for each channel whether or not FuzzyEn has been calculated.*/

    int                                     m_iChannelCount;            /**< Number of channels.*/
    int                                     m_iDataLength;              /**< Length of data.*/

    int                                     m_iKurtosisHistoryPosition; /**< Position inside the history matrix where the next next Kurtosis value is to be stored.*/
    int                                     m_iP2PHistoryPosition;      /**< Position inside the history matrix where the next next peak-to-peak magnitude value is to be stored.*/
    int                                     m_iFuzzyEnHistoryPosition;  /**< Position inside the history matrix where the next next Fuzzy Entropy value is to be stored.*/

    int                                     m_iFuzzyEnStart;            /**< FuzzyEn calculation begins at this position.*/

    bool                                    m_bSetNewP2P;               /**< True if there is a new P2P value to be stored inside m_dmatP2PHistory.*/
    bool                                    m_bSetNewKurtosis;          /**< True if there is a new Kurtosis value to be stored inside m_dmatKurtosisHistory.*/
    bool                                    m_bSetNewFuzzyEn;           /**< True if there is a new FuzzyEn value to be stored inside m_dmatFuzzyEnHistory.*/

    QList<int>                              m_lFuzzyEnUsedChs;          /**< List containing the indizes for all the channels for which FuzzyEn has been calculated.*/

    Eigen::VectorXd                         m_dvecP2P;                  /**< Contains the current peak-to-peak magnitude value for each channel.*/
    Eigen::VectorXd                         m_dvecKurtosis;             /**< Contains the current Kurtosis value for each channel.*/
    Eigen::VectorXd                         m_dvecFuzzyEn;              /**< Contains the current Fuzzy Entropy value for each channel.*/

    Eigen::MatrixXd                         m_dmatP2PHistory;           /**< Contains the history of peak-to-peak magnitude values for each channel.*/
    Eigen::MatrixXd                         m_dmatKurtosisHistory;      /**< Contains the history of Kurtosis magnitude values for each channel.*/
    Eigen::MatrixXd                         m_dmatFuzzyEnHistory;       /**< Contains the history of Fuzzy Entropy values for each channel.*/

    Eigen::VectorXd                         m_dvecStdDev;               /**< Contains the standard deviation for each channel.*/
    Eigen::VectorXd                         m_dvecMean;                 /**< Contains the mean value for each channel.*/
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


#endif // CALCMETRIC_H
