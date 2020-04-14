#include "annotationmodel.h"

AnnotationModel::AnnotationModel()
{

}

//=============================================================================================================

QStringList AnnotationModel::getEventTypeList() const
{
    return m_eventTypeList;
}

//=============================================================================================================

bool AnnotationModel::insertRows(int position, int span, const QModelIndex & parent)
{
    if(m_dataSamples.isEmpty()) {
        m_dataSamples.insert(0, m_iSamplePos);
        m_dataTypes.insert(0, 1);

    }
    else {
        for (int i = 0; i < span; ++i) {
            for(int t = 0; t<m_dataSamples.size(); t++) {
                if(m_dataSamples[t] >= m_iSamplePos) {
                    m_dataSamples.insert(t, m_iSamplePos);

                    if(m_sFilterEventType == "All"){
                        m_dataTypes.insert(t, 1);
                    }else{
                        m_dataTypes.insert(t, m_sFilterEventType.toInt());
                    }
                    break;
                }

                if(t == m_dataSamples.size()-1) {
                    m_dataSamples.append(m_iSamplePos);

                    if(m_sFilterEventType == "All"){
                        m_dataTypes.append(1);
                    }else{
                        m_dataTypes.append(m_sFilterEventType.toInt());
                    }
                }
            }
        }
    }

    beginInsertRows(QModelIndex(), position, position+span-1);

    endInsertRows();

    //setEventFilterType(m_sFilterEventType);

    return true;
}

//=============================================================================================================

void AnnotationModel::setSamplePos(int iSamplePos)
{
    m_iSamplePos = iSamplePos;
}

//=============================================================================================================

int AnnotationModel::rowCount(const QModelIndex &parent) const
{
    if(m_dataSamples.size() != 0) {
        return m_dataSamples.size();
    }else{
        return 0;
    }
}

//=============================================================================================================

int AnnotationModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

//=============================================================================================================

QVariant AnnotationModel::data(const QModelIndex &index, int role) const
{
    return QVariant();
}
