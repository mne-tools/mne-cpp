//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     covariancemodel.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.7
 * @date     November, 2020
 * @brief    Definition of the CovarianceModel Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "covariancemodel.h"

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

CovarianceModel::CovarianceModel(const QString &sFilePath,
                       const QByteArray& byteLoadedData,
                       QObject* parent)
:AbstractModel(sFilePath, parent)
{
    Q_UNUSED(byteLoadedData);
}

//=============================================================================================================

int CovarianceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 1;
}

//=============================================================================================================

int CovarianceModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 1;
}

//=============================================================================================================

QVariant CovarianceModel::data(const QModelIndex &index,
                             int role) const
{
    Q_UNUSED(index);
    Q_UNUSED(role);

    return QVariant();
}

//=============================================================================================================

Qt::ItemFlags CovarianceModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index);
}
