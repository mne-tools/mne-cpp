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
#include "../../detail/map_value.h"
#include <mne/base/log.h>

#include "mneoperator.h"
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_ch_name.h>
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector3D>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cctype>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;
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
    //Return number of stored evoked sets
    if (!m_pFiffInfo->chs.empty())
        return static_cast<int>(m_pFiffInfo->chs.size());
    else
        return 0;
}

//=============================================================================================================

int ChannelInfoModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 13;
}

//=============================================================================================================

QVariant ChannelInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    Qt::Alignment a;
    switch (role) {
        case Qt::TextAlignmentRole:
            a = Qt::AlignHCenter | Qt::AlignVCenter;
            return QVariant(a);

        case Qt::DisplayRole: {
            //Return the number and description/comment of the fiff evoked data in the set as vertical header
            if (orientation == Qt::Vertical)
                if (section < std::ssize(m_pFiffInfo->chs))
                    return QString("Ch %1").arg(section);

            //Return the horizontal header
            if (orientation == Qt::Horizontal) {
                switch (section) {
                    case 0:
                        return QString("%1").arg("Data number");

                    case 1:
                        return QString("%1").arg("Original name");

                    case 2:
                        return QString("%1").arg("Alias");

                    case 3:
                        return QString("%1").arg("Mapped layout name");

                    case 4:
                        return QString("%1").arg("Channel kind");

                    case 5:
                        return QString("%1").arg("MEG type");

                    case 6:
                        return QString("%1").arg("Unit");

                    case 7:
                        return QString("%1").arg("Position");

                    case 8:
                        return QString("%1").arg("Digitizer (cm)");

                    case 9:
                        return QString("%1").arg("Active filter");

                    case 10:
                        return QString("%1").arg("Coil Type");

                    case 11:
                        return QString("%1").arg("Bad channel");

                    case 12:
                        return QString("%1").arg("# of Compensators");
                }
            }
        }
    }

    return QVariant();
}

//=============================================================================================================

