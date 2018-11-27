//=============================================================================================================
/**
* @file     butterflyview.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "butterflyview.h"

#include "helpers/evokedsetmodel.h"
#include "helpers/channelinfomodel.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QDebug>
#include <QSvgGenerator>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ButterflyView::ButterflyView(QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
, m_pEvokedModel(NULL)
, m_bIsInit(false)
, m_iNumChannels(-1)
, m_bShowMAG(true)
, m_bShowGRAD(true)
, m_bShowEEG(true)
, m_bShowEOG(true)
, m_bShowMISC(true)
, m_colCurrentBackgroundColor(Qt::white)
, m_fMaxMAG(0.0)
, m_fMaxGRAD(0.0)
, m_fMaxEEG(0.0)
, m_fMaxEOG(0.0)
, m_fMaxMISC(0.0)
{
}


//*************************************************************************************************************

void ButterflyView::setModel(QSharedPointer<EvokedSetModel> model)
{
    m_pEvokedModel = model;

    connect(m_pEvokedModel.data(), &EvokedSetModel::dataChanged,
            this, &ButterflyView::dataUpdate);
}


//*************************************************************************************************************

void ButterflyView::dataUpdate(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    Q_UNUSED(topLeft);
    Q_UNUSED(bottomRight);
    Q_UNUSED(roles);

    if(!m_bIsInit && m_pEvokedModel->isInit())
    {
        m_iNumChannels = m_pEvokedModel->rowCount();
        m_bIsInit = true;
    }

    update();
}


//*************************************************************************************************************

QList<Modality> ButterflyView::getModalities()
{
    return m_qListModalities;
}


//*************************************************************************************************************

void ButterflyView::setModalities(const QList<Modality>& p_qListModalities)
{
    m_qListModalities = p_qListModalities;

    for(qint32 i = 0; i < m_qListModalities.size(); ++i)
    {
        if(m_qListModalities[i].m_sName == ("GRAD"))
        {
            m_bShowGRAD = m_qListModalities[i].m_bActive;
            m_fMaxGRAD = m_qListModalities[i].m_fNorm;
        }

        if(m_qListModalities[i].m_sName == ("MAG"))
        {
            m_bShowMAG = m_qListModalities[i].m_bActive;
            m_fMaxMAG = m_qListModalities[i].m_fNorm;
        }
        if(m_qListModalities[i].m_sName == ("EEG"))
        {
            m_bShowEEG = m_qListModalities[i].m_bActive;
            m_fMaxEEG = m_qListModalities[i].m_fNorm;

        }
        if(m_qListModalities[i].m_sName == ("EOG"))
        {
            m_bShowEOG = m_qListModalities[i].m_bActive;
            m_fMaxEOG = m_qListModalities[i].m_fNorm;

        }
        if(m_qListModalities[i].m_sName == ("MISC"))
        {
            m_bShowMISC = m_qListModalities[i].m_bActive;
            m_fMaxMISC = m_qListModalities[i].m_fNorm;

        }
    }

    update();
}


//*************************************************************************************************************

void ButterflyView::setSelectedChannels(const QList<int> &selectedChannels)
{
    m_lSelectedChannels = selectedChannels;

    update();
}


//*************************************************************************************************************

void ButterflyView::updateView()
{
    update();
}


//*************************************************************************************************************

void ButterflyView::setBackgroundColor(const QColor& backgroundColor)
{
    m_colCurrentBackgroundColor = backgroundColor;

    update();
}


//*************************************************************************************************************

const QColor& ButterflyView::getBackgroundColor()
{
    return m_colCurrentBackgroundColor;
}



//*************************************************************************************************************

void ButterflyView::takeScreenshot(const QString& fileName)
{
    if(fileName.contains(".svg", Qt::CaseInsensitive))
    {
        // Generate screenshot
        QSvgGenerator svgGen;
        svgGen.setFileName(fileName);
        svgGen.setSize(this->size());
        svgGen.setViewBox(this->rect());

        this->render(&svgGen);
    }

    if(fileName.contains(".png", Qt::CaseInsensitive))
    {
        QImage image(this->size(), QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        this->render(&painter);
        image.save(fileName);
    }
}


//*************************************************************************************************************

void ButterflyView::setAverageInformationMap(const QMap<double, AverageSelectionInfo>& mapAvr)
{
    m_qMapAverageColor = mapAvr;

    update();
}


//*************************************************************************************************************

void ButterflyView::setChannelInfoModel(QSharedPointer<ChannelInfoModel> &pChannelInfoModel)
{
    m_pChannelInfoModel = pChannelInfoModel;
}


//*************************************************************************************************************

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


//*************************************************************************************************************

void ButterflyView::paintEvent(QPaintEvent* paintEvent)
{
    QPainter painter(this);

    painter.save();
    painter.setBrush(QBrush(m_colCurrentBackgroundColor));
    painter.drawRect(QRect(0,0,this->width()-1,this->height()-1));
    painter.restore();

    painter.setRenderHint(QPainter::Antialiasing, false);

    if(m_bIsInit)
    {
        //Draw baseline correction area
        if(m_pEvokedModel->getBaselineInfo().first.toString() != "None" &&
                m_pEvokedModel->getBaselineInfo().second.toString() != "None") {
            float from = m_pEvokedModel->getBaselineInfo().first.toFloat();
            float to = m_pEvokedModel->getBaselineInfo().second.toFloat();

            painter.save();
            painter.setPen(QPen(Qt::red, 1, Qt::DashLine));
            painter.setBrush(Qt::red);
            painter.setOpacity(0.1);

            float fDx = (float)(this->width()) / ((float)m_pEvokedModel->getNumSamples());

            float fromSamp = ((from)*m_pEvokedModel->getSamplingFrequency())+m_pEvokedModel->getNumPreStimSamples();
            float posX = fDx*(fromSamp);
            float toSamp = ((to)*m_pEvokedModel->getSamplingFrequency())+m_pEvokedModel->getNumPreStimSamples();
            float width = fDx*(toSamp-fromSamp);

            QRect rect(posX,0,width,this->height());

            painter.drawRect(rect);

            painter.restore();
        }

        //Stimulus bar
        if(m_pEvokedModel->getNumSamples() > 0) {
            painter.save();
            painter.setPen(QPen(Qt::red, 1, Qt::DashLine));

            float fDx = (float)(this->width()) / ((float)m_pEvokedModel->getNumSamples());
            float posX = fDx * ((float)m_pEvokedModel->getNumPreStimSamples());
            painter.drawLine(posX, 1, posX, this->height());

            painter.drawText(QPointF(posX+5,this->rect().bottomRight().y()-5), QString("0ms / Stimulus"));

            painter.restore();
        }

        //Vertical time spacers
        if(m_pEvokedModel->getNumberOfTimeSpacers() > 0)
        {
            painter.save();
            QColor colorTimeSpacer = Qt::black;
            colorTimeSpacer.setAlphaF(0.5);
            painter.setPen(QPen(colorTimeSpacer, 1, Qt::DashLine));

            float yStart = this->rect().topLeft().y();
            float yEnd = this->rect().bottomRight().y();

            float fDx = 1;
            if(m_pEvokedModel->getNumSamples() != 0) {
                fDx = (float)(this->width()) / ((float)m_pEvokedModel->getNumSamples());
            }

            float sampleCounter = m_pEvokedModel->getNumPreStimSamples();
            int counter = 1;
            float timeDistanceMSec = 50.0;
            float timeDistanceSamples = (timeDistanceMSec/1000.0)*m_pEvokedModel->getSamplingFrequency(); //time distance corresponding to sampling frequency

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
            sampleCounter = m_pEvokedModel->getNumPreStimSamples();
            while(sampleCounter+timeDistanceSamples<m_pEvokedModel->getNumSamples()) {
                sampleCounter+=timeDistanceSamples;
                float x = fDx*sampleCounter;
                painter.drawLine(x, yStart, x, yEnd);
                painter.drawText(QPointF(x+5,yEnd-5), QString("%1ms").arg(timeDistanceMSec*counter));
                counter++;
            }

            painter.restore();
        }

        //Zero line
        if(m_pEvokedModel->getNumSamples() > 0) {
            painter.save();
            painter.setPen(QPen(Qt::black, 1, Qt::DashLine));

            painter.drawLine(0, this->height()/2, this->width(), this->height()/2);

            painter.restore();
        }

        painter.translate(0,this->height()/2);

        //Actual average data
        for(qint32 r = 0; r < m_iNumChannels; ++r) {
            if(m_lSelectedChannels.contains(r)) {
                qint32 kind = m_pEvokedModel->getKind(r);

                //Display only selected kinds
                switch(kind) {
                    case FIFFV_MEG_CH: {
                        qint32 unit = m_pEvokedModel->getUnit(r);
                        if(unit == FIFF_UNIT_T_M) {
                            if(m_bShowGRAD)
                                break;
                            else
                                continue;
                        }
                        else if(unit == FIFF_UNIT_T)
                        {
                            if(m_bShowMAG)
                                break;
                            else
                                continue;
                        }
                        continue;
                    }
                    case FIFFV_EEG_CH: {
                        if(m_bShowEEG)
                            break;
                        else
                            continue;
                    }
                    case FIFFV_EOG_CH: {
                        if(m_bShowEOG)
                            break;
                        else
                            continue;
                    }
                    case FIFFV_MISC_CH: {
                        if(m_bShowMISC)
                            break;
                        else
                            continue;
                    }
                    default:
                        continue;
                }

                painter.save();

                if(m_pEvokedModel->isFreezed()) {
                    QColor freezeColor = m_pEvokedModel->getColor(r);
                    freezeColor.setAlphaF(0.5);
                    painter.setPen(QPen(freezeColor, 1));
                } else {
                    painter.setPen(QPen(m_pEvokedModel->getColor(r), 1));
                }

                createPlotPath(r, painter);

                painter.restore();
            }
        }
    }

    return QWidget::paintEvent(paintEvent);
}


//*************************************************************************************************************

void ButterflyView::createPlotPath(qint32 row, QPainter& painter) const
{
    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)
    qint32 kind = m_pEvokedModel->getKind(row);
    float fMaxValue = 1e-9f;

    switch(kind) {
        case FIFFV_MEG_CH: {
            qint32 unit = m_pEvokedModel->getUnit(row);
            if(unit == FIFF_UNIT_T_M) { //gradiometers
                fMaxValue = 1e-10f;
                if(m_pEvokedModel->getScaling().contains(FIFF_UNIT_T_M))
                    fMaxValue = m_pEvokedModel->getScaling()[FIFF_UNIT_T_M];
            }
            else if(unit == FIFF_UNIT_T) //magnitometers
            {
//                if(m_pEvokedModel->getCoil(row) == FIFFV_COIL_BABY_MAG)
//                    fMaxValue = 1e-11f;
//                else
                fMaxValue = 1e-11f;

                if(m_pEvokedModel->getScaling().contains(FIFF_UNIT_T))
                    fMaxValue = m_pEvokedModel->getScaling()[FIFF_UNIT_T];
            }
            break;
        }

        case FIFFV_REF_MEG_CH: {  /*11/04/14 Added by Limin: MEG reference channel */
            fMaxValue = 1e-11f;
            if(m_pEvokedModel->getScaling().contains(FIFF_UNIT_T))
                fMaxValue = m_pEvokedModel->getScaling()[FIFF_UNIT_T];
            break;
        }
        case FIFFV_EEG_CH: {
            fMaxValue = 1e-4f;
            if(m_pEvokedModel->getScaling().contains(FIFFV_EEG_CH))
                fMaxValue = m_pEvokedModel->getScaling()[FIFFV_EEG_CH];
            break;
        }
        case FIFFV_EOG_CH: {
            fMaxValue = 1e-3f;
            if(m_pEvokedModel->getScaling().contains(FIFFV_EOG_CH))
                fMaxValue = m_pEvokedModel->getScaling()[FIFFV_EOG_CH];
            break;
        }
        case FIFFV_STIM_CH: {
            fMaxValue = 5;
            if(m_pEvokedModel->getScaling().contains(FIFFV_STIM_CH))
                fMaxValue = m_pEvokedModel->getScaling()[FIFFV_STIM_CH];
            break;
        }
        case FIFFV_MISC_CH: {
            fMaxValue = 1e-3f;
            if(m_pEvokedModel->getScaling().contains(FIFFV_MISC_CH))
                fMaxValue = m_pEvokedModel->getScaling()[FIFFV_MISC_CH];
            break;
        }
    }

    float fValue;
    float fScaleY = this->height()/(2*fMaxValue);

    //restrictions for paint performance
    float fWinMaxVal = ((float)this->height()-2)/2.0f;
    qint32 iDownSampling = (m_pEvokedModel->getNumSamples() * 4 / (this->width()-2));
    if(iDownSampling < 1) {
        iDownSampling = 1;
    }

    QPointF qSamplePosition;

    float fDx = (float)(this->width()-2) / ((float)m_pEvokedModel->getNumSamples()-1.0f);//((float)option.rect.width()) / m_pEvokedModel->getMaxSamples();

    QList<DISPLIB::AvrTypeRowVector> rowVec = m_pEvokedModel->data(row,1).value<QList<DISPLIB::AvrTypeRowVector> >();

    //Do for all average types
    for(int j = 0; j < rowVec.size(); ++j) {
        if(m_qMapAverageColor[rowVec.at(j).first].active) {
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
                fValue = (val/*-rowVec.at(j)[m_pEvokedModel->getNumPreStimSamples()-1]*/)*fScaleY;//ToDo -> -2 PreStim is one too short

                float newY = y_base+fValue;

                qSamplePosition.setY(-newY);
                qSamplePosition.setX(path.currentPosition().x());

                path.moveTo(qSamplePosition);
            }

            //create lines from one to the next sample
            qint32 i;
            for(i = 1; i < rowVec.at(j).second.cols() && path.elementCount() <= this->width(); i += dsFactor) {
                float val = /*rowVec.at(j)[m_pEvokedModel->getNumPreStimSamples()-1] - */rowVec.at(j).second[i]; //remove first sample data[0] as offset
                fValue = val*fScaleY;

                //Cut plotting if out of widget area
                fValue = fValue > fWinMaxVal ? fWinMaxVal : fValue < -fWinMaxVal ? -fWinMaxVal : fValue;

                float newY = y_base+fValue;

                qSamplePosition.setY(-newY);

                qSamplePosition.setX(path.currentPosition().x()+fDx);

                path.lineTo(qSamplePosition);
            }

        //    //create lines from one to the next sample for last path
        //    qint32 sample_offset = m_pEvokedModel->numVLines() + 1;
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
