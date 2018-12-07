//=============================================================================================================
/**
* @file     rawdelegate.cpp
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     January, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Florian Schlembach, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
* @brief    Definition of delegate of mne_browse
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rawdelegate.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QBrush>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RawDelegate::RawDelegate(QObject *parent)
: QAbstractItemDelegate(parent)
, m_qSettings()
, m_bShowSelectedEventsOnly(false)
, m_bActivateEvents(true)
, m_bRemoveDC(false)
, m_dMaxValue(65530)
, m_dScaleY(1)
, m_pScaleWindow(Q_NULLPTR)
{
    m_iDefaultPlotHeight = DELEGATE_PLOT_HEIGHT;
    m_dDx = DELEGATE_DX;
    m_nhlines = DELEGATE_NHLINES;

    m_pEventModel = new EventModel(NULL);
    m_pEventView = new QTableView(NULL);
    m_pRawView = new QTableView(NULL);

    //Init m_scaleMap
    m_scaleMap["MEG_grad"] = 400 * 1e-15 * 100; //*100 because data in fiff files is stored as fT/m not fT/cm
    m_scaleMap["MEG_mag"] = 1.2 * 1e-12;
    m_scaleMap["MEG_EEG"] = 30 * 1e-06;
    m_scaleMap["MEG_EOG"] = 150 * 1e-06;
    m_scaleMap["MEG_EMG"] = 1 * 1e-03;
    m_scaleMap["MEG_ECG"] = 1 * 1e-03;
    m_scaleMap["MEG_MISC"] = 1 * 1;
    m_scaleMap["MEG_STIM"] = 5 * 1;
}


//*************************************************************************************************************

void RawDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    float t_fPlotHeight = option.rect.height();
    painter->setRenderHint(QPainter::Antialiasing, true);

    switch(index.column()) {
    case 0: { //chnames
        painter->save();

        painter->rotate(-90);
        painter->drawText(QRectF(-option.rect.y()-t_fPlotHeight,0,t_fPlotHeight,20),Qt::AlignCenter,index.model()->data(index,Qt::DisplayRole).toString());

        painter->restore();
        break;
    }
    case 1: { //data plot
        painter->save();

        //draw special background when channel is marked as bad
        QVariant v = index.model()->data(index,Qt::BackgroundRole);
        if(v.canConvert<QBrush>()/* && !(option.state & QStyle::State_Selected)*/) {
            QPointF oldBO = painter->brushOrigin();
            painter->setBrushOrigin(option.rect.topLeft());
            painter->fillRect(option.rect, qvariant_cast<QBrush>(v));
            painter->setBrushOrigin(oldBO);
        }

        //Get data and mean
        QVariant variant = index.model()->data(index,Qt::DisplayRole);
        QList<RowVectorPair> listPairs = variant.value<QList<RowVectorPair> >();

        double channelMean = 0;
        if(m_bRemoveDC) {
            QModelIndex meanIndex = index.model()->index(index.row(),2);
            QVariant channelMeanVariant = index.model()->data(meanIndex,RawModelRoles::GetChannelMean);
            channelMean = channelMeanVariant.toDouble();
        }

        const RawModel* t_rawModel = (static_cast<const RawModel*>(index.model()));

        QPainterPath path(QPointF(option.rect.x()+t_rawModel->relFiffCursor()-1,option.rect.y()));

        //Plot grid
        createGridPath(path,option,listPairs);

        painter->save();
        QPen pen;
        pen.setStyle(Qt::DotLine);
        pen.setWidthF(0.5);
        painter->setPen(pen);
        painter->drawPath(path);
        painter->restore();

        //Plot data path
        path = QPainterPath(QPointF(option.rect.x()+t_rawModel->relFiffCursor(), option.rect.y()));
        createPlotPath(index, option, path, listPairs, channelMean);

        if(option.state & QStyle::State_Selected) {
            pen.setStyle(Qt::SolidLine);
            pen.setWidthF(1);
            pen.setColor(Qt::red);
            painter->setPen(pen);
        }

        painter->translate(0,t_fPlotHeight/2);
        painter->drawPath(path);
        painter->restore();

        //Plot events
        painter->save();
        if(m_pEventModel->rowCount()!=0 && m_bActivateEvents)
            plotEvents(index, option, painter);
        painter->restore();

        break;
        }
    }
}


