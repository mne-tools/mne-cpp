//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     bidsview.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.7
 * @date     October 2020
 * @brief    Tree view of the BIDS dataset hierarchy backed by @ref BidsViewModel.
 *
 * BidsView wraps a @c QTreeView around a @ref BidsViewModel so the
 * user can browse a loaded BIDS dataset (subjects → sessions →
 * runs → derivatives) and double-click items to load them into the
 * application. Context-menu actions expose import / export / convert
 * operations that the surrounding plugin handles.
 */

#ifndef BIDSVIEW_H
#define BIDSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QStandardItem>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class BidsViewWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
/**
 * @brief QTreeView wrapper around a @ref BidsViewModel for browsing a BIDS dataset.
 *
 * Forwards double-click activations on data files / derivatives as
 * @c loadItem signals so the surrounding plugin can open the chosen
 * resource.
 */
class DISPSHARED_EXPORT BidsView : public AbstractView
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs the DataManagerView
     *
     * @param[in] parent     If parent is not NULL the QWidget becomes a child of QWidget inside parent.
     */
    explicit BidsView(QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the DataManagerView.
     */
    virtual ~BidsView();

    //=========================================================================================================
    /**
     * Sets the model to the tree view.
     *
     * @param[in] pModel       The new model.
     */
    void setModel(QAbstractItemModel *pModel);

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    virtual void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    virtual void loadSettings();

    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param[in] mode     The new mode (Clinical=0, Research=1).
     */
    virtual void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param[in] mode     The new mode (RealTime=0, Offline=1).
     */
    virtual void updateProcessingMode(ProcessingMode mode);

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView();

private:

    //=========================================================================================================
    /**
     * Brings up contextual menu for user to interact with data manager
     *
     * @param[in] pos position on screen where the user right clicked.
     */
    void customMenuRequested(QPoint pos);

    //=========================================================================================================
    /**
     * Sends signal to trigger model change when a new model is selcted
     *
     * @param[in] selected     New item being selected.
     * @param[in] deselected   UNUSED - previously selected item.
     */
    void onCurrentItemChanged(const QItemSelection &selected,
                              const QItemSelection &deselected);

    //=========================================================================================================
    /**
     * Uses the indeces of newly added subject and file to select it in the GUI view
     *
     * @param[in] iSubject     index of the subject the new file was added to.
     * @param[in] iModel       index of the new model file relative to the subject.
     */
    void onNewFileLoaded(int iSubject,
                         int iModel);

    //=========================================================================================================
    /**
     * Select new item and expands parent's tree view heirarchy
     *
     * @param[in] itemIndex    new item to be selected.
     */
    void onNewItemIndex(QModelIndex itemIndex);

    //=========================================================================================================
    /**
     * Expands tree view items on reset
     */
    void onModelReset();

    //=========================================================================================================
    /**
     * Procss keyobard input. Currently only delete.
     *
     * @param[in] event    Key event for delete key to delete selected item.
     */
    void keyPressEvent(QKeyEvent *event);

    Ui::BidsViewWidget *m_pUi;   /**< The user interface. */

signals:
    //=========================================================================================================
    /**
     * Sends index of item to be removed from model
     *
     * @param[in] pIndex   index of item to be removed.
     */
    void removeItem(const QModelIndex& pIndex);

    //=========================================================================================================
    /**
     * Sends model in 'data' to be send via the event manager
     *
     * @param[in] data     new selected model in a QVariant.
     */
    void selectedModelChanged(const QVariant& data);

    //=========================================================================================================
    /**
     * Sends the index of currently selected item to update saved current items
     *
     * @param[in] pIndex   index of new item.
     */
    void selectedItemChanged(const QModelIndex& pIndex);

    //=========================================================================================================
    /**
     * Triggers a subject to be added with name sSubjectName
     *
     * @param[in] sSubjectName     name of new subject.
     */
    void onAddSubject(const QString &sSubjectName);

    //=========================================================================================================
    /**
     * Triggers a session  to be added with name sSessionName to subject at index subjectIndex
     *
     * @param[in] subjectIndex     index of subject to which the session will be added.
     * @param[in] sSessionName     name of new session.
     */
    void onAddSession(QModelIndex subjectIndex,
                      const QString &sSessionName);

    //=========================================================================================================
    /**
     * Triggers session at sessionIndex to be moved to subject at subjectIndex
     *
     * @param[in] subjectIndex     index of destination subject.
     * @param[in] sessionIndex     index of session to be moved.
     */
    void onMoveSession(QModelIndex subjectIndex,
                       QModelIndex sessionIndex);

    //=========================================================================================================
    /**
     * Triggers data at dataIndex to be moved to session at sessionIndex
     *
     * @param[in] sessionIndex     index of destination session.
     * @param[in] dataIndex        index of data to me moved.
     */
    void onMoveData(QModelIndex sessionIndex,
                    QModelIndex dataIndex);

};
} // NAMESPACE DISPLIB
#endif // BIDSVIEW_H
