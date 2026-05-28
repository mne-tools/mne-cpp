//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file rt_detect_trigger.h
 * @since 2026
 * @date  March 2026
 * @brief Threshold and edge-based trigger detection on streaming stim channels.
 *
 * The functions in this header scan the rows of an incoming data matrix —
 * typically the stimulus / digital-trigger channels of a MEG or EEG file —
 * for sign or value transitions and return the sample index and amplitude
 * of every detected flank. @ref detectTriggerFlanks uses Eigen's @c maxCoeff
 * over the per-sample derivative to localise rising edges, with optional
 * de-bouncing controlled by a minimum inter-event distance.
 *
 * @ref toEventMatrix turns the per-channel @c QMap of detected events into
 * the canonical three-column @c Eigen::MatrixXi event layout (sample,
 * previous code, new code) consumed by the rest of the mne-cpp pipeline
 * (averaging, epoching, annotations).
 */

#ifndef RT_DETECT_TRIGGER_RTPROCESSING_H
#define RT_DETECT_TRIGGER_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"

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
DSPSHARED_EXPORT QList<Eigen::MatrixXi> toEventMatrix(QMap<int,QList<QPair<int,double> > > mapTriggers);

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
DSPSHARED_EXPORT QMap<int, QList<QPair<int, double> > > detectTriggerFlanksMax(const Eigen::MatrixXd &data,
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
DSPSHARED_EXPORT QList<QPair<int,double> > detectTriggerFlanksMax(const Eigen::MatrixXd &data,
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
DSPSHARED_EXPORT QMap<int,QList<QPair<int,double> > > detectTriggerFlanksGrad(const Eigen::MatrixXd &data,
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
DSPSHARED_EXPORT QList<QPair<int,double> > detectTriggerFlanksGrad(const Eigen::MatrixXd &data,
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

#endif // RT_DETECT_TRIGGER_RTPROCESSING_H
