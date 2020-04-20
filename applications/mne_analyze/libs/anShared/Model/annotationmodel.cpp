
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotationmodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QBrush>
#include <QDebug>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnnotationModel::AnnotationModel(QObject* parent)
: QAbstractTableModel(parent)
, m_iSamplePos(0)
, m_iFirstSample(0)
, m_fFreq(600)
, m_sFilterEventType("All")
{
    qDebug() << "ANNOTATION MODEL CONSTRUCTOR";

    m_eventTypeColor[1] = QColor(Qt::black);
    m_eventTypeColor[2] = QColor(Qt::magenta);
    m_eventTypeColor[3] = QColor(Qt::green);
    m_eventTypeColor[4] = QColor(Qt::red);
    m_eventTypeColor[5] = QColor(Qt::cyan);
    m_eventTypeColor[32] = QColor(Qt::yellow);
    m_eventTypeColor[998] = QColor(Qt::darkBlue);
    m_eventTypeColor[999] = QColor(Qt::darkCyan);
}

//=============================================================================================================

QStringList AnnotationModel::getEventTypeList() const
{
    return m_eventTypeList;
}

//=============================================================================================================

bool AnnotationModel::insertRows(int position, int span, const QModelIndex & parent)
{
    Q_UNUSED(parent);

    qDebug() << "AnnotationModel::insertRows here";
    qDebug() << "iSamplePos:" << m_iSamplePos;

    if(m_dataSamples.isEmpty()) {
        m_dataSamples.insert(0, m_iSamplePos);
        m_dataTypes.insert(0, 1);
        m_dataIsUserEvent.insert(0, 1);
    }
    else {
        for (int i = 0; i < span; ++i) {
            for(int t = 0; t<m_dataSamples.size(); t++) {
                if(m_dataSamples[t] >= m_iSamplePos) {
                    m_dataSamples.insert(t, m_iSamplePos);

                    if(m_sFilterEventType == "All")
                        m_dataTypes.insert(t, 1);
                    else
                        m_dataTypes.insert(t, m_sFilterEventType.toInt());

                    m_dataIsUserEvent.insert(t, 1);
                    break;
                }

                if(t == m_dataSamples.size()-1) {
                    m_dataSamples.append(m_iSamplePos);

                    if(m_sFilterEventType == "All")
                        m_dataTypes.append(1);
                    else
                        m_dataTypes.append(m_sFilterEventType.toInt());

                    m_dataIsUserEvent.append(1);
                    break;
                }
            }
        }
    }

    beginInsertRows(QModelIndex(), position, position+span-1);

    endInsertRows();

    //Update filtered event data
    setEventFilterType(m_sFilterEventType);

    return true;
}

//=============================================================================================================

void AnnotationModel::setSamplePos(int iSamplePos)
{
    qDebug() << "iSamplePos:" << iSamplePos;
    m_iSamplePos = iSamplePos;
}

//=============================================================================================================

int AnnotationModel::rowCount(const QModelIndex &parent) const
{

    return m_dataSamples_Filtered.size();
}

//=============================================================================================================

int AnnotationModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

//=============================================================================================================

QVariant AnnotationModel::data(const QModelIndex &index, int role) const
{
    //qDebug() << "AnnotationModel::data";

    if(role == Qt::TextAlignmentRole)
        return QVariant(Qt::AlignCenter | Qt::AlignVCenter);

    if(role != Qt::DisplayRole && role != Qt::BackgroundRole)
        return QVariant();

    if(index.row()>=m_dataSamples_Filtered.size())
        return QVariant();

    if (index.isValid()) {
        //******** first column (sample index) ********
        if(index.column()==0) {
            switch(role) {
                case Qt::DisplayRole:
                    return QVariant(m_dataSamples_Filtered.at(index.row())-m_iFirstSample);

            case Qt::BackgroundRole:
                //Paint different background if event was set by user
                if(m_dataIsUserEvent_Filtered.at(index.row()) == 1) {
                    QBrush brush;
                    brush.setStyle(Qt::SolidPattern);
                    QColor colorTemp(Qt::red);
                    colorTemp.setAlpha(15);
                    brush.setColor(colorTemp);
                    return QVariant(brush);
                }
            }
        }

        //******** second column (event time plot) ********
        if(index.column()==1){
            switch(role) {
                case Qt::DisplayRole: {
                    int time = ((m_dataSamples_Filtered.at(index.row()) - m_iFirstSample) / m_fFreq) * 100;

                    return QVariant((double)time / 100);
                }

            case Qt::BackgroundRole:
                //Paint different background if event was set by user
                if(m_dataIsUserEvent_Filtered.at(index.row()) == 1) {
                    QBrush brush;
                    brush.setStyle(Qt::SolidPattern);
                    QColor colorTemp(Qt::red);
                    colorTemp.setAlpha(15);
                    brush.setColor(colorTemp);
                    return QVariant(brush);
                }
            }
        }

        //******** third column (event type plot) ********
        if(index.column()==2) {
            switch(role) {
                case Qt::DisplayRole:
                    return QVariant(m_dataTypes_Filtered.at(index.row()));

                case Qt::BackgroundRole: {
                    QBrush brush;
                    brush.setStyle(Qt::SolidPattern);
                    brush.setColor(Qt::white);

                    QColor colorTemp = brush.color();
                    colorTemp.setAlpha(110);
                    brush.setColor(colorTemp);
                    return QVariant(brush);
                }
            }
        }

    }
    return QVariant();
}

