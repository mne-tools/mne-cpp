//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     rtfiffrawviewmodel.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Robert Dicamillo <rd521@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     May 2014
 * @brief    Implementation of the RtFiffRawViewModel ring-buffer table model.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtfiffrawviewmodel.h"
#include "../../detail/map_value.h"
#include <mne/base/log.h>

#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_types.h>
#include <mne/fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QBrush>
#include <QCoreApplication>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

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

RtFiffRawViewModel::RtFiffRawViewModel(QObject* parent)
: QAbstractTableModel(parent)
, m_fSps(1024.0f)
, m_iT(10)
, m_iDownsampling(10)
, m_iDistanceTimerSpacer(1000)
, m_iDetectedTriggers(0)
, m_pFiffInfo(std::make_shared<FiffInfo>())
, m_colBackground(Qt::white)
{
}

//=============================================================================================================

RtFiffRawViewModel::~RtFiffRawViewModel()
{
}

//=============================================================================================================
//virtual functions
int RtFiffRawViewModel::rowCount(const QModelIndex& /*parent*/) const
{
    if (!m_pFiffInfo->chs.empty()) {
        return static_cast<int>(m_pFiffInfo->chs.size());
    } else {
        return 0;
    }
}

//=============================================================================================================

int RtFiffRawViewModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 3;
}

//=============================================================================================================

QVariant RtFiffRawViewModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::BackgroundRole) {
        return QVariant();
    }

    if (role == Qt::BackgroundRole) {
        return QVariant(QBrush(m_colBackground));
    }

    if (index.isValid()) {
        qint32 row = detail::valueOr(m_qMapIdxRowSelection, index.row(), 0);

        //******** first column (chname) ********
        if (index.column() == 0 && role == Qt::DisplayRole)
            return QString::fromStdString(m_pFiffInfo->ch_names[row]);

        //******** second column (data plot) ********
        if (index.column() == 1) {
            QVariant v;
            RowVectorPair rowVectorPair;

            switch (role) {
                case Qt::DisplayRole: {
                    // The frozen copy while frozen
                    const viz::SweepBuffer::Matrix& matData = m_sweep.layer(m_display.filtering());
                    rowVectorPair.first = matData.data() + row * matData.cols();
                    rowVectorPair.second = static_cast<int>(matData.cols());
                    v.setValue(rowVectorPair);

                    return v;
                }
            } // end role switch
        } // end column check

        //******** third column (bad channel) ********
        if (index.column() == 2 && role == Qt::DisplayRole) {
            return QVariant(std::ranges::contains(m_pFiffInfo->bads, m_pFiffInfo->ch_names[row]));
        } // end column check

    } // end index.valid() check

    return QVariant();
}

//=============================================================================================================

QVariant RtFiffRawViewModel::headerData(int section, Qt::Orientation orientation, int role) const
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

void RtFiffRawViewModel::initSphara()
{
    m_display.load_sphara_bases((QCoreApplication::applicationDirPath() + QString("/../resources/mne_scan/plugins/noisereduction/SPHARA")).toStdString());

    //Create Sphara operator for the first time
    updateSpharaOptions("BabyMEG", 270, 105);
}

//=============================================================================================================

void RtFiffRawViewModel::setFiffInfo(std::shared_ptr<mne::FiffInfo>& p_pFiffInfo)
{
    if (p_pFiffInfo) {
        m_pFiffInfo = p_pFiffInfo;

        resetSelection();

        //Resize the sweep to the channels and clear it
        m_sweep.resize(static_cast<int>(m_pFiffInfo->chs.size()), m_sweep.window(), true);

        //Reset SSP, compensator, filter state and SPHARA channels
        m_display.set_info(m_pFiffInfo);

        initSphara();
    } else {
        m_display.set_info(nullptr);
    }
}

//=============================================================================================================

void RtFiffRawViewModel::setSamplingInfo(float sps, int T, bool bSetZero)
{
    beginResetModel();

    m_iT = T;

    //Resize the sweep without touching the stored values
    m_sweep.resize(static_cast<int>(m_pFiffInfo->chs.size()), static_cast<int>(ceil(static_cast<double>(sps) * T)), bSetZero);

    endResetModel();
}

