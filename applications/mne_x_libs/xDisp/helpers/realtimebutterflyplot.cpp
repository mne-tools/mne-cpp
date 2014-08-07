
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
, showGRAD(false)
, showEEG(false)
, showEOG(false)
, showMISC(false)
{
}


//*************************************************************************************************************

void RealTimeButterflyPlot::dataUpdate(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
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

    if(m_bIsInit)
    {
        //Stimulus bar
        if(m_pRealTimeEvokedModel->getNumSamples() > 0)
        {

            painter.save();
            painter.setPen(QPen(Qt::black, 1, Qt::DashLine));

            float fDx = (float)(this->width()-2) / (float)m_pRealTimeEvokedModel->getNumSamples();
            float posX = fDx * ((float)m_pRealTimeEvokedModel->getNumPreStimSamples() + 1.0f);
            painter.drawLine(posX, 1, posX, this->height()-2);

            painter.restore();
        }

        painter.translate(0,this->height()/2);

        for(qint32 r = 0; r < m_iNumChannels; ++r)
        {
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
            painter.setPen(QPen(m_pRealTimeEvokedModel->getColor(r), 1));

            QPainterPath path(QPointF(1,0));
            createPlotPath(r,path);

            painter.drawPath(path);

            painter.restore();
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
            qint32 unit = m_pRealTimeEvokedModel->getUnit(row);
            if(unit == FIFF_UNIT_T_M)
                fMaxValue = fMaxGRAD;
            else if(unit == FIFF_UNIT_T)
                fMaxValue = fMaxMAG;
            break;
        }
        case FIFFV_EEG_CH: {
            fMaxValue = fMaxEEG;
            break;
        }
        case FIFFV_EOG_CH: {
            fMaxValue = fMaxEOG;
            break;
        }
        case FIFFV_MISC_CH: {
            fMaxValue = fMaxMISC;
            break;
        }
        default:
            return;
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

    float fDx = (float)(this->width()-2) / (float)m_pRealTimeEvokedModel->getNumSamples();//((float)option.rect.width()) / t_pModel->getMaxSamples();
//    fDx *= iDownSampling;

    RowVectorXd rowVec = m_pRealTimeEvokedModel->data(row,1).value<RowVectorXd>();
    //Move to initial starting point
    if(rowVec.size() > 0)
    {
        float val = rowVec[0];
        fValue = (val-rowVec[m_pRealTimeEvokedModel->getNumPreStimSamples()-1])*fScaleY;//ToDo -> -2 PreStim is one too short

        float newY = y_base+fValue;

        qSamplePosition.setY(newY);
        qSamplePosition.setX(path.currentPosition().x());

        path.moveTo(qSamplePosition);
    }

    //create lines from one to the next sample
    qint32 i;
    for(i = 1; i < rowVec.size(); ++i) {

        if(i != m_pRealTimeEvokedModel->getNumPreStimSamples() - 2)
        {
            float val = rowVec[m_pRealTimeEvokedModel->getNumPreStimSamples()-1] - rowVec[i]; //remove first sample data[0] as offset
            fValue = val*fScaleY;

            fValue = fValue > fWinMaxVal ? fWinMaxVal : fValue < -fWinMaxVal ? -fWinMaxVal : fValue;

            float newY = y_base+fValue;

            qSamplePosition.setY(newY);
        }
        else
            qSamplePosition.setY(y_base);


        qSamplePosition.setX(path.currentPosition().x()+fDx);

        path.lineTo(qSamplePosition);
    }

//    //create lines from one to the next sample for last path
//    qint32 sample_offset = t_pModel->numVLines() + 1;
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
