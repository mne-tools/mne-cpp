//=============================================================================================================
/**
* @file     PluginDockWidget.cpp
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
* @brief    Contains the implementation of the PluginDockWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "plugindockwidget.h"

#include <mne_x/Management/pluginmanager.h>

#include <mne_x/Interfaces/IPlugin.h>
#include <mne_x/Interfaces/ISensor.h>
#include <mne_x/Interfaces/IAlgorithm.h>
#include <mne_x/Interfaces/IIO.h>

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

PluginDockWidget::PluginDockWidget( const QString & title, QWidget * parent, Qt::WindowFlags flags )
: QDockWidget(title, parent, flags)
, m_iCurrentPluginIdx(-1)
, m_pCurrentItem(0)
, m_pTreeWidgetPluginList(new QTreeWidget(this))
{
    QTreeWidgetItem* CSA_TreeView = new QTreeWidgetItem(m_pTreeWidgetPluginList);
    CSA_TreeView->setText(0, CInfo::AppNameShort());
    CSA_TreeView->setIcon(0, QIcon(":/images/csa_rt.png"));
    CSA_TreeView->setToolTip(0, tr("Some Tipps for CSA RT"));
    m_pCurrentItem = CSA_TreeView;


    QTreeWidgetItem* sensors = new QTreeWidgetItem(m_pTreeWidgetPluginList);
    sensors->setText(0, tr("Sensors"));
    sensors->setIcon(0, QIcon(":/images/sensor.png"));
    sensors->setToolTip(0, tr("Some Tipps for Sensors"));

    QTreeWidgetItem* algorithms = new QTreeWidgetItem(m_pTreeWidgetPluginList);
    algorithms->setText(0, tr("Algorithms"));
    algorithms->setIcon(0, QIcon(":/images/algorithm.png"));
    algorithms->setToolTip(0, tr("Some Tipps for Algorithms"));

    QTreeWidgetItem* ios = new QTreeWidgetItem(m_pTreeWidgetPluginList);
    ios->setText(0, tr("I/Os"));
    ios->setIcon(0, QIcon(":/images/io.png"));
    ios->setToolTip(0, tr("Some Tipps for I/Os"));


    QVector<IPlugin*>::const_iterator iterPlugins = PluginManager::s_vecPlugins.begin();

    for(int i = 0; iterPlugins != PluginManager::s_vecPlugins.end(); ++i, ++iterPlugins)
    {

        QTreeWidgetItem* item(0);
        QString name((*iterPlugins)->getName());
//        qDebug()<<"SLM:"<<name;

        int plugin_type = (*iterPlugins)->getType();

        if(plugin_type == _ISensor)
        {
            item = new QTreeWidgetItem(sensors);
        }
        else if(plugin_type == _IAlgorithm)
        {
            item = new QTreeWidgetItem(algorithms);
        }
        else if(plugin_type == _IIO)
        {
            item = new QTreeWidgetItem(ios);
        }

        item->setText(0, name);
        if ((*iterPlugins)->isActive())
            item->setCheckState(0, Qt::Checked);
        else
            item->setCheckState(0, Qt::Unchecked);

        m_ItemQMap[i] = item;
    }

    m_pTreeWidgetPluginList->header()->hide();

    setWidget(m_pTreeWidgetPluginList);

    setMaximumWidth(250);//ToDo dirty hack; maybe its should be resize able as it want to be

    connect(m_pTreeWidgetPluginList, SIGNAL(itemPressed(QTreeWidgetItem*, int)),
            this, SLOT(itemSelected(QTreeWidgetItem*)));

    connect(m_pTreeWidgetPluginList, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
            this, SLOT(itemToggled(QTreeWidgetItem*)));
}


//*************************************************************************************************************

PluginDockWidget::~PluginDockWidget()
{
    //Garbage collecting
    foreach(QTreeWidgetItem* item, m_ItemQMap)
        delete item;

    if(m_pTreeWidgetPluginList)
        delete m_pTreeWidgetPluginList;
}


//*************************************************************************************************************

bool PluginDockWidget::isValidPlugin(QTreeWidgetItem *item)
{
    //ToDo exception for CSA root Item

    bool validPlugin = false;
    int n = PluginManager::findByName(item->text(0));

    qDebug() << "PluginsDockWidget::changeItem: " << item->text(0) << "PluginNum: " << n;

    if(n >= 0)
    {
        m_iCurrentPluginIdx = n;
        validPlugin = true;
    }

    return validPlugin;
}


//*************************************************************************************************************

void PluginDockWidget::itemSelected(QTreeWidgetItem *selectedItem)
{
    if(m_pCurrentItem != selectedItem)
    {
        m_pCurrentItem = selectedItem;

        if(isValidPlugin(selectedItem))
            emit pluginChanged(m_iCurrentPluginIdx, m_pCurrentItem);

        emit itemChanged();
    }
}


//*************************************************************************************************************

void PluginDockWidget::itemToggled(QTreeWidgetItem *item)
{
    if (isValidPlugin(item))
    {
       int n = PluginManager::findByName(item->text(0));

       if (item->checkState(0) == Qt::Checked)
           activateItem(n, true);
       else
           activateItem(n, false);

        if (m_pCurrentItem == item)
        {
            // the status of the current item changed, emit signal so the gui updates
            emit pluginChanged(m_iCurrentPluginIdx, m_pCurrentItem);
        }
    }
}

//*************************************************************************************************************

void PluginDockWidget::setTogglingEnabled(bool enabled)
{
    Qt::ItemFlags flags;
    QTreeWidgetItemIterator it(m_pTreeWidgetPluginList);
    while (*it)
    {
        flags = (*it)->flags();
        if (enabled)
            flags |=  Qt::ItemIsUserCheckable;
        else
            flags = ~(flags & Qt::ItemIsUserCheckable);
        (*it)->setFlags(flags);
        ++it;
    }
}

//*************************************************************************************************************

void PluginDockWidget::contextMenuEvent (QContextMenuEvent* event)
{
    QPoint p(    event->pos().x()-m_pTreeWidgetPluginList->pos().x(),
                event->pos().y()-m_pTreeWidgetPluginList->pos().y());

    if (isValidPlugin(m_pTreeWidgetPluginList->itemAt(p)))
    {
        // TODO: context menu
    }
}

//*************************************************************************************************************

bool PluginDockWidget::isActivated(int n) const
{
    return n >= 0 ? PluginManager::s_vecPlugins[n]->isActive() : false;
}


//*************************************************************************************************************

void PluginDockWidget::activateItem(int n, bool status)
{
    qDebug() << "PluginsDockWidget::activateItem " << n << "; "<< status;

    if(n < PluginManager::s_vecPlugins.size())
    {
        PluginManager::s_vecPlugins[n]->setStatus(status);
    }
}