QVariant ChannelInfoModel::data(const QModelIndex& index, int role) const
{
    if (index.row() >= std::ssize(m_pFiffInfo->chs))
        return QVariant();

    if (index.isValid()) {
        //******** first column (channel number - corresponds to row in fiff data matrix) ********
        if (index.column() == 0) {
            QVariant v;

            switch (role) {
                case Qt::DisplayRole:
                    v.setValue(index.row());
                    return v;

                case ChannelInfoModelRoles::GetChNumber:
                    v.setValue(index.row());
                    return v;

                case Qt::TextAlignmentRole:
                    Qt::Alignment a = Qt::AlignHCenter | Qt::AlignVCenter;
                    return QVariant(a);
            }
        } //end column check

        //******** second column (original channel name) ********
        if (index.column() == 1) {
            QVariant v;

            switch (role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_pFiffInfo->chs.at(index.row()).ch_name));
                    return v;

                case ChannelInfoModelRoles::GetOrigChName:
                    v.setValue(QString("%1").arg(m_pFiffInfo->chs.at(index.row()).ch_name));
                    return v;

                case Qt::TextAlignmentRole:
                    Qt::Alignment a = Qt::AlignHCenter | Qt::AlignVCenter;
                    return QVariant(a);
            }
        } //end column check

        //******** third column (channel alias) ********
        if (index.column() == 2) {
            QVariant v;

            switch (role) {
                case Qt::DisplayRole:
                    if (index.row() < std::ssize(m_aliasNames))
                        v.setValue(QString::fromStdString(m_aliasNames.at(index.row())));
                    return v;

                case ChannelInfoModelRoles::GetChAlias:
                    if (index.row() < std::ssize(m_aliasNames))
                        v.setValue(QString::fromStdString(m_aliasNames.at(index.row())));
                    return v;

                case Qt::TextAlignmentRole:
                    Qt::Alignment a = Qt::AlignHCenter | Qt::AlignVCenter;
                    return QVariant(a);
            }
        } //end column check

        //******** fourth column (mapped layout channel name) ********
        if (index.column() == 3) {
            QVariant v;

            switch (role) {
                case Qt::DisplayRole:
                    if (index.row() < std::ssize(m_mappedLayoutChNames))
                        v.setValue(QString::fromStdString(m_mappedLayoutChNames.at(index.row())));
                    return v;

                case ChannelInfoModelRoles::GetMappedLayoutChName:
                    if (index.row() < std::ssize(m_mappedLayoutChNames))
                        v.setValue(QString::fromStdString(m_mappedLayoutChNames.at(index.row())));
                    return v;

                case Qt::TextAlignmentRole:
                    Qt::Alignment a = Qt::AlignHCenter | Qt::AlignVCenter;
                    return QVariant(a);
            }
        } //end column check

        //******** fifth column (channel kind - MEG, EEG, etc) ********
        if (index.column() == 4) {
            QVariant v;

            switch (role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_pFiffInfo->chs.at(index.row()).kind));
                    return v;

                case ChannelInfoModelRoles::GetChKind:
                    v.setValue(m_pFiffInfo->chs.at(index.row()).kind);
                    return v;

                case Qt::TextAlignmentRole:
                    Qt::Alignment a = Qt::AlignHCenter | Qt::AlignVCenter;
                    return QVariant(a);
            }
        } //end column check

        //******** sixth column (MEG type) ********
        if (index.column() == 5) {
            QVariant v;

            v.setValue(QString("%1").arg("non_MEG"));

            if (m_pFiffInfo->chs.at(index.row()).kind == FIFFV_MEG_CH) {
                qint32 unit = m_pFiffInfo->chs.at(index.row()).unit;
                if (unit == FIFF_UNIT_T_M)
                    v.setValue(QString("MEG_grad"));
                else if (unit == FIFF_UNIT_T)
                    v.setValue(QString("MEG_mag"));
            }

            switch (role) {
                case Qt::DisplayRole:
                    return v;

                case ChannelInfoModelRoles::GetMEGType:
                    return v;

                case Qt::TextAlignmentRole:
                    Qt::Alignment a = Qt::AlignHCenter | Qt::AlignVCenter;
                    return QVariant(a);
            }
        } //end column check

        //******** seventh column (channel unit) ********
        if (index.column() == 6) {
            QVariant v;

            switch (role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_pFiffInfo->chs.at(index.row()).unit));
                    return v;

                case ChannelInfoModelRoles::GetChUnit:
                    v.setValue(m_pFiffInfo->chs.at(index.row()).unit);
                    return v;

                case Qt::TextAlignmentRole:
                    Qt::Alignment a = Qt::AlignHCenter | Qt::AlignVCenter;
                    return QVariant(a);
            }
        } //end column check

        //******** eigth column (channel layout position) ********
        if (index.column() == 7) {
            QVariant v;

            QPointF point = detail::valueOr(m_layoutMap, m_mappedLayoutChNames.at(index.row()));

            switch (role) {
                case Qt::DisplayRole:
                    v.setValue(QString("(%1|%2)").arg(point.x()).arg(point.y()));
                    return v;

                case ChannelInfoModelRoles::GetChPosition:
                    v.setValue(point);
                    return v;

                case Qt::TextAlignmentRole:
                    Qt::Alignment a = Qt::AlignHCenter | Qt::AlignVCenter;
                    return QVariant(a);
            }
        } //end column check

        //******** ninth column (channel digitizer position) ********
        if (index.column() == 8) {
            QVariant v;

            QVector3D point3D(m_pFiffInfo->chs.at(index.row()).chpos.r0[0] * 100, //convert to cm
                              m_pFiffInfo->chs.at(index.row()).chpos.r0[1] * 100,
                              m_pFiffInfo->chs.at(index.row()).chpos.r0[2] * 100);

            switch (role) {
                case Qt::DisplayRole:
                    v.setValue(QString("(%1|%2|%3)").arg(point3D.x()).arg(point3D.y()).arg(point3D.z()));
                    return v;

                case ChannelInfoModelRoles::GetChDigitizer:
                    v.setValue(point3D);
                    return v;

                case Qt::TextAlignmentRole:
                    Qt::Alignment a = Qt::AlignHCenter | Qt::AlignVCenter;
                    return QVariant(a);
            }
        } //end column check

        //******** tenth column (active channel filter type) ********
        if (index.column() == 9) {
            QVariant v;

            //            //Check if mne operator is a filter operator
            //            std::shared_ptr<MNEOperator> operatorPtr = m_assignedOperators.value(index.row(), std::make_shared<MNEOperator>());
            //            std::shared_ptr<FilterOperator> filterOperator;

            switch (role) {
                case Qt::DisplayRole: {
                    return v;
                }
                    //                    if(operatorPtr->m_OperatorType == MNEOperator::FILTER) {
                    //                        filterOperator = operatorPtr.staticCast<FilterOperator>();
                    //                    }
                    //                    else {
                    //                        v.setValue(QString("%1").arg("none"));
                    //                        return v;
                    //                    }

                    //                    switch(filterOperator->m_Type) {
                    //                        case FilterOperator::LPF: {
                    //                            v.setValue(QString("%1 | %2").arg("LP").arg(filterOperator->m_dCenterFreq*m_pFiffInfo->sfreq/2));
                    //                            return v;
                    //                        }

                    //                        case FilterOperator::HPF: {
                    //                            v.setValue(QString("%1 | %2").arg("HP").arg(filterOperator->m_dCenterFreq*m_pFiffInfo->sfreq/2));
                    //                            return v;
                    //                        }

                    //                        case FilterOperator::BPF: {
                    //                            double fsample = m_pFiffInfo->sfreq;
                    //                            double low = (filterOperator->m_dCenterFreq*fsample/2) - (filterOperator->m_dBandwidth*fsample/4); // /4 because we also need to devide by 2 to get the nyquist freq
                    //                            double high = (filterOperator->m_dCenterFreq*fsample/2) + (filterOperator->m_dBandwidth*fsample/4);
                    //                            v.setValue(QString("%1 | %2 | %3").arg("BP").arg(low).arg(high));
                    //                            return v;
                    //                        }

                    //                        case FilterOperator::NOTCH: {
                    //                            double fsample = m_pFiffInfo->sfreq;
                    //                            double low = (filterOperator->m_dCenterFreq*fsample/2) - (filterOperator->m_dBandwidth*fsample/4);
                    //                            double high = (filterOperator->m_dCenterFreq*fsample/2) + (filterOperator->m_dBandwidth*fsample/4);
                    //                            v.setValue(QString("%1 | %2 | %3").arg("NOTCH").arg(low).arg(high));
                    //                            return v;
                    //                        }
                    //                    }
                    //                }

                    //                case ChannelInfoModelRoles::GetChActiveFilter: {
                    //                    if(operatorPtr->m_OperatorType == MNEOperator::FILTER) {
                    //                        filterOperator = operatorPtr.staticCast<FilterOperator>();
                    //                    }
                    //                    else {
                    //                        v.setValue(QString("%1").arg("none"));
                    //                        return v;
                    //                    }

                    //                    v.setValue(operatorPtr);
                    //                    return v;
                    //                }

                case Qt::TextAlignmentRole:
                    Qt::Alignment a = Qt::AlignHCenter | Qt::AlignVCenter;
                    return QVariant(a);
            }
        }

        //******** eleventh column (coil type) ********
        if (index.column() == 10) {
            QVariant v;

            switch (role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_pFiffInfo->chs.at(index.row()).chpos.coil_type));
                    return v;

                case ChannelInfoModelRoles::GetChCoilType:
                    v.setValue(m_pFiffInfo->chs.at(index.row()).chpos.coil_type);
                    return v;

                case Qt::TextAlignmentRole:
                    Qt::Alignment a = Qt::AlignHCenter | Qt::AlignVCenter;
                    return QVariant(a);
            }
        } //end column check

        //******** twelve column (channel bad) ********
        if (index.column() == 11) {
            QVariant v;
            const bool isBad = m_pFiffInfo->isBad(m_pFiffInfo->chs.at(index.row()).ch_name);

            switch (role) {
                case Qt::DisplayRole:
                    v.setValue(isBad);
                    return v;

                case ChannelInfoModelRoles::GetIsBad:
                    v.setValue(isBad);
                    return v;

                case Qt::TextAlignmentRole:
                    Qt::Alignment a = Qt::AlignHCenter | Qt::AlignVCenter;
                    return QVariant(a);
            }
        } //end column check

        //******** twelve column (compensators bad) ********
        if (index.column() == 12) {
            QVariant v;

            switch (role) {
                case Qt::DisplayRole:
                    v.setValue(m_pFiffInfo->comps.size());
                    return v;

                case Qt::TextAlignmentRole:
                    Qt::Alignment a = Qt::AlignHCenter | Qt::AlignVCenter;
                    return QVariant(a);
            }
        } //end column check
    } // end index.valid() check

    return QVariant();
}

