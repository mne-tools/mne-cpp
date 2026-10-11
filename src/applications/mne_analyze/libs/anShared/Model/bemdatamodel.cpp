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
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;
using namespace MNELIB;

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
, m_pBem(MNEBem::SPtr::create())
{
    if (byteLoadedData.isEmpty()) {
        m_file.setFileName(sFilePath);
        initBemData(m_file);
    } else {
        m_byteLoadedData = byteLoadedData;
        m_buffer.setData(m_byteLoadedData);
        initBemData(m_buffer);
    }
}

//=============================================================================================================

BemDataModel::~BemDataModel()
{
}

//=============================================================================================================

void BemDataModel::initBemData(QIODevice& qIODevice)
{
    // build Bem
    FIFFLIB::FiffStream::SPtr stream(new FIFFLIB::FiffStream(&qIODevice));
    MNEBem bem;
    const bool read = MNEBem::readFromStream(stream, true, bem);
    qIODevice.close();
    if (!read || bem.isEmpty()) {
        qWarning() << "[BemDataModel::initBemData] File does not contain any Bem data";
        return;
    }

    *m_pBem = bem;
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
