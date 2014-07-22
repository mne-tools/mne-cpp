
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimebutterflyplot.h"

#include <time.h>


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
{
}


//*************************************************************************************************************

void RealTimeButterflyPlot::dataUpdate(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    if(!m_bIsInit && m_pRealTimeEvokedModel->isInit())
    {
        qsrand(time(NULL));
        m_iNumChannels = m_pRealTimeEvokedModel->rowCount();
        m_qListColors.clear();
        for(qint32 i = 0; i < m_iNumChannels; ++i)
            m_qListColors.append(QColor(qrand() % 256, qrand() % 256, qrand() % 256));

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
        painter.translate(0,this->height()/2);

        for(qint32 r = 0; r < m_iNumChannels; ++r)
        {
            painter.save();
            painter.setPen(QPen(m_qListColors[r], 1));

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
//        case FIFFV_MEG_CH: {
//            qint32 unit = m_pRealTimeEvokedModel->getUnit(row);
//            if(unit == FIFF_UNIT_T_M) {
//                fMaxValue = 1e-10f;// m_qSettings.value("RawDelegate/max_meg_grad").toDouble();
//            }
//            else if(unit == FIFF_UNIT_T)
//            {
//                if(m_pRealTimeEvokedModel->getCoil(row) == FIFFV_COIL_BABY_MAG)
//                    fMaxValue = 1e-4f;
//                else
//                    fMaxValue = 1e-11f;// m_qSettings.value("RawDelegate/max_meg_mag").toDouble();
//            }
//            break;
//        }
        case FIFFV_EEG_CH: {
            fMaxValue = 1e-5f;// m_qSettings.value("RawDelegate/max_eeg").toDouble();
            break;
        }
//        case FIFFV_EOG_CH: {
//            fMaxValue = 1e-3f; //m_qSettings.value("RawDelegate/max_eog").toDouble();
//            break;
//        }
        default:
            return;
    }

    float fValue;
    float fScaleY = this->height()/(2*fMaxValue);

    //restrictions for paint performance
    float fWinMaxVal = ((float)this->height()-2)/2.0f;
    qint32 iDownSampling = (m_pRealTimeEvokedModel->getNumSamples() * 8 / (this->width()-2));
    if(iDownSampling < 1)
        iDownSampling = 1;

    float y_base = path.currentPosition().y();
    QPointF qSamplePosition;

    float fDx = (float)(this->width()-2) / (float)m_pRealTimeEvokedModel->getNumSamples();//((float)option.rect.width()) / t_pModel->getMaxSamples();
    fDx *= iDownSampling;

    RowVectorXd rowVec = m_pRealTimeEvokedModel->data(row,1).value<RowVectorXd>();
    //Move to initial starting point
    if(rowVec.size() > 0)
    {
//        float val = data[0];
        fValue = 0;//(val-data[0])*fScaleY;

        float newY = y_base+fValue;

        qSamplePosition.setY(newY);
        qSamplePosition.setX(path.currentPosition().x());

        path.moveTo(qSamplePosition);
    }

    //create lines from one to the next sample
    qint32 i;
    for(i = 1; i < rowVec.size(); i += iDownSampling) {
        float val = rowVec[i] - rowVec[0]; //remove first sample data[0] as offset
        fValue = val*fScaleY;

        fValue = fValue > fWinMaxVal ? fWinMaxVal : fValue < -fWinMaxVal ? -fWinMaxVal : fValue;

        float newY = y_base+fValue;

        qSamplePosition.setY(newY);
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