//=============================================================================================================

bool ChannelInfoModel::insertRows(int position, int span, const QModelIndex& parent)
{
    Q_UNUSED(position);
    Q_UNUSED(span);
    Q_UNUSED(parent);

    return true;
}

//=============================================================================================================

bool ChannelInfoModel::removeRows(int position, int span, const QModelIndex& parent)
{
    Q_UNUSED(position);
    Q_UNUSED(span);
    Q_UNUSED(parent);

    return true;
}

//=============================================================================================================

Qt::ItemFlags ChannelInfoModel::flags(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable /*| Qt::ItemIsEditable*/;
}

//=============================================================================================================

bool ChannelInfoModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);

    return true;
}

//=============================================================================================================

void ChannelInfoModel::setFiffInfo(std::shared_ptr<FiffInfo>& pFiffInfo)
{
    beginResetModel();

    m_pFiffInfo = pFiffInfo;
    m_aliasNames = m_pFiffInfo->ch_names;
    m_mappedLayoutChNames = m_aliasNames;

    mapLayoutToChannels();

    endResetModel();

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

    m_layoutMap = layoutMap;
    m_aliasNames = m_pFiffInfo->ch_names;
    m_mappedLayoutChNames = m_aliasNames;

    mapLayoutToChannels();

    endResetModel();

    emit dataChanged(createIndex(0, 0), createIndex(rowCount(), columnCount()));
}

