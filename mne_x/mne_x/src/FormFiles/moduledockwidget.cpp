//=============================================================================================================
/**
* @file		moduledockwidget.cpp
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the implementation of the ModuleDockWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "moduledockwidget.h"

#include "../management/modulemanager.h"

#include "../interfaces/IModule.h"
#include "../interfaces/ISensor.h"
#include "../interfaces/IRTAlgorithm.h"
#include "../interfaces/IRTVisualization.h"
#include "../interfaces/IRTRecord.h"
#include "../interfaces/IAlert.h"

#include "../preferences/info.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTreeWidget>
#include <QHeaderView>
#include <QContextMenuEvent>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ModuleDockWidget::ModuleDockWidget( const QString & title, QWidget * parent, Qt::WindowFlags flags )
: QDockWidget(title, parent, flags)
, m_iCurrentModuleIdx(-1)
, m_pCurrentItem(0)
, m_pTreeWidgetModuleList(new QTreeWidget(this))
{
    QTreeWidgetItem* CSA_TreeView = new QTreeWidgetItem(m_pTreeWidgetModuleList);
    CSA_TreeView->setText(0, CInfo::AppNameShort());
    CSA_TreeView->setIcon(0, QIcon(":/images/csa_rt.png"));
    CSA_TreeView->setToolTip(0, tr("Some Tipps for CSA RT"));
    m_pCurrentItem = CSA_TreeView;


    QTreeWidgetItem* sensors = new QTreeWidgetItem(m_pTreeWidgetModuleList);
    sensors->setText(0, tr("Sensors"));
    sensors->setIcon(0, QIcon(":/images/sensor.png"));
    sensors->setToolTip(0, tr("Some Tipps for Sensors"));

    QTreeWidgetItem* algorithms = new QTreeWidgetItem(m_pTreeWidgetModuleList);
    algorithms->setText(0, tr("Algorithms"));
    algorithms->setIcon(0, QIcon(":/images/algorithm.png"));
    algorithms->setToolTip(0, tr("Some Tipps for Algorithms"));

    QTreeWidgetItem* displays = new QTreeWidgetItem(m_pTreeWidgetModuleList);
    displays->setText(0, tr("Visualisations"));
    displays->setIcon(0, QIcon(":/images/visualisation.png"));
    displays->setToolTip(0, tr("Some Tipps for Visualisations"));


    QVector<IModule*>::const_iterator iterModules = ModuleManager::s_vecModules.begin();

    for(int i = 0; iterModules != ModuleManager::s_vecModules.end(); ++i, ++iterModules)
    {

        QTreeWidgetItem* item(0);
        QString name((*iterModules)->getName());

        int module_type = (*iterModules)->getType();

        if(module_type == _ISensor)
        {
            item = new QTreeWidgetItem(sensors);
        }
        else if(module_type == _IRTAlgorithm)
        {
            item = new QTreeWidgetItem(algorithms);
        }
        else if(module_type == _IRTVisualization)
        {
            item = new QTreeWidgetItem(displays);
        }

        item->setText(0, name);
        m_ItemQMap[i] = item;
    }

    m_pTreeWidgetModuleList->header()->hide();

    setWidget(m_pTreeWidgetModuleList);

    setMaximumWidth(250);//ToDo dirty hack; maybe its should be resize able as it want to be

    connect(m_pTreeWidgetModuleList, SIGNAL(itemPressed(QTreeWidgetItem*, int)),
            this, SLOT(itemSelected(QTreeWidgetItem*)));
}


//*************************************************************************************************************

ModuleDockWidget::~ModuleDockWidget()
{
    //Todo
}


//*************************************************************************************************************

bool ModuleDockWidget::isValidModule(QTreeWidgetItem *item)
{
    //ToDo exception for CSA root Item

    bool validModule = false;
    int n = ModuleManager::findByName(item->text(0));

    qDebug() << "ModulesDockWidget::changeItem: " << item->text(0) << "ModuleNum: " << n;

    if(n >= 0)
    {
        m_iCurrentModuleIdx = n;
        validModule = true;
    }

    return validModule;
}


//*************************************************************************************************************

void ModuleDockWidget::itemSelected(QTreeWidgetItem *selectedItem)
{
    if(m_pCurrentItem != selectedItem)
    {
        m_pCurrentItem = selectedItem;

        if(isValidModule(selectedItem))
            emit moduleChanged(m_iCurrentModuleIdx, m_pCurrentItem);

        emit itemChanged();
    }
}


//*************************************************************************************************************

void ModuleDockWidget::contextMenuEvent (QContextMenuEvent* event)
{
    QPoint p(    event->pos().x()-m_pTreeWidgetModuleList->pos().x(),
                event->pos().y()-m_pTreeWidgetModuleList->pos().y());

    if (isValidModule(m_pTreeWidgetModuleList->itemAt(p)))
    {
        qDebug() << m_pCurrentItem->text(0);
        int n = m_iCurrentModuleIdx;

        bool bNewStatus;//curiosity bool doesn't work in release mode
        if(isActivated(n))
            bNewStatus = false;
        else
            bNewStatus = true;

        qDebug() << "ModulesDockWidget::contextMenuEvent -> new status: " << bNewStatus;
        activateItem(n, bNewStatus);
    }
}


//*************************************************************************************************************

bool ModuleDockWidget::isActivated(int n) const
{
    return n >= 0 ? ModuleManager::s_vecModules[n]->isActive() : false;
}


//*************************************************************************************************************

void ModuleDockWidget::activateItem(int n, bool& status)
{
    qDebug() << "ModulesDockWidget::activateItem " << n << "; "<< status;

    if(n < ModuleManager::s_vecModules.size())
    {
        ModuleManager::s_vecModules[n]->setStatus(status);

        QFont font = m_ItemQMap[n]->font(0);
        font.setStrikeOut(!status);

        QColor color = status ? QColor(Qt::black) : QColor(Qt::gray);

        m_ItemQMap[n]->setFont(0,font);
        m_ItemQMap[n]->setTextColor(0, color);
    }
}
