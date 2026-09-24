//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     averagingdatamodel.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.6
 * @date     September, 2020
 * @brief    Definition of the AveragingDataModel Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagingdatamodel.h"

#include <fiff/fiff_evoked_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>

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

AveragingDataModel::AveragingDataModel(QSharedPointer<FIFFLIB::FiffEvokedSet> pEvokedSet,
                                       QObject* parent)
: AbstractModel(parent)
, m_pFiffEvokedSet(pEvokedSet)
, m_bFromFile(false)
{

}

//=============================================================================================================

AveragingDataModel::AveragingDataModel(const QString &sFilePath,
                                       const QByteArray& byteLoadedData,
                                       QObject* parent)
: AbstractModel(sFilePath, parent)
, m_bFromFile(true)
{
    Q_UNUSED(byteLoadedData);

    QFile file(sFilePath);
    m_pFiffEvokedSet = QSharedPointer<FIFFLIB::FiffEvokedSet>::create(file);
}

//=============================================================================================================

AveragingDataModel::~AveragingDataModel()
{

}

//=============================================================================================================

QVariant AveragingDataModel::data(const QModelIndex &index,
                                  int role) const
{
    Q_UNUSED(index);
    Q_UNUSED(role);

    return QVariant::fromValue(m_pFiffEvokedSet);
}

//=============================================================================================================

Qt::ItemFlags AveragingDataModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index);
}

//=============================================================================================================

int AveragingDataModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

//=============================================================================================================

int AveragingDataModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

//=============================================================================================================

void AveragingDataModel::setEvokedSet(QSharedPointer<FIFFLIB::FiffEvokedSet> pEvokedSet)
{
    m_pFiffEvokedSet = pEvokedSet;
}

//=============================================================================================================

bool AveragingDataModel::isFromFile()
{
    return m_bFromFile;
}

//=============================================================================================================

QSharedPointer<FIFFLIB::FiffInfo> AveragingDataModel::getFiffInfo()
{
    QSharedPointer<FIFFLIB::FiffInfo> pInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FIFFLIB::FiffInfo(m_pFiffEvokedSet->info));
    return pInfo;
}

//=============================================================================================================

QSharedPointer<FIFFLIB::FiffEvokedSet> AveragingDataModel::getEvokedSet()
{
    return m_pFiffEvokedSet;
}
