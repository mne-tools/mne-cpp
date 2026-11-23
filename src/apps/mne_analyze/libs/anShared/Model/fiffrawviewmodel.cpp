//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     fiffrawviewmodel.cpp
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2018
 * @brief    FiffRawViewModel class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "fiffrawviewmodel.h"

#include "eventmodel.h"
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff.h>
#include <mne/dsp/filterkernel.h>
#include <mne/dsp/rt/rt_filter.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QElapsedTimer>
#include <QFile>
#include <QBrush>
#include <QFileDialog>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <memory>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

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

FiffRawViewModel::FiffRawViewModel(QObject* pParent)
: AbstractModel(pParent)
{
    Log::info("[FiffRawViewModel::FiffRawViewModel] Default constructor called !");
}

//=============================================================================================================

FiffRawViewModel::FiffRawViewModel(const QString& sFilePath,
                                   const QByteArray& byteLoadedData,
                                   qint32 iVisibleWindowSize,
                                   qint32 iPreloadBufferSize,
                                   QObject* pParent)
: AbstractModel(sFilePath, pParent)
, m_dDx(1.0)
, m_iSamplesPerBlock(1024)
, m_iVisibleWindowSize(iVisibleWindowSize)
, m_iPreloadBufferSize(std::max(2, iPreloadBufferSize))
, m_iTotalBlockCount(m_iVisibleWindowSize + 2 * m_iPreloadBufferSize)
, m_iFiffCursorBegin(-1)
, m_bStartOfFileReached(true)
, m_bEndOfFileReached(false)
, m_bCurrentlyLoading(false)
, m_pRtFilter(std::make_shared<FilterOverlapAdd>())
, m_bPerformFiltering(false)
, m_iDistanceTimerSpacer(1000)
, m_iScroller(0)
, m_iScrollPos(0)
, m_bDispEvent(true)
, m_bRealtime(false)
, m_iLastFileEndSample(0)
//, m_pEventModel(QSharedPointer<EventModel>::create())
{
    auto io = QSharedPointer<FiffIO>::create();
    if (byteLoadedData.isEmpty()) {
        m_file.setFileName(sFilePath);
        io->read(sFilePath.toStdU16String());
    } else {
        const auto* begin = reinterpret_cast<const std::byte*>(byteLoadedData.constData());
        io->read(std::vector<std::byte>(begin, begin + byteLoadedData.size()));
    }
    initFiffData(io);

    updateEndStartFlags();
}

//=============================================================================================================

FiffRawViewModel::~FiffRawViewModel()
{
    if (m_bRealtime) {
        m_file.remove();
    }
}

//=============================================================================================================

bool FiffRawViewModel::initFiffData(QSharedPointer<mne::FiffIO> io)
{
    m_pFiffIO = std::move(io);

    if (m_pFiffIO->raw.empty()) {
        Log::warning("[FiffRawViewModel::loadFiffData] File does not contain any Fiff data");
        return false;
    }

    m_ChannelInfoList.clear();

    // load channel infos
    for (qint32 i = 0; i < m_pFiffIO->raw[0]->info.nchan; ++i) {
        m_ChannelInfoList.push_back(m_pFiffIO->raw[0]->info.chs[i]);
    }

    // load FiffInfo
    m_pFiffInfo = std::make_shared<FiffInfo>(m_pFiffIO->raw[0]->info);

    // build datastructure that is to be filled with data from the file
    MatrixXd data, times;

    // Fiff file is not empty, set cursor somewhere into Fiff file
    m_iFiffCursorBegin = m_pFiffIO->raw[0]->first_samp;
    m_iSamplesPerBlock = m_pFiffInfo->sfreq;
    reloadAllData();

    Log::info("[FiffRawViewModel::initFiffData] Loaded {} blocks with size {} x {}", m_lData.size(), data.rows(), m_iSamplesPerBlock);

    Log::debug("FIRST SAMPLE OFFSET IN ANALYZE: {}", m_pFiffIO->raw[0]->first_samp);

    m_bIsInit = true;

    return true;
}

//=============================================================================================================

