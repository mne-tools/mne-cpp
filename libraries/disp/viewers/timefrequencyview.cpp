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
#include "helpers/timefrequencymodel.h"
#include "helpers/timefrequencymodel.h"

#include <disp/plots/tfplot.h>
#include <disp/plots/helpers/colormap.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

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
    m_pLayout = new QVBoxLayout();
    this->setLayout(m_pLayout);
}

//=============================================================================================================

TimeFrequencyView::TimeFrequencyView(const QString& sSettingsPath,
                                     QWidget *parent,
                                     Qt::WindowFlags f)
: AbstractView(parent, f)
, m_iChartBorderSpacing(30)
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
//    QUrl source = QUrl::fromLocalFile("../libraries/disp/viewers/qml/tfview.qml");
//    QQuickWidget* widget = new QQuickWidget();
//    widget->setSource(source);
//    widget->setResizeMode(QQuickWidget::SizeRootObjectToView);

//    QVBoxLayout* layout = new QVBoxLayout();
//    layout->addWidget(widget);


//    this->setLayout(layout);
}

//=============================================================================================================

void TimeFrequencyView::paintEvent(QPaintEvent *event)
{
    if (m_pEvokedSetModel){
        QPainter painter(this);

        painter.save();
        painter.setBrush(QBrush());
        painter.drawRect(QRect(-1,-1,this->width()+2,this->height()+2));
        painter.restore();




        //paint chart
        painter.save();
        painter.setPen(QPen(Qt::black, 1, Qt::SolidLine));
        QRect chartBound(m_iChartBorderSpacing,m_iChartBorderSpacing,this->width()-m_iChartBorderSpacing * 4 ,this->height()- m_iChartBorderSpacing * 2);
        painter.drawRect(chartBound);
        painter.restore();

        painter.save();
        painter.drawPixmap(chartBound, m_Pixmap);

        painter.restore();


        //paint gradient bar

        painter.save();

        QLinearGradient linGrad(this->width() - m_iChartBorderSpacing * 1.5f, chartBound.topRight().y(), this->width() - m_iChartBorderSpacing * 1.5f, chartBound.bottomRight().y());
        painter.save();
        painter.setBrush(linGrad);
        painter.drawRect(chartBound.topRight().x() + m_iChartBorderSpacing, chartBound.topRight().y(), m_iChartBorderSpacing, chartBound.height());

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
                float x = chartBound.bottomLeft().x() + fDx*sampleCounter;
                painter.drawLine(x, yStart, x, yEnd);
                painter.drawText(QPointF(x - m_iChartBorderSpacing/3, yEnd + m_iChartBorderSpacing/2), QString("-%1ms").arg(timeDistanceMSec*counter));
                counter++;
            }

            //spacers after stim
            counter = 1;
            sampleCounter = m_pEvokedSetModel->getNumPreStimSamples();
            while(sampleCounter+timeDistanceSamples<m_pEvokedSetModel->getNumSamples()) {
                sampleCounter+=timeDistanceSamples;
                float x = chartBound.bottomLeft().x() + fDx*sampleCounter;
                painter.drawLine(x, yStart, x, yEnd);
                painter.drawText(QPointF(x - m_iChartBorderSpacing/3, yEnd + m_iChartBorderSpacing/2), QString("%1ms").arg(timeDistanceMSec*counter));
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

//=============================================================================================================

//void TimeFrequencyView::paintChart(QPainter& painter,
//                                   const QRect chartBound)
//{

//}

////=============================================================================================================

//void TimeFrequencyView::paintAxes(QPainter& painter,
//                                  const QRect chartBound)
//{

//}

//=============================================================================================================

void TimeFrequencyView::setTimeFrequencyModel(QSharedPointer<DISPLIB::TimeFrequencyModel> pModel)
{
    qDebug() << "[TimeFrequencyView::setTimeFrequencyModel]";

    m_pTFModel = pModel;

    if(pModel){
        m_pTFModel = pModel;
        connect(m_pTFModel.data(), &TimeFrequencyModel::dataChanged,
                this, &TimeFrequencyView::updateData, Qt::UniqueConnection);
    }

//    DISPLIB::TFplot* tfplot = new DISPLIB::TFplot(m_pTFModel->data(1,1).value<Eigen::MatrixXd>(), m_pTFModel->getSamplingFrequency(), 0, 100, DISPLIB::ColorMaps::Jet);

//    tfplot->show();
}

//=============================================================================================================

void TimeFrequencyView::updateData()
{
    if(!m_pTFModel){
        return;
    }

//    if(!m_pLayout->isEmpty()){
//        m_pPlot->hide();
//        m_pLayout->removeWidget(m_pPlot);
//        delete m_pPlot;
//    }

    auto freqRange = m_pTFModel->getFreqRange();

//    m_pPlot = new TFplot(m_pTFModel->data(1,1).value<Eigen::MatrixXd>(),  m_pTFModel->getSamplingFrequency(), freqRange.first, freqRange.second, DISPLIB::ColorMaps::Jet);
//    m_pLayout->addWidget(m_pPlot);

//    //m_pPlot->adjustSize();
//    m_pPlot->adjustViewSize();

    auto data = m_pTFModel->data(1,1);
    if (!data.isNull()) {
        Eigen::MatrixXd tf_matrix = data.value<Eigen::MatrixXd>();
        qreal sample_rate = m_pTFModel->getSamplingFrequency();
        qreal lower_frq = freqRange.first;
        qreal upper_frq = freqRange.second;
        ColorMaps cmap = DISPLIB::ColorMaps::Jet;

        qreal max_frq = sample_rate/2.0;
        qreal frq_per_px = max_frq/tf_matrix.rows();

        if(upper_frq > max_frq || upper_frq <= 0) upper_frq = max_frq;
        if(lower_frq < 0 || lower_frq >= max_frq) lower_frq = 0;
        if(upper_frq < lower_frq) {
            qreal temp = upper_frq;
            upper_frq = lower_frq;
            lower_frq = temp;
        }

        qint32 lower_px = floor(lower_frq / frq_per_px);
        qint32 upper_px = floor(upper_frq / frq_per_px);

        Eigen::MatrixXd zoomed_tf_matrix = Eigen::MatrixXd::Zero(upper_px-lower_px, tf_matrix.cols());

        qint32 pxls = 0;
        for(qint32 it = lower_px; it < upper_px; it++) {
            zoomed_tf_matrix.row(pxls) = tf_matrix.row(it);
            pxls++;
        }

        generatePixmap(zoomed_tf_matrix, cmap);
    }

    repaint();
}

//=============================================================================================================

void TimeFrequencyView::generatePixmap(Eigen::MatrixXd tf_matrix,
                                       ColorMaps cmap)
{

    qreal norm1 = tf_matrix.maxCoeff();
    qreal mnorm = tf_matrix.minCoeff();
    if(std::fabs(mnorm) > norm1) norm1 = mnorm;
    tf_matrix /= norm1;

    QImage * image_to_tf_plot = new QImage(tf_matrix.cols(), tf_matrix.rows(), QImage::Format_RGB32);

    //setup pixelcolors in image
    QColor color;
    for ( qint32 y = 0; y < tf_matrix.rows(); y++ ) {
        for ( qint32 x = 0; x < tf_matrix.cols(); x++ ) {
            switch  (cmap) {
                case Jet:
                    color.setRgb(ColorMap::valueToJet(std::fabs(tf_matrix(y, x))));
                    break;
                case Hot:
                    color.setRgb(ColorMap::valueToHot(std::fabs(tf_matrix(y, x))));
                    break;
                case HotNeg1:
                    color.setRgb(ColorMap::valueToHotNegative1(std::fabs(tf_matrix(y, x))));
                    break;
                case HotNeg2:
                    color.setRgb(ColorMap::valueToHotNegative2(std::fabs(tf_matrix(y, x))));
                    break;
                case Bone:
                    color.setRgb(ColorMap::valueToBone(std::fabs(tf_matrix(y, x))));
                    break;
                case RedBlue:
                    color.setRgb(ColorMap::valueToRedBlue(std::fabs(tf_matrix(y, x))));
                    break;
            }
            image_to_tf_plot->setPixel(x, tf_matrix.rows() - 1 -  y,  color.rgb());
        }
    }

     *image_to_tf_plot = image_to_tf_plot->scaled(tf_matrix.cols(), tf_matrix.cols()/2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
     *image_to_tf_plot = image_to_tf_plot->scaledToWidth(/*0.9 **/ 1026, Qt::SmoothTransformation);

    m_Pixmap = QPixmap::fromImage(*image_to_tf_plot);
}