//=============================================================================================================

const std::vector<std::string>& ChannelInfoModel::getMappedChannelsList()
{
    return m_mappedLayoutChNames;
}

//=============================================================================================================

int ChannelInfoModel::getIndexFromOrigChName(const std::string& chName)
{
    return mne::channel_index(m_pFiffInfo->ch_names, chName);
}

//=============================================================================================================

int ChannelInfoModel::getIndexFromMappedChName(const std::string& chName)
{
    return mne::index_of(m_mappedLayoutChNames, chName);
}

//=============================================================================================================

StringList ChannelInfoModel::getBadChannelList()
{
    return m_pFiffInfo->bads;
}

//=============================================================================================================

namespace
{

/** Removes every @p kind and the characters - _ / | and whitespace from @p name in one left-to-right pass. */
std::string layoutNumber(std::string_view name, std::string_view kind)
{
    std::string residual;
    for (std::size_t i = 0; i < name.size();) {
        if (name.substr(i, kind.size()) == kind) {
            i += kind.size();
        } else if (std::string_view("-_/|").find(name[i]) != std::string_view::npos || std::isspace(static_cast<unsigned char>(name[i]))) {
            ++i;
        } else {
            residual += name[i++];
        }
    }
    return residual;
}

} // namespace

//=============================================================================================================

void ChannelInfoModel::mapLayoutToChannels()
{
    //Map channels to layout
    const std::vector<FiffChInfo>& channelList = m_pFiffInfo->chs;
    for (int i = 0; i < std::ssize(channelList); i++) {
        //Get current channel information
        const FiffChInfo& chInfo = channelList[i];
        switch (chInfo.kind) {
            case FIFFV_MEG_CH:
                //Remove the MEG prefix and separators, keep the residual channel number
                m_mappedLayoutChNames[i] = "MEG " + layoutNumber(chInfo.ch_name, "MEG");
                break;

            case FIFFV_EEG_CH:
                //Remove the EEG prefix and separators, keep the residual channel number
                m_mappedLayoutChNames[i] = "EEG " + layoutNumber(chInfo.ch_name, "EEG");
                break;
        }
    } //end fiff chs

    emit channelsMappedToLayout(m_mappedLayoutChNames);
}

//=============================================================================================================

void ChannelInfoModel::clearModel()
{
    beginResetModel();

    m_pFiffInfo = std::make_shared<FiffInfo>();
    m_layoutMap.clear();
    m_aliasNames.clear();
    m_mappedLayoutChNames.clear();

    endResetModel();

    mne::Log::debug("ChannelInfoModel cleared.");
}
