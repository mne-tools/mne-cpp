
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

using namespace XDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeButterflyPlot::RealTimeButterflyPlot(QWidget *parent)
: QWidget(parent)
, m_pRealTimeEvokedModel(NULL)
, m_bIsInit(false)
, m_iNumChannels(-1)
, showMAG(true)
, showGRAD(true)
, showEEG(true)
, showEOG(true)
, showMISC(true)
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

void RealTimeButterflyPlot::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    painter.save();
    painter.setBrush(QBrush(Qt::white));
    painter.drawRect(QRect(0,0,this->width()-1,this->height()-1));
    painter.restore();

    painter.setRenderHint(QPainter::Antialiasing, true);

    if(m_bIsInit)
    {
        //Draw baseline correction area
        if(m_pRealTimeEvokedModel->getBaselineInfo().first.toString() != "None" &&
                m_pRealTimeEvokedModel->getBaselineInfo().second.toString() != "None") {
            int from = m_pRealTimeEvokedModel->getBaselineInfo().first.toInt();
            int to = m_pRealTimeEvokedModel->getBaselineInfo().second.toInt();

            painter.save();
            painter.setPen(QPen(Qt::red, 1, Qt::DashLine));
            painter.setBrush(Qt::red);
            painter.setOpacity(0.15);

            float fDx = (float)(this->width()) / ((float)m_pRealTimeEvokedModel->getNumSamples());

            float fromSamp = ((from/1000.0)*m_pRealTimeEvokedModel->getSamplingFrequency())+m_pRealTimeEvokedModel->getNumPreStimSamples();
            float posX = fDx*(fromSamp);
            float toSamp = ((to/1000.0)*m_pRealTimeEvokedModel->getSamplingFrequency())+m_pRealTimeEvokedModel->getNumPreStimSamples();
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
            float fDx = (float)(this->width()) / ((float)m_pRealTimeEvokedModel->getNumSamples());

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

        for(qint32 r = 0; r < m_iNumChannels; ++r) {
            if(m_lSelectedChannels.contains(r)) {
                qint32 kind = m_pRealTimeEvokedModel->getKind(r);

                //Display only selected kinds
                switch(kind) {
                    case FIFFV_MEG_CH: {
                        qint32 unit = m_pRealTimeEvokedModel->getUnit(r);
                        if(unit == FIFF_UNIT_T_M) {
                            if(showGRAD)
                                break;
                            else
                                continue;
                        }
                        else if(unit == FIFF_UNIT_T)
                        {
                            if(showMAG)
                                break;
                            else
                                continue;
                        }
                        continue;
                    }
                    case FIFFV_EEG_CH: {
                        if(showEEG)
                            break;
                        else
                            continue;
                    }
                    case FIFFV_EOG_CH: {
                        if(showEOG)
                            break;
                        else
                            continue;
                    }
                    case FIFFV_MISC_CH: {
                        if(showMISC)
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
                } else
                    painter.setPen(QPen(m_pRealTimeEvokedModel->getColor(r), 1));

                QPainterPath path(QPointF(1,0));
                createPlotPath(r,path);

                painter.drawPath(path);

                painter.restore();
            }
        }
    }
}


//*************************************************************************************************************

void RealTimeButterflyPlot::createPlotPath(qint32 row, QPainterPath& path) const
{
    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)
    qint32 kind = m_pRealTimeEvokedModel->getKind(row);
    float fMaxValue = 1e-9f;

    switch(kind) {
        case FIFFV_MEG_CH: {
            qint32 unit =m_pRealTimeEvokedModel->getUnit(row);
            if(unit == FIFF_UNIT_T_M) { //gradiometers
                fMaxValue = 1e-10f;
                if(m_pRealTimeEvokedModel->getScaling().contains(FIFF_UNIT_T_M))
                    fMaxValue = m_pRealTimeEvokedModel->getScaling()[FIFF_UNIT_T_M];
            }
            else if(unit == FIFF_UNIT_T) //magnitometers
            {
                if(m_pRealTimeEvokedModel->getCoil(row) == FIFFV_COIL_BABY_MAG)
                    fMaxValue = 1e-11f;
                else
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
    if(iDownSampling < 1)
        iDownSampling = 1;

    float y_base = path.currentPosition().y();
    QPointF qSamplePosition;

    float fDx = (float)(this->width()-2) / ((float)m_pRealTimeEvokedModel->getNumSamples()-1.0f);//((float)option.rect.width()) / m_pRealTimeEvokedModel->getMaxSamples();
//    fDx *= iDownSampling;

    RowVectorXd rowVec = m_pRealTimeEvokedModel->data(row,1).value<RowVectorXd>();
    //Move to initial starting point
    if(rowVec.size() > 0)
    {
        float val = rowVec[0];
        fValue = (val/*-rowVec[m_pRealTimeEvokedModel->getNumPreStimSamples()-1]*/)*fScaleY;//ToDo -> -2 PreStim is one too short

        float newY = y_base+fValue;

        qSamplePosition.setY(newY);
        qSamplePosition.setX(path.currentPosition().x());

        path.moveTo(qSamplePosition);
    }

    //create lines from one to the next sample
    qint32 i;
    for(i = 1; i < rowVec.size(); ++i) {

//        if(i != m_pRealTimeEvokedModel->getNumPreStimSamples() - 2)
//        {
            float val = /*rowVec[m_pRealTimeEvokedModel->getNumPreStimSamples()-1] - */rowVec[i]; //remove first sample data[0] as offset
            fValue = val*fScaleY;

            fValue = fValue > fWinMaxVal ? fWinMaxVal : fValue < -fWinMaxVal ? -fWinMaxVal : fValue;

            float newY = y_base+fValue;

            qSamplePosition.setY(newY);
//        }
//        else
//            qSamplePosition.setY(y_base);


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
}


//*************************************************************************************************************

void RealTimeButterflyPlot::setSettings(const QList< Modality >& p_qListModalities)
{
    for(qint32 i = 0; i < p_qListModalities.size(); ++i)
    {
        if(p_qListModalities[i].m_sName == ("GRAD"))
        {
            showGRAD = p_qListModalities[i].m_bActive;
            fMaxGRAD = p_qListModalities[i].m_fNorm;
        }

        if(p_qListModalities[i].m_sName == ("MAG"))
        {
            showMAG = p_qListModalities[i].m_bActive;
            fMaxMAG = p_qListModalities[i].m_fNorm;
        }
        if(p_qListModalities[i].m_sName == ("EEG"))
        {
            showEEG = p_qListModalities[i].m_bActive;
            fMaxEEG = p_qListModalities[i].m_fNorm;

        }
        if(p_qListModalities[i].m_sName == ("EOG"))
        {
            showEOG = p_qListModalities[i].m_bActive;
            fMaxEOG = p_qListModalities[i].m_fNorm;

        }
        if(p_qListModalities[i].m_sName == ("MISC"))
        {
            showMISC = p_qListModalities[i].m_bActive;
            fMaxMISC = p_qListModalities[i].m_fNorm;

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
