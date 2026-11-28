//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     itemlistview.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ItemListView class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "itemlistview.h"
#include <mne/viz/models/item_list.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAbstractListModel>
#include <QItemSelectionModel>
#include <QKeyEvent>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

/** The items of the list as a Qt list model. */
class ItemListView::Model : public QAbstractListModel
{
public:
    Model(const ItemList& list, QObject* parent)
    : QAbstractListModel(parent)
    , m_list(list)
    {
    }

    int rowCount(const QModelIndex& parent) const override
    {
        return parent.isValid() ? 0 : m_list.count();
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (role != Qt::DisplayRole || !index.isValid() || index.row() >= m_list.count()) {
            return {};
        }
        return QString::fromStdString(m_list.items()[static_cast<std::size_t>(index.row())]);
    }

    void reset()
    {
        beginResetModel();
        endResetModel();
    }

private:
    const ItemList& m_list;
};

//=============================================================================================================

ItemListView::ItemListView(std::shared_ptr<ItemList> list, QWidget* parent)
: QListView(parent)
, m_list(std::move(list))
, m_model(new Model(*m_list, this))
{
    setModel(m_model);
    switch (m_list->selection_mode()) {
        case ItemList::SelectionMode::None:
            setSelectionMode(QAbstractItemView::NoSelection);
            break;
        case ItemList::SelectionMode::Single:
            setSelectionMode(QAbstractItemView::SingleSelection);
            break;
        case ItemList::SelectionMode::Multi:
            setSelectionMode(QAbstractItemView::ExtendedSelection);
            break;
    }
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    const int rowHeight = sizeHintForRow(0) > 0 ? sizeHintForRow(0) : fontMetrics().height() + 4;
    setMinimumHeight(rowHeight * m_list->visible_rows() + 2 * frameWidth());

    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        pushSelection();
    });
    m_listener = m_list->add_listener([this](ItemList::Change change) {
        if (m_syncing) {
            return;
        }
        if (change == ItemList::Change::Items) {
            m_model->reset();
        }
        pullSelection();
    });
    pullSelection();
}

//=============================================================================================================

ItemListView::~ItemListView()
{
    m_list->remove_listener(m_listener);
}

//=============================================================================================================

void ItemListView::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) && m_list->removable()) {
        m_list->remove_selected();
        event->accept();
        return;
    }
    QListView::keyPressEvent(event);
}

//=============================================================================================================

void ItemListView::pullSelection()
{
    m_syncing = true;
    QItemSelection selection;
    for (const int row : m_list->selected()) {
        const QModelIndex index = m_model->index(row);
        selection.select(index, index);
    }
    selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
    if (m_list->current() >= 0) {
        selectionModel()->setCurrentIndex(m_model->index(m_list->current()), QItemSelectionModel::NoUpdate);
    }
    m_syncing = false;
}

//=============================================================================================================

void ItemListView::pushSelection()
{
    if (m_syncing) {
        return;
    }
    std::vector<int> rows;
    for (const QModelIndex& index : selectionModel()->selectedIndexes()) {
        rows.push_back(index.row());
    }
    m_syncing = true;
    m_list->set_selected(std::move(rows));
    m_syncing = false;
}
