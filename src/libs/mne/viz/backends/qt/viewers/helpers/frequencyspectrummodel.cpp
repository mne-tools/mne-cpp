//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     frequencyspectrummodel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>
 * @since    0.1.0
 * @date     May 2014
 * @brief    Implementation of the FrequencySpectrumModel FFT-amplitude table model.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "frequencyspectrummodel.h"

#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <optional>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FrequencySpectrumModel::FrequencySpectrumModel(QObject* parent)
: QAbstractTableModel(parent)
{
}

//=============================================================================================================

int FrequencySpectrumModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_table.row_count();
}

//=============================================================================================================

int FrequencySpectrumModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 2;
}

//=============================================================================================================

QVariant FrequencySpectrumModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DisplayRole || !index.isValid()) {
        return QVariant();
    }

    //******** first column (chname) ********
    if (index.column() == 0) {
        return m_table.info() ? QVariant(QString::fromStdString(m_table.channel_name(index.row()))) : QVariant();
    }

    //******** second column (data plot) ********
    if (index.column() == 1) {
        const std::optional<RowVectorXd> spectrum = m_table.spectrum(index.row());
        if (!spectrum) {
            return QVariant();
        }
        QVariant v;
        v.setValue(*spectrum);
        return v;
    }

    return QVariant();
}

//=============================================================================================================

QVariant FrequencySpectrumModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case 0: //chname column
                return QVariant();
            case 1: //data plot column
                switch (role) {
                    case Qt::DisplayRole:
                        return QVariant("data plot");
                    case Qt::TextAlignmentRole:
                        return QVariant(Qt::AlignLeft);
                }
                return QVariant("data plot");
        }
    } else if (orientation == Qt::Vertical) {
        QModelIndex chname = createIndex(section, 0);
        switch (role) {
            case Qt::DisplayRole:
                return QVariant(data(chname).toString());
        }
    }

    return QVariant();
}

//=============================================================================================================

void FrequencySpectrumModel::setInfo(std::shared_ptr<FiffInfo>& info)
{
    beginResetModel();
    m_table.set_info(info);
    endResetModel();
}

//=============================================================================================================

void FrequencySpectrumModel::setScaleType(qint8 ScaleType)
{
    m_table.set_scale(ScaleType ? viz::SpectrumTable::Scale::Log : viz::SpectrumTable::Scale::Linear);
}

//=============================================================================================================

void FrequencySpectrumModel::addData(const MatrixXd& data)
{
    m_table.set_data(data);

    //Update data content
    emit dataChanged(index(0, 1), index(m_table.data_rows() - 1, 1), {Qt::DisplayRole});
}

//=============================================================================================================

void FrequencySpectrumModel::selectRows(const std::vector<int>& selection)
{
    beginResetModel();
    m_table.select(selection);
    emit newSelection(selection);
    endResetModel();
}

//=============================================================================================================

void FrequencySpectrumModel::resetSelection()
{
    beginResetModel();
    m_table.reset_selection();
    endResetModel();
}

//=============================================================================================================

void FrequencySpectrumModel::toggleFreeze(const QModelIndex& /*index*/)
{
    m_table.toggle_frozen();

    //Update data content
    emit dataChanged(index(0, 1), index(m_table.data_rows() - 1, 1), {Qt::DisplayRole});
}

//=============================================================================================================

void FrequencySpectrumModel::setBoundaries(float fLowerFrqBound, float fUpperFrqBound)
{
    if (m_table.freq_scale().size() == 0) {
        return;
    }

    beginResetModel();
    m_table.set_bounds(fLowerFrqBound, fUpperFrqBound);
    endResetModel();
}
