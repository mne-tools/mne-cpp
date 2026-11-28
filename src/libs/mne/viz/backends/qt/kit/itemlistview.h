//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     itemlistview.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ItemListView shows any toolkit-free viz::ItemList as a QListView.
 */

#ifndef VIZ_QT_ITEMLISTVIEW_H
#define VIZ_QT_ITEMLISTVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_qt_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListView>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne::viz
{
class ItemList;
} // namespace mne::viz

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

//=============================================================================================================
/**
 * @brief The Qt view of a viz::ItemList: its items, its selection kept in sync both ways, and Delete removing the
 * selected items of removable lists.
 *
 * @since 3.0.0
 */
class VIZQTSHARED_EXPORT ItemListView : public QListView
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Shows a list.
     *
     * @param[in] list     The list, shared with its owner.
     * @param[in] parent   Parent widget.
     */
    explicit ItemListView(std::shared_ptr<viz::ItemList> list, QWidget* parent = nullptr);

    ~ItemListView() override;

    //=========================================================================================================
    /**
     * @return The list.
     */
    viz::ItemList& list() const
    {
        return *m_list;
    }

protected:
    //=========================================================================================================
    /**
     * Removes the selected items of a removable list on Delete or Backspace.
     *
     * @param[in] event   The key event.
     */
    void keyPressEvent(QKeyEvent* event) override;

private:
    class Model;

    void pullSelection();
    void pushSelection();

    std::shared_ptr<viz::ItemList> m_list; /**< The list. */
    Model* m_model = nullptr;              /**< Qt model of the list. */
    int m_listener = 0;                    /**< Listener on the list. */
    bool m_syncing = false;                /**< Whether the selection is being copied. */
};

} // namespace mne::viz::qt

#endif // VIZ_QT_ITEMLISTVIEW_H
