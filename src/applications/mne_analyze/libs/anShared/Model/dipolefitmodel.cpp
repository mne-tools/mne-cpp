//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     dipolefitmodel.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.7
 * @date     November, 2020
 * @brief    Definition of the DipoleFitModel Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dipolefitmodel.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DipoleFitModel::DipoleFitModel(INVLIB::InvEcdSet InvEcdSet,
                               const QString &sFilePath,
                               const QByteArray& byteLoadedData,
                               QObject* parent)
:AbstractModel(parent)
{
    Q_UNUSED(byteLoadedData);
    Q_UNUSED(sFilePath);

    m_ECD_Set = InvEcdSet;
}

//=============================================================================================================

DipoleFitModel::DipoleFitModel(const QString &sFilePath,
                               const QByteArray& byteLoadedData,
                               QObject* parent)
:AbstractModel(sFilePath, parent)
{
    Q_UNUSED(byteLoadedData);

}

//=============================================================================================================

int DipoleFitModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 1;
}

//=============================================================================================================

int DipoleFitModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 1;
}

//=============================================================================================================

QVariant DipoleFitModel::data(const QModelIndex &index,
                             int role) const
{
    Q_UNUSED(index);
    Q_UNUSED(role);

    return QVariant::fromValue(m_ECD_Set);
}

//=============================================================================================================

Qt::ItemFlags DipoleFitModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index);
}
