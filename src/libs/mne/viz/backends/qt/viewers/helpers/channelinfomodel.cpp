//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     channelinfomodel.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Jens Haueisen <jens.haueisen@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     November 2014
 * @brief    Implementation of the ChannelInfoModel per-channel info table model.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channelinfomodel.h"
#include <mne/base/log.h>

#include "mneoperator.h"
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector3D>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <memory>
#include <utility>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;
using namespace Eigen;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelInfoModel::ChannelInfoModel(std::shared_ptr<FiffInfo>& pFiffInfo, QObject* parent)
: QAbstractTableModel(parent)
, m_pFiffInfo(pFiffInfo)
{
    setFiffInfo(m_pFiffInfo);
}

//=============================================================================================================

ChannelInfoModel::ChannelInfoModel(QObject* parent)
: QAbstractTableModel(parent)
, m_pFiffInfo(std::make_shared<FiffInfo>())
{
}

//=============================================================================================================

int ChannelInfoModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_table.row_count();
}

//=============================================================================================================

int ChannelInfoModel::columnCount(const QModelIndex& /*parent*/) const
{
    return viz::ChannelTable::column_count;
}

//=============================================================================================================

QVariant ChannelInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::Alignment(Qt::AlignHCenter | Qt::AlignVCenter));
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Vertical) {
        return section < m_table.row_count() ? QVariant(QString("Ch %1").arg(section)) : QVariant();
    }

    if (section >= 0 && section < viz::ChannelTable::column_count) {
        return QString::fromStdString(viz::ChannelTable::header(static_cast<viz::ChannelTable::Column>(section)));
    }

    return QVariant();
}

//=============================================================================================================

QVariant ChannelInfoModel::data(const QModelIndex& index, int role) const
{
    using Column = viz::ChannelTable::Column;

    if (!index.isValid() || index.row() >= m_table.row_count() || index.column() >= viz::ChannelTable::column_count) {
        return QVariant();
    }

    if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::Alignment(Qt::AlignHCenter | Qt::AlignVCenter));
    }

    const int row = index.row();
    const auto column = static_cast<Column>(index.column());
    const FiffChInfo& ch = m_table.info()->chs.at(static_cast<std::size_t>(row));
    const auto text = [&] {
        return QVariant(QString::fromStdString(m_table.text(row, column)));
    };

    switch (column) {
        case Column::Number:
            return role == Qt::DisplayRole || role == ChannelInfoModelRoles::GetChNumber ? QVariant(row) : QVariant();
        case Column::OriginalName:
            return role == Qt::DisplayRole || role == ChannelInfoModelRoles::GetOrigChName ? text() : QVariant();
        case Column::Alias:
            return role == Qt::DisplayRole || role == ChannelInfoModelRoles::GetChAlias ? text() : QVariant();
        case Column::MappedLayoutName:
            return role == Qt::DisplayRole || role == ChannelInfoModelRoles::GetMappedLayoutChName ? text() : QVariant();
        case Column::Kind:
            if (role == ChannelInfoModelRoles::GetChKind) {
                return QVariant(ch.kind);
            }
            return role == Qt::DisplayRole ? text() : QVariant();
        case Column::MegType:
            return role == Qt::DisplayRole || role == ChannelInfoModelRoles::GetMEGType ? text() : QVariant();
        case Column::Unit:
            if (role == ChannelInfoModelRoles::GetChUnit) {
                return QVariant(ch.unit);
            }
            return role == Qt::DisplayRole ? text() : QVariant();
        case Column::Position:
            if (role == ChannelInfoModelRoles::GetChPosition) {
                const std::array<double, 2> point = m_table.position(row);
                return QVariant(QPointF(point[0], point[1]));
            }
            return role == Qt::DisplayRole ? text() : QVariant();
        case Column::Digitizer:
            if (role == ChannelInfoModelRoles::GetChDigitizer) {
                const Eigen::Vector3f point = m_table.digitizer_cm(row);
                return QVariant(QVector3D(point.x(), point.y(), point.z()));
            }
            return role == Qt::DisplayRole ? text() : QVariant();
        case Column::ActiveFilter:
            return QVariant();
        case Column::CoilType:
            if (role == ChannelInfoModelRoles::GetChCoilType) {
                return QVariant(ch.chpos.coil_type);
            }
            return role == Qt::DisplayRole ? text() : QVariant();
        case Column::Bad:
            return role == Qt::DisplayRole || role == ChannelInfoModelRoles::GetIsBad ? QVariant(m_table.is_bad(row)) : QVariant();
        case Column::Compensators:
            return role == Qt::DisplayRole ? QVariant(static_cast<int>(m_table.info()->comps.size())) : QVariant();
    }

    return QVariant();
}

