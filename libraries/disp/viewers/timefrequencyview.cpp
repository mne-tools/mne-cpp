//=============================================================================================================
/**
 * @file     timefrequencyview.cpp
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
 * @brief    Definition of the TimeFrequencyView Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "timefrequencyview.h"

#include "helpers/evokedsetmodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QQuickItem>
#include <QQuickWidget>
#include <QBoxLayout>
#include <QPainter>
#include <QPainterPath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TimeFrequencyView::TimeFrequencyView()
: TimeFrequencyView("", Q_NULLPTR)
{

}

//=============================================================================================================

TimeFrequencyView::TimeFrequencyView(const QString& sSettingsPath,
                                     QWidget *parent,
                                     Qt::WindowFlags f)
: AbstractView(parent, f)
{
    m_sSettingsPath = sSettingsPath;
    //initQMLView();
}

//=============================================================================================================

void TimeFrequencyView::updateGuiMode(GuiMode mode)
{
    switch(mode) {
        case GuiMode::Clinical:
            break;
        default: // default is research mode
            break;
    }
}

//=============================================================================================================

void TimeFrequencyView::updateProcessingMode(ProcessingMode mode)
{
    switch(mode) {
        case ProcessingMode::Offline:
            break;
        default: // default is realtime mode
            break;
    }
}

//=============================================================================================================

void TimeFrequencyView::saveSettings()
{

}

//=============================================================================================================

void TimeFrequencyView::loadSettings()
{

}


//=============================================================================================================

void TimeFrequencyView::clearView()
{

}

//=============================================================================================================

void TimeFrequencyView::initQMLView()
{
    QUrl source = QUrl::fromLocalFile("../libraries/disp/viewers/qml/tfview.qml");
    QQuickWidget* widget = new QQuickWidget();
    widget->setSource(source);
    widget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(widget);


    this->setLayout(layout);
}

//=============================================================================================================

void TimeFrequencyView::paintEvent(QPaintEvent *event)
{
    if (m_pEvokedSetModel){

        int chartBorders = 30;

        QPainter painter(this);

        painter.save();
        painter.setBrush(QBrush());
        painter.drawRect(QRect(-1,-1,this->width()+2,this->height()+2));
        painter.restore();




        //paint chart
        painter.save();
        painter.setPen(QPen(Qt::black, 1, Qt::SolidLine));
        QRect chartBound(chartBorders,chartBorders,this->width()-chartBorders * 4 ,this->height()- chartBorders * 2);
        painter.drawRect(chartBound);
        painter.restore();

        //paint gradient bar

        painter.save();

        QLinearGradient linGrad(this->width() - chartBorders * 1.5f, chartBound.topRight().y(), this->width() - chartBorders * 1.5f, chartBound.bottomRight().y());
        painter.save();
        painter.setBrush(linGrad);
        painter.drawRect(chartBound.topRight().x() + chartBorders, chartBound.topRight().y(), chartBorders, chartBound.height());

        painter.restore();


        //paint axis labels
        //test
        if(m_pEvokedSetModel->getNumSamples() > 0) {
            painter.save();
            painter.setPen(QPen(Qt::red, 1, Qt::DashLine));

            float fDx = (float)(chartBound.width()) / ((float)m_pEvokedSetModel->getNumSamples());
            float posX = fDx * ((float)m_pEvokedSetModel->getNumPreStimSamples());
            painter.drawLine(chartBound.bottomLeft().x()+posX, chartBound.bottomRight().y(), chartBound.bottomLeft().x() + posX, chartBound.topRight().y());

            painter.drawText(QPointF(posX+chartBound.bottomLeft().x(),chartBound.bottomRight().y()-5), QString("0ms / Stimulus"));

            painter.restore();

            painter.save();
            QColor colorTimeSpacer = Qt::black;
            colorTimeSpacer.setAlphaF(0.5);
            painter.setPen(QPen(colorTimeSpacer, 1, Qt::DashLine));

            float yStart = chartBound.topLeft().y();
            float yEnd = chartBound.bottomRight().y();

            float sampleCounter = m_pEvokedSetModel->getNumPreStimSamples();
            int counter = 1;
            float timeDistanceMSec = 50.0;
            float timeDistanceSamples = (timeDistanceMSec/1000.0)*m_pEvokedSetModel->getSamplingFrequency(); //time distance corresponding to sampling frequency

            //spacers before stim
            while(sampleCounter-timeDistanceSamples>0) {
                sampleCounter-=timeDistanceSamples;
                float x = fDx*sampleCounter;
                painter.drawLine(x, yStart, x, yEnd);
                painter.drawText(QPointF(x+5,yEnd-5), QString("-%1ms").arg(timeDistanceMSec*counter));
                counter++;
            }

            //spacers after stim
            counter = 1;
            sampleCounter = m_pEvokedSetModel->getNumPreStimSamples();
            while(sampleCounter+timeDistanceSamples<m_pEvokedSetModel->getNumSamples()) {
                sampleCounter+=timeDistanceSamples;
                float x = fDx*sampleCounter;
                painter.drawLine(x, yStart, x, yEnd);
                painter.drawText(QPointF(x+5,yEnd-5), QString("%1ms").arg(timeDistanceMSec*counter));
                counter++;
            }

            painter.restore();

        }

    }

    return QWidget::paintEvent(event);
}

//=============================================================================================================

void TimeFrequencyView::setEvokedSetModel(QSharedPointer<EvokedSetModel> model)
{
    m_pEvokedSetModel = model;

//    connect(m_pEvokedSetModel.data(), &EvokedSetModel::dataChanged,
//            this, &ButterflyView::dataUpdate, Qt::UniqueConnection);
}
