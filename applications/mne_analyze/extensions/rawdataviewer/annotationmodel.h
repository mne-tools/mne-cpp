#ifndef ANNOTATIONMODEL_H
#define ANNOTATIONMODEL_H

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>

class AnnotationModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    AnnotationModel(QObject* parent);

    //=============================================================================================================
    bool insertRows(int position, int span, const QModelIndex & parent);
    bool removeRows(int position, int span, const QModelIndex & parent = QModelIndex());
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex & index) const;

    QStringList getEventTypeList() const;

    void setSamplePos(int iSamplePos);

    void setEventFilterType(const QString eventType);

    void setFirstLastSample(int firstSample, int lastSample);

    QPair<int, int> getFirstLastSample() const;

    float getSampleFreq() const;

    void setSampleFreq(float fFreq);



private:

    QStringList         m_eventTypeList;

    QVector<int>        m_dataSamples;
    QVector<int>        m_dataTypes;
    QVector<int>        m_dataIsUserEvent;

    QVector<int>        m_dataSamples_Filtered;
    QVector<int>        m_dataTypes_Filtered;
    QVector<int>        m_dataIsUserEvent_Filtered;

    int                 m_iSamplePos;
    int                 m_iFirstSample;
    int                 m_iLastSample;

    float               m_fFreq;

    QString             m_sFilterEventType;
    QMap<int, QColor>   m_eventTypeColor;

};

#endif // ANNOTATIONMODEL_H
