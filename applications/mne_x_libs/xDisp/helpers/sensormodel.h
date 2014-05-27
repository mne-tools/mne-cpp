#ifndef SENSORMODEL_H
#define SENSORMODEL_H

#include <QAbstractTableModel>
#include <QDebug>

#include "sensorlayout.h"
#include "sensorgroup.h"

class SensorItem;

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

    inline QList<SensorLayout> getSensorLayouts() const;

    inline qint32 getNumLayouts() const;

    inline QList<SensorGroup> getSensorGroups() const;

    inline qint32 getCurrentLayout() const;

    void setCurrentLayout(int id);

    void updateChannelState(SensorItem* item);

signals:
    void newSelection(QList<qint32> selection);
    void newLayout();

private:
    void createSelection();

    bool read(QIODevice* device);

    qint32 m_iCurrentLayoutId;
    QList<SensorLayout> m_qListSensorLayouts;
    QList<SensorGroup> m_qListSensorGroups;

    QMap<qint32, bool>      m_qMapSelection;
    QMap<QString, qint32>   m_qMapNameId;
    QMap<qint32, QString>   m_qMapIdName;
};


inline QVariant SensorModel::data(int row, int column, int role) const
{
    return data(index(row, column), role);
}


inline QList<SensorLayout> SensorModel::getSensorLayouts() const
{
    return m_qListSensorLayouts;
}


inline qint32 SensorModel::getNumLayouts() const
{
    return m_qListSensorLayouts.size();
}


inline QList<SensorGroup> SensorModel::getSensorGroups() const
{
    return m_qListSensorGroups;
}


inline qint32 SensorModel::getCurrentLayout() const
{
    return m_iCurrentLayoutId;
}


#endif // SENSORMODEL_H