QVariant FiffRawViewModel::data(const QModelIndex& index,
                                int role) const
{
    if (role != Qt::DisplayRole && role != Qt::BackgroundRole) {
        //qInfo() << "[FiffRawViewModel::data] Role " << role << " not implemented yet.";
        return QVariant();
    }

    if (role == Qt::BackgroundRole) {
        return QVariant(QBrush(m_colBackground));
    }

    if (index.isValid()) {
        // channel names
        if (index.column() == 0) {
            return QVariant(QString::fromStdString(m_ChannelInfoList[index.row()].ch_name));
        }

        // whole data
        else if (index.column() == 1) {
            QVariant result;

            switch (role) {
                case Qt::DisplayRole: {
                    // in order to avoid extensive copying of data, we simply give out smartpointers to the matrices (wrapped inside the ChannelData container)
                    // wait until its save to access data (that is if no data insertion is going on right now)
                    m_dataMutex.lock();

                    // wrap in ChannelData container and then wrap into QVariant
                    if (m_bPerformFiltering) {
                        result.setValue(ChannelData(m_lFilteredData, index.row()));
                    } else {
                        result.setValue(ChannelData(m_lData, index.row()));
                    }

                    m_dataMutex.unlock();

                    return result;
                }
            }
        }

        // whether channel is marked as bad
        else if (index.column() == 2) {
            return QVariant(std::ranges::contains(m_pFiffInfo->bads, m_pFiffInfo->ch_names[index.row()]));
        } else {
            Log::warning("[FiffRawViewModel::data] Column {} not implemented !", index.column());
            return QVariant();
        }
    }

    Log::warning("[FiffRawViewModel::data] Warning, non of the presumed cases took effect");
    return QVariant();
}

//=============================================================================================================

bool FiffRawViewModel::saveToFile(const QString& sPath)
{
    if (!m_pFiffIO || m_pFiffIO->raw.empty()) {
        return false;
    }
#ifdef WASMBUILD
    // The browser offers the bytes as a download; sPath has no meaning there.
    Q_UNUSED(sPath)
    const QString target = QDir::temp().filePath(getModelName());
#else
    const QString& target = sPath;
#endif
    const std::filesystem::path path = target.toStdU16String();
    // The same channels as in the view; an empty pick list would filter every channel, triggers included
    const bool ok = m_bPerformFiltering && m_lFilterChannelList.size() > 0
        ? mne::filterFile(path, m_pFiffIO->raw[0], m_filterKernel, m_lFilterChannelList)
        : m_pFiffIO->write_raw(path, 0);
#ifdef WASMBUILD
    QFile file(target);
    if (ok && file.open(QIODevice::ReadOnly)) {
        QFileDialog::saveFileContent(file.readAll(), getModelName());
    }
    file.remove();
#endif
    return ok;
}

//=============================================================================================================

QVariant FiffRawViewModel::headerData(int section,
                                      Qt::Orientation orientation,
                                      int role) const
{
    if (role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    if (orientation == Qt::Vertical) {
        QModelIndex chname = createIndex(section, 0);
        switch (role) {
            case Qt::DisplayRole:
                return QVariant(data(chname).toString());
        }
    }

    return QVariant();
}

//=============================================================================================================

Qt::ItemFlags FiffRawViewModel::flags(const QModelIndex& index) const
{
    // TODO implement stuff
    return QAbstractItemModel::flags(index);
}

//=============================================================================================================

QModelIndex FiffRawViewModel::index(int row,
                                    int column,
                                    const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column);
}

//=============================================================================================================

QModelIndex FiffRawViewModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    // TODO implement stuff
    return QModelIndex();
}

//=============================================================================================================

int FiffRawViewModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_ChannelInfoList.size());
}

//=============================================================================================================

int FiffRawViewModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    // TODO implement stuff
    return 3;
}

//=============================================================================================================

bool FiffRawViewModel::hasChildren(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    // TODO implement stuff
    return false;
}

//=============================================================================================================

std::shared_ptr<mne::FiffInfo> FiffRawViewModel::getFiffInfo() const
{
    return m_pFiffInfo;
}

//=============================================================================================================

