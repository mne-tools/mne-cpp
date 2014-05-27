#ifndef SENSORWIDGET_H
#define SENSORWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>

#include "sensormodel.h"

class SensorWidget : public QWidget
{
    Q_OBJECT
public:
    SensorWidget(QWidget *parent = 0);

    void SensorWidget::createUI();

    void drawChannels();

    void setModel(SensorModel *model);

//    void setRTMSA();


    void contextUpdate(const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles = QVector<int> ());

    void contextUpdate();

signals:


private:
    SensorModel*    m_pSensorModel;
    QGraphicsView*  m_pGraphicsView;
    QGraphicsScene* m_pGraphicsScene;

};

#endif // SENSORWIDGET_H
