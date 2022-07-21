//=============================================================================================================
/**
 * @file     channelselectionview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh, Gabriel B Motta. All rights reserved.
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
 * @brief    Contains the declaration of the ChannelSelectionView class.
 *
 */

#ifndef CHANNELSELECTIONVIEW_H
#define CHANNELSELECTIONVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QListWidget>
#include <QGraphicsItem>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QListWidgetItem;

namespace Ui {
    class ChannelSelectionViewWidget;
} //This must be defined outside of the DISPLIB namespace

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class ChannelInfoModel;
class SelectionScene;

/**
 * DECLARE CLASS ChannelSelectionView
 *
 * @brief The ChannelSelectionView class provides a channel selection window.
 */
class DISPSHARED_EXPORT ChannelSelectionView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<ChannelSelectionView> SPtr;              /**< Shared pointer type for ChannelSelectionView. */
    typedef QSharedPointer<const ChannelSelectionView> ConstSPtr;   /**< Const shared pointer type for ChannelSelectionView. */

    //=========================================================================================================
    /**
     * Constructs a ChannelSelectionView which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new ChannelSelectionView becomes a window. If parent is another widget, ChannelSelectionView becomes a child window inside parent. ChannelSelectionView is deleted when its parent is deleted.
     * @param[in] pChannelInfoModel pointer to the channel info model.
     */
    ChannelSelectionView(const QString& sSettingsPath = "",
                         QWidget *parent = 0,
                         QSharedPointer<ChannelInfoModel> pChannelInfoModel = QSharedPointer<ChannelInfoModel>(0),
                         Qt::WindowType f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the ChannelSelectionView.
     * All ChannelSelectionView's children are deleted first. The application exits if ChannelSelectionView is the main widget.
     */
    ~ChannelSelectionView();

    //=========================================================================================================
    /**
     * Sets the currently mapped fiff channels. used to create the group All.
     *
     * @param[in] mappedLayoutChNames the currently to layout mapped channels.
     */
    void setCurrentlyMappedFiffChannels(const QStringList &mappedLayoutChNames);

    //=========================================================================================================
    /**
     * Highlight channels
     * This function highlights channels which were selected outside this selection manager (i.e in the DataWindow's Table View)
     *
     * @param[in] channelList channels which are be to set as selected.
     */
    void highlightChannels(QModelIndexList channelIndexList);

    //=========================================================================================================
    /**
     * Select channels
     * This function selects channels which were selected outside this selection manager (i.e in the DataWindow's Table View)
     *
     * @param[in] channelList channels which are be to set as selected.
     */
    void selectChannels(QStringList channelList);

    //=========================================================================================================
    /**
     * Current selected channels
     * This function returns the current channel selection
     */
    QStringList getSelectedChannels();

    //=========================================================================================================
    /**
     * gets the item corresponding to text in listWidget
     *
     * @param[in] listWidget QListWidget which inhibits the needed item.
     * @param[in] channelName the corresponding channel name.
     */
    QListWidgetItem* getItemForChName(QListWidget *listWidget,
                                      const QString& channelName);

    //=========================================================================================================
    /**
     * returns the current layout map.
     */
    const QMap<QString,QPointF>& getLayoutMap();

    //=========================================================================================================
    /**
     * call this whenever a new file was loaded.
     */
    void newFiffFileLoaded(QSharedPointer<FIFFLIB::FiffInfo>& pFiffInfo);

    //=========================================================================================================
    /**
     * Returns the currently selected layout file.
     *
     * @return the currently selected layout file.
     */
    QString getCurrentLayoutFile();

    //=========================================================================================================
    /**
     * Returns the currently loaded group file
     *
     * @return currently loaded selection group file.
     */
    QString getCurrentGroupFile();

    //=========================================================================================================
    /**
     * Sets the current layout file.
     *
     * @param[in] currentLayoutFile the current layout file.
     */
    void setCurrentLayoutFile(QString currentLayoutFile);

    //=========================================================================================================
    /**
     * Update the scene items according to the bad channel list in the fiff info file.
     */
    void updateBadChannels();

    //=========================================================================================================
    /**
     * Updates data view.
     *
     */
    void updateDataView();

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    void loadSettings();

    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param[in] mode     The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param[in] mode     The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode);

    //=========================================================================================================
    /**
     * Returns pointer to the portion of the channel view with the graphics view
     *
     * @return pointer to the channelselectionview view.
     */
    QWidget* getViewWidget();

    //=========================================================================================================
    /**
     * Returns pointer to the portion of the channel view with the controls
     *
     * @return pointer to the channelselectionview controls.
     */
    QWidget* getControlWidget();

    //=========================================================================================================
    /**
     * Returns whether there is a user selection
     *
     * @return true if there is a user selection, false if not.
     */
    bool isSelectionEmpty();

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView();

