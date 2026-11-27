//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     evokedsetmodel.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August 2016
 * @brief    Implementation of the EvokedSetModel FiffEvokedSet table model.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "evokedsetmodel.h"
#include "../../detail/map_value.h"

#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_evoked_set.h>
#include <mne/viz/models/evoked_conditions.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


#include <QColor>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>
#include <utility>
#include <vector>
#include <iostream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;
using namespace mne::viz::qt;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EvokedSetModel::EvokedSetModel(QObject* parent)
: QAbstractTableModel(parent)
, m_qMapAverageColor(std::make_shared<std::map<std::string, QColor>>())
, m_qMapAverageActivation(std::make_shared<std::map<std::string, bool>>())
, m_qMapAverageColorOld(std::make_shared<std::map<std::string, QColor>>())
, m_qMapAverageActivationOld(std::make_shared<std::map<std::string, bool>>())
, m_bIsInit(false)
, m_bIsFreezed(false)
, m_fSps(1024.0f)
{
}

//=============================================================================================================

EvokedSetModel::~EvokedSetModel()
{
}

//=============================================================================================================
//virtual functions
int EvokedSetModel::rowCount(const QModelIndex& /*parent*/) const
{
    if (m_pEvokedSet) {
        return m_pEvokedSet->info.nchan;
    }

    return 0;
}

//=============================================================================================================

int EvokedSetModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 3;
}

//=============================================================================================================

QVariant EvokedSetModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::BackgroundRole && role != EvokedSetModelRoles::GetAverageData) {
        return QVariant();
    }

    if (index.isValid()) {
        qint32 row = detail::valueOr(m_qMapIdxRowSelection, index.row());

        //******** first column (chname) ********
        if (index.column() == 0 && role == Qt::DisplayRole) {
            QStringList names;
            for (const std::string& name : m_pEvokedSet->info.ch_names)
                names << QString::fromStdString(name);
            return QVariant(names);
        }

        //******** second column (butterfly data plot) ********
        //TODO: Bring this into a better structure see colum three
        if (index.column() == 1) {
            QVariant v;

            std::vector<mne::viz::qt::AvrTypeRowVector> lRowDataPerTrigType;
            mne::viz::qt::AvrTypeRowVector pairItem;

            switch (role) {
                case Qt::DisplayRole: {
                    //pack all adjacent (after reload) RowVectorPairs into a QList

                    if (m_bIsFreezed) {
                        // data freeze
                        for (int i = 0; i < std::ssize(m_matDataFreeze); ++i) {
                            pairItem.first = m_lAvrTypesFreeze.at(i);
                            pairItem.second = m_matDataFreeze.at(i).row(row);
                            lRowDataPerTrigType.push_back(pairItem);
                        }
                    } else {
                        // data stream
                        for (int i = 0; i < std::ssize(m_matData); ++i) {
                            pairItem.first = m_lAvrTypes.at(i);
                            pairItem.second = m_matData.at(i).row(row);
                            lRowDataPerTrigType.push_back(pairItem);
                        }
                    }

                    v.setValue(lRowDataPerTrigType);

                    return v;
                }
                case Qt::BackgroundRole: {
                    //                    if(m_fiffInfo.bads.contains(m_chInfolist[row].ch_name)) {
                    //                        QBrush brush;
                    //                        brush.setStyle(Qt::SolidPattern);
                    //    //                    qDebug() << m_chInfolist[row].ch_name << "is marked as bad, index:" << row;
                    //                        brush.setColor(Qt::red);
                    //                        return QVariant(brush);
                    //                    }
                    //                    else
                    return QVariant();
                }
            } // end role switch
        } // end column check

        //******** third column (2D layout data plot, this third column is needed because the te data needs to be in another structure for the 2D layout to work) ********
        if (index.column() == 2) {
            QVariant v;
            std::vector<mne::viz::qt::AvrTypeRowVectorPair> lRowDataPerTrigType;
            mne::viz::qt::AvrTypeRowVectorPair averagedData;

            switch (role) {
                case EvokedSetModelRoles::GetAverageData: {
                    if (m_bIsFreezed) {
                        // data freeze
                        for (int i = 0; i < std::ssize(m_matDataFreeze); ++i) {
                            averagedData.first = m_lAvrTypesFreeze.at(i);
                            averagedData.second.first = m_matDataFreeze.at(i).data();
                            averagedData.second.second = static_cast<int>(m_matDataFreeze.at(i).cols());

                            lRowDataPerTrigType.push_back(averagedData);
                        }
                    } else {
                        for (int i = 0; i < std::ssize(m_matData); ++i) {
                            averagedData.first = m_lAvrTypes.at(i);
                            averagedData.second.first = m_matData.at(i).data();
                            averagedData.second.second = static_cast<int>(m_matData.at(i).cols());

                            lRowDataPerTrigType.push_back(averagedData);
                        }
                    }

                    v.setValue(lRowDataPerTrigType);
                }
            }

            return v;
        } //end column check

    } // end index.valid() check

    return QVariant();
}