//=============================================================================================================

bool ChannelInfoModel::insertRows(int /*position*/, int /*span*/, const QModelIndex& /*parent*/)
{
    return true;
}

//=============================================================================================================

bool ChannelInfoModel::removeRows(int /*position*/, int /*span*/, const QModelIndex& /*parent*/)
{
    return true;
}

//=============================================================================================================

Qt::ItemFlags ChannelInfoModel::flags(const QModelIndex& /*index*/) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

//=============================================================================================================

bool ChannelInfoModel::setData(const QModelIndex& /*index*/, const QVariant& /*value*/, int /*role*/)
{
    return true;
}

//=============================================================================================================

void ChannelInfoModel::setFiffInfo(std::shared_ptr<FiffInfo>& pFiffInfo)
{
    beginResetModel();

    m_pFiffInfo = pFiffInfo;
    m_table.set_info(m_pFiffInfo);

    endResetModel();

    emit channelsMappedToLayout(m_table.mapped_names());
    emit dataChanged(createIndex(0, 0), createIndex(rowCount(), columnCount()));
}

//=============================================================================================================

void ChannelInfoModel::assignedOperatorsChanged(const QMultiMap<int, std::shared_ptr<MNEOperator>>& assignedOperators)
{
    beginResetModel();

    m_assignedOperators = assignedOperators;

    endResetModel();

    emit dataChanged(createIndex(0, 0), createIndex(rowCount(), columnCount()));
}

//=============================================================================================================

void ChannelInfoModel::layoutChanged(const std::map<std::string, QPointF>& layoutMap)
{
    beginResetModel();

    std::map<std::string, std::array<double, 2>> layout;
    for (const auto& [name, point] : layoutMap) {
        layout.emplace(name, std::array<double, 2>{point.x(), point.y()});
    }
    m_table.set_layout(std::move(layout));

    endResetModel();

    emit channelsMappedToLayout(m_table.mapped_names());
    emit dataChanged(createIndex(0, 0), createIndex(rowCount(), columnCount()));
}

//=============================================================================================================

void ChannelInfoModel::setLayout(const std::map<std::string, viz::Point>& layout)
{
    std::map<std::string, QPointF> layoutMap;
    for (const auto& [name, point] : layout) {
        layoutMap.emplace(name, QPointF(point.x, point.y));
    }
    layoutChanged(layoutMap);
}

//=============================================================================================================

const std::vector<std::string>& ChannelInfoModel::getMappedChannelsList()
{
    return m_table.mapped_names();
}

//=============================================================================================================

int ChannelInfoModel::getIndexFromOrigChName(const std::string& chName)
{
    return m_table.index_of_original(chName);
}

//=============================================================================================================

int ChannelInfoModel::getIndexFromMappedChName(const std::string& chName)
{
    return m_table.index_of_mapped(chName);
}

//=============================================================================================================

StringList ChannelInfoModel::getBadChannelList()
{
    return m_table.bad_channels();
}

//=============================================================================================================

void ChannelInfoModel::clearModel()
{
    beginResetModel();

    m_pFiffInfo = std::make_shared<FiffInfo>();
    m_table.clear();

    endResetModel();

    mne::Log::debug("ChannelInfoModel cleared.");
}