//*************************************************************************************************************

QSize RawDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size;

    switch(index.column()) {
    case 0:
        size = QSize(20,option.rect.height());
        break;
    case 1:
        QList<RowVectorPair> listPairs = index.model()->data(index).value<QList<RowVectorPair> >();
        qint32 nsamples = (static_cast<const RawModel*>(index.model()))->lastSample()-(static_cast<const RawModel*>(index.model()))->firstSample();

        size = QSize(nsamples*m_dDx,option.rect.height());
        break;
    }

    Q_UNUSED(option);

    return size;
}


//*************************************************************************************************************

void RawDelegate::setModelView(EventModel *eventModel, QTableView* eventView, QTableView* rawView)
{
    m_pEventModel = eventModel;
    m_pEventView = eventView;
    m_pRawView = rawView;
}


//*************************************************************************************************************

void RawDelegate::setScaleMap(const QMap<QString,double> &scaleMap)
{
    m_scaleMap = scaleMap;
}


//*************************************************************************************************************

void RawDelegate::createPlotPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, QList<RowVectorPair>& listPairs, double channelMean) const
{
    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)
    qint32 kind = (static_cast<const RawModel*>(index.model()))->m_chInfolist[index.row()].kind;
    double dMaxValue = 1e-9;

    switch(kind) {
    case FIFFV_MEG_CH: {
        qint32 unit = (static_cast<const RawModel*>(index.model()))->m_pfiffIO->m_qlistRaw[0]->info.chs[index.row()].unit;
        if(unit == FIFF_UNIT_T_M) {
            dMaxValue = m_scaleMap["MEG_grad"];
        }
        else if(unit == FIFF_UNIT_T)
            dMaxValue = m_scaleMap["MEG_mag"];
        break;
    }
    case FIFFV_EEG_CH: {
        dMaxValue = m_scaleMap["MEG_EEG"];
        break;
    }
    case FIFFV_EOG_CH: {
        dMaxValue = m_scaleMap["MEG_EOG"];
        break;
    }
    case FIFFV_STIM_CH: {
        dMaxValue = m_scaleMap["MEG_STIM"];
        break;
    }
    case FIFFV_EMG_CH: {
        dMaxValue = m_scaleMap["MEG_EMG"];
        break;
    }
    case FIFFV_MISC_CH: {
        dMaxValue = m_scaleMap["MEG_MISC"];
        break;
    }
    }

    double dValue;
    double dScaleY = option.rect.height()/(2*dMaxValue);

    double y_base = -path.currentPosition().y();
    QPointF qSamplePosition;

    path.moveTo(path.currentPosition().x(), -(y_base + ((*(listPairs[0].first) - channelMean)*dScaleY)));

    //plot all rows from list of pairs
    for(qint8 i=0; i < listPairs.size(); ++i) {
        //create lines from one to the next sample
        for(qint32 j=0; j < listPairs[i].second; ++j)
        {
            double val = *(listPairs[i].first+j);

            //subtract mean of the channel here (if wanted by the user)
            dValue = (val - channelMean)*dScaleY;

            double newY = y_base+dValue;

            qSamplePosition.setY(-newY);
            qSamplePosition.setX(path.currentPosition().x()+m_dDx);

            path.lineTo(qSamplePosition);
        }
    }

//    qDebug("Plot-PainterPath created!");
}


//*************************************************************************************************************