//=============================================================================================================

bool AnnotationModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    qDebug() << "AnnotationModel::setData";
    if(index.row() >= m_dataSamples.size() || index.column() >= columnCount())
        return false;

    if(role == Qt::EditRole) {
        int column = index.column();
        switch(column) {
            case 0: //sample values
                m_dataSamples[index.row()] = value.toInt() + m_iFirstSample;
                break;

            case 1: //time values
                m_dataSamples[index.row()] = value.toDouble() * m_fFreq + m_iFirstSample;
                break;

            case 2: //type
                QString string = value.toString();
                m_dataTypes[index.row()] = string.toInt();
                break;
        }
    }

    //Update filtered event data
    setEventFilterType(m_sFilterEventType);

    return true;
}

//=============================================================================================================

void AnnotationModel::setEventFilterType(const QString eventType)
{
    qDebug() << "AnnotationModel::setEventFilterType";
    m_sFilterEventType = eventType;

    //Clear filtered event data
    m_dataSamples_Filtered.clear();
    m_dataTypes_Filtered.clear();
    m_dataIsUserEvent_Filtered.clear();

    //Fill filtered event data depending on the user defined event filter type
    if(eventType == "All") {
        m_dataSamples_Filtered = m_dataSamples;
        m_dataTypes_Filtered = m_dataTypes;
        m_dataIsUserEvent_Filtered = m_dataIsUserEvent;
    }
    else {
        for(int i = 0; i<m_dataSamples.size(); i++) {
            if(m_dataTypes[i] == eventType.toInt()) {
                m_dataSamples_Filtered.append(m_dataSamples[i]);
                m_dataTypes_Filtered.append(m_dataTypes[i]);
                m_dataIsUserEvent_Filtered.append(m_dataIsUserEvent[i]);
            }
        }
    }

    emit dataChanged(createIndex(0,0), createIndex(m_dataSamples_Filtered.size(), 0));
    emit headerDataChanged(Qt::Vertical, 0, m_dataSamples_Filtered.size());

    emit annotationsToDraw(&m_dataSamples_Filtered);

    qDebug() << "Samp:" << m_dataSamples;
    qDebug() << "Filt:" << m_dataSamples_Filtered;
}

//=============================================================================================================

Qt::ItemFlags AnnotationModel::flags(const QModelIndex &index) const
{
    //Return editable mode only for user events an when event type filtering is deactivated
    if(m_dataIsUserEvent_Filtered[index.row()] == 1 && m_sFilterEventType == "All")
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    else
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;}

//=============================================================================================================

QVariant AnnotationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    if(role==Qt::TextAlignmentRole)
        return Qt::AlignHCenter + Qt::AlignVCenter;

    if(orientation == Qt::Horizontal) {
        switch(section) {
            case 0: //sample column
                return QVariant("Sample");
            case 1: //time value column
                return QVariant("Time (s)");
            case 2: //event type column
                return QVariant("Type");
            }
    }
    else if(orientation == Qt::Vertical) {
        return QString("Event %1").arg(section);
    }

    return QVariant();
}

//=============================================================================================================

bool AnnotationModel::removeRows(int position, int span, const QModelIndex &parent)
{
    Q_UNUSED(parent);

    for (int i = 0; i < span; ++i) {
        //Only user events can be deleted
        if(m_dataIsUserEvent[position] == 1) {
            m_dataSamples.removeAt(position);
            m_dataTypes.removeAt(position);
            m_dataIsUserEvent.removeAt(position);
        }
    }

    beginRemoveRows(QModelIndex(), position, position+span-1);

    endRemoveRows();

    //Update filtered event data
    setEventFilterType(m_sFilterEventType);

    return true;
}


//=============================================================================================================

void AnnotationModel::setFirstLastSample(int firstSample, int lastSample)
{
    m_iFirstSample = firstSample;
    m_iLastSample = lastSample;
}

//=============================================================================================================

QPair<int,int> AnnotationModel::getFirstLastSample() const
{
    QPair<int, int> pair(m_iFirstSample, m_iLastSample);
    return pair;
}

//=============================================================================================================

float AnnotationModel::getSampleFreq() const
{
    return m_fFreq;
}

//=============================================================================================================

void AnnotationModel::setSampleFreq(float fFreq)
{
    m_fFreq = fFreq;
}

//=============================================================================================================

int AnnotationModel::getNumberOfAnnotations() const
{
    return m_dataSamples_Filtered.size();
}

//=============================================================================================================

int AnnotationModel::getAnnotation(int iIndex) const
{
    return m_dataSamples_Filtered.at(iIndex);
}

//=============================================================================================================

void AnnotationModel::addNewAnnotationType(const QString &eventType, const QColor &typeColor)
{
    m_eventTypeColor[eventType.toInt()] = typeColor;

    if(!m_eventTypeList.contains(eventType))
        m_eventTypeList<<eventType;

    emit updateEventTypes(eventType);
}