//=============================================================================================================

QVariant EvokedSetModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::TextAlignmentRole) {
        return QVariant();
    }

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

void EvokedSetModel::setEvokedSet(std::shared_ptr<FiffEvokedSet> pEvokedSet)
{
    m_pEvokedSet = std::move(pEvokedSet);

    if (!m_bIsInit) {
        init();
    }

    updateData();
}

//=============================================================================================================

void EvokedSetModel::init()
{
    if (!m_pEvokedSet) {
        return;
    }

    beginResetModel();

    m_fSps = m_pEvokedSet->info.sfreq;

    m_correction.reset(static_cast<int>(m_pEvokedSet->info.chs.size()));

    m_qMapAverageActivation->clear();
    m_qMapAverageColor->clear();
    m_qMapAverageActivationOld->clear();
    m_qMapAverageColorOld->clear();

    //Create the initial SSP projector
    updateProjection(m_pEvokedSet->info.projs);

    //Create the initial Compensator projector
    updateCompensator(0);

    endResetModel();

    resetSelection();

    m_bIsInit = true;
}

//=============================================================================================================

void EvokedSetModel::updateData()
{
    if (!m_pEvokedSet) {
        return;
    }

    m_matData.clear();
    m_lAvrTypes.clear();

    for (const FiffEvoked& evoked : m_pEvokedSet->evoked) {
        m_matData.push_back(m_correction.apply(evoked.data));
        m_pairBaseline = evoked.baseline;
        m_lAvrTypes.push_back(evoked.comment);
    }

    // Update average selection information map. Use old colors if existing.
    std::vector<std::string> slCurrentAvrComments;
    for (const FiffEvoked& evoked : m_pEvokedSet->evoked) {
        slCurrentAvrComments.push_back(evoked.comment);
    }

    const bool bActivationChanged = viz::reconcile_conditions(*m_qMapAverageActivation, *m_qMapAverageActivationOld, slCurrentAvrComments, true);
    const bool bColorChanged = viz::reconcile_conditions(*m_qMapAverageColor, *m_qMapAverageColorOld, slCurrentAvrComments, QColor(Qt::yellow));

    // Only emit new colors and activations if evoked types were added or deleted
    if (bColorChanged) {
        emit newAverageColorMap(m_qMapAverageColor);
    }

    if (bActivationChanged) {
        emit newAverageActivationMap(m_qMapAverageActivation);
    }

    //Update data content
    QModelIndex topLeft = this->index(0, 1);
    QModelIndex bottomRight = this->index(m_pEvokedSet->info.nchan - 1, 1);
    emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole});
}

//=============================================================================================================

std::shared_ptr<std::map<std::string, QColor>> EvokedSetModel::getAverageColor() const
{
    return m_qMapAverageColor;
}

//=============================================================================================================

std::shared_ptr<std::map<std::string, bool>> EvokedSetModel::getAverageActivation() const
{
    return m_qMapAverageActivation;
}

//=============================================================================================================

void EvokedSetModel::setAverageColor(const std::shared_ptr<std::map<std::string, QColor>>& qMapAverageColor)
{
    m_qMapAverageColor = qMapAverageColor;
}

//=============================================================================================================

void EvokedSetModel::setAverageActivation(const std::shared_ptr<std::map<std::string, bool>>& qMapAverageActivation)
{
    m_qMapAverageActivation = qMapAverageActivation;
}

//=============================================================================================================

fiff_int_t EvokedSetModel::getKind(qint32 row) const
{
    if (row < std::ssize(m_qMapIdxRowSelection)) {
        qint32 chRow = detail::valueOr(m_qMapIdxRowSelection, row);
        return m_pEvokedSet->info.chs[chRow].kind;
    }

    return 0;
}

//=============================================================================================================

bool EvokedSetModel::getIsChannelBad(qint32 row) const
{
    bool bIsBad = false;

    if (row < std::ssize(m_qMapIdxRowSelection)) {
        qint32 chRow = detail::valueOr(m_qMapIdxRowSelection, row);
        bIsBad = std::ranges::contains(m_pEvokedSet->info.bads, m_pEvokedSet->info.chs[chRow].ch_name);
    }

    return bIsBad;
}

//=============================================================================================================

fiff_int_t EvokedSetModel::getUnit(qint32 row) const
{
    if (row < std::ssize(m_qMapIdxRowSelection)) {
        qint32 chRow = detail::valueOr(m_qMapIdxRowSelection, row);
        return m_pEvokedSet->info.chs[chRow].unit;
    }

    return FIFF_UNIT_NONE;
}

//=============================================================================================================

fiff_int_t EvokedSetModel::getCoil(qint32 row) const
{
    if (row < std::ssize(m_qMapIdxRowSelection)) {
        qint32 chRow = detail::valueOr(m_qMapIdxRowSelection, row);
        return m_pEvokedSet->info.chs[chRow].chpos.coil_type;
    }

    return FIFFV_COIL_NONE;
}

