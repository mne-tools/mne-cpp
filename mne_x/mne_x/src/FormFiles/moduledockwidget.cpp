//=============================================================================================================
/**
* @file     moduledockwidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the ModuleDockWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "moduledockwidget.h"

#include <mne_x/Management/modulemanager.h>

#include <mne_x/Interfaces/IModule.h>
#include <mne_x/Interfaces/ISensor.h>
#include <mne_x/Interfaces/IRTAlgorithm.h>
#include <mne_x/Interfaces/IRTVisualization.h>
#include <mne_x/Interfaces/IRTRecord.h>
#include <mne_x/Interfaces/IAlert.h>

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
