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

}


//*************************************************************************************************************

void RealTimeMultiSampleArrayDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    float t_fPlotHeight = option.rect.height();
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
//            QVariant v = index.model()->data(index,Qt::BackgroundRole);
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
            QList< QVector<float> > data = variant.value< QList< QVector<float> > >();


            const RealTimeMultiSampleArrayModel* t_pModel = static_cast<const RealTimeMultiSampleArrayModel*>(index.model());

            if(data.size()>0)
            {
    //            const RealTimeMultiSampleArrayModel* t_rtmsaModel = (static_cast<const RealTimeMultiSampleArrayModel*>(index.model()));

                QPainterPath path(QPointF(option.rect.x(),option.rect.y()));//QPointF(option.rect.x()+t_rtmsaModel->relFiffCursor()-1,option.rect.y()));

                //Plot grid
                painter->setRenderHint(QPainter::Antialiasing, false);
                createGridPath(index, option, path,data);

                painter->save();
                QPen pen;
                pen.setStyle(Qt::DotLine);
                pen.setWidthF(0.5);
                painter->setPen(pen);
                painter->drawPath(path);
                painter->restore();

                //Plot data path
                path = QPainterPath(QPointF(option.rect.x(),option.rect.y()));//QPointF(option.rect.x()+t_rtmsaModel->relFiffCursor(),option.rect.y()));
                QPainterPath lastPath(QPointF(option.rect.x(),option.rect.y()));

                createPlotPath(index, option, path, lastPath, data[0], data[1]);

                painter->save();
                painter->translate(0,t_fPlotHeight/2);
                painter->setRenderHint(QPainter::Antialiasing, true);

                if(option.state & QStyle::State_Selected)
                    painter->setPen(QPen(t_pModel->isFreezed() ? Qt::darkRed : Qt::red, 1, Qt::SolidLine));
                else
                    painter->setPen(QPen(t_pModel->isFreezed() ? Qt::darkGray : Qt::darkBlue, 1, Qt::SolidLine));

                painter->drawPath(path);
                painter->restore();

                //Plot last data path
                painter->translate(0,t_fPlotHeight/2);
                painter->setRenderHint(QPainter::Antialiasing, true);
                if(option.state & QStyle::State_Selected)
                    painter->setPen(QPen(t_pModel->isFreezed() ? Qt::darkRed : Qt::red, 1, Qt::SolidLine));
                else
                    painter->setPen(QPen(t_pModel->isFreezed() ? Qt::darkGray : Qt::darkBlue, 1, Qt::SolidLine));
                painter->drawPath(lastPath);

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
        size = QSize(20,option.rect.height());
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

void RealTimeMultiSampleArrayDelegate::createPlotPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, QPainterPath& lastPath, QVector<float>& data, QVector<float>& lastData) const
{
    const RealTimeMultiSampleArrayModel* t_pModel = static_cast<const RealTimeMultiSampleArrayModel*>(index.model());

    //get maximum range of respective channel type (range value in FiffChInfo does not seem to contain a reasonable value)
    qint32 kind = t_pModel->getKind(index.row());
    float fMaxValue = 1e-9f;

    switch(kind) {
        case FIFFV_MEG_CH: {
            qint32 unit =t_pModel->getUnit(index.row());
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
            fMaxValue = 1e-3f; //m_qSettings.value("RawDelegate/max_eog").toDouble();
            break;
        }
        case FIFFV_STIM_CH: {
            fMaxValue = 5; //m_qSettings.value("RawDelegate/max_stim").toDouble();
            break;
        }
    }

    float fValue;
    float fScaleY = option.rect.height()/(2*fMaxValue);

    float y_base = path.currentPosition().y();
    QPointF qSamplePosition;

    float fDx = ((float)option.rect.width()) / t_pModel->getMaxSamples();

    //Move to initial starting point
    if(data.size() > 0)
    {
        float val = data[0];
        fValue = val*fScaleY;

        float newY = y_base+fValue;

        qSamplePosition.setY(newY);
        qSamplePosition.setX(path.currentPosition().x());

        path.moveTo(qSamplePosition);
    }

    //create lines from one to the next sample
    qint32 i;
    for(i = 1; i < data.size(); ++i) {
        float val = data[i];
        fValue = val*fScaleY;

        float newY = y_base+fValue;

        qSamplePosition.setY(newY);
        qSamplePosition.setX(path.currentPosition().x()+fDx);

        path.lineTo(qSamplePosition);
    }

    //create lines from one to the next sample for last path
    qint32 offset = t_pModel->numVLines() + 1;
    qSamplePosition.setX(qSamplePosition.x() + fDx*offset);
    lastPath.moveTo(qSamplePosition);

    for(i += offset; i < lastData.size(); ++i) {
        float val = lastData[i];
        fValue = val*fScaleY;

        float newY = y_base+fValue;

        qSamplePosition.setY(newY);
        qSamplePosition.setX(lastPath.currentPosition().x()+fDx);

        lastPath.lineTo(qSamplePosition);
    }

//    qDebug("Plot-PainterPath created!");
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayDelegate::createGridPath(const QModelIndex &index, const QStyleOptionViewItem &option, QPainterPath& path, QList< QVector<float> >& data) const
{
    const RealTimeMultiSampleArrayModel* t_pModel = static_cast<const RealTimeMultiSampleArrayModel*>(index.model());

    if(t_pModel->numVLines() > 0)
    {
        //horizontal lines
        float distance = option.rect.width()/(t_pModel->numVLines()+1);

        float yStart = option.rect.topLeft().y();

        float yEnd = option.rect.bottomRight().y();

        for(qint8 i = 0; i < t_pModel->numVLines(); ++i) {
            float x = distance*(i+1);
            path.moveTo(x,yStart);
            path.lineTo(x,yEnd);
        }
    }
}
