//=============================================================================================================
/**
 * @file     epochmodel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  2.1.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of the EpochModel class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "epochmodel.h"

#include <algorithm>

using namespace MNEBROWSE;

//=============================================================================================================

EpochModel::EpochModel(QObject *parent)
: QAbstractTableModel(parent)
{
}

//=============================================================================================================

EpochModel::~EpochModel() = default;

//=============================================================================================================

int EpochModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_entries.size();
}

//=============================================================================================================

int EpochModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return 6;
}

//=============================================================================================================

QVariant EpochModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole) {
        return QVariant();
    }

    if(role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);
    }

    if(orientation == Qt::Vertical) {
        return QStringLiteral("Epoch %1").arg(section);
    }

    switch(section) {
        case 0:
            return QStringLiteral("Use");
        case 1:
            return QStringLiteral("Event");
        case 2:
            return QStringLiteral("Sample");
        case 3:
            return QStringLiteral("Time (s)");
        case 4:
            return QStringLiteral("Auto Reject");
        case 5:
            return QStringLiteral("Status");
        default:
            break;
    }

    return QVariant();
}

//=============================================================================================================

QVariant EpochModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return QVariant();
    }

    const EpochEntry& entry = m_entries.at(index.row());

    if(role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
    }

    if(index.column() == 0 && role == Qt::CheckStateRole) {
        return epochIsIncluded(entry) ? Qt::Checked : Qt::Unchecked;
    }

    if(role == Qt::DisplayRole) {
        switch(index.column()) {
            case 0:
                return QVariant();
            case 1:
                return entry.eventCode;
            case 2:
                return entry.sample;
            case 3:
                return QString::number(entry.timeSeconds, 'f', 3);
            case 4:
                return entry.epoch && entry.epoch->bReject ? QStringLiteral("yes")
                                                           : QStringLiteral("no");
            case 5:
                return epochStatusText(entry);
            default:
                break;
        }
    }

    return QVariant();
}

//=============================================================================================================

bool EpochModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return false;
    }

    EpochEntry& entry = m_entries[index.row()];
    if(index.column() != 0 || role != Qt::CheckStateRole || !entry.epoch) {
        return false;
    }

    if(m_bRespectAutoRejects && entry.epoch->bReject) {
        return false;
    }

    const bool checked = value.toInt() == Qt::Checked;
    const bool newUserReject = !checked;
    if(entry.epoch->bUserReject == newUserReject) {
        return false;
    }

    entry.epoch->bUserReject = newUserReject;

    const QModelIndex firstIndex = createIndex(index.row(), 0);
    const QModelIndex lastIndex = createIndex(index.row(), columnCount() - 1);
    emit dataChanged(firstIndex, lastIndex, {Qt::DisplayRole, Qt::CheckStateRole});
    emit epochsChanged();
    return true;
}

//=============================================================================================================

Qt::ItemFlags EpochModel::flags(const QModelIndex &index) const
{
    if(!index.isValid()) {
        return Qt::NoItemFlags;
    }

    Qt::ItemFlags itemFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if(index.column() == 0) {
        const EpochEntry& entry = m_entries.at(index.row());
        if(!m_bRespectAutoRejects || !entry.epoch || !entry.epoch->bReject) {
            itemFlags |= Qt::ItemIsUserCheckable;
        }
    }

    return itemFlags;
}

//=============================================================================================================

void EpochModel::setEpochs(const QList<MNELIB::MNEEpochDataList>& epochLists,
                           const QList<int>& eventCodes,
                           int firstSample,
                           float sfreq)
{
    beginResetModel();

    m_entries.clear();

    for(int eventIndex = 0; eventIndex < epochLists.size(); ++eventIndex) {
        const int eventCode = eventIndex < eventCodes.size() ? eventCodes.at(eventIndex) : 0;
        const MNELIB::MNEEpochDataList& epochList = epochLists.at(eventIndex);

        for(const auto& epoch : epochList) {
            if(!epoch) {
                continue;
            }

            EpochEntry entry;
            entry.epoch = epoch;
            entry.eventCode = eventCode;
            entry.absoluteSample = static_cast<int>(epoch->eventSample);
            entry.sample = entry.absoluteSample - firstSample;
            entry.timeSeconds = sfreq > 0.0f
                ? static_cast<double>(entry.sample) / static_cast<double>(sfreq)
                : 0.0;

            m_entries.append(entry);
        }
    }

    std::sort(m_entries.begin(), m_entries.end(),
              [](const EpochEntry& left, const EpochEntry& right) {
                  if(left.sample == right.sample) {
                      return left.eventCode < right.eventCode;
                  }
                  return left.sample < right.sample;
              });

    endResetModel();
    emit epochsChanged();
}

//=============================================================================================================

void EpochModel::clearModel()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit epochsChanged();
}

//=============================================================================================================

void EpochModel::setRespectAutoRejects(bool respectAutoRejects)
{
    if(m_bRespectAutoRejects == respectAutoRejects) {
        return;
    }

    m_bRespectAutoRejects = respectAutoRejects;

    if(!m_entries.isEmpty()) {
        emit dataChanged(createIndex(0, 0),
                         createIndex(m_entries.size() - 1, columnCount() - 1),
                         {Qt::DisplayRole, Qt::CheckStateRole});
    }
    emit epochsChanged();
}

//=============================================================================================================

bool EpochModel::respectAutoRejects() const
{
    return m_bRespectAutoRejects;
}

//=============================================================================================================

void EpochModel::resetManualExclusions()
{
    bool changed = false;

    for(EpochEntry& entry : m_entries) {
        if(entry.epoch && entry.epoch->bUserReject) {
            entry.epoch->bUserReject = false;
            changed = true;
        }
    }

    if(!changed) {
        return;
    }

    emit dataChanged(createIndex(0, 0),
                     createIndex(m_entries.size() - 1, columnCount() - 1),
                     {Qt::DisplayRole, Qt::CheckStateRole});
    emit epochsChanged();
}

//=============================================================================================================

int EpochModel::sampleAt(int row) const
{
    if(row < 0 || row >= m_entries.size()) {
        return 0;
    }

    return m_entries.at(row).absoluteSample;
}

//=============================================================================================================

QString EpochModel::summaryText() const
{
    int keptCount = 0;
    int autoRejectCount = 0;
    int manualRejectCount = 0;

    for(const EpochEntry& entry : m_entries) {
        if(entry.epoch && entry.epoch->bReject) {
            ++autoRejectCount;
        }

        if(entry.epoch && entry.epoch->bUserReject) {
            ++manualRejectCount;
        }

        if(epochIsIncluded(entry)) {
            ++keptCount;
        }
    }

    return QStringLiteral("%1 / %2 epochs currently contribute to the evoked response. %3 auto-rejected, %4 manually excluded.")
        .arg(keptCount)
        .arg(m_entries.size())
        .arg(autoRejectCount)
        .arg(manualRejectCount);
}

//=============================================================================================================

bool EpochModel::epochIsIncluded(const EpochEntry& entry) const
{
    return entry.epoch && !entry.epoch->isRejected(m_bRespectAutoRejects);
}

//=============================================================================================================

QString EpochModel::epochStatusText(const EpochEntry& entry) const
{
    if(!entry.epoch) {
        return QStringLiteral("missing");
    }

    if(entry.epoch->bUserReject) {
        return entry.epoch->bReject ? QStringLiteral("auto + manual")
                                    : QStringLiteral("manual drop");
    }

    if(m_bRespectAutoRejects && entry.epoch->bReject) {
        return QStringLiteral("auto drop");
    }

    return QStringLiteral("kept");
}

//=============================================================================================================

QVector<double> EpochModel::ptpAmplitudes() const
{
    QVector<double> result;
    result.reserve(m_entries.size());

    for(const EpochEntry& entry : m_entries) {
        if(!epochIsIncluded(entry) || !entry.epoch || entry.epoch->epoch.cols() == 0)
            continue;

        const Eigen::MatrixXd& data = entry.epoch->epoch;
        // PTP per channel, then take max across channels
        Eigen::VectorXd ptp = data.rowwise().maxCoeff() - data.rowwise().minCoeff();
        result.append(ptp.maxCoeff());
    }

    return result;
}
