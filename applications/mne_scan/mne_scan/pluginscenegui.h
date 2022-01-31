//=============================================================================================================
/**
 * @file     plugingui.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh. All rights reserved.
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

#ifndef PLUGINSCENEGUI_H
#define PLUGINSCENEGUI_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginitem.h"
#include "pluginscene.h"

#include <scShared/Management/pluginmanager.h>
#include <scShared/Management/pluginscenemanager.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMainWindow>
#include <QMap>

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

//=============================================================================================================
// DEFINE NAMESPACE MNESCAN
//=============================================================================================================

namespace MNESCAN
{

class PluginSceneGui : public QMainWindow
{
    Q_OBJECT

    friend class PluginScene;

public:
    PluginSceneGui(SCSHAREDLIB::PluginManager *pPluginManager, SCSHAREDLIB::PluginSceneManager *pPluginSceneManager);

    ~PluginSceneGui();

    //=========================================================================================================
    /**
     * Clear scene
     */
    void clearScene();

    //=========================================================================================================
    /**
     * Loads a current plug in configuration from a given file
     *
     * @param[in] sPath         The path to the file.
     * @param[in] sFileName     The file name to load the configuration from.
     */
    void loadConfig(const QString& sPath,
                    const QString& sFileName);

    //=========================================================================================================
    /**
     * Saves the current plug in configuration to a given file
     *
     * @param[in] sPath         The path to the file.
     * @param[in] sFileName     The file name to store the configuration to.
     */
    void saveConfig(const QString& sPath,
                    const QString& sFileName);

    inline SCSHAREDLIB::AbstractPlugin::SPtr getCurrentPlugin();

    void uiSetupRunningState(bool state);

signals:
   void selectedPluginChanged(SCSHAREDLIB::AbstractPlugin::SPtr pPlugin);

   void selectedConnectionChanged(SCSHAREDLIB::PluginConnectorConnection::SPtr pConnection);

private:

    void pointerGroupClicked(int id);
    void actionGroupTriggered(QAction* action);

    bool removePlugin(SCSHAREDLIB::AbstractPlugin::SPtr pPlugin);

    void itemInserted(PluginItem *item);
    void newItemSelected();

    void deleteItem();
    void bringToFront();
    void sendToBack();

    void createActions();
    void createMenuItem();
    void createToolbars();

    QAction* createItemAction(QString name, QMenu* menu);

    SCSHAREDLIB::PluginManager*          m_pPluginManager;       /**< Corresponding plugin manager. */
    SCSHAREDLIB::PluginSceneManager*     m_pPluginSceneManager;  /**< Corresponding plugin scene manager. */

    SCSHAREDLIB::AbstractPlugin::SPtr                   m_pCurrentPlugin;
    SCSHAREDLIB::PluginConnectorConnection::SPtr m_pCurrentConnection;

    PluginScene*    m_pPluginScene;         /**< Plugin graph. */
    QGraphicsView*  m_pGraphicsView;        /**< View to show graph. */

    QToolButton*    m_pSensorToolButton;
    QToolButton*    m_pAlgorithmToolButton;
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

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline SCSHAREDLIB::AbstractPlugin::SPtr PluginSceneGui::getCurrentPlugin()
{
    return m_pCurrentPlugin;
}
} //NAMESPACE

#endif // PLUGINSCENEGUI_H