void FiffRawViewModel::setScaling(const std::map<int, float>& p_qMapChScaling)
{
    beginResetModel();
    m_qMapChScaling = p_qMapChScaling;
    endResetModel();
}

//=============================================================================================================

qint32 FiffRawViewModel::getKind(qint32 index) const
{
    return m_ChannelInfoList.at(index).kind;
}

//=============================================================================================================

qint32 FiffRawViewModel::getUnit(qint32 index) const
{
    return m_ChannelInfoList.at(index).unit;
}

//=============================================================================================================

void FiffRawViewModel::setBackgroundColor(const QColor& color)
{
    m_colBackground = color;
}

//=============================================================================================================

void FiffRawViewModel::setWindowSize(int iNumSeconds,
                                     int iColWidth,
                                     int iScrollPos)
{
    Q_UNUSED(iScrollPos);

    beginResetModel();

    m_iVisibleWindowSize = iNumSeconds;
    m_iTotalBlockCount = m_iVisibleWindowSize + 2 * m_iPreloadBufferSize;

    //reload data to accomodate new size
    reloadAllData();

    //Update m_dDx based on new size
    setDataColumnWidth(iColWidth);

    endResetModel();
}

//=============================================================================================================

void FiffRawViewModel::distanceTimeSpacerChanged(int iNewValue)
{
    if (iNewValue <= 0) {
        m_iDistanceTimerSpacer = 1000;
    } else {
        m_iDistanceTimerSpacer = iNewValue;
    }
}

//=============================================================================================================

float FiffRawViewModel::getNumberOfTimeSpacers() const
{
    return static_cast<float>(1000 / m_iDistanceTimerSpacer);
}

//=============================================================================================================

int FiffRawViewModel::getTimeMarks(int iIndex) const
{
    if (m_pEventModel) {
        return m_pEventModel->getEvent(iIndex);
    } else {
        return 0;
    }
}

//=============================================================================================================

int FiffRawViewModel::getTimeListSize() const
{
    if (m_pEventModel) {
        return m_pEventModel->getNumberOfEventsToDisplay();
    } else {
        return 0;
    }
}

//=============================================================================================================

int FiffRawViewModel::getSampleScrollPos() const
{
    return m_iScrollPos;
}

//=============================================================================================================

int FiffRawViewModel::getTotalBlockCount() const
{
    return m_iVisibleWindowSize + m_iPreloadBufferSize;
}

//=============================================================================================================

void FiffRawViewModel::toggleDispEvent(int iToggleDisp)
{
    m_bDispEvent = (iToggleDisp ? true : false);
}

//=============================================================================================================

bool FiffRawViewModel::shouldDisplayEvent() const
{
    return (m_bDispEvent && getTimeListSize());
}

//=============================================================================================================

QSharedPointer<EventModel> FiffRawViewModel::getEventModel() const
{
    if (m_pEventModel) {
        return m_pEventModel;
    } else {
        return QSharedPointer<EventModel>::create();
    }
}

//=============================================================================================================

void FiffRawViewModel::setFilter(const FilterKernel& filterData)
{
    m_filterKernel = filterData;

    if (m_bPerformFiltering) {
        reloadAllData();
    }
}
//=============================================================================================================

const FilterKernel& FiffRawViewModel::getFilter()
{
    return m_filterKernel;
}

//=============================================================================================================

void FiffRawViewModel::setFilterActive(bool bState)
{
    m_bPerformFiltering = bState;

    if (m_bPerformFiltering) {
        reloadAllData();
    }
    emit dataChanged(createIndex(0, 0), createIndex(rowCount(), columnCount()));
}

//=============================================================================================================

