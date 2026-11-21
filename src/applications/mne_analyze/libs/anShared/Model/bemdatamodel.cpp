//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     bemdatamodel.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.6
 * @date     August, 2020
 * @brief    BemDataModel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bemdatamodel.h"

#include <fiff/fiff_stream.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::analyze;
using namespace mne;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BemDataModel::BemDataModel(QObject* pParent)
: AbstractModel(pParent)
{
    qInfo() << "[BemDataModel::BemDataModel] Default constructor called !";
}

//=============================================================================================================

BemDataModel::BemDataModel(const QString& sFilePath,
                           const QByteArray& byteLoadedData,
                           QObject* pParent)
: AbstractModel(sFilePath, pParent)
, m_pBem(std::make_shared<MNEBem>())
{
    if (byteLoadedData.isEmpty()) {
        initBemData(std::make_unique<mne::FiffStream>(sFilePath.toStdU16String()));
    } else {
        const auto* begin = reinterpret_cast<const std::byte*>(byteLoadedData.constData());
        initBemData(std::make_unique<mne::FiffStream>(std::make_unique<mne::MemoryByteStream>(std::vector<std::byte>(begin, begin + byteLoadedData.size()))));
    }
}

//=============================================================================================================

BemDataModel::~BemDataModel()
{
}

//=============================================================================================================

void BemDataModel::initBemData(std::unique_ptr<mne::FiffStream> stream)
{
    auto bem = MNEBem::read(*stream);
    if (!bem) {
        qWarning() << "[BemDataModel::initBemData]" << bem.error().what();
        return;
    }

    *m_pBem = std::move(*bem);
    emit newBemAvailable(m_pBem);
    m_bIsInit = true;
}

//=============================================================================================================

QVariant BemDataModel::data([[maybe_unused]] const QModelIndex& index,
                            [[maybe_unused]] int role) const
{
    return QVariant();
}

//=============================================================================================================

Qt::ItemFlags BemDataModel::flags(const QModelIndex& index) const
{
    return QAbstractItemModel::flags(index);
}

//=============================================================================================================

QModelIndex BemDataModel::index(int row,
                                int column,
                                const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column);
}

//=============================================================================================================

QModelIndex BemDataModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

//=============================================================================================================

int BemDataModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 0;
}

//=============================================================================================================

int BemDataModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 0;
}