//=============================================================================================================

MatrixXd RtFiffRawViewModel::getLastBlock()
{
    return m_sweep.last_block(m_display.filtering());
}

//=============================================================================================================

void RtFiffRawViewModel::addData(std::span<const MatrixXd> data)
{
    MatrixXdR& matDataRaw = m_sweep.raw();
    MatrixXdR& matDataFiltered = m_sweep.filtered();
    const int iMaxCols = static_cast<int>(matDataRaw.cols());
    if (iMaxCols == 0) {
        return;
    }

    const bool doSphara = m_display.applies_sphara(matDataRaw.rows());
    const bool doFilter = m_display.filtering();
    const int iTriggerCh = m_display.trigger_channel();

    //Copy new data into the ring buffer: stream sample s goes to column s mod iMaxCols
    for (const MatrixXd& matBlock : data) {
        if (matBlock.rows() != matDataRaw.rows()) {
            mne::Log::debug("incoming data does not match internal data row size. Returning...");
            return;
        }

        const int nCol = static_cast<int>(matBlock.cols());
        const mne::RtRawDisplay::Corrected corrected = m_display.correct(matBlock);

        m_sweep.write(nCol, [this] { startNewSweep(); }, [&](int iCol, int iOffset, int iLength) {
            matDataRaw.middleCols(iCol, iLength) = corrected.data.middleCols(iOffset, iLength);
            if (!doFilter) {
                matDataFiltered.middleCols(iCol, iLength).setZero();
                if (doSphara) {
                    matDataRaw.middleCols(iCol, iLength) = m_display.sphara_operator() * matDataRaw.middleCols(iCol, iLength);
                }
            }

            for (const mne::TriggerFlank& trigger : corrected.triggers) {
                if (trigger.sample >= iOffset && trigger.sample < iOffset + iLength) {
                    m_qMapDetectedTrigger[iTriggerCh].push_back({iCol + trigger.sample - iOffset, trigger.value});
                }
            } });

        if (!corrected.triggers.empty()) {
            m_iDetectedTriggers += static_cast<int>(corrected.triggers.size());
            emit triggerDetected(m_iDetectedTriggers, m_qMapDetectedTrigger);
        }

        //Filter the whole block at once so the overlap-add stays continuous across the wrap
        if (doFilter) {
            const int iBlockStart = m_sweep.write_position() - nCol;
            writeFiltered(corrected.data, iBlockStart);

            //Perform SPHARA on filtered data after actual filtering - SPHARA should be applied on the best possible data
            if (doSphara) {
                viz::for_wrapped_span(iBlockStart - m_display.filter_delay(), nCol, iMaxCols, [&](int iCol, int, int iLength) {
                    matDataFiltered.middleCols(iCol, iLength) = m_display.sphara_operator() * matDataFiltered.middleCols(iCol, iLength);
                });
            }
        }
    }

    //Update data content
    QModelIndex topLeft = this->index(0, 1);
    QModelIndex bottomRight = this->index(static_cast<int>(m_pFiffInfo->ch_names.size()) - 1, 1);
    emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole});
}

//=============================================================================================================

void RtFiffRawViewModel::startNewSweep()
{
    //Store old detected triggers
    m_qMapDetectedTriggerOld = m_qMapDetectedTrigger;

    //Clear detected triggers
    if (m_display.trigger_detection_active()) {
        for (auto& [channel, triggers] : m_qMapDetectedTrigger) {
            triggers.clear();
        }
    }
}

//=============================================================================================================

fiff_int_t RtFiffRawViewModel::getKind(qint32 row) const
{
    if (row < std::ssize(m_qMapIdxRowSelection)) {
        qint32 chRow = detail::valueOr(m_qMapIdxRowSelection, row);
        return m_pFiffInfo->chs.at(chRow).kind;
    }

    return 0;
}

//=============================================================================================================

fiff_int_t RtFiffRawViewModel::getUnit(qint32 row) const
{
    if (row < std::ssize(m_qMapIdxRowSelection)) {
        qint32 chRow = detail::valueOr(m_qMapIdxRowSelection, row);
        return m_pFiffInfo->chs.at(chRow).unit;
    }

    return FIFF_UNIT_NONE;
}