void FiffRawViewModel::setFilterChannelType(const QString& channelType)
{
    if (!m_pFiffInfo) {
        return;
    }

    m_sFilterChannelType = channelType;

    //This version is for when all channels of a type are to be filtered (not only the visible ones).
    //Create channel filter list independent from channelNames
    m_lFilterChannelList.resize(0);

    for (int i = 0; i < std::ssize(m_pFiffInfo->chs); ++i) {
        if ((m_pFiffInfo->chs.at(i).kind == FIFFV_MEG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_EEG_CH ||
             m_pFiffInfo->chs.at(i).kind == FIFFV_EOG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_ECG_CH ||
             m_pFiffInfo->chs.at(i).kind == FIFFV_EMG_CH) /* && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)*/) {
            if (m_sFilterChannelType == "All") {
                m_lFilterChannelList.conservativeResize(m_lFilterChannelList.cols() + 1);
                m_lFilterChannelList[m_lFilterChannelList.cols() - 1] = i;
            } else if ((m_pFiffInfo->chs.at(i).ch_name.find(m_sFilterChannelType.toStdString()) != std::string::npos)) {
                m_lFilterChannelList.conservativeResize(m_lFilterChannelList.cols() + 1);
                m_lFilterChannelList[m_lFilterChannelList.cols() - 1] = i;
            }
        }
    }

    if (m_bPerformFiltering) {
        reloadAllData();
    }
}

//=============================================================================================================

bool FiffRawViewModel::isFilterActive() const
{
    return m_bPerformFiltering;
}

//=============================================================================================================

QSharedPointer<mne::FiffIO> FiffRawViewModel::getFiffIO() const
{
    return m_pFiffIO;
}

//=============================================================================================================

void FiffRawViewModel::updateHorizontalScrollPosition(qint32 newScrollPosition)
{
    // check if we are currently loading something in the background. This is a rudimentary solution.
    if (m_bCurrentlyLoading) {
        Log::info("[FiffRawViewModel::updateScrollPosition] Background operation still pending, try again later...");
        return;
    }

    m_iScrollPos = newScrollPosition;

    // Convert scroll position to fiff sample space via m_dDx
    qint32 targetCursor = (newScrollPosition / m_dDx) + absoluteFirstSample();

    if (targetCursor < m_iFiffCursorBegin + (m_iPreloadBufferSize - 1) * m_iSamplesPerBlock && !m_bStartOfFileReached) {
        // Calculate the amount of data we need to load
        qint32 sampleDist = (m_iFiffCursorBegin + (m_iPreloadBufferSize - 1) * m_iSamplesPerBlock) - targetCursor;

        // Calculate the needed blocks based on the amount of samples to load
        qint32 blockDist = static_cast<qint32>(ceil((static_cast<double>(sampleDist)) / (static_cast<double>(m_iSamplesPerBlock))));

        if (blockDist >= m_iTotalBlockCount) {
            // we must "jump" to the new cursor ...
            m_iFiffCursorBegin = std::max(absoluteFirstSample(), m_iFiffCursorBegin - (blockDist * m_iSamplesPerBlock));

            // and load all the data anew
            reloadAllData();
        } else {
            // there are some blocks in the intersection of the old and the new window that can stay in the buffer:
            // simply load earlier blocks
            postBlockLoad(loadEarlierBlocks(blockDist));
        }
    } else if (targetCursor + (m_iVisibleWindowSize * m_iSamplesPerBlock) >= m_iFiffCursorBegin + ((m_iPreloadBufferSize + 1) + m_iVisibleWindowSize) * m_iSamplesPerBlock && !m_bEndOfFileReached) {
        // Calculate the amount of data we need to load
        qint32 sampleDist = targetCursor + (m_iVisibleWindowSize * m_iSamplesPerBlock) - (m_iFiffCursorBegin + ((m_iPreloadBufferSize + 1) + m_iVisibleWindowSize) * m_iSamplesPerBlock);

        // Calculate the needed blocks based on the amount of samples to load
        qint32 blockDist = static_cast<qint32>(ceil((static_cast<double>(sampleDist)) / (static_cast<double>(m_iSamplesPerBlock))));

        if (blockDist >= m_iTotalBlockCount) {
            // we must "jump" to the new cursor ...
            int iResidual = absoluteLastSample() % m_iSamplesPerBlock;
            m_iFiffCursorBegin = std::min(absoluteLastSample() - (m_iTotalBlockCount * m_iSamplesPerBlock + iResidual), m_iFiffCursorBegin + (blockDist * m_iSamplesPerBlock));

            // and load all the data anew
            reloadAllData();
        } else {
            // there are some blocks in the intersection of the old and the new window that can stay in the buffer:
            // simply load later blocks
            postBlockLoad(loadLaterBlocks(blockDist));
        }
    }
}

