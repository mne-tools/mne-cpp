//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eventdelegate.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     August, 2014
 * @version  2.1.0
 * @brief Contains the declaration of the EventDelegate class.
 */

#ifndef EVENTDELEGATE_H
#define EVENTDELEGATE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../Models/eventmodel.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QItemDelegate>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDebug>
#include <QSettings>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{


//=============================================================================================================
/**
 * DECLARE CLASS EventDelegate
 */
class EventDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    EventDelegate(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Reimplemented virtual functions
     *
     */
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    QSettings       m_qSettings;            /**< QSettings variable used to write or read from independent application sessions. */
};

} //NAMESPACE

#endif // EVENTDELEGATE_H
