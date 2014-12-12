//=============================================================================================================
/**
* @file     selectionmanagerwindow.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the SelectionManagerWindow class.
*
*/

#ifndef SELECTIONMANAGERWINDOW_H
#define SELECTIONMANAGERWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_selectionmanagerwindow.h"
#include "utils/layoutloader.h"             //MNE-CPP utils
#include "utils/selectionloader.h"          //MNE-CPP utils
#include "utils/layoutmaker.h"              //MNE-CPP utils
#include "../Utils/selectionscene.h"        //MNE Browse Raw QT utils
#include "fiff/fiff.h"
#include "../Models/chinfomodel.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>
#include <QMutableStringListIterator>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBrowseRawQt
//=============================================================================================================

namespace MNEBrowseRawQt
{


//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================


/**
* DECLARE CLASS SelectionManagerWindow
*
* @brief The SelectionManagerWindow class provides a channel selection window.
*/
class SelectionManagerWindow : public QDockWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a SelectionManagerWindow which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new SelectionManagerWindow becomes a window. If parent is another widget, SelectionManagerWindow becomes a child window inside parent. SelectionManagerWindow is deleted when its parent is deleted.
    * @param [in] pChInfoModel pointer to the channel info model.
    */
    SelectionManagerWindow(QWidget *parent = 0, ChInfoModel *pChInfoModel = 0);

    //=========================================================================================================
    /**
    * Destroys the SelectionManagerWindow.
    * All SelectionManagerWindow's children are deleted first. The application exits if SelectionManagerWindow is the main widget.
    */
    ~SelectionManagerWindow();

    //=========================================================================================================
    /**
    * Sets the currently mapped fiff channels. used to create the group All.
    *
    * @param [in] mappedLayoutChNames the currently to layout mapped channels
    */
    void setCurrentlyMappedFiffChannels(const QStringList &mappedLayoutChNames);

    //=========================================================================================================
    /**
    * Highlight channels
    * This function highlights channels which were selected outside this selection manager (i.e in the DataWindow's Table View)
    *
    * @param [in] channelList channels which are be to set as selected
    */
    void highlightChannels(QStringList channelList);

    //=========================================================================================================
    /**
    * Select channels
    * This function selects channels which were selected outside this selection manager (i.e in the DataWindow's Table View)
    *
    * @param [in] channelList channels which are be to set as selected
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
    * @param [in] listWidget QListWidget which inhibits the needed item
    * @param [in] channelName the corresponding channel name
    */
    QListWidgetItem* getItemForChName(QListWidget *listWidget, QString channelName);

    //=========================================================================================================
    /**
    * returns the current layout map.
    */
    const QMap<QString,QPointF>& getLayoutMap();

    //=========================================================================================================
    /**
    * call this whenever a new file was loaded.
    */
    void newFiffFileLoaded();

signals:
    //=========================================================================================================
    /**
    * emit this signal whenever the user or group selection has changed
    *
    * @param [in] selectedChannels currently user selected channels or items which are in the visible list widget
    */
    void showSelectedChannelsOnly(QStringList selectedChannels);

    //=========================================================================================================
    /**
    * emit this signal whenever the selection in the scene has changed
    *
    * @param [in] selectedChannelItems currently user selected channels
    */
    void selectionChanged(const QList<QGraphicsItem*> &selectedChannelItems);

    //=========================================================================================================
    /**
    * emit this signal whenever a new layout was loaded
    *
    * @param [in] layoutMap currently loaded layout
    */
    void loadedLayoutMap(const QMap<QString,QPointF> &layoutMap);

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
    * Loads a new layout from given file path.
    *
    * @param [in] path holds file pathloll
    */
    bool loadLayout(QString path);

    //=========================================================================================================
    /**
    * Loads a new selection from given file path.
    *
    * @param [in] path holds file path
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
    * @param [in] current the current selection group list item
    * @param [in] previous the previous selection group list item
    */
    void updateSelectionGroupsList(QListWidgetItem* current, QListWidgetItem* previous);

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
    * Updates data view.
    *
    */
    void updateDataView();

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

    Ui::SelectionManagerWindow*     ui;                                 /**< Pointer to the qt designer generated ui class. */

    ChInfoModel*                    m_pChInfoModel;                     /**< Pointer to the channel info model. */

    QMap<QString,QPointF>           m_layoutMap;                        /**< QMap with the loaded layout. each channel name correspond to a QPointF variable. */
    QMap<QString,QStringList>       m_selectionGroupsMap;               /**< QMap with the loaded selection groups. Each group name holds a string list with the corresponding channels of the group.*/

    SelectionScene*                 m_pSelectionScene;                  /**< Pointer to the selection scene class. */

    QStringList                     m_currentlyLoadedFiffChannels;      /**< List of currently loaded fiff data channels.*/
};

} // NAMESPACE MNEBrowseRawQt

#endif // SELECTIONMANAGERWINDOW_H