//=============================================================================================================

bool FiffRawViewModel::filterDataBlock(MatrixXd& matData,
                                       bool bFilterEnd,
                                       bool bKeepOverhead)
{
    if (!m_bPerformFiltering) {
        return false;
    }

    if (m_lFilterChannelList.cols() == 0) {
        Log::warning("[FiffRawViewModel::filterDataBlock] No channels to filter specified.");
        return false;
    }

    // In WASM mode do not use multithreading for filtering
    bool bUseThread = true;
#ifdef WASMBUILD
    bUseThread = false;
#endif

    matData = m_pRtFilter->calculate(matData,
                                     m_filterKernel,
                                     m_lFilterChannelList,
                                     bFilterEnd,
                                     bUseThread,
                                     bKeepOverhead);

    return true;
}

//=============================================================================================================

void FiffRawViewModel::updateEndStartFlags()
{
    m_bStartOfFileReached = m_iFiffCursorBegin <= absoluteFirstSample();
    m_bEndOfFileReached = (m_iFiffCursorBegin + m_iTotalBlockCount * m_iSamplesPerBlock) >= absoluteLastSample();
}

//=============================================================================================================

int FiffRawViewModel::loadEarlierBlocks(qint32 numBlocks)
{
    // check if start of file was reached:
    int leftSamples = (m_iFiffCursorBegin - numBlocks * m_iSamplesPerBlock) - absoluteFirstSample();

    if (leftSamples <= 0) {
        Log::info("[FiffRawViewModel::loadEarlierBlocks] Reached start of file !");
        // see how many blocks we still can load
        int maxNumBlocks = (m_iFiffCursorBegin - absoluteFirstSample()) / m_iSamplesPerBlock;
        Log::info("[FiffRawViewModel::loadEarlierBlocks] Loading {} earlier blocks instead of requested {}", maxNumBlocks, numBlocks);
        if (maxNumBlocks != 0) {
            numBlocks = maxNumBlocks;
        } else {
            // nothing to be done, cant load any more blocks
            // return 0, meaning that this was a loading of earlier blocks
            return 0;
        }
    }

    // we expect m_lNewData and m_lFilteredNewData to be empty:
    if (m_lNewData.empty() == false ||
        m_lFilteredNewData.empty() == false) {
        Log::warning("[FiffRawViewModel::loadEarlierBlocks] Warning! Temporary data storage non empty !");
        return -1;
    }

    // build data structures to be filled from file
    MatrixXd matData, matTimes;

    // initialize start and end indices
    int start = m_iFiffCursorBegin - (numBlocks * m_iSamplesPerBlock);
    int end = m_iFiffCursorBegin - 1;

    // Update m_iFiffCursorBegin before we account for the filter delay
    if (start <= absoluteFirstSample()) {
        m_iFiffCursorBegin = absoluteFirstSample();
    } else {
        m_iFiffCursorBegin = start;
    }

    // When filtering load more data than we need in order to allow arbiritary filter lengths
    int iFilterDelay = 0;
    bool bBeforeStartReached = false;

    if (m_bPerformFiltering) {
        iFilterDelay = m_filterKernel.filter_order() / 2;
        end += m_filterKernel.filter_order() / 2;

        // Check if we have reached the beginning/end of the file
        if (start - iFilterDelay >= absoluteFirstSample()) {
            start -= m_filterKernel.filter_order() / 2;
        } else {
            // Add another filter delay because we need enough data to sucessfully filter with arbiritary filter lengths
            end += m_filterKernel.filter_order() / 2;
            iFilterDelay = 0;
            bBeforeStartReached = true;
        }
    }

    // Read the raw data
    if (m_pFiffIO->raw[0]->read_raw_segment(matData, matTimes, start, end)) {
        // qDebug() << "[FiffRawViewModel::loadFiffData] Successfully read a block ";
    } else {
        Log::warning("[FiffRawViewModel::loadEarlierBlocks] Could not read block ");
        return -1;
    }

    for (int i = 0; i < numBlocks; ++i) {
        m_lNewData.push_front(QSharedPointer<QPair<MatrixXd, MatrixXd>>::create(qMakePair(matData.block(0, i * m_iSamplesPerBlock + iFilterDelay, matData.rows(), m_iSamplesPerBlock),
                                                                                          matTimes.block(0, i * m_iSamplesPerBlock + iFilterDelay, matTimes.rows(), m_iSamplesPerBlock))));
    }

    // Filter data if activated, otherwise set to raw data
    if (m_bPerformFiltering) {
        bool bFilterSuccess = false;
        if (bBeforeStartReached) {
            iFilterDelay = m_filterKernel.filter_order() / 4;
            bFilterSuccess = filterDataBlock(matData, true, true);
        } else {
            bFilterSuccess = filterDataBlock(matData, true);
        }

        if (!bFilterSuccess) {
            m_lFilteredNewData = m_lNewData;
            return 0;
        }

        for (int i = 0; i < numBlocks; ++i) {
            m_lFilteredNewData.push_front(QSharedPointer<QPair<MatrixXd, MatrixXd>>::create(qMakePair(matData.block(0, (i * m_iSamplesPerBlock) + (2 * iFilterDelay), matData.rows(), m_iSamplesPerBlock),
                                                                                                      matTimes.block(0, (i * m_iSamplesPerBlock) + (2 * iFilterDelay), matTimes.rows(), m_iSamplesPerBlock))));
        }
    } else {
        m_lFilteredNewData = m_lNewData;
    }

    // return 0, meaning that this was a loading of earlier blocks
    return 0;
}

