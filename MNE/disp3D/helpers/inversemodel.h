#ifndef INVERSEMODEL_H
#define INVERSEMODEL_H

#include <QAbstractTableModel>

class InverseModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    InverseModel(QObject *parent = 0);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

signals:

public slots:

};

#endif // INVERSEMODEL_H
