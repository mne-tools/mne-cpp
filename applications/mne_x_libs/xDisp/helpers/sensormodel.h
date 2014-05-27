#ifndef SENSORMODEL_H
#define SENSORMODEL_H

#include <QAbstractTableModel>
#include <QDebug>

#include "sensorlayout.h"

#include "sensorgroup.h"

class SensorModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    SensorModel(QObject *parent = 0);

    SensorModel(QIODevice* device, QObject *parent = 0);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

    inline QVariant data(int row, int column, int role = Qt::DisplayRole) const;

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    inline qint32 getNumLayouts() const;

    void setCurrentLayout(qint32 id);


private:
    bool read(QIODevice* device);

    qint32 m_iCurrentLayoutId;
    QList<SensorLayout> m_qListSensorLayouts;
    QList<SensorGroup> m_qListSensorGroups;
};


inline QVariant SensorModel::data(int row, int column, int role) const
{
    return data(createIndex(row, column), role);
}

inline qint32 SensorModel::getNumLayouts() const
{
    return m_qListSensorLayouts.size();
}


#endif // SENSORMODEL_H
