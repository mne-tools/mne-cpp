#include "realtimemultisamplearraydelegate.h"

#include "realtimemultisamplearraymodel.h"

#include <QPainter>
#include <QPainterPath>
#include <QDebug>
#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeMultiSampleArrayDelegate::RealTimeMultiSampleArrayDelegate(QObject *parent)
: QAbstractItemDelegate(parent)
{
    m_fPlotHeight = 80;//m_qSettings.value("RawDelegate/plotheight").toDouble();
    m_fDx = 1.0f;//m_qSettings.value("RawDelegate/dx").toDouble();
    m_nhlines = 10;//m_qSettings.value("RawDelegate/nhlines").toDouble();

//    Q_UNUSED(parent);
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    switch(index.column()) {
        case 0: { //chnames
            painter->save();

            painter->rotate(-90);
            painter->drawText(QRectF(-option.rect.y()-m_fPlotHeight,0,m_fPlotHeight,20),Qt::AlignCenter,index.model()->data(index,Qt::DisplayRole).toString());

            painter->restore();
            break;
        }
        case 1: { //data plot
            painter->save();

            //draw special background when channel is marked as bad
            QVariant v = index.model()->data(index,Qt::BackgroundRole);
//            if(v.canConvert<QBrush>() && !(option.state & QStyle::State_Selected)) {
//                QPointF oldBO = painter->brushOrigin();
//                painter->setBrushOrigin(option.rect.topLeft());
//                painter->fillRect(option.rect, qvariant_cast<QBrush>(v));
//                painter->setBrushOrigin(oldBO);
//            }

//            //Highlight selected channels
//            if(option.state & QStyle::State_Selected) {
//                QPointF oldBO = painter->brushOrigin();
//                painter->setBrushOrigin(option.rect.topLeft());
//                painter->fillRect(option.rect, option.palette.highlight());
//                painter->setBrushOrigin(oldBO);
//            }

            //Get data
            QVariant variant = index.model()->data(index,Qt::DisplayRole);
            QVector<float> data = variant.value< QVector<float> >();

            if(data.size()>0)
            {
    //            const RealTimeMultiSampleArrayModel* t_rtmsaModel = (static_cast<const RealTimeMultiSampleArrayModel*>(index.model()));

                QPainterPath path(QPointF(option.rect.x(),option.rect.y()));//QPointF(option.rect.x()+t_rtmsaModel->relFiffCursor()-1,option.rect.y()));

    //            //Plot grid
    //            painter->setRenderHint(QPainter::Antialiasing, false);
    //            createGridPath(path,data);

    //            painter->save();
    //            QPen pen;
    //            pen.setStyle(Qt::DotLine);
    //            pen.setWidthF(0.5);
    //            painter->setPen(pen);
    //            painter->drawPath(path);
    //            painter->restore();

    //            //Plot data path
    //            path = QPainterPath(QPointF(0,0));//QPointF(option.rect.x()+t_rtmsaModel->relFiffCursor(),option.rect.y()));

                createPlotPath(index,path,data);

                painter->translate(0,m_fPlotHeight/2);

                painter->setRenderHint(QPainter::Antialiasing, true);
                painter->drawPath(path);

                painter->restore();
            }
            break;
        }
    }

}


//*************************************************************************************************************

QSize RealTimeMultiSampleArrayDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size;

    switch(index.column()) {
    case 0:
        size = QSize(20,m_fPlotHeight);
        break;
    case 1:
        QList< QVector<float> > data = index.model()->data(index).value< QList<QVector<float> > >();
//        qint32 nsamples = (static_cast<const RealTimeMultiSampleArrayModel*>(index.model()))->lastSample()-(static_cast<const RealTimeMultiSampleArrayModel*>(index.model()))->firstSample();

//        size = QSize(nsamples*m_dDx,m_dPlotHeight);
        break;
    }

    Q_UNUSED(option);

    return size;
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayDelegate::createPlotPath(const QModelIndex &index, QPainterPath& path, QVector<float>& data) const
{
    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)
    qint32 kind = (static_cast<const RealTimeMultiSampleArrayModel*>(index.model()))->m_qListChInfo[index.row()].getKind();
    float fMaxValue = 1e-9f;

    switch(kind) {
        case FIFFV_MEG_CH: {
            qint32 unit = (static_cast<const RealTimeMultiSampleArrayModel*>(index.model()))->m_qListChInfo[index.row()].getUnit();
            if(unit == FIFF_UNIT_T_M) {
                fMaxValue = 1e-10f;// m_qSettings.value("RawDelegate/max_meg_grad").toDouble();
            }
            else if(unit == FIFF_UNIT_T)
                fMaxValue = 1e-11f;// m_qSettings.value("RawDelegate/max_meg_mag").toDouble();
            break;
        }
        case FIFFV_EEG_CH: {
            fMaxValue = 1e-4f;// m_qSettings.value("RawDelegate/max_eeg").toDouble();
            break;
        }
        case FIFFV_EOG_CH: {
            fMaxValue = 1e-3; //m_qSettings.value("RawDelegate/max_eog").toDouble();
            break;
        }
        case FIFFV_STIM_CH: {
            fMaxValue = 5; //m_qSettings.value("RawDelegate/max_stim").toDouble();
            break;
        }
    }

    float fValue;
    float fScaleY = m_fPlotHeight/(2*fMaxValue);

    float y_base = path.currentPosition().y();
    QPointF qSamplePosition;

    //create lines from one to the next sample
    for(qint32 i = 0; i < data.size(); ++i) {
        float val = data[i];
        fValue = val*fScaleY;

        float newY = y_base+fValue;

        qSamplePosition.setY(newY);
        qSamplePosition.setX(path.currentPosition().x()+m_fDx);

        path.lineTo(qSamplePosition);
    }

//    qDebug("Plot-PainterPath created!");
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayDelegate::createGridPath(QPainterPath& path, QList< QVector<float> >& data) const
{
    //horizontal lines
    float distance = m_fPlotHeight/m_nhlines;

    QPointF startpos = path.currentPosition();
    QPointF endpoint(path.currentPosition().x()+data[0].size()*data.size()*m_fDx,path.currentPosition().y());

    for(qint8 i=0; i < m_nhlines-1; ++i) {
        endpoint.setY(endpoint.y()+distance);
        path.moveTo(startpos.x(),endpoint.y());
        path.lineTo(endpoint);
    }

//    qDebug("Grid-PainterPath created!");
}
