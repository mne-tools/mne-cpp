//=============================================================================================================
/**
 * @file     timefrequencysceneitem.cpp
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
 * @brief    Definition of the TimeFrequencySceneItem Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "timefrequencysceneitem.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TimeFrequencySceneItem::TimeFrequencySceneItem(const QString& channelName,
                                               int channelNumber,
                                               const QPointF& channelPosition,
                                               int channelKind,
                                               int channelUnit)
: m_sChannelName(channelName)
, m_iChannelNumber(channelNumber)
, m_iChannelKind(channelKind)
, m_iChannelUnit(channelUnit)
, m_iTotalNumberChannels(0)
, m_iFontTextSize(15)
, m_iMaxWidth(1500)
, m_iMaxHeigth(150)
, m_bIsBad(false)
, m_qpChannelPosition(channelPosition)
, m_pPlot(Q_NULLPTR)
{
    //initQMLView();

//    m_rectBoundingRect = QRectF(-m_iMaxWidth/2, -m_iMaxHeigth/2, m_iMaxWidth, m_iMaxHeigth);
//    QLabel* widget = new QLabel("Test");

    m_pLayout = new QVBoxLayout();
//    m_pLayout->addWidget(widget);
    this->setLayout(m_pLayout);
}

//=============================================================================================================

void TimeFrequencySceneItem::initQMLView()
{
//    QUrl source = QUrl::fromLocalFile("../libraries/disp/viewers/qml/tfview.qml");
//    QQuickWidget* widget = new QQuickWidget();
//    widget->setSource(source);
//    widget->setResizeMode(QQuickWidget::SizeRootObjectToView);

//    QLabel* widget = new QLabel("Test");

//    QVBoxLayout* layout = new QVBoxLayout();
//    layout->addWidget(widget);


//    this->setLayout(layout);
}

//=============================================================================================================

int TimeFrequencySceneItem::getChannelNumber() const
{
    return m_iChannelNumber;
}

//=============================================================================================================

void TimeFrequencySceneItem::setData(const Eigen::MatrixXd &data)
{
    m_data = data;

    if(!m_pLayout->isEmpty()){
        m_pLayout->removeWidget(m_pPlot);
        m_pPlot->deleteLater();
    }

    m_pPlot = new TFplot(m_data, m_fSampleRate, 0, 100, DISPLIB::ColorMaps::Jet);
    m_pPlot->show();

//    m_pLayout->addWidget(m_pPlot);

//    m_pLayout->addWidget(m_pPlot);
//    this->setLayout(m_pLayout);
}

//=============================================================================================================

void TimeFrequencySceneItem::setSampleRate(float fFreq)
{
    m_fSampleRate = fFreq;
    qDebug() << "freq:" << fFreq;
}
