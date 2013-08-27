//=============================================================================================================
/**
* @file     plugingui.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2013
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    PluginGui class declaration
*
*/

#ifndef PLUGINGUI_H
#define PLUGINGUI_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginitem.h"
#include "pluginscene.h"

#include <mne_x/Management/pluginmanager.h>
#include <mne_x/Management/pluginscenemanager.h>

#include <QMainWindow>
#include <QMap>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QAction;
class QToolBox;
class QSpinBox;
class QComboBox;
class QButtonGroup;
class QActionGroup;
class QLineEdit;
class QToolButton;
class QAbstractButton;
class QGraphicsView;



//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{


class PluginGui : public QMainWindow
{
    Q_OBJECT
    friend class PluginScene;
public:
    PluginGui(MNEX::PluginManager::SPtr &pPluginManager, MNEX::PluginSceneManager::SPtr &pPluginSceneManager);

    inline IPlugin::SPtr getCurrentPlugin();

    void uiSetupRunningState(bool state);

signals:
   void selectedPluginChanged(IPlugin::SPtr pPlugin);

private:

    void pointerGroupClicked(int id);
    void actionGroupTriggered(QAction* action);

    bool removePlugin(IPlugin::SPtr pPlugin);

    void itemInserted(PluginItem *item);
    void newItemSelected();

    void deleteItem();
    void bringToFront();
    void sendToBack();

    void createActions();
    void createMenuItem();
    void createToolbars();

    QAction* createItemAction(QString name, QMenu* menu);

    PluginManager::SPtr       m_pPluginManager;       /**< Corresponding plugin manager. */
    PluginSceneManager::SPtr  m_pPluginSceneManager;  /**< Corresponding plugin scene manager. */

    IPlugin::SPtr             m_pCurrentPlugin;

    PluginScene*    m_pPluginScene;         /**< Plugin graph */
    QGraphicsView*  m_pGraphicsView;        /**< View to show graph */
    QToolBar*       m_pToolBarPlugins;
    QActionGroup*   m_pActionGroupPlugins;

    QToolBar *      m_pToolBarPointer;
    QButtonGroup *  m_pButtonGroupPointers;

    QToolBar*   m_pToolBarItem;
    QMenu*      m_pMenuItem;
    QAction*    deleteAction;
    QAction*    toFrontAction;
    QAction*    sendBackAction;
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline IPlugin::SPtr PluginGui::getCurrentPlugin()
{
    return m_pCurrentPlugin;
}

} //NAMESPACE

#endif // PLUGINGUI_H