//=============================================================================================================

int FiffRawViewModel::loadLaterBlocks(qint32 numBlocks)
{
    // check if end of file is reached:
    int leftSamples = absoluteLastSample() - (m_iFiffCursorBegin + (m_iTotalBlockCount + numBlocks) * m_iSamplesPerBlock);
    if (leftSamples < 0) {
        Log::info("[FiffRawViewModel::loadLaterBlocks] Reached end of file !");
        // see how many blocks we still can load
        int maxNumBlocks = (absoluteLastSample() - (m_iFiffCursorBegin + m_iTotalBlockCount * m_iSamplesPerBlock)) / m_iSamplesPerBlock;
        //qInfo() << "[FiffRawViewModel::loadLaterBlocks] Loading " << maxNumBlocks << " later blocks instead of requested " << numBlocks;
        if (maxNumBlocks != 0) {
            numBlocks = maxNumBlocks;
        } else {
            // nothing to be done, cant load any more blocks
            // return 1, meaning that this was a loading of later blocks
            return 1;
        }
    }

    // we expect m_lNewData and m_lFilteredNewData to be empty:
    if (m_lNewData.empty() == false ||
        m_lFilteredNewData.empty() == false) {
        Log::warning("[FiffRawViewModel::loadEarlierBlocks] Warning! Temporary data storage non empty !");
        return -1;
    }

    // build data structures to be filled from file
    MatrixXd matData, matTimes;

    // initialize start and end indices
    int start = m_iFiffCursorBegin + (m_iTotalBlockCount * m_iSamplesPerBlock);
    int end = start + (m_iSamplesPerBlock * numBlocks) - 1;

    // adjust fiff cursor
    m_iFiffCursorBegin += numBlocks * m_iSamplesPerBlock;

    // Account for filter delay of current filter kernel
    int iFilterDelay = 0;

    if (m_bPerformFiltering) {
        iFilterDelay = m_filterKernel.filter_order() / 2;

        // Check if we have reached the beginning/end of the file
        if (start - iFilterDelay > absoluteFirstSample()) {
            start -= m_filterKernel.filter_order() / 2;
        } else {
            iFilterDelay = 0;
        }

        if (end + iFilterDelay < m_pFiffIO->raw[0]->last_samp) {
            end += m_filterKernel.filter_order() / 2;
        } else {
            iFilterDelay = 0;
        }
    }

    // read data
    if (m_pFiffIO->raw[0]->read_raw_segment(matData, matTimes, start, end)) {
        // qDebug() << "[FiffRawViewModel::loadFiffData] Successfully read a block ";
    } else {
        Log::warning("[FiffRawViewModel::loadLaterBlocks] Could not read block ");
        return -1;
    }

    for (int i = 0; i < numBlocks; ++i) {
        m_lNewData.push_back(QSharedPointer<QPair<MatrixXd, MatrixXd>>::create(qMakePair(matData.block(0, i * m_iSamplesPerBlock + iFilterDelay, matData.rows(), m_iSamplesPerBlock),
                                                                                         matTimes.block(0, i * m_iSamplesPerBlock + iFilterDelay, matTimes.rows(), m_iSamplesPerBlock))));
    }

    // Filter data if activated, otherwise set to raw data
    if (m_bPerformFiltering) {
        if (!filterDataBlock(matData, true)) {
            m_lFilteredNewData = m_lNewData;
            return 1;
        }

        for (int i = 0; i < numBlocks; ++i) {
            m_lFilteredNewData.push_back(QSharedPointer<QPair<MatrixXd, MatrixXd>>::create(qMakePair(matData.block(0, i * m_iSamplesPerBlock + (2 * iFilterDelay), matData.rows(), m_iSamplesPerBlock),
                                                                                                     matTimes.block(0, i * m_iSamplesPerBlock + (2 * iFilterDelay), matTimes.rows(), m_iSamplesPerBlock))));
        }
    } else {
        m_lFilteredNewData = m_lNewData;
    }

    // return 1, meaning that this was a loading of later blocks
    return 1;
}

