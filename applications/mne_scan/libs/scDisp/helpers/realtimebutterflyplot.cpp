
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimebutterflyplot.h"
#include "../realtimeevokedwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QPainter>
#include <QDebug>
#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeButterflyPlot::RealTimeButterflyPlot(QWidget *parent)
: QWidget(parent)
, m_pRealTimeEvokedModel(NULL)
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

void RealTimeButterflyPlot::dataUpdate(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    Q_UNUSED(topLeft);
    Q_UNUSED(bottomRight);
    Q_UNUSED(roles);

    if(!m_bIsInit && m_pRealTimeEvokedModel->isInit())
    {
        m_iNumChannels = m_pRealTimeEvokedModel->rowCount();
        m_bIsInit = true;
    }

    update();
}


//*************************************************************************************************************

void RealTimeButterflyPlot::paintEvent(QPaintEvent* paintEvent)
{
    QPainter painter(this);

    painter.save();
    painter.setBrush(QBrush(m_colCurrentBackgroundColor));
    painter.drawRect(QRect(0,0,this->width()-1,this->height()-1));
    painter.restore();

    painter.setRenderHint(QPainter::Antialiasing, false);

    if(m_bIsInit)
    {
        QElapsedTimer time;
        time.start();

        //Draw baseline correction area
        if(m_pRealTimeEvokedModel->getBaselineInfo().first.toString() != "None" &&
                m_pRealTimeEvokedModel->getBaselineInfo().second.toString() != "None") {
            float from = m_pRealTimeEvokedModel->getBaselineInfo().first.toFloat();
            float to = m_pRealTimeEvokedModel->getBaselineInfo().second.toFloat();

            painter.save();
            painter.setPen(QPen(Qt::red, 1, Qt::DashLine));
            painter.setBrush(Qt::red);
            painter.setOpacity(0.1);

            float fDx = (float)(this->width()) / ((float)m_pRealTimeEvokedModel->getNumSamples());

            float fromSamp = ((from)*m_pRealTimeEvokedModel->getSamplingFrequency())+m_pRealTimeEvokedModel->getNumPreStimSamples();
            float posX = fDx*(fromSamp);
            float toSamp = ((to)*m_pRealTimeEvokedModel->getSamplingFrequency())+m_pRealTimeEvokedModel->getNumPreStimSamples();
            float width = fDx*(toSamp-fromSamp);

            QRect rect(posX,0,width,this->height());

            painter.drawRect(rect);

            painter.restore();
        }

        //Stimulus bar
        if(m_pRealTimeEvokedModel->getNumSamples() > 0) {
            painter.save();
            painter.setPen(QPen(Qt::red, 1, Qt::DashLine));

            float fDx = (float)(this->width()) / ((float)m_pRealTimeEvokedModel->getNumSamples());
            float posX = fDx * ((float)m_pRealTimeEvokedModel->getNumPreStimSamples());
            painter.drawLine(posX, 1, posX, this->height());

            painter.drawText(QPointF(posX+5,this->rect().bottomRight().y()-5), QString("0ms / Stimulus"));

            painter.restore();
        }

        //Vertical time spacers
        if(m_pRealTimeEvokedModel->getNumberOfTimeSpacers() > 0)
        {
            painter.save();
            QColor colorTimeSpacer = Qt::black;
            colorTimeSpacer.setAlphaF(0.5);
            painter.setPen(QPen(colorTimeSpacer, 1, Qt::DashLine));

            float yStart = this->rect().topLeft().y();
            float yEnd = this->rect().bottomRight().y();

            float fDx = 1;
            if(m_pRealTimeEvokedModel->getNumSamples() != 0) {
                fDx = (float)(this->width()) / ((float)m_pRealTimeEvokedModel->getNumSamples());
            }

            float sampleCounter = m_pRealTimeEvokedModel->getNumPreStimSamples();
            int counter = 1;
            float timeDistanceMSec = 50.0;
            float timeDistanceSamples = (timeDistanceMSec/1000.0)*m_pRealTimeEvokedModel->getSamplingFrequency(); //time distance corresponding to sampling frequency

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
            sampleCounter = m_pRealTimeEvokedModel->getNumPreStimSamples();
            while(sampleCounter+timeDistanceSamples<m_pRealTimeEvokedModel->getNumSamples()) {
                sampleCounter+=timeDistanceSamples;
                float x = fDx*sampleCounter;
                painter.drawLine(x, yStart, x, yEnd);
                painter.drawText(QPointF(x+5,yEnd-5), QString("%1ms").arg(timeDistanceMSec*counter));
                counter++;
            }

            painter.restore();
        }

        //Zero line
        if(m_pRealTimeEvokedModel->getNumSamples() > 0) {
            painter.save();
            painter.setPen(QPen(Qt::black, 1, Qt::DashLine));

            painter.drawLine(0, this->height()/2, this->width(), this->height()/2);

            painter.restore();
        }

        painter.translate(0,this->height()/2);

        //Actual average data
        for(qint32 r = 0; r < m_iNumChannels; ++r) {
            if(m_lSelectedChannels.contains(r)) {
                qint32 kind = m_pRealTimeEvokedModel->getKind(r);

                //Display only selected kinds
                switch(kind) {
                    case FIFFV_MEG_CH: {
                        qint32 unit = m_pRealTimeEvokedModel->getUnit(r);
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

                if(m_pRealTimeEvokedModel->isFreezed()) {
                    QColor freezeColor = m_pRealTimeEvokedModel->getColor(r);
                    freezeColor.setAlphaF(0.5);
                    painter.setPen(QPen(freezeColor, 1));
                } else {
                    painter.setPen(QPen(m_pRealTimeEvokedModel->getColor(r), 1));
                }

                createPlotPath(r, painter);

                painter.restore();
            }
        }        

        qInfo()<<time.elapsed()<<"Averaging Plot";
    }

    return QWidget::paintEvent(paintEvent);
}


//*************************************************************************************************************

void RealTimeButterflyPlot::createPlotPath(qint32 row, QPainter& painter) const
{
    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)
    qint32 kind = m_pRealTimeEvokedModel->getKind(row);
    float fMaxValue = 1e-9f;

    switch(kind) {
        case FIFFV_MEG_CH: {
            qint32 unit = m_pRealTimeEvokedModel->getUnit(row);
            if(unit == FIFF_UNIT_T_M) { //gradiometers
                fMaxValue = 1e-10f;
                if(m_pRealTimeEvokedModel->getScaling().contains(FIFF_UNIT_T_M))
                    fMaxValue = m_pRealTimeEvokedModel->getScaling()[FIFF_UNIT_T_M];
            }
            else if(unit == FIFF_UNIT_T) //magnitometers
            {
//                if(m_pRealTimeEvokedModel->getCoil(row) == FIFFV_COIL_BABY_MAG)
//                    fMaxValue = 1e-11f;
//                else
                fMaxValue = 1e-11f;

                if(m_pRealTimeEvokedModel->getScaling().contains(FIFF_UNIT_T))
                    fMaxValue = m_pRealTimeEvokedModel->getScaling()[FIFF_UNIT_T];
            }
            break;
        }

        case FIFFV_REF_MEG_CH: {  /*11/04/14 Added by Limin: MEG reference channel */
            fMaxValue = 1e-11f;
            if(m_pRealTimeEvokedModel->getScaling().contains(FIFF_UNIT_T))
                fMaxValue = m_pRealTimeEvokedModel->getScaling()[FIFF_UNIT_T];
            break;
        }
        case FIFFV_EEG_CH: {
            fMaxValue = 1e-4f;
            if(m_pRealTimeEvokedModel->getScaling().contains(FIFFV_EEG_CH))
                fMaxValue = m_pRealTimeEvokedModel->getScaling()[FIFFV_EEG_CH];
            break;
        }
        case FIFFV_EOG_CH: {
            fMaxValue = 1e-3f;
            if(m_pRealTimeEvokedModel->getScaling().contains(FIFFV_EOG_CH))
                fMaxValue = m_pRealTimeEvokedModel->getScaling()[FIFFV_EOG_CH];
            break;
        }
        case FIFFV_STIM_CH: {
            fMaxValue = 5;
            if(m_pRealTimeEvokedModel->getScaling().contains(FIFFV_STIM_CH))
                fMaxValue = m_pRealTimeEvokedModel->getScaling()[FIFFV_STIM_CH];
            break;
        }
        case FIFFV_MISC_CH: {
            fMaxValue = 1e-3f;
            if(m_pRealTimeEvokedModel->getScaling().contains(FIFFV_MISC_CH))
                fMaxValue = m_pRealTimeEvokedModel->getScaling()[FIFFV_MISC_CH];
            break;
        }
    }

    float fValue;
    float fScaleY = this->height()/(2*fMaxValue);

    //restrictions for paint performance
    float fWinMaxVal = ((float)this->height()-2)/2.0f;
    qint32 iDownSampling = (m_pRealTimeEvokedModel->getNumSamples() * 4 / (this->width()-2));
    if(iDownSampling < 1) {
        iDownSampling = 1;
    }

    QPointF qSamplePosition;

    float fDx = (float)(this->width()-2) / ((float)m_pRealTimeEvokedModel->getNumSamples()-1.0f);//((float)option.rect.width()) / m_pRealTimeEvokedModel->getMaxSamples();

    QList<SCDISPLIB::AvrTypeRowVector> rowVec = m_pRealTimeEvokedModel->data(row,1).value<QList<SCDISPLIB::AvrTypeRowVector> >();

    //Do for all average types
    for(int j = 0; j < rowVec.size(); ++j) {
        if(m_qMapAverageColor[rowVec.at(j).first].second.second) {
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
                fValue = (val/*-rowVec.at(j)[m_pRealTimeEvokedModel->getNumPreStimSamples()-1]*/)*fScaleY;//ToDo -> -2 PreStim is one too short

                float newY = y_base+fValue;

                qSamplePosition.setY(-newY);
                qSamplePosition.setX(path.currentPosition().x());

                path.moveTo(qSamplePosition);
            }

            //create lines from one to the next sample
            qint32 i;
            for(i = 1; i < rowVec.at(j).second.cols() && path.elementCount() <= this->width(); i += dsFactor) {
                float val = /*rowVec.at(j)[m_pRealTimeEvokedModel->getNumPreStimSamples()-1] - */rowVec.at(j).second[i]; //remove first sample data[0] as offset
                fValue = val*fScaleY;

                //Cut plotting if out of widget area
                fValue = fValue > fWinMaxVal ? fWinMaxVal : fValue < -fWinMaxVal ? -fWinMaxVal : fValue;

                float newY = y_base+fValue;

                qSamplePosition.setY(-newY);

                qSamplePosition.setX(path.currentPosition().x()+fDx);

                path.lineTo(qSamplePosition);
            }

        //    //create lines from one to the next sample for last path
        //    qint32 sample_offset = m_pRealTimeEvokedModel->numVLines() + 1;
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


//*************************************************************************************************************

void RealTimeButterflyPlot::setSettings(const QList< Modality >& p_qListModalities)
{
    for(qint32 i = 0; i < p_qListModalities.size(); ++i)
    {
        if(p_qListModalities[i].m_sName == ("GRAD"))
        {
            m_bShowGRAD = p_qListModalities[i].m_bActive;
            m_fMaxGRAD = p_qListModalities[i].m_fNorm;
        }

        if(p_qListModalities[i].m_sName == ("MAG"))
        {
            m_bShowMAG = p_qListModalities[i].m_bActive;
            m_fMaxGRAD = p_qListModalities[i].m_fNorm;
        }
        if(p_qListModalities[i].m_sName == ("EEG"))
        {
            m_bShowEEG = p_qListModalities[i].m_bActive;
            m_fMaxEEG = p_qListModalities[i].m_fNorm;

        }
        if(p_qListModalities[i].m_sName == ("EOG"))
        {
            m_bShowEOG = p_qListModalities[i].m_bActive;
            m_fMaxEOG = p_qListModalities[i].m_fNorm;

        }
        if(p_qListModalities[i].m_sName == ("MISC"))
        {
            m_bShowMISC = p_qListModalities[i].m_bActive;
            m_fMaxMISC = p_qListModalities[i].m_fNorm;

        }
    }
    update();
}


//*************************************************************************************************************

void RealTimeButterflyPlot::setSelectedChannels(const QList<int> &selectedChannels)
{
    m_lSelectedChannels = selectedChannels;

    update();
}


//*************************************************************************************************************

void RealTimeButterflyPlot::updateView()
{
    update();
}


//*************************************************************************************************************

void RealTimeButterflyPlot::setBackgroundColor(const QColor& backgroundColor)
{
    m_colCurrentBackgroundColor = backgroundColor;

    update();
}


//*************************************************************************************************************

const QColor& RealTimeButterflyPlot::getBackgroundColor()
{
    return m_colCurrentBackgroundColor;
}


//*************************************************************************************************************

void RealTimeButterflyPlot::setAverageInformationMap(const QMap<double, QPair<QColor, QPair<QString,bool> > >& mapAvr)
{
    m_qMapAverageColor = mapAvr;

    update();
}

