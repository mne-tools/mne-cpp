//=============================================================================================================
/**
 * @file     timefrequencysceneitem.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief    Declaration of the TimeFrequencySceneItem Class.
 *
 */

#ifndef TIMEFREQUENCYSCENEITEM_H
#define TIMEFREQUENCYSCENEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"

#include <disp/plots/tfplot.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QGraphicsWidget>
#include <QBoxLayout>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
class DISPSHARED_EXPORT TimeFrequencySceneItem : public QGraphicsObject
{
    Q_OBJECT
public:
    TimeFrequencySceneItem(const QString& channelName,
                           int channelNumber,
                           const QPointF& channelPosition,
                           int channelKind,
                           int channelUnit);

    void setData(const Eigen::MatrixXd& data);

    void setSampleRate(float fFreq);

    int getChannelNumber() const;

    QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
    void initQMLView();

    void paintPlot();

private:

    QString                     m_sChannelName;             /**< The channel name.*/
    int                         m_iChannelNumber;           /**< The channel number.*/
    int                         m_iChannelKind;             /**< The channel kind.*/
    int                         m_iChannelUnit;             /**< The channel unit.*/
    int                         m_iTotalNumberChannels;     /**< The total number of channels loaded in the curent evoked data set.*/
    int                         m_iFontTextSize;            /**< The font text size of the electrode names.*/
    int                         m_iMaxWidth;                /**< The max width. */
    int                         m_iMaxHeigth;               /**< The max heigth. */

    bool                        m_bIsBad;                   /**< Whether this channel is bad. */

    QPointF                     m_qpChannelPosition;        /**< The channels 2D position in the scene.*/

    Eigen::MatrixXd             m_data;
    float                       m_fSampleRate;
    QRectF                      m_rectBoundingRect;         /**< The bounding rect. */

    QPointer<QVBoxLayout>       m_pLayout;

    QPointer<TFplot>            m_pPlot;

};
}//namespace
#endif // TIMEFREQUENCYSCENEITEM_H
