//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     channelinfomodel.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Jens Haueisen <jens.haueisen@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November 2014
 * @brief    QAbstractTableModel exposing per-channel FIFF metadata (name, type, unit, position, bad flag, mapping).
 *
 * ChannelInfoModel turns a @c FiffInfo into a sortable / filterable
 * table model. Each row maps a FIFF channel to its presentation
 * attributes — display name, modality, unit, scaling, layout-mapped
 * alias and bad flag — and listens for live edits coming from the
 * channel-selection / mapping dialogs to update the underlying
 * @c FiffInfo in place.
 */

#ifndef CHANNELINFOMODEL_H
#define CHANNELINFOMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../viz_qt_global.h"
#include <mne/viz/figures/geometry.h>
#include <mne/viz/models/channel_table.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>
#include <QPointF>
#include <QMultiMap>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

//=============================================================================================================
// mne::viz::qt FORWARD DECLARATIONS
//=============================================================================================================

class MNEOperator;

namespace ChannelInfoModelRoles
{
enum ItemRole
{
    GetOrigChName = Qt::UserRole + 1009,
    GetMappedLayoutChName = Qt::UserRole + 1010,
    GetChNumber = Qt::UserRole + 1011,
    GetChKind = Qt::UserRole + 1012,
    GetMEGType = Qt::UserRole + 1013,
    GetChUnit = Qt::UserRole + 1014,
    GetChAlias = Qt::UserRole + 1015,
    GetChPosition = Qt::UserRole + 1016,
    GetChDigitizer = Qt::UserRole + 1017,
    GetChActiveFilter = Qt::UserRole + 1018,
    GetChCoilType = Qt::UserRole + 1019,
    GetIsBad = Qt::UserRole + 1020
};
}

//=============================================================================================================
/**
 * @brief QAbstractTableModel exposing per-channel FIFF metadata (name, type, unit, position, bad flag, mapping).
 *
 * Each row maps a FIFF channel to its presentation attributes and
 * listens for live edits from the channel-selection / mapping
 * dialogs so the underlying @c FiffInfo is updated in place.
 */
class VIZQTSHARED_EXPORT ChannelInfoModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ChannelInfoModel(std::shared_ptr<mne::FiffInfo>& pFiffInfo, QObject* parent = 0);
    ChannelInfoModel(QObject* parent = 0);

    //=========================================================================================================
    /**
     * Reimplemented virtual functions
     *
     * @param[in] parent  Parent index (unused; the model is a flat list).
     *
     * @return Number of channels in the attached measurement info.
     */
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;
    virtual bool insertRows(int position, int span, const QModelIndex& parent = QModelIndex());
    virtual bool removeRows(int position, int span, const QModelIndex& parent = QModelIndex());

    //=========================================================================================================
    /**
     * Updates the fiff info
     *
     * @param[in] pFiffInfo fiff info variabel.
     */
    void setFiffInfo(std::shared_ptr<mne::FiffInfo>& pFiffInfo);

    //=========================================================================================================
    /**
     * Updates the fiff info
     *
     * @param[in] assignedOperators the filter operators which are currently active.
     */
    void assignedOperatorsChanged(const QMultiMap<int, std::shared_ptr<MNEOperator>>& assignedOperators);

    //=========================================================================================================
    /**
     * Updates the layout map
     *
     * @param[in] layoutMap the layout map with the 2D positions.
     */
    void layoutChanged(const std::map<std::string, QPointF>& layoutMap);

    //=========================================================================================================
    /**
     * Updates the layout map from toolkit-free layout positions, e.g. those viz::ChannelSelectionPanel loaded.
     *
     * @param[in] layout   Layout position of each channel by layout name.
     *
     * @since 3.0.0
     */
    void setLayout(const std::map<std::string, viz::Point>& layout);

    //=========================================================================================================
    /**
     * @return The toolkit-free channel rows the model adapts.
     *
     * @since 3.0.0
     */
    const viz::ChannelTable& table() const
    {
        return m_table;
    }

    //=========================================================================================================
    /**
     * @param[in] model   A model.
     *
     * @return Its channel rows, keeping the model alive, e.g. for viz::ChannelSelectionPanel::set_channel_table().
     *
     * @since 3.0.0
     */
    static std::shared_ptr<const viz::ChannelTable> sharedTable(const std::shared_ptr<ChannelInfoModel>& model)
    {
        return model ? std::shared_ptr<const viz::ChannelTable>(model, &model->m_table) : nullptr;
    }

    //=========================================================================================================
    /**
     * Updates the layout map
     *
     * @return the current mapped channel list.
     */
    const std::vector<std::string>& getMappedChannelsList();

    //=========================================================================================================
    /**
     * Returns the model index for the given input channel fro mthe original channel list
     *
     * @param[in] chName the channel name for which the model index is needed.
     * @return the index number. if channel was not found in the data this functions returns -1.
     */
    int getIndexFromOrigChName(const std::string& chName);

    //=========================================================================================================
    /**
     * Returns the model index for the given input channel fro mthe mapped channel list
     *
     * @param[in] chName the channel name for which the model index is needed.
     * @return the index number. if channel was not found in the data this functions returns -1.
     */
    int getIndexFromMappedChName(const std::string& chName);

    //=========================================================================================================
    /**
     * Returns bad channel list.
     *
     * @return the bad channel list.
     */
    StringList getBadChannelList();

    //=========================================================================================================
    /**
     * clearModel clears all model's members
     */
    void clearModel();

protected:
    std::shared_ptr<mne::FiffInfo> m_pFiffInfo; /**< The fiff info of the currently loaded fiff file. */

    QMultiMap<int, std::shared_ptr<MNEOperator>> m_assignedOperators; /**< Map of MNEOperator types to channels.*/

    mne::viz::ChannelTable m_table; /**< Channel rows, layout mapping and cell contents. */

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever channels where mapped to a layout
     *
     */
    void channelsMappedToLayout(const std::vector<std::string>& mappedLayoutChNames);
};
} // NAMESPACE mne_disp

#endif // CHANNELINFOMODEL_H
