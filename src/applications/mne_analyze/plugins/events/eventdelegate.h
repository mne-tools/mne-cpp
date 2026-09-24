//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     eventdelegate.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.9
 * @date     March, 2021
 * @brief    Contains the declaration of the eventdelegate class.
 */

#ifndef EVENTDELEGATE_H
#define EVENTDELEGATE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <anShared/Model/eventmodel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QItemDelegate>

//=============================================================================================================
/**
 * Delegate for the events and event manager
 */
class EventDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    EventDelegate(QObject *parent = 0);

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // EVENTDELEGATE_H