private:
    //=========================================================================================================
    /**
     * Initialises all tabel widgets in the selection window.
     *
     */
    void initListWidgets();

    //=========================================================================================================
    /**
     * Initialises all graphic views in the selection window.
     *
     */
    void initSelectionSceneView();

    //=========================================================================================================
    /**
     * Initialises all combo boxes in the selection window.
     *
     */
    void initComboBoxes();

    //=========================================================================================================
    /**
     * Initialises all buttons in the selection window.
     *
     */
    void initButtons();

    //=========================================================================================================
    /**
     * Initialises all check boxes in the selection window.
     *
     */
    void initCheckBoxes();

    //=========================================================================================================
    /**
     * Loads a new layout from given file path.
     *
     * @param[in] path holds file pathloll.
     */
    bool loadLayout(QString path);

    //=========================================================================================================
    /**
     * Loads a new selection from given file path.
     *
     * @param[in] path holds file path.
     */
    bool loadSelectionGroups(QString path);

    //=========================================================================================================
    /**
     * Delete all MEG channels from the selection groups which are not in the loaded layout. This needs to be done to guarantee consistency between the selection files and layout files (the selection files always include ALL MEG channels (gradiometers+magnitometers))
     *
     */
    void cleanUpMEGChannels();

    //=========================================================================================================
    /**
     * Updates selection group widget in this window.
     *
     * @param[in] current the current selection group list item.
     * @param[in] previous the previous selection group list item.
     */
    void updateSelectionGroupsList(QListWidgetItem* current,
                                   QListWidgetItem* previous);

    //=========================================================================================================
    /**
     * Updates the scene regarding the selecting channel QList.
     *
     */
    void updateSceneItems();

    //=========================================================================================================
    /**
     * Updates user defined selections.
     *
     */
    void updateUserDefinedChannelsList();

    //=========================================================================================================
    /**
     * loads a user selection file.
     *
     */
    void onBtnLoadUserSelection();

    //=========================================================================================================
    /**
     * Saves a user selection file.
     *
     */
    void onBtnSaveUserSelection();

    //=========================================================================================================
    /**
     * Add the user defined channels to the selection groups.
     *
     */
    void onBtnAddToSelectionGroups();

    //=========================================================================================================
    /**
     * Loads a new layout selected from the layout combo box.
     *
     */
    void onComboBoxLayoutChanged();

    //=========================================================================================================
    /**
     * Reimplemented resize event.
     *
     */
    void resizeEvent(QResizeEvent* event);

    //=========================================================================================================
    /**
     * Installed event filter.
     *
     */
    bool eventFilter(QObject *obj, QEvent *event);

    Ui::ChannelSelectionViewWidget*     m_pUi;                              /**< Pointer to the qt designer generated ui class. */

    QSharedPointer<ChannelInfoModel>    m_pChannelInfoModel;                /**< Pointer to the channel info model. */

    QMap<QString,QPointF>               m_layoutMap;                        /**< QMap with the loaded layout. each channel name correspond to a QPointF variable. */
    QMultiMap<QString,QStringList>      m_selectionGroupsMap;               /**< QMultiMap with the loaded selection groups. Each group name holds a string list with the corresponding channels of the group.*/

    SelectionScene*                     m_pSelectionScene;                  /**< Pointer to the selection scene class. */

    QStringList                         m_currentlyLoadedFiffChannels;      /**< List of currently loaded fiff data channels.*/

    QString                             m_sSettingsPath;                    /**< The settings path to store the GUI settings to. */

    bool                                m_bSetup;
signals:
    //=========================================================================================================
    /**
     * emit this signal whenever the user or group selection has changed
     *
     * @param[in] selectedChannels currently user selected channels or items which are in the visible list widget.
     */
    void showSelectedChannelsOnly(QStringList selectedChannels);

    //=========================================================================================================
    /**
     * emit this signal whenever the selection in the scene has changed
     *
     * @param[in] selectedChannelItems currently user selected channels.
     */
    void selectionChanged(const QList<QGraphicsItem*> &selectedChannelItems);

    //=========================================================================================================
    /**
     * emit this signal whenever a new layout was loaded
     *
     * @param[in] layoutMap currently loaded layout.
     */
    void loadedLayoutMap(const QMap<QString,QPointF> &layoutMap);
};
} // NAMESPACE DISPLIB

#endif // CHANNELSELECTIONVIEW_H
