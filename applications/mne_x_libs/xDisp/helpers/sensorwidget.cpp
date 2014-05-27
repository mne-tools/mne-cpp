#include "sensorwidget.h"
#include "sensoritem.h"

#include <QDebug>

#include <QFile>
#include <QToolButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>

SensorWidget::SensorWidget(QWidget *parent)
: QWidget(parent)
, m_pSensorModel(NULL)
{
    m_pGraphicsView = new QGraphicsView(this);

    m_pGraphicsScene = new QGraphicsScene(this);

    m_pGraphicsView->setScene(m_pGraphicsScene);


    QToolButton *selection1 = new QToolButton;
    QToolButton *selection2 = new QToolButton;
    QToolButton *selection3 = new QToolButton;

    QVBoxLayout *layoutSelection = new QVBoxLayout;
    layoutSelection->addWidget(selection1);
    layoutSelection->addWidget(selection2);
    layoutSelection->addWidget(selection3);


    QToolButton *MEGLayout = new QToolButton;
    MEGLayout->setText(tr("MEG"));
    MEGLayout->setCheckable(true);
    MEGLayout->setChecked(true);
    QToolButton *EEGLayout = new QToolButton;
    EEGLayout->setText(tr("EEG"));
    EEGLayout->setCheckable(true);
    EEGLayout->setChecked(false);

    QButtonGroup *buttonGroup = new QButtonGroup;
    buttonGroup->setExclusive(true);
    buttonGroup->addButton(MEGLayout);
    buttonGroup->addButton(EEGLayout);

    QHBoxLayout *layoutButtonGroup = new QHBoxLayout;
    layoutButtonGroup->addWidget(MEGLayout);
    layoutButtonGroup->addWidget(EEGLayout);

    QGridLayout *topLayout = new QGridLayout;
    topLayout->addWidget(m_pGraphicsView, 0, 0);
    topLayout->addLayout(layoutSelection, 0, 1);
    topLayout->addLayout(layoutButtonGroup, 1, 0);


    setLayout(topLayout);

}


void SensorWidget::setModel(SensorModel *model)
{
    m_pSensorModel = model;


//    QGraphicsItem *item = new SensorItem(QPointF(1.0, 20.0));
////    item->setPos(QPointF(i, j));
//    m_pGraphicsScene->addItem(item);

    for(qint32 i = 0; i < m_pSensorModel->rowCount(); ++i)
    {
        QString name = m_pSensorModel->data(i, 0).toString();
        QPointF loc = m_pSensorModel->data(i, 1).toPointF();
        QGraphicsItem *item = new SensorItem(name, loc);
        item->setPos(loc);
        m_pGraphicsScene->addItem(item);
    }

}
