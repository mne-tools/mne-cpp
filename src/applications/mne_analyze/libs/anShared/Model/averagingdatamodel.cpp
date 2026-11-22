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

#include <base/log.h>
#include "averagingdatamodel.h"

#include <fiff/fiff_evoked_set.h>
#include <base/string_utils.h>
#ifdef WASMBUILD
#include <QFileDialog>
#include <QTemporaryDir>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <utility>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::analyze;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AveragingDataModel::AveragingDataModel(std::shared_ptr<mne::FiffEvokedSet> pEvokedSet,
                                       QObject* parent)
: AbstractModel(parent)
, m_pFiffEvokedSet(std::move(pEvokedSet))
, m_bFromFile(false)
{
}

//=============================================================================================================

AveragingDataModel::AveragingDataModel(const QString& sFilePath,
                                       const QByteArray& byteLoadedData,
                                       QObject* parent)
: AbstractModel(sFilePath, parent)
, m_bFromFile(true)
{
    Q_UNUSED(byteLoadedData);

    m_pFiffEvokedSet = std::make_shared<mne::FiffEvokedSet>(sFilePath.toStdU16String());
}

//=============================================================================================================

AveragingDataModel::~AveragingDataModel()
{
}

//=============================================================================================================

QVariant AveragingDataModel::data(const QModelIndex& index,
                                  int role) const
{
    Q_UNUSED(index);
    Q_UNUSED(role);

    return QVariant::fromValue(m_pFiffEvokedSet);
}

//=============================================================================================================

Qt::ItemFlags AveragingDataModel::flags(const QModelIndex& index) const
{
    return QAbstractItemModel::flags(index);
}

//=============================================================================================================

int AveragingDataModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

//=============================================================================================================

int AveragingDataModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

//=============================================================================================================

void AveragingDataModel::setEvokedSet(std::shared_ptr<mne::FiffEvokedSet> pEvokedSet)
{
    m_pFiffEvokedSet = std::move(pEvokedSet);
}

//=============================================================================================================

bool AveragingDataModel::isFromFile()
{
    return m_bFromFile;
}

//=============================================================================================================

std::shared_ptr<mne::FiffInfo> AveragingDataModel::getFiffInfo()
{
    std::shared_ptr<mne::FiffInfo> pInfo = std::make_shared<mne::FiffInfo>(m_pFiffEvokedSet->info);
    return pInfo;
}

//=============================================================================================================

std::shared_ptr<mne::FiffEvokedSet> AveragingDataModel::getEvokedSet()
{
    return m_pFiffEvokedSet;
}

//=============================================================================================================

bool AveragingDataModel::saveToFile(const QString& sPath)
{
    if (!m_pFiffEvokedSet || m_pFiffEvokedSet->evoked.empty()) {
        return false;
    }
#ifdef WASMBUILD
    // The browser offers the bytes as a download; sPath has no meaning there.
    Q_UNUSED(sPath)
    QTemporaryDir dir;
    const QString tmpPath = dir.filePath("evoked-ave.fif");
    if (!dir.isValid() || !m_pFiffEvokedSet->save(tmpPath.toStdU16String())) {
        return false;
    }
    QFile file(tmpPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QFileDialog::saveFileContent(file.readAll(), getModelName());
    return true;
#else
    const auto saved = m_pFiffEvokedSet->save(sPath.toStdU16String());
    if (!saved) {
        mne::Log::warning("[AveragingDataModel] {}", saved.error().what());
    }
    return saved.has_value();
#endif
}
