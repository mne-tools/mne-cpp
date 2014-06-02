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

    createUI();

}



void SensorWidget::contextUpdate(const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles)
{
    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)
    contextUpdate();
}


void SensorWidget::contextUpdate()
{
    qDebug() << "SensorWidget::contextUpdate()";

    drawChannels();
}

void SensorWidget::createUI()
{
    if(m_pSensorModel)
    {
        // Sensor Selection
        QButtonGroup *qBGSensorSelection = new QButtonGroup;
        qBGSensorSelection->setExclusive(true);

        QVBoxLayout *VBoxSensorSelection = new QVBoxLayout;
        for(qint32 i = 0; i < m_pSensorModel->getSensorGroups().size(); ++i)
        {
            QToolButton *sensorSelectionButton = new QToolButton;
            sensorSelectionButton->setText(m_pSensorModel->getSensorGroups()[i].getGroupName());
            qBGSensorSelection->addButton(sensorSelectionButton,i);
            VBoxSensorSelection->addWidget(sensorSelectionButton);
        }

        connect(qBGSensorSelection, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), m_pSensorModel, &SensorModel::applySensorGroup);


        // Layout Selection
        QButtonGroup *qBGLayout = new QButtonGroup;
        qBGLayout->setExclusive(true);

        QHBoxLayout *HBoxButtonGroupLayout = new QHBoxLayout;

        for(qint32 i = 0; i < m_pSensorModel->getNumLayouts(); ++i)
        {
            QToolButton *buttonLayout = new QToolButton;
            buttonLayout->setText(m_pSensorModel->getSensorLayouts()[i].getName());
            buttonLayout->setCheckable(true);

            if(i == 0)
                buttonLayout->setChecked(true);
            else
                buttonLayout->setChecked(false);

            qBGLayout->addButton(buttonLayout, i);

            HBoxButtonGroupLayout->addWidget(buttonLayout);
        }

        connect(qBGLayout, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), m_pSensorModel, &SensorModel::setCurrentLayout);


        QGridLayout *topLayout = new QGridLayout;
        topLayout->addWidget(m_pGraphicsView, 0, 0);
        topLayout->addLayout(VBoxSensorSelection, 0, 1);
        topLayout->addLayout(HBoxButtonGroupLayout, 1, 0);

        setLayout(topLayout);
    }
}


void SensorWidget::setModel(SensorModel *model)
{
    m_pSensorModel = model;

    drawChannels();

    connect(m_pSensorModel, &QAbstractTableModel::dataChanged, this, static_cast<void (SensorWidget::*)(const QModelIndex &, const QModelIndex &, const QVector<int> &)>(&SensorWidget::contextUpdate));
    connect(m_pSensorModel, &QAbstractTableModel::modelReset, this, static_cast<void (SensorWidget::*)(void)>(&SensorWidget::contextUpdate));

    connect(m_pSensorModel, &SensorModel::newLayout, this, &SensorWidget::drawChannels);

    createUI();
}


void SensorWidget::drawChannels()
{
    if(m_pGraphicsScene)
    {
        m_pGraphicsScene->clear();

        for(qint32 i = 0; i < m_pSensorModel->rowCount(); ++i)
        {
            QString dispChName = m_pSensorModel->data(i, 0).toString();
            QString fullChName = m_pSensorModel->data(i, 1).toString();
            QPointF loc = m_pSensorModel->data(i, 2).toPointF();
            qint32 chNum = m_pSensorModel->getNameIdMap()[fullChName];
            SensorItem *item = new SensorItem(dispChName, chNum, loc);
            item->setSelected(m_pSensorModel->data(i, 3).toBool());

            //            qDebug() << "m_pSensorModel->getNameIdMap()" << m_pSensorModel->getNameIdMap()[fullChName];
            item->setPos(loc);

            connect(item, &SensorItem::itemChanged, m_pSensorModel, &SensorModel::updateChannelState);
            m_pGraphicsScene->addItem(item);
        }
    }
}
