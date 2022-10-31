//=============================================================================================================
/**
 * @file     detecttrigger.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.3
 * @date     June, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    DetectTrigger declarations.
 *
 */

#ifndef DETECTTRIGGER_RTPROCESSING_H
#define DETECTTRIGGER_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPair>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
// RTPROCESSINGLIB FORWARD DECLARATIONS
//=============================================================================================================

//=========================================================================================================
/**
 * Transforms QMap with stored information about events per stim channel to a list of event matrices.
 *
 * @param[in]       mapTriggers  The QMap to be transformed.
 *
 * @param[in]       A list of transformed Eigen matrices.
 */
RTPROCESINGSHARED_EXPORT QList<Eigen::MatrixXi> toEventMatrix(QMap<int,QList<QPair<int,double> > > mapTriggers);

//=========================================================================================================
/**
 * detectTriggerFlanks detects flanks from a given data matrix in row wise order. This function uses a simple
 * maxCoeff function implemented by eigen to locate the triggers.
 *
 * @param[in]        data  the data used to find the trigger flanks.
 * @param[in]        lTriggerChannels  The indeces of the trigger channels.
 * @param[in]        iOffsetIndex  the offset index gets added to the found trigger flank index.
 * @param[in]        dThreshold  the signal threshold value used to find the trigger flank.
 * @param[in]        bRemoveOffset  remove the first sample as offset.
 * @param[in]        iBurstLengthMs  The length in samples which is skipped after a trigger was found.
 *
 * @return     This map holds the indices of the channels which are to be read from data. For each
 *                   index/channel the found triggersand corresponding signal values are written to the value of the map.
 */
RTPROCESINGSHARED_EXPORT QMap<int, QList<QPair<int, double> > > detectTriggerFlanksMax(const Eigen::MatrixXd &data,
                                                                                       const QList<int>& lTriggerChannels,
                                                                                       int iOffsetIndex,
                                                                                       double dThreshold,
                                                                                       bool bRemoveOffset,
                                                                                       int iBurstLengthSamp = 100);

//=========================================================================================================
/**
 * detectTriggerFlanks detects flanks from a given data matrix in row wise order. This function uses a simple maxCoeff function implemented by eigen to locate the triggers.
 *
 * @param[in]        data  the data used to find the trigger flanks.
 * @param[in]        iTriggerChannelIdx  the index of the trigger channel in the matrix.
 * @param[in]        iOffsetIndex  the offset index gets added to the found trigger flank index.
 * @param[in]        dThreshold  the signal threshold value used to find the trigger flank.
 * @param[in]        bRemoveOffset  remove the first sample as offset.
 * @param[in]        iBurstLengthMs  The length in samples which is skipped after a trigger was found.
 *
 * @return     This list holds the found trigger indices and corresponding signal values.
 */
RTPROCESINGSHARED_EXPORT QList<QPair<int,double> > detectTriggerFlanksMax(const Eigen::MatrixXd &data,
                                                                          int iTriggerChannelIdx,
                                                                          int iOffsetIndex,
                                                                          double dThreshold,
                                                                          bool bRemoveOffset,
                                                                          int iBurstLengthSamp = 100);

//=========================================================================================================
/**
 * detectTriggerFlanksGrad detects flanks from a given data matrix in row wise order. This function uses a simple gradient to locate the triggers.
 *
 * @param[in]    data  the data used to find the trigger flanks.
 * @param[in]    lTriggerChannels  The indeces of the trigger channels.
 * @param[in]    iOffsetIndex  the offset index gets added to the found trigger flank index.
 * @param[in]    iThreshold  the gradient threshold value used to find the trigger flank.
 * @param[in]    bRemoveOffset  remove the first sample as offset.
 * @param[in]    type  detect rising or falling flank. Use "Rising" or "Falling" as input.
 * @param[in]    iBurstLengthMs  The length in samples which is skipped after a trigger was found.
 *
 * @return     This map holds the indices of the channels which are to be read from data. For each index/channel the found triggers and corresponding signal values are written to the value of the map.
 */
RTPROCESINGSHARED_EXPORT QMap<int,QList<QPair<int,double> > > detectTriggerFlanksGrad(const Eigen::MatrixXd &data,
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
 * @param[in]    data  the data used to find the trigger flanks.
 * @param[in]    iTriggerChannelIdx  the index of the trigger channel in the matrix.
 * @param[in]    iOffsetIndex  the offset index gets added to the found trigger flank index.
 * @param[in]    iThreshold  the gradient threshold value used to find the trigger flank.
 * @param[in]    bRemoveOffset  remove the first sample as offset.
 * @param[in]    type  detect rising or falling flank. Use "Rising" or "Falling" as input.
 * @param[in]    iBurstLengthMs  The length in samples which is skipped after a trigger was found.
 *
 * @return     This list holds the found trigger indices and corresponding signal values.
 */
RTPROCESINGSHARED_EXPORT QList<QPair<int,double> > detectTriggerFlanksGrad(const Eigen::MatrixXd &data,
                                                                           int iTriggerChannelIdx,
                                                                            int iOffsetIndex,
                                                                           double dThreshold,
                                                                           bool bRemoveOffset,
                                                                           const QString& type,
                                                                           int iBurstLengthSamp = 100);

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // DETECTTRIGGER_RTPROCESSING_H
