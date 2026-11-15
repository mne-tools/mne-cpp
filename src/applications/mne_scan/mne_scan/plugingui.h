//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     plugingui.h
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    PluginGui class declaration
 */

#ifndef PLUGINGUI_H
#define PLUGINGUI_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginitem.h"
#include "pluginscene.h"

#include <scShared/Management/pluginmanager.h>
#include <scShared/Management/pluginscenemanager.h>

#include <mna/mna_project.h>

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

class PluginGui : public QMainWindow
{
    Q_OBJECT

    friend class PluginScene;

public:
    PluginGui(mne::scan::PluginManager* pPluginManager, mne::scan::PluginSceneManager* pPluginSceneManager);

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

    //=========================================================================================================
    /**
     * Returns the UI layout blob (base64-encoded QMainWindow state) stored under
     * \a sWindowKey in the currently loaded MNA project's \c ui.layout.* section.
     * Returns an empty QByteArray if no layout was stored.
     */
    QByteArray uiLayoutBlob(const QString& sWindowKey) const;

    //=========================================================================================================
    /**
     * Stores a UI layout blob (base64-encoded QMainWindow state) under \a sWindowKey
     * inside the currently loaded MNA project. The blob is persisted on the next
     * saveConfig() call.
     */
    void setUiLayoutBlob(const QString& sWindowKey, const QByteArray& blob);

    inline mne::scan::AbstractPlugin::SPtr getCurrentPlugin();

    void uiSetupRunningState(bool state);

signals:
    void selectedPluginChanged(mne::scan::AbstractPlugin::SPtr pPlugin);

    void selectedConnectionChanged(mne::scan::PluginConnectorConnection::SPtr pConnection);

private:
    void pointerGroupClicked();
    void actionGroupTriggered(QAction* action);

    bool removePlugin(mne::scan::AbstractPlugin::SPtr pPlugin);

    void itemInserted(PluginItem* item);
    void newItemSelected();

    //=========================================================================================================
    /**
     * Sets an explicit scene rect covering the items plus a margin.
     *
     * Without one the scene rect is derived from the item bounding rect and
     * changes whenever a plugin is added or moved, which rescales the
     * scrollbars underneath the user.
     */
    void updateSceneRect();

    void deleteItem();
    void bringToFront();
    void sendToBack();

    void createActions();
    void createMenuItem();
    void createToolbars();

    QAction* createItemAction(QString name, QMenu* menu);

    //=========================================================================================================
    /**
     * Load config from MNA/MNX file.
     */
    void loadConfigMna(const QString& fullPath);

    //=========================================================================================================
    /**
     * Save config to MNA/MNX file.
     */
    void saveConfigMna(const QString& fullPath);

    mne::scan::PluginManager* m_pPluginManager;           /**< Corresponding plugin manager. */
    mne::scan::PluginSceneManager* m_pPluginSceneManager; /**< Corresponding plugin scene manager. */

    mne::scan::AbstractPlugin::SPtr m_pCurrentPlugin;
    mne::scan::PluginConnectorConnection::SPtr m_pCurrentConnection;

    PluginScene* m_pPluginScene;    /**< Plugin graph. */
    QGraphicsView* m_pGraphicsView; /**< View to show graph. */

    QToolButton* m_pSensorToolButton;
    QToolButton* m_pAlgorithmToolButton;
    QToolBar* m_pToolBarPlugins;
    QActionGroup* m_pActionGroupPlugins;

    QToolButton* m_pPointerButton;
    QToolButton* m_pLinePointerButton;
    QToolBar* m_pToolBarPointer;
    QButtonGroup* m_pButtonGroupPointers;

    QToolBar* m_pToolBarItem;
    QMenu* m_pMenuItem;
    QAction* deleteAction;
    QAction* toFrontAction;
    QAction* sendBackAction;

    mne::MnaProject m_loadedMnaProject; /**< Loaded project preserved for enriching round-trip saves. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline mne::scan::AbstractPlugin::SPtr PluginGui::getCurrentPlugin()
{
    return m_pCurrentPlugin;
}
} //NAMESPACE

#endif // PLUGINGUI_H