//=============================================================================================================

fiff_int_t RtFiffRawViewModel::getCoil(qint32 row) const
{
    if (row < std::ssize(m_qMapIdxRowSelection)) {
        qint32 chRow = detail::valueOr(m_qMapIdxRowSelection, row);
        return m_pFiffInfo->chs.at(chRow).chpos.coil_type;
    }

    return FIFFV_COIL_NONE;
}

//=============================================================================================================

void RtFiffRawViewModel::selectRows(const std::vector<int>& selection)
{
    beginResetModel();

    m_qMapIdxRowSelection.clear();

    qint32 count = 0;
    for (qint32 i = 0; i < std::ssize(selection); ++i) {
        if (selection[i] < std::ssize(m_pFiffInfo->chs)) {
            m_qMapIdxRowSelection.insert_or_assign(count, selection[i]);
            ++count;
        }
    }

    emit newSelection(selection);

    endResetModel();
}

//=============================================================================================================

void RtFiffRawViewModel::hideRows(const std::vector<int>& selection)
{
    beginResetModel();

    for (const int row : selection) {
        m_qMapIdxRowSelection.erase(row);
    }

    emit newSelection(selection);

    endResetModel();
}

//=============================================================================================================

void RtFiffRawViewModel::resetSelection()
{
    beginResetModel();

    m_qMapIdxRowSelection.clear();

    for (qint32 i = 0; i < std::ssize(m_pFiffInfo->chs); ++i) {
        m_qMapIdxRowSelection.insert_or_assign(i, i);
    }

    endResetModel();
}

//=============================================================================================================

void RtFiffRawViewModel::toggleFreeze(const QModelIndex&)
{
    m_sweep.set_frozen(!m_sweep.frozen());

    if (m_sweep.frozen()) {
        m_qMapDetectedTriggerFreeze = m_qMapDetectedTrigger;
        m_qMapDetectedTriggerOldFreeze = m_qMapDetectedTriggerOld;
    }

    //Update data content
    QModelIndex topLeft = this->index(0, 1);
    QModelIndex bottomRight = this->index(static_cast<int>(m_pFiffInfo->chs.size()) - 1, 1);
    emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole});
}

//=============================================================================================================

void RtFiffRawViewModel::setScaling(const std::map<int, float>& p_qMapChScaling)
{
    beginResetModel();
    m_qMapChScaling = p_qMapChScaling;
    endResetModel();
}

//=============================================================================================================

void RtFiffRawViewModel::updateProjection(const std::vector<mne::FiffProj>& projs)
{
    if (m_pFiffInfo) {
        m_pFiffInfo->projs = projs;
        m_display.update_projection();
        mne::Log::debug("RtFiffRawViewModel::updateProjection - New projection calculated.");
    }
}

//=============================================================================================================

void RtFiffRawViewModel::updateCompensator(int to)
{
    m_display.update_compensator(to);
}

//=============================================================================================================

void RtFiffRawViewModel::updateSpharaActivation(bool state)
{
    m_display.set_sphara_active(state);
}

//=============================================================================================================

void RtFiffRawViewModel::updateSpharaOptions(const std::string& sSytemType, int nBaseFctsFirst, int nBaseFctsSecond)
{
    mne::Log::debug("RtFiffRawViewModel::updateSpharaOptions - Creating SPHARA operator for {}", sSytemType);
    m_display.update_sphara_options(sSytemType, nBaseFctsFirst, nBaseFctsSecond);
}

//=============================================================================================================

void RtFiffRawViewModel::setFilter(const std::vector<FilterKernel>& filterData)
{
    m_display.set_filters(filterData);
}

//=============================================================================================================

void RtFiffRawViewModel::setFilterActive(bool state)
{
    m_display.set_filter_active(state);
}

//=============================================================================================================

void RtFiffRawViewModel::setBackgroundColor(const QColor& color)
{
    m_colBackground = color;
}

//=============================================================================================================