//=============================================================================================================

bool EvokedSetModel::isInit() const
{
    return m_bIsInit;
}

//=============================================================================================================

qint32 EvokedSetModel::getNumSamples() const
{
    qint32 iNumSamples = 0;

    if (!m_matData.empty()) {
        iNumSamples = m_matData.front().cols();
    }

    return m_bIsInit ? iNumSamples : 0;
}

//=============================================================================================================

QVariant EvokedSetModel::data(int row, int column, int role) const
{
    return data(index(row, column), role);
}

//=============================================================================================================

const std::map<int, int>& EvokedSetModel::getIdxSelMap() const
{
    return m_qMapIdxRowSelection;
}

//=============================================================================================================

qint32 EvokedSetModel::numVLines() const
{
    qint32 iNumSamples = 0;

    if (!m_matData.empty()) {
        iNumSamples = m_matData.front().cols();
    }

    return static_cast<qint32>(iNumSamples / m_fSps) - 1;
}

//=============================================================================================================

qint32 EvokedSetModel::getNumPreStimSamples() const
{
    int iPreSamples = 0;

    if (!m_pEvokedSet) {
        return iPreSamples;
    }

    if (!m_pEvokedSet->evoked.empty()) {
        const RowVectorXf& times = m_pEvokedSet->evoked.front().times;
        iPreSamples = viz::pre_stimulus_samples(std::span<const float>(times.data(), static_cast<std::size_t>(times.size())));
    }

    return iPreSamples;
}

//=============================================================================================================

float EvokedSetModel::getSamplingFrequency() const
{
    return m_fSps;
}

//=============================================================================================================

bool EvokedSetModel::isFreezed() const
{
    return m_bIsFreezed;
}

//=============================================================================================================

int EvokedSetModel::getNumberOfTimeSpacers() const
{
    //std::cout<<floor((m_matData.cols()/m_fSps)*10)<<std::endl;
    qint32 iNumSamples = 0;

    if (!m_matData.empty()) {
        iNumSamples = m_matData.front().cols();
    }

    return std::floor(static_cast<double>((iNumSamples / m_fSps) * 10));
}

//=============================================================================================================

std::pair<float, float> EvokedSetModel::getBaselineInfo() const
{
    //qDebug()<<floor((m_matData.cols()/m_fSps)*10);
    return m_pairBaseline;
}

//=============================================================================================================

int EvokedSetModel::getNumAverages() const
{
    return static_cast<int>(m_matData.size());
}

//=============================================================================================================

void EvokedSetModel::selectRows(const std::vector<int>& selection)
{
    if (!m_pEvokedSet) {
        return;
    }

    beginResetModel();

    m_qMapIdxRowSelection.clear();

    qint32 count = 0;
    for (qint32 i = 0; i < std::ssize(selection); ++i) {
        if (selection[i] < m_pEvokedSet->info.nchan) {
            m_qMapIdxRowSelection.insert_or_assign(count, selection[i]);
            ++count;
        }
    }

    emit newSelection(selection);

    endResetModel();
}

//=============================================================================================================

void EvokedSetModel::resetSelection()
{
    if (!m_pEvokedSet) {
        return;
    }

    beginResetModel();

    m_qMapIdxRowSelection.clear();

    for (qint32 i = 0; i < m_pEvokedSet->info.nchan; ++i) {
        m_qMapIdxRowSelection.insert_or_assign(i, i);
    }

    endResetModel();
}

//=============================================================================================================

void EvokedSetModel::updateProjection(const std::vector<FiffProj>& projs)
{
    if (!m_pEvokedSet || m_pEvokedSet->info.chs.empty()) {
        return;
    }

    m_pEvokedSet->info.projs = projs;
    m_correction.update_projection(m_pEvokedSet->info);
}

//=============================================================================================================

void EvokedSetModel::updateCompensator(int to)
{
    if (!m_pEvokedSet || m_pEvokedSet->info.chs.empty()) {
        return;
    }

    m_correction.update_compensator(m_pEvokedSet->info, to);
}

//=============================================================================================================

void EvokedSetModel::toggleFreeze()
{
    m_bIsFreezed = !m_bIsFreezed;

    if (m_bIsFreezed) {
        m_matDataFreeze = m_matData;
        m_lAvrTypesFreeze = m_lAvrTypes;
    }

    //Update data content
    QModelIndex topLeft = this->index(0, 1);
    QModelIndex bottomRight = this->index(this->rowCount(), 1);
    emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole});
}

//=============================================================================================================

std::shared_ptr<mne::FiffEvokedSet> EvokedSetModel::getEvokedSet()
{
    if (!m_pEvokedSet) {
        return nullptr;
    } else {
        return m_pEvokedSet;
    }
}