void RawDelegate::createGridPath(QPainterPath& path, const QStyleOptionViewItem &option, QList<RowVectorPair>& listPairs) const
{
    //horizontal lines
    double distance = double(option.rect.height()) / m_nhlines;

    QPointF startpos = path.currentPosition();
    QPointF endpoint(path.currentPosition().x()+listPairs[0].second*listPairs.size()*m_dDx,path.currentPosition().y());

    for(qint8 i=0; i < m_nhlines-1; ++i) {
        endpoint.setY(endpoint.y()+distance);
        path.moveTo(startpos.x(),endpoint.y());
        path.lineTo(endpoint);
    }

//    qDebug("Grid-PainterPath created!");
}


//*************************************************************************************************************

void RawDelegate::plotEvents(const QModelIndex &index, const QStyleOptionViewItem &option, QPainter* painter) const
{
    const RawModel* rawModel = static_cast<const RawModel*>(index.model());

    qint32 sampleRangeLow = rawModel->relFiffCursor();
    qint32 sampleRangeHigh = sampleRangeLow + rawModel->sizeOfPreloadedData();

    QPen pen;
    pen.setWidth(EVENT_MARKER_WIDTH);

    QColor colorTemp;

    QMap<int, QColor> eventTypeColor = m_pEventModel->getEventTypeColors();

    if(!m_bShowSelectedEventsOnly) { //Plot all events
        for(int i = 0; i<m_pEventModel->rowCount(); i++) {
            int sampleValue = m_pEventModel->data(m_pEventModel->index(i,0)).toInt();
            int type = m_pEventModel->data(m_pEventModel->index(i,2)).toInt();

            if(sampleValue>=sampleRangeLow && sampleValue<=sampleRangeHigh) {
                //Set color for pen depending on current event type
                pen.setColor(eventTypeColor.value(type, Qt::black));

                colorTemp = pen.color();
                colorTemp.setAlpha(EVENT_MARKER_OPACITY);
                pen.setColor(colorTemp);
                painter->setPen(pen);

                //Draw line from sample position (x) and highest to lowest y position of the column widget - Add -m_qSettings.value("EventDesignParameters/event_marker_width").toInt() to avoid painting ovre the edge of the column widget
                painter->drawLine(option.rect.x() + sampleValue, option.rect.y(), option.rect.x() + sampleValue, option.rect.y() + option.rect.height() - EVENT_MARKER_WIDTH);
            } // END for statement
        } // END if statement event in data range
    } // END if statement plot all
    else { //Only plot selected events
        QModelIndexList indexes = m_pEventView->selectionModel()->selectedIndexes();

        for(int i = 0; i<indexes.size(); i++) {
            qDebug()<<indexes.at(i).row();
            int currentRow = indexes.at(i).row();
            int sampleValue = m_pEventModel->data(m_pEventModel->index(currentRow,0)).toInt();
            int type = m_pEventModel->data(m_pEventModel->index(currentRow,2)).toInt();

            if(sampleValue>=sampleRangeLow && sampleValue<=sampleRangeHigh) {
                //qDebug()<<"currentRow"<<currentRow<<"sampleValue"<<sampleValue<<"sampleRangeLow"<<sampleRangeLow<<"sampleRangeHigh"<<sampleRangeHigh;

                //Set color for pen depending on current event type
                pen.setColor(eventTypeColor.value(type, Qt::black));

                colorTemp = pen.color();
                colorTemp.setAlpha(EVENT_MARKER_OPACITY);
                pen.setColor(colorTemp);
                painter->setPen(pen);

                //Draw line from sample position (x) and highest to lowest y position of the column widget - Add +m_qSettings.value("EventDesignParameters/event_marker_width").toInt() to avoid painting ovre the edge of the column widget
                painter->drawLine(option.rect.x() + sampleValue, option.rect.y(), option.rect.x() + sampleValue, option.rect.y() - option.rect.height() + EVENT_MARKER_WIDTH);
            } // END for statement
        } // END if statement
    } // END else statement
}

