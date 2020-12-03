//=============================================================================================================
/**
 * @file     butterflyview.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Gabriel B Motta, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the ButterflyView class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "butterflyview.h"

#include "scalingview.h"

#include "helpers/evokedsetmodel.h"
#include "helpers/channelinfomodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QPainterPath>
#include <QDebug>
#include <QSvgGenerator>
#include <QSurfaceFormat>
#include <QSettings>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ButterflyView::ButterflyView(const QString& sSettingsPath,
                             QWidget *parent,
                             Qt::WindowFlags f)
:
#if !defined(NO_QOPENGLWIDGET)
  QOpenGLWidget(parent, f)
#else
  QWidget(parent, f)
#endif
, m_sSettingsPath(sSettingsPath)
, m_pEvokedSetModel(NULL)
, m_bIsInit(false)
, m_bShowMAG(true)
, m_bShowGRAD(true)
, m_bShowEEG(true)
, m_bShowEOG(true)
, m_bShowMISC(true)
, m_colCurrentBackgroundColor(Qt::white)
, m_qMapAverageActivation(QSharedPointer<QMap<QString, bool> >::create())
, m_qMapAverageColor(QSharedPointer<QMap<QString, QColor> >::create())
{
    m_sSettingsPath = sSettingsPath;
//    // Activate anti aliasing
//    QSurfaceFormat fmt;
//    fmt.setSamples(4);
//    this->setFormat(fmt);
    loadSettings();
}

//=============================================================================================================

ButterflyView::~ButterflyView()
{
    saveSettings();
}

//=============================================================================================================

void ButterflyView::updateOpenGLViewport()
{
#if !defined(NO_QOPENGLWIDGET)
    // Activate anti aliasing
    initializeGL();
#endif
}

//=============================================================================================================

void ButterflyView::setEvokedSetModel(QSharedPointer<EvokedSetModel> model)
{
    m_pEvokedSetModel = model;

    connect(m_pEvokedSetModel.data(), &EvokedSetModel::dataChanged,
            this, &ButterflyView::dataUpdate, Qt::UniqueConnection);
}

//=============================================================================================================

void ButterflyView::dataUpdate()
{
    if(!m_bIsInit && m_pEvokedSetModel->isInit()) {
        m_bIsInit = true;
    }

    setAverageActivation(m_qMapAverageActivation);

    update();
}

//=============================================================================================================

QMap<QString, bool> ButterflyView::getModalityMap()
{
    return m_modalityMap;
}

//=============================================================================================================

void ButterflyView::setModalityMap(const QMap<QString, bool> &modalityMap)
{
    m_modalityMap = modalityMap;
    update();
}

//=============================================================================================================

void ButterflyView::setScaleMap(const QMap<qint32,float> &scaleMap)
{
    m_scaleMap = scaleMap;    
    update();
}

//=============================================================================================================

void ButterflyView::setSelectedChannels(const QList<int> &selectedChannels)
{
    m_lSelectedChannels = selectedChannels;
    update();
}

//=============================================================================================================

void ButterflyView::updateView()
{
    update();
}

//=============================================================================================================

void ButterflyView::setBackgroundColor(const QColor& backgroundColor)
{
    m_colCurrentBackgroundColor = backgroundColor;
    update();
}

//=============================================================================================================

const QColor& ButterflyView::getBackgroundColor()
{
    return m_colCurrentBackgroundColor;
}

//=============================================================================================================

void ButterflyView::takeScreenshot(const QString& fileName)
{
    if(fileName.contains(".svg", Qt::CaseInsensitive)) {
        // Generate screenshot
        QSvgGenerator svgGen;
        svgGen.setFileName(fileName);
        svgGen.setSize(this->size());
        svgGen.setViewBox(this->rect());

        this->render(&svgGen);
    }

    if(fileName.contains(".png", Qt::CaseInsensitive)) {
        QImage image(this->size(), QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        this->render(&painter);
        image.save(fileName);
    }
}

//=============================================================================================================

QSharedPointer<QMap<QString, QColor> > ButterflyView::getAverageColor() const
{
    return m_qMapAverageColor;
}

//=============================================================================================================

QSharedPointer<QMap<QString, bool> > ButterflyView::getAverageActivation() const
{
    return m_qMapAverageActivation;
}

//=============================================================================================================

void ButterflyView::setAverageColor(const QSharedPointer<QMap<QString, QColor> > qMapAverageColor)
{
    m_qMapAverageColor = qMapAverageColor;
    update();
}

//=============================================================================================================

void ButterflyView::setSingleAverageColor(const QColor& avgColor)
{
    for (QString mapKey : m_qMapAverageColor->keys())
        m_qMapAverageColor->insert(mapKey, avgColor);
}

//=============================================================================================================

void ButterflyView::setAverageActivation(const QSharedPointer<QMap<QString, bool> > qMapAverageActivation)
{
    m_qMapAverageActivation = qMapAverageActivation;
    update();
}

//=============================================================================================================

void ButterflyView::setChannelInfoModel(QSharedPointer<ChannelInfoModel> &pChannelInfoModel)
{
    m_pChannelInfoModel = pChannelInfoModel;
}

//=============================================================================================================

void ButterflyView::showSelectedChannelsOnly(const QStringList& selectedChannels)
{
    if(!m_pChannelInfoModel) {
        qDebug() << "ButterflyView::showSelectedChannelsOnly - m_pChannelInfoModel is NULL. Returning. ";
        return;
    }

    QList<int> selectedChannelsIndexes;

    for(int i = 0; i<selectedChannels.size(); i++)
        selectedChannelsIndexes<<m_pChannelInfoModel->getIndexFromOrigChName(selectedChannels.at(i));

    setSelectedChannels(selectedChannelsIndexes);
}

//=============================================================================================================

void ButterflyView::showSelectedChannels(const QList<int> selectedChannelsIndexes)
{
    setSelectedChannels(selectedChannelsIndexes);
}

//=============================================================================================================

void ButterflyView::showAllChannels()
{
    if (m_pEvokedSetModel) {
        QList<int> lAllChannels;
        for(int i = 0; i < m_pEvokedSetModel->rowCount(); i++) {
            lAllChannels.append(i);
        }
        setSelectedChannels(lAllChannels);
    }
}

//=============================================================================================================

void ButterflyView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Save Settings
    QSettings settings("MNECPP");
}

//=============================================================================================================

void ButterflyView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings("MNECPP");
}

//=============================================================================================================

#if !defined(NO_QOPENGLWIDGET)
    void ButterflyView::paintGL()
#else
    void ButterflyView::paintEvent(QPaintEvent *event)
#endif
{
    QPainter painter(this);

    painter.save();
    painter.setBrush(QBrush(m_colCurrentBackgroundColor));
    painter.drawRect(QRect(-1,-1,this->width()+2,this->height()+2));
    painter.restore();

    painter.setRenderHint(QPainter::Antialiasing, true);

    if(m_bIsInit && m_pEvokedSetModel)
    {
        //Draw baseline correction area
        if(m_pEvokedSetModel->getBaselineInfo().first.toString() != "None" &&
                m_pEvokedSetModel->getBaselineInfo().second.toString() != "None") {
            float from = m_pEvokedSetModel->getBaselineInfo().first.toFloat();
            float to = m_pEvokedSetModel->getBaselineInfo().second.toFloat();

            painter.save();
            painter.setPen(QPen(Qt::red, 1, Qt::DashLine));
            painter.setBrush(Qt::red);
            painter.setOpacity(0.1);

            if(m_pEvokedSetModel->getNumSamples() == 0){
                qDebug() << "Unable to get data. Returning early.";
                return;
            }

            float fDx = (float)(this->width()) / ((float)m_pEvokedSetModel->getNumSamples());

            float fromSamp = ((from)*m_pEvokedSetModel->getSamplingFrequency())+m_pEvokedSetModel->getNumPreStimSamples();
            float posX = fDx*(fromSamp);
            float toSamp = ((to)*m_pEvokedSetModel->getSamplingFrequency())+m_pEvokedSetModel->getNumPreStimSamples();
            float width = fDx*(toSamp-fromSamp);

            QRect rect(posX,0,width,this->height());

            painter.drawRect(rect);

            painter.restore();
        }

        //Stimulus bar
        if(m_pEvokedSetModel->getNumSamples() > 0) {
            painter.save();
            painter.setPen(QPen(Qt::red, 1, Qt::DashLine));

            float fDx = (float)(this->width()) / ((float)m_pEvokedSetModel->getNumSamples());
            float posX = fDx * ((float)m_pEvokedSetModel->getNumPreStimSamples());
            painter.drawLine(posX, 1, posX, this->height());

            painter.drawText(QPointF(posX+5,this->rect().bottomRight().y()-5), QString("0ms / Stimulus"));

            painter.restore();
        }

        //Vertical time spacers
        if(m_pEvokedSetModel->getNumberOfTimeSpacers() > 0)
        {
            painter.save();
            QColor colorTimeSpacer = Qt::black;
            colorTimeSpacer.setAlphaF(0.5);
            painter.setPen(QPen(colorTimeSpacer, 1, Qt::DashLine));

            float yStart = this->rect().topLeft().y();
            float yEnd = this->rect().bottomRight().y();

            float fDx = 1;
            if(m_pEvokedSetModel->getNumSamples() != 0) {
                fDx = (float)(this->width()) / ((float)m_pEvokedSetModel->getNumSamples());
            }

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

        //Zero line
        if(m_pEvokedSetModel->getNumSamples() > 0) {
            painter.save();
            painter.setPen(QPen(Qt::black, 1, Qt::DashLine));

            painter.drawLine(0, this->height()/2, this->width(), this->height()/2);

            painter.restore();
        }

        painter.translate(0,this->height()/2);

        //Actual average data
        for(qint32 r = 0; r < m_pEvokedSetModel->rowCount(); ++r) {
            if(m_lSelectedChannels.contains(r)) {
                qint32 kind = m_pEvokedSetModel->getKind(r);

                //Display only selected kinds
                switch(kind) {
                    case FIFFV_MEG_CH: {
                        qint32 unit = m_pEvokedSetModel->getUnit(r);
                        if(unit == FIFF_UNIT_T_M) {
                            if(m_modalityMap["GRAD"])
                                break;
                            else
                                continue;
                        }
                        else if(unit == FIFF_UNIT_T)
                        {
                            if(m_modalityMap["MAG"])
                                break;
                            else
                                continue;
                        }
                        continue;
                    }
                    case FIFFV_EEG_CH: {
                        if(m_modalityMap["EEG"])
                            break;
                        else
                            continue;
                    }
                    case FIFFV_EOG_CH: {
                        if(m_modalityMap["EOG"])
                            break;
                        else
                            continue;
                    }
                    case FIFFV_MISC_CH: {
                        if(m_modalityMap["MISC"])
                            break;
                        else
                            continue;
                    }
                    default:
                        continue;
                }

                painter.save();

                createPlotPath(r, painter);

                painter.restore();
            }
        }
    }

#if !defined(NO_QOPENGLWIDGET)
    return QOpenGLWidget::paintGL();
#else
    return QWidget::paintEvent(event);
#endif
}

//=============================================================================================================

void ButterflyView::createPlotPath(qint32 row, QPainter& painter) const
{
    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)
    qint32 kind = m_pEvokedSetModel->getKind(row);
    float fMaxValue = DISPLIB::getScalingValue(m_scaleMap, kind, m_pEvokedSetModel->getUnit(row));
    bool bIsBad = m_pEvokedSetModel->getIsChannelBad(row);

    if(bIsBad) {
        painter.setOpacity(0.20);
    } else {
        painter.setOpacity(0.75);
    }

    float fValue;
    float fScaleY = this->height()/(2*fMaxValue);

    //restrictions for paint performance
    float fWinMaxVal = ((float)this->height()-2)/2.0f;
//    qint32 iDownSampling = (m_pEvokedSetModel->getNumSamples() * 4 / (this->width()-2));
//    if(iDownSampling < 1) {
//        iDownSampling = 1;
//    }

    QPointF qSamplePosition;

    float fDx = (float)(this->width()-2) / ((float)m_pEvokedSetModel->getNumSamples()-1.0f);//((float)option.rect.width()) / m_pEvokedSetModel->getMaxSamples();

    QList<DISPLIB::AvrTypeRowVector> rowVec = m_pEvokedSetModel->data(row,1).value<QList<DISPLIB::AvrTypeRowVector> >();

    //Do for all average types
    for(int j = 0; j < rowVec.size(); ++j) {
        QString sAvrComment = rowVec.at(j).first;

        // Select color for each average
        if(m_pEvokedSetModel->isFreezed()) {
            QColor freezeColor = m_qMapAverageColor->value(sAvrComment);
            freezeColor.setAlphaF(0.5);
            painter.setPen(QPen(freezeColor, 1));
        } else {
            painter.setPen(QPen(m_qMapAverageColor->value(sAvrComment)));
        }

        if(m_qMapAverageActivation->value(sAvrComment)) {
            //Calculate downsampling factor of averaged data in respect to the items width
            int dsFactor;
            rowVec.at(j).second.cols() / this->width() < 1 ? dsFactor = 1 : dsFactor = rowVec.at(j).second.cols() / this->width();
            if(dsFactor == 0) {
                dsFactor = 1;
            }

            QPainterPath path(QPointF(1,0));
            float y_base = path.currentPosition().y();

            //Move to initial starting point
            if(rowVec.at(j).second.cols() > 0)
            {
                float val = rowVec.at(j).second[0];
                fValue = (val/*-rowVec.at(j)[m_pEvokedSetModel->getNumPreStimSamples()-1]*/)*fScaleY;//ToDo -> -2 PreStim is one too short

                float newY = y_base+fValue;

                qSamplePosition.setY(-newY);
                qSamplePosition.setX(path.currentPosition().x());

                path.moveTo(qSamplePosition);
            }

            //create lines from one to the next sample
            qint32 i;
            for(i = 1; i < rowVec.at(j).second.cols() && path.elementCount() <= this->width(); i += dsFactor) {
                float val = /*rowVec.at(j)[m_pEvokedSetModel->getNumPreStimSamples()-1] - */rowVec.at(j).second[i]; //remove first sample data[0] as offset
                fValue = val*fScaleY;

                //Cut plotting if out of widget area
                fValue = fValue > fWinMaxVal ? fWinMaxVal : fValue < -fWinMaxVal ? -fWinMaxVal : fValue;

                float newY = y_base+fValue;

                qSamplePosition.setY(-newY);

                qSamplePosition.setX(path.currentPosition().x()+fDx);

                path.lineTo(qSamplePosition);
            }

        //    //create lines from one to the next sample for last path
        //    qint32 sample_offset = m_pEvokedSetModel->numVLines() + 1;
        //    qSamplePosition.setX(qSamplePosition.x() + fDx*sample_offset);
        //    lastPath.moveTo(qSamplePosition);

        //    for(i += sample_offset; i < lastData.size(); ++i) {
        //        float val = lastData[i] - lastData[0]; //remove first sample lastData[0] as offset
        //        fValue = val*fScaleY;

        //        float newY = y_base+fValue;

        //        qSamplePosition.setY(newY);
        //        qSamplePosition.setX(lastPath.currentPosition().x()+fDx);

        //        lastPath.lineTo(qSamplePosition);
        //    }

            painter.drawPath(path);
        }
    }
}

//=============================================================================================================

QSharedPointer<EvokedSetModel> ButterflyView::getEvokedSetModel()
{
    return m_pEvokedSetModel;
}

//=============================================================================================================

void ButterflyView::clearView()
{
    setEvokedSetModel(Q_NULLPTR);
}