void RtFiffRawViewModel::setFilterChannelType(const std::string& channelType)
{
    m_display.set_filter_channel_type(channelType);
}

//=============================================================================================================

void RtFiffRawViewModel::markChBad(QModelIndex ch, bool status)
{
    const std::vector<FiffChInfo>& chInfolist = m_pFiffInfo->chs;

    if (status) {
        if (!std::ranges::contains(m_pFiffInfo->bads, chInfolist[ch.row()].ch_name))
            m_pFiffInfo->bads.push_back(chInfolist[ch.row()].ch_name);
        mne::Log::debug("RawModel: {} marked as bad.", chInfolist[ch.row()].ch_name);
    } else if (std::ranges::contains(m_pFiffInfo->bads, chInfolist[ch.row()].ch_name)) {
        int index = mne::index_of(m_pFiffInfo->bads, chInfolist[ch.row()].ch_name);
        m_pFiffInfo->bads.erase(m_pFiffInfo->bads.begin() + index);
        mne::Log::debug("RawModel: {} marked as good.", chInfolist[ch.row()].ch_name);
    }

    emit dataChanged(ch, ch);
}

//=============================================================================================================

void RtFiffRawViewModel::triggerInfoChanged(const std::map<double, QColor>& colorMap, bool active, const std::string& triggerCh, double threshold)
{
    m_qMapTriggerColor = colorMap;
    int iTriggerCh = m_display.trigger_channel();

    //Find channel index and initialise detected trigger map if channel name changed
    if (m_sCurrentTriggerCh != triggerCh) {
        m_sCurrentTriggerCh = triggerCh;
        m_qMapDetectedTrigger.clear();

        for (int i = 0; i < std::ssize(m_pFiffInfo->chs); ++i) {
            if (m_pFiffInfo->chs[i].ch_name == m_sCurrentTriggerCh) {
                iTriggerCh = i;
                m_qMapDetectedTrigger.insert_or_assign(i, std::vector<std::pair<int, double>>());
                break;
            }
        }
    }

    m_display.set_trigger_detection(active, iTriggerCh, threshold);
}

//=============================================================================================================

void RtFiffRawViewModel::distanceTimeSpacerChanged(int value)
{
    if (value <= 0) {
        m_iDistanceTimerSpacer = 1000;
    } else {
        m_iDistanceTimerSpacer = value;
    }
}

//=============================================================================================================

void RtFiffRawViewModel::resetTriggerCounter()
{
    m_iDetectedTriggers = 0;
}

//=============================================================================================================

void RtFiffRawViewModel::markChBad(QModelIndexList chlist, bool status)
{
    const std::vector<FiffChInfo>& chInfolist = m_pFiffInfo->chs;

    for (int i = 0; i < chlist.size(); ++i) {
        if (status) {
            if (!std::ranges::contains(m_pFiffInfo->bads, chInfolist[chlist[i].row()].ch_name))
                m_pFiffInfo->bads.push_back(chInfolist[chlist[i].row()].ch_name);
        } else {
            if (std::ranges::contains(m_pFiffInfo->bads, chInfolist[chlist[i].row()].ch_name)) {
                int index = mne::index_of(m_pFiffInfo->bads, chInfolist[chlist[i].row()].ch_name);
                m_pFiffInfo->bads.erase(m_pFiffInfo->bads.begin() + index);
            }
        }

        emit dataChanged(chlist[i], chlist[i]);
    }
}

//=============================================================================================================

void RtFiffRawViewModel::writeFiltered(const MatrixXd& data, int iDataIndex)
{
    const std::optional<mne::RtRawDisplay::Filtered> filtered = m_display.filter(data);
    if (!filtered) {
        return;
    }

    MatrixXdR& matDataFiltered = m_sweep.filtered();
    const int iMaxCols = static_cast<int>(matDataFiltered.cols());
    const int nCol = static_cast<int>(data.cols());

    //Filter output sample k of this block belongs iFilterDelay samples before the block start
    viz::for_wrapped_span(iDataIndex - m_display.filter_delay(), nCol, iMaxCols, [&](int iCol, int iOffset, int iLength) {
        for (int iRow : filtered->filtered_rows) {
            matDataFiltered.row(iRow).segment(iCol, iLength) = filtered->data.row(iRow).segment(iOffset, iLength);
        }
    });

    //Fill filtered data with raw data if the channel was not filtered
    viz::for_wrapped_span(iDataIndex, nCol, iMaxCols, [&](int iCol, int iOffset, int iLength) {
        for (int iRow : filtered->passthrough_rows) {
            matDataFiltered.row(iRow).segment(iCol, iLength) = data.row(iRow).segment(iOffset, iLength);
        }
    });
}