//=============================================================================================================

void FiffRawViewModel::postBlockLoad(int result)
{
    switch (result) {
        case -1:
            Log::warning("[FiffRawViewModel::postBlockLoad] block load returned an error: {}", result);
            break;
        case 0: {
            // insertion of earlier blocks
            int iNewBlocks = static_cast<int>(m_lNewData.size());

            m_dataMutex.lock();
            for (int i = 0; i < iNewBlocks; ++i) {
                //Raw data
                m_lData.push_front(m_lNewData.front());
                m_lData.pop_back(); // @TODO check if this really frees the associated memory

                //Filtered data
                m_lFilteredData.push_front(m_lFilteredNewData.front());
                m_lFilteredData.pop_back();

                //Pop new data, which is now stored in m_lData and m_lFilteredData
                m_lNewData.pop_front();
                m_lFilteredNewData.pop_front();
            }
            m_dataMutex.unlock();

            emit newBlocksLoaded();

            break;
        }
        case 1: {
            // insertion of later blocks
            int iNewBlocks = static_cast<int>(m_lNewData.size());

            m_dataMutex.lock();
            for (int i = 0; i < iNewBlocks; ++i) {
                //Raw data
                m_lData.push_back(m_lNewData.front());
                m_lData.pop_front();

                //Filtered data
                m_lFilteredData.push_back(m_lFilteredNewData.front());
                m_lFilteredData.pop_front();

                //Pop new data, which is now stored in m_lData and m_lFilteredData
                m_lNewData.pop_front();
                m_lFilteredNewData.pop_front();
            }
            m_dataMutex.unlock();

            emit newBlocksLoaded();

            break;
        }
        default:
            Log::warning("[FiffRawViewModel::postBlockLoad] FATAL Non-intended return value: {}", result);
    }

    updateEndStartFlags();
    m_bCurrentlyLoading = false;

    emit dataChanged(createIndex(0, 0), createIndex(rowCount(), columnCount()));
}

//=============================================================================================================

