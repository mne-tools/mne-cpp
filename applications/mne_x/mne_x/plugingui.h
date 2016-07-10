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
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
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

#include <xShared/Management/pluginmanager.h>
#include <xShared/Management/pluginscenemanager.h>

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
    PluginGui(XSHAREDLIB::PluginManager *pPluginManager, XSHAREDLIB::PluginSceneManager *pPluginSceneManager);

    ~PluginGui();

    //=========================================================================================================
    /**
    * Clear scene
    */
    void clearScene();

    //=========================================================================================================
    /**
    * Loads a current plug in configuration from a given file
    *
    * @param [in] sPath         The path to the file.
    * @param [in] sFileName     The file name to load the configuration from.
    */
    void loadConfig(const QString& sPath, const QString& sFileName);

    //=========================================================================================================
    /**
    * Saves the current plug in configuration to a given file
    *
    * @param [in] sPath         The path to the file.
    * @param [in] sFileName     The file name to store the configuration to.
    */
    void saveConfig(const QString& sPath, const QString& sFileName);


    inline XSHAREDLIB::IPlugin::SPtr getCurrentPlugin();

    void uiSetupRunningState(bool state);

signals:
   void selectedPluginChanged(XSHAREDLIB::IPlugin::SPtr pPlugin);

   void selectedConnectionChanged(XSHAREDLIB::PluginConnectorConnection::SPtr pConnection);

private:

    void pointerGroupClicked(int id);
    void actionGroupTriggered(QAction* action);

    bool removePlugin(XSHAREDLIB::IPlugin::SPtr pPlugin);

    void itemInserted(PluginItem *item);
    void newItemSelected();

    void deleteItem();
    void bringToFront();
    void sendToBack();

    void createActions();
    void createMenuItem();
    void createToolbars();

    QAction* createItemAction(QString name, QMenu* menu);

    XSHAREDLIB::PluginManager*          m_pPluginManager;       /**< Corresponding plugin manager. */
    XSHAREDLIB::PluginSceneManager*     m_pPluginSceneManager;  /**< Corresponding plugin scene manager. */

    XSHAREDLIB::IPlugin::SPtr                   m_pCurrentPlugin;
    XSHAREDLIB::PluginConnectorConnection::SPtr m_pCurrentConnection;

    PluginScene*    m_pPluginScene;         /**< Plugin graph */
    QGraphicsView*  m_pGraphicsView;        /**< View to show graph */

    QToolButton*    m_pSensorToolButton;
    QToolButton*    m_pAlgorithmToolButton;
    QToolButton*    m_pIOToolButton;
    QToolBar*       m_pToolBarPlugins;
    QActionGroup*   m_pActionGroupPlugins;

    QToolButton*    m_pPointerButton;
    QToolButton*    m_pLinePointerButton;
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

inline XSHAREDLIB::IPlugin::SPtr PluginGui::getCurrentPlugin()
{
    return m_pCurrentPlugin;
}

} //NAMESPACE

#endif // PLUGINGUI_H
