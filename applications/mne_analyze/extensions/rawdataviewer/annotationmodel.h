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
    AnnotationModel();

    //=============================================================================================================
    bool insertRows(int position, int span, const QModelIndex & parent);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QStringList getEventTypeList() const;

    void setSamplePos(int iSamplePos);



private:

    QStringList         m_eventTypeList;

    QVector<int>        m_dataSamples;
    QVector<int>        m_dataTypes;

    int                 m_iSamplePos;
    QString             m_sFilterEventType;
};

#endif // ANNOTATIONMODEL_H
