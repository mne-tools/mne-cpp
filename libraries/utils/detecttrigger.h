//=============================================================================================================
/**
 * @file     detecttrigger.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     July, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch. All rights reserved.
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
 * @brief    DetectTrigger class declaration
 *
 */

#ifndef DETECTTRIGGER_H
#define DETECTTRIGGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{


//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
 * Routines for detecting trigger flanks in a given signal
 *
 * @brief Trigger flank detection
 */
class UTILSSHARED_EXPORT DetectTrigger
{

public:
    typedef QSharedPointer<DetectTrigger> SPtr;            /**< Shared pointer type for DetectTrigger class. */
    typedef QSharedPointer<const DetectTrigger> ConstSPtr; /**< Const shared pointer type for DetectTrigger class. */

    //=========================================================================================================
    /**
     * Destroys the DetectTrigger class.
     */
    DetectTrigger();

    //=========================================================================================================
    /**
     * detectTriggerFlanks detects flanks from a given data matrix in row wise order. This function uses a simple maxCoeff function implemented by eigen to locate the triggers.
     *
     * @param[in]        data  the data used to find the trigger flanks
     * @param[in]        lTriggerChannels  The indeces of the trigger channels
     * @param[in]        iOffsetIndex  the offset index gets added to the found trigger flank index
     * @param[in]        dThreshold  the signal threshold value used to find the trigger flank
     * @param[in]        bRemoveOffset  remove the first sample as offset
     * @param[in]        iBurstLengthMs  The length in samples which is skipped after a trigger was found
     *
     * @param return     This map holds the indices of the channels which are to be read from data. For each index/channel the found triggersand corresponding signal values are written to the value of the map.
     */
    static QMap<int, QList<QPair<int, double> > > detectTriggerFlanksMax(const Eigen::MatrixXd &data,
                                                                         const QList<int>& lTriggerChannels,
                                                                         int iOffsetIndex,
                                                                         double dThreshold,
                                                                         bool bRemoveOffset,
                                                                         int iBurstLengthSamp = 100);

    //=========================================================================================================
    /**
     * detectTriggerFlanks detects flanks from a given data matrix in row wise order. This function uses a simple maxCoeff function implemented by eigen to locate the triggers.
     *
     * @param[in]        data  the data used to find the trigger flanks
     * @param[in]        iTriggerChannelIdx  the index of the trigger channel in the matrix.
     * @param[in]        iOffsetIndex  the offset index gets added to the found trigger flank index
     * @param[in]        dThreshold  the signal threshold value used to find the trigger flank
     * @param[in]        bRemoveOffset  remove the first sample as offset
     * @param[in]        iBurstLengthMs  The length in samples which is skipped after a trigger was found
     *
     * @param return     This list holds the found trigger indices and corresponding signal values.
     */
    static QList<QPair<int,double> > detectTriggerFlanksMax(const Eigen::MatrixXd &data,
                                                            int iTriggerChannelIdx,
                                                            int iOffsetIndex,
                                                            double dThreshold,
                                                            bool bRemoveOffset,
                                                            int iBurstLengthSamp = 100);

    //=========================================================================================================
    /**
     * detectTriggerFlanksGrad detects flanks from a given data matrix in row wise order. This function uses a simple gradient to locate the triggers.
     *
     * @param[in]    data  the data used to find the trigger flanks
     * @param[in]    lTriggerChannels  The indeces of the trigger channels
     * @param[in]    iOffsetIndex  the offset index gets added to the found trigger flank index
     * @param[in]    iThreshold  the gradient threshold value used to find the trigger flank
     * @param[in]    bRemoveOffset  remove the first sample as offset
     * @param[in]    type  detect rising or falling flank. Use "Rising" or "Falling" as input
     * @param[in]    iBurstLengthMs  The length in samples which is skipped after a trigger was found
     *
     * @param return     This map holds the indices of the channels which are to be read from data. For each index/channel the found triggers and corresponding signal values are written to the value of the map.
     */
    static QMap<int,QList<QPair<int,double> > > detectTriggerFlanksGrad(const Eigen::MatrixXd &data,
                                                                        const QList<int>& lTriggerChannels,
                                                                        int iOffsetIndex,
                                                                        double dThreshold,
                                                                        bool bRemoveOffset,
                                                                        const QString& type,
                                                                        int iBurstLengthSamp = 100);

    //=========================================================================================================
    /**
     * detectTriggerFlanksGrad detects flanks from a given data matrix in row wise order. This function uses a simple gradient to locate the triggers.
     *
     * @param[in]    data  the data used to find the trigger flanks
     * @param[in]    iTriggerChannelIdx  the index of the trigger channel in the matrix.
     * @param[in]    iOffsetIndex  the offset index gets added to the found trigger flank index
     * @param[in]    iThreshold  the gradient threshold value used to find the trigger flank
     * @param[in]    bRemoveOffset  remove the first sample as offset
     * @param[in]    type  detect rising or falling flank. Use "Rising" or "Falling" as input
     * @param[in]    iBurstLengthMs  The length in samples which is skipped after a trigger was found
     *
     * @param return     This list holds the found trigger indices and corresponding signal values.
     */
    static QList<QPair<int,double> > detectTriggerFlanksGrad(const Eigen::MatrixXd &data,
                                                             int iTriggerChannelIdx,
                                                             int iOffsetIndex,
                                                             double dThreshold,
                                                             bool bRemoveOffset,
                                                             const QString& type,
                                                             int iBurstLengthSamp = 100);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // DETECTTRIGGER_H