void FiffRawViewModel::reloadAllData()
{
    if (!m_pFiffInfo) {
        return;
    }

    m_lData.clear();
    m_lFilteredData.clear();

    MatrixXd matData, matTimes;

    int start = m_iFiffCursorBegin;

    // for some reason the read_raw_segment function works with inclusive upper bound
    int end = start + (m_iSamplesPerBlock * m_iTotalBlockCount) - 1;

    // When filtering load more data than we need in order to allow arbiritary filter lengths
    int iFilterDelay = 0;
    bool bBeforeStartReached = false;

    if (m_bPerformFiltering) {
        iFilterDelay = m_filterKernel.filter_order() / 2;
        end += m_filterKernel.filter_order() / 2;

        // Check if we have reached the beginning of the file
        if (start - iFilterDelay >= m_pFiffIO->raw[0]->first_samp) {
            start -= m_filterKernel.filter_order() / 2;
        } else {
            // Add another filter delay because we need enough data to sucessfully filter with arbiritary filter lengths
            end += m_filterKernel.filter_order() / 2;
            iFilterDelay = 0;
            bBeforeStartReached = true;
        }
    }

    // read in all blocks
    if (m_pFiffIO->raw[0]->read_raw_segment(matData, matTimes, start, end)) {
        // qDebug() << "[FiffRawmodel::loadFiffData] Successfully read a block ";
    } else {
        Log::warning("[FiffRawViewModel::loadFiffData] Could not read samples {} to {}", start, end);
        return;
    }

    // append a matrix pair for each block
    for (int i = 0; i < m_iTotalBlockCount; ++i) {
        m_lData.push_back(QSharedPointer<QPair<MatrixXd, MatrixXd>>::create(qMakePair(matData.block(0, i * m_iSamplesPerBlock + iFilterDelay, matData.rows(), m_iSamplesPerBlock),
                                                                                      matTimes.block(0, i * m_iSamplesPerBlock + iFilterDelay, matTimes.rows(), m_iSamplesPerBlock))));
    }

    // Filter data if activated, otherwise set to raw data
    if (m_bPerformFiltering) {
        bool bFilterSuccess = false;
        if (bBeforeStartReached) {
            iFilterDelay = m_filterKernel.filter_order() / 4;
            bFilterSuccess = filterDataBlock(matData, true, true);
        } else {
            bFilterSuccess = filterDataBlock(matData, true);
        }

        if (!bFilterSuccess) {
            m_lFilteredData = m_lData;
            return;
        }

        for (int i = 0; i < m_iTotalBlockCount; ++i) {
            m_lFilteredData.push_back(QSharedPointer<QPair<MatrixXd, MatrixXd>>::create(qMakePair(matData.block(0, (i * m_iSamplesPerBlock) + (2 * iFilterDelay), matData.rows(), m_iSamplesPerBlock),
                                                                                                  matTimes.block(0, (i * m_iSamplesPerBlock) + (2 * iFilterDelay), matTimes.rows(), m_iSamplesPerBlock))));
        }
    } else {
        m_lFilteredData = m_lData;
    }

    emit dataChanged(createIndex(0, 0), createIndex(rowCount(), columnCount()));
}

//=============================================================================================================

bool FiffRawViewModel::hasEventModel()
{
    return m_pEventModel != nullptr;
}

//=============================================================================================================

void FiffRawViewModel::setEventModel(QSharedPointer<mne::analyze::EventModel> pModel)
{
    m_pEventModel = pModel;
}

//=============================================================================================================

void FiffRawViewModel::setScrollerSample(int iScrollerPos)
{
    m_iScroller = iScrollerPos;
}

//=============================================================================================================

int FiffRawViewModel::getScrollerPosition() const
{
    return m_iScroller;
}

//=============================================================================================================

void FiffRawViewModel::setRealtime(bool bRealtime)
{
    m_bRealtime = bRealtime;
    if (m_bRealtime) {
        m_FileSharer.clear();
        m_watcher.addPath(QString::fromStdU16String(m_FileSharer.directory().u16string()));
        connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] {
            for (const std::filesystem::path& path : m_FileSharer.new_files()) {
                readFromRealtimeFile(QString::fromStdU16String(path.u16string()));
            } }, Qt::UniqueConnection);
    }
}

//=============================================================================================================

bool FiffRawViewModel::isRealtime()
{
    return m_bRealtime;
}

//=============================================================================================================

void FiffRawViewModel::readFromRealtimeFile(const QString& path)
{
    m_iLastFileEndSample = this->absoluteLastSample();
    m_file.remove();
    m_file.setFileName(path);
    if (initFiffData(QSharedPointer<FiffIO>::create(std::filesystem::path(path.toStdU16String())))) {
        updateEndStartFlags();
        emit newRealtimeData();
    }
}

//=============================================================================================================

int FiffRawViewModel::getPreviousLastSample()
{
    return m_iLastFileEndSample;
}