//=============================================================================================================

void RtFiffRawViewModel::clearModel()
{
    beginResetModel();

    m_sweep.clear();
    m_display.clear_filter_state();

    endResetModel();
}

//=============================================================================================================

double RtFiffRawViewModel::getMaxValueFromRawViewModel(int row) const
{
    double dMaxValue;
    qint32 kind = getKind(row);

    switch (kind) {
        case FIFFV_MEG_CH: {
            dMaxValue = 1e-11f;
            qint32 unit = getUnit(row);
            if (unit == FIFF_UNIT_T_M) { //gradiometers
                dMaxValue = 1e-10f;
                if (getScaling().contains(FIFF_UNIT_T_M))
                    dMaxValue = getScaling().at(FIFF_UNIT_T_M);
            } else if (unit == FIFF_UNIT_T) //magnetometers
            {
                dMaxValue = 1e-11f;
                if (getScaling().contains(FIFF_UNIT_T))
                    dMaxValue = getScaling().at(FIFF_UNIT_T);
            }
            break;
        }

        case FIFFV_REF_MEG_CH: {
            dMaxValue = 1e-11f;
            if (getScaling().contains(FIFF_UNIT_T))
                dMaxValue = getScaling().at(FIFF_UNIT_T);
            break;
        }
        case FIFFV_EEG_CH: {
            dMaxValue = 1e-4f;
            if (getScaling().contains(FIFFV_EEG_CH))
                dMaxValue = getScaling().at(FIFFV_EEG_CH);
            break;
        }
        case FIFFV_EOG_CH: {
            dMaxValue = 1e-3f;
            if (getScaling().contains(FIFFV_EOG_CH))
                dMaxValue = getScaling().at(FIFFV_EOG_CH);
            break;
        }
        case FIFFV_STIM_CH: {
            dMaxValue = 5;
            if (getScaling().contains(FIFFV_STIM_CH))
                dMaxValue = getScaling().at(FIFFV_STIM_CH);
            break;
        }
        case FIFFV_MISC_CH: {
            dMaxValue = 1e-3f;
            if (getScaling().contains(FIFFV_MISC_CH))
                dMaxValue = getScaling().at(FIFFV_MISC_CH);
            break;
        }
        default:
            dMaxValue = 1e-9f;
            break;
    }

    return dMaxValue;
}

//=============================================================================================================

void RtFiffRawViewModel::addEvent(int iSample)
{
    if (m_fnAddEvent) {
        m_fnAddEvent(iSample);
    }
}

//=============================================================================================================

int RtFiffRawViewModel::getSampleAtColumn(int iColumn) const
{
    return m_sweep.sample_at_column(iColumn);
}

//=============================================================================================================

double RtFiffRawViewModel::getValueAtColumn(int iRow, int iColumn) const
{
    const RowVectorPair rowData = data(index(iRow, 1)).value<RowVectorPair>();
    if (!rowData.first || iColumn < 0 || iColumn >= rowData.second) {
        return 0.0;
    }

    return rowData.first[iColumn];
}

//=============================================================================================================

std::vector<int> RtFiffRawViewModel::getEventsToDisplay(int iBegin, int iEnd) const
{
    if (m_fnGetEventSamples) {
        return m_fnGetEventSamples(iBegin, iEnd);
    }
    return {};
}

//=============================================================================================================

void RtFiffRawViewModel::setEventCallbacks(std::function<void(int)> addFn,
                                           std::function<std::vector<int>(int, int)> getFn)
{
    m_fnAddEvent = std::move(addFn);
    m_fnGetEventSamples = std::move(getFn);
}
