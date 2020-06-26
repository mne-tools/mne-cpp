//=============================================================================================================
/**
 * @file     fiffrawviewmodel.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     October, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke, Gabriel Motta. All rights reserved.
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
 * @brief    FiffRawViewModel class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffrawviewmodel.h"

#include "../Utils/metatypes.h"

#include "annotationmodel.h"

#include <fiff/fiff.h>

#include <rtprocessing/helpers/filterkernel.h>
#include <utils/mnemath.h>

#include <rtprocessing/filter.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtConcurrent/QtConcurrent>
#include <QElapsedTimer>
#include <QFile>
#include <QBrush>
#include <QFileDialog>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffRawViewModel::FiffRawViewModel(QObject *pParent)
: AbstractModel(pParent)
{
    qInfo() << "[FiffRawViewModel::FiffRawViewModel] Default constructor called !";
}

//=============================================================================================================

FiffRawViewModel::FiffRawViewModel(const QString &sFilePath,
                                   const QByteArray& byteLoadedData,
                                   qint32 iVisibleWindowSize,
                                   qint32 iPreloadBufferSize,
                                   QObject *pParent)
: AbstractModel(pParent)
, m_dDx(1.0)
, m_iSamplesPerBlock(1024)
, m_iVisibleWindowSize(iVisibleWindowSize)
, m_iPreloadBufferSize(std::max(2, iPreloadBufferSize))
, m_iTotalBlockCount(m_iVisibleWindowSize + 2 * m_iPreloadBufferSize)
, m_iFiffCursorBegin(-1)
, m_bStartOfFileReached(true)
, m_bEndOfFileReached(false)
, m_blockLoadFutureWatcher()
, m_bCurrentlyLoading(false)
, m_iDistanceTimerSpacer(1000)
, m_iScrollPos(0)
, m_bDispAnnotation(true)
, m_bPerformFiltering(false)
, m_pAnnotationModel(QSharedPointer<AnnotationModel>::create())
, m_pRtFilter(Filter::SPtr::create())
{
    Q_UNUSED(sFilePath)

    // connect data reloading: this will be run concurrently
    connect(&m_blockLoadFutureWatcher, &QFutureWatcher<int>::finished,
            [this]() {
                postBlockLoad(m_blockLoadFutureWatcher.future().result());
            });

    if(byteLoadedData.isEmpty()) {
        m_file.setFileName(sFilePath);
        initFiffData(m_file);
    } else {
        m_byteLoadedData = byteLoadedData;
        m_buffer.setData(m_byteLoadedData);
        initFiffData(m_buffer);
    }

    updateEndStartFlags();
}

//=============================================================================================================

FiffRawViewModel::~FiffRawViewModel()
{

}

//=============================================================================================================

void FiffRawViewModel::initFiffData(QIODevice& p_IODevice)
{
    // build FiffIO
    m_pFiffIO = QSharedPointer<FiffIO>::create(p_IODevice);

    if(m_pFiffIO->m_qlistRaw.empty()) {
        qWarning() << "[FiffRawViewModel::loadFiffData] File does not contain any Fiff data";
        return;
    }

    // load channel infos
    for(qint32 i=0; i < m_pFiffIO->m_qlistRaw[0]->info.nchan; ++i) {
        m_ChannelInfoList.append(m_pFiffIO->m_qlistRaw[0]->info.chs[i]);
    }

    // load FiffInfo
    m_pFiffInfo = FiffInfo::SPtr(new FiffInfo(m_pFiffIO->m_qlistRaw[0]->info));

    // build datastructure that is to be filled with data from the file
    MatrixXd data, times;

    // Fiff file is not empty, set cursor somewhere into Fiff file
    m_iFiffCursorBegin = m_pFiffIO->m_qlistRaw[0]->first_samp;

    int start = m_iFiffCursorBegin;
    m_iSamplesPerBlock = m_pFiffInfo->sfreq;

    // for some reason the read_raw_segment function works with inclusive upper bound
    int end = start + (m_iSamplesPerBlock * m_iTotalBlockCount) - 1;

    // read in all blocks, use the already prepared list m_lData and m_lFilteredData
    if(m_pFiffIO->m_qlistRaw[0]->read_raw_segment(data,
                                                  times,
                                                  start,
                                                  end)) {
        // qDebug() << "[FiffRawmodel::loadFiffData] Successfully read a block ";
    } else {
        qWarning() << "[FiffRawViewModel::loadFiffData] Could not read samples " << start << " to " << end;
        return;
    }

    // append a matrix pair for each block
    for(int i = 0; i < m_iTotalBlockCount; ++i) {
        m_lData.push_back(QSharedPointer<QPair<MatrixXd, MatrixXd> >::create(qMakePair(data.block(0, i*m_iSamplesPerBlock, data.rows(), m_iSamplesPerBlock),
                                                                                       times.block(0, i*m_iSamplesPerBlock, times.rows(), m_iSamplesPerBlock))));
    }

    filterDataBlock(data, true);
    for(int i = 0; i < m_iTotalBlockCount; ++i) {
        m_lFilteredData.push_back(QSharedPointer<QPair<MatrixXd, MatrixXd> >::create(qMakePair(data.block(0, i*m_iSamplesPerBlock, data.rows(), m_iSamplesPerBlock),
                                                                                               times.block(0, i*m_iSamplesPerBlock, times.rows(), m_iSamplesPerBlock))));
    }

    qInfo() << "[FiffRawViewModel::initFiffData] Loaded" << m_lData.size() << "blocks with size"<<data.rows()<<"x"<<m_iSamplesPerBlock;

    // need to close the file manually
    p_IODevice.close();

    m_bIsInit = true;
}

//=============================================================================================================

QVariant FiffRawViewModel::data(const QModelIndex &index,
                                int role) const
{
    if(role != Qt::DisplayRole && role != Qt::BackgroundRole) {
        qInfo() << "[FiffRawViewModel::data] Role " << role << " not implemented yet.";
        return QVariant();
    }

    if (role == Qt::BackgroundRole) {
        return QVariant(QBrush(m_colBackground));
    }

    if (index.isValid()) {
        // channel names
        if(index.column() == 0) {
            return QVariant(m_ChannelInfoList[index.row()].ch_name);
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
                    if(m_bPerformFiltering) {
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
        else if(index.column() == 2) {
            return QVariant(m_pFiffInfo->bads.contains(m_pFiffInfo->ch_names[index.row()]));
        } else {
            qWarning() << "[FiffRawViewModel::data] Column " << index.column() << " not implemented !";
            return QVariant();
        }
    }

    qWarning() << "[FiffRawViewModel::data] Warning, non of the presumed cases took effect";
    return QVariant();
}

//=============================================================================================================

bool FiffRawViewModel::saveToFile(const QString& sPath)
{
#ifdef WASMBUILD
    QBuffer* bufferOut = new QBuffer;

    if(m_pFiffIO->m_qlistRaw.size() > 0) {
        if(m_bPerformFiltering) {
            // Write to file with a better filter kernel with 4096 filter taps
            int iOrder = 4096;
            QList<FilterKernel> lFilterKernelNew = m_filterKernel;
            for(int i = 0; i < lFilterKernelNew.size(); ++i) {
                if(lFilterKernelNew[i].getFilterOrder() < iOrder) {
                    lFilterKernelNew[i] = FilterKernel(lFilterKernelNew[i].getName(),
                                                       lFilterKernelNew[i].m_Type,
                                                       iOrder,
                                                       lFilterKernelNew[i].getCenterFrequency(),
                                                       lFilterKernelNew[i].getBandwidth(),
                                                       lFilterKernelNew[i].getParksWidth(),
                                                       lFilterKernelNew[i].getSamplingFrequency(),
                                                       lFilterKernelNew[i].m_designMethod);
                }
            }

            Filter filter;
            return filter.filterData(*bufferOut, m_pFiffIO->m_qlistRaw[0], lFilterKernelNew);
        } else {
            return m_pFiffIO->write_raw(*bufferOut, 0);
        }

        // Wee need to call the QFileDialog here instead of the data load plugin since we need access to the QByteArray
        QFileDialog::saveFileContent(bufferOut->data(), getModelName());

        return true;
    }

    //bufferOut->deleteLater();

    return false;
#else
    QFile fFileOut(sPath);

    if(m_pFiffIO->m_qlistRaw.size() > 0) {
        if(m_bPerformFiltering) {
            // Write to file with a better filter kernel with 4096 filter taps
            int iOrder = 4096;
            QList<FilterKernel> lFilterKernelNew = m_filterKernel;
            for(int i = 0; i < lFilterKernelNew.size(); ++i) {
                if(lFilterKernelNew[i].getFilterOrder() < iOrder) {
                    lFilterKernelNew[i] = FilterKernel(lFilterKernelNew[i].getName(),
                                                       lFilterKernelNew[i].m_Type,
                                                       iOrder,
                                                       lFilterKernelNew[i].getCenterFrequency(),
                                                       lFilterKernelNew[i].getBandwidth(),
                                                       lFilterKernelNew[i].getParksWidth(),
                                                       lFilterKernelNew[i].getSamplingFrequency(),
                                                       lFilterKernelNew[i].m_designMethod);
                }
            }

            Filter filter;
            return filter.filterFile(fFileOut, m_pFiffIO->m_qlistRaw[0], lFilterKernelNew);
        } else {
            return m_pFiffIO->write_raw(fFileOut, 0);
        }
    }

    return false;
#endif
}

//=============================================================================================================

QVariant FiffRawViewModel::headerData(int section,
                                      Qt::Orientation orientation,
                                      int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    if(orientation == Qt::Vertical) {
        QModelIndex chname = createIndex(section,0);
        switch(role) {
        case Qt::DisplayRole:
            return QVariant(data(chname).toString());
        }
    }

    return QVariant();
}

//=============================================================================================================

Qt::ItemFlags FiffRawViewModel::flags(const QModelIndex &index) const
{
    // TODO implement stuff
    return QAbstractItemModel::flags(index);
}

//=============================================================================================================

QModelIndex FiffRawViewModel::index(int row,
                                    int column,
                                    const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column);
}

//=============================================================================================================

QModelIndex FiffRawViewModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);
    // TODO implement stuff
    return QModelIndex();
}

//=============================================================================================================

int FiffRawViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if(m_ChannelInfoList.empty() == false)
        return m_ChannelInfoList.size();

    return 0;
}

//=============================================================================================================

int FiffRawViewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    // TODO implement stuff
    return 3;
}

//=============================================================================================================

bool FiffRawViewModel::hasChildren(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    // TODO implement stuff
    return false;
}

//=============================================================================================================

void FiffRawViewModel::updateHorizontalScrollPosition(qint32 newScrollPosition)
{
    // check if we are currently loading something in the background. This is a rudimentary solution.
    if (m_bCurrentlyLoading) {
        qInfo() << "[FiffRawViewModel::updateScrollPosition] Background operation still pending, try again later...";
        return;
    }

    m_iScrollPos = newScrollPosition;
    qint32 targetCursor = (newScrollPosition / m_dDx) + absoluteFirstSample() ;

    if (targetCursor < m_iFiffCursorBegin + (m_iPreloadBufferSize - 1) * m_iSamplesPerBlock
        && m_bStartOfFileReached == false) {
        // time to move the loaded window. Calculate distance in blocks
        qint32 sampleDist = (m_iFiffCursorBegin + (m_iPreloadBufferSize - 1) * m_iSamplesPerBlock) - targetCursor;
        qint32 blockDist = (qint32) ceil(((double) sampleDist) / ((double) m_iSamplesPerBlock));

        if (blockDist >= m_iTotalBlockCount) {
            // we must "jump" to the new cursor ...
            m_iFiffCursorBegin = std::max(absoluteFirstSample(), m_iFiffCursorBegin - blockDist * m_iSamplesPerBlock);

            // ... and load the whole model anew
            //startBackgroundOperation(&FiffRawViewModel::loadLaterBlocks, m_iTotalBlockCount);
            postBlockLoad(loadEarlierBlocks(m_iTotalBlockCount));
            updateHorizontalScrollPosition(newScrollPosition);
        } else {
            // there are some blocks in the intersection of the old and the new window that can stay in the buffer:
            // simply load earlier blocks
            //startBackgroundOperation(&FiffRawViewModel::loadEarlierBlocks, blockDist);
            postBlockLoad(loadEarlierBlocks(blockDist));
        }
    } else if (targetCursor + (m_iVisibleWindowSize * m_iSamplesPerBlock) >= m_iFiffCursorBegin + ((m_iPreloadBufferSize + 1) + m_iVisibleWindowSize) * m_iSamplesPerBlock
               && m_bEndOfFileReached == false) {
        // time to move the loaded window. Calculate distance in blocks
        qint32 sampleDist = targetCursor + (m_iVisibleWindowSize * m_iSamplesPerBlock) - (m_iFiffCursorBegin + ((m_iPreloadBufferSize + 1) + m_iVisibleWindowSize) * m_iSamplesPerBlock);
        qint32 blockDist = (qint32) ceil(((double) sampleDist) / ((double) m_iSamplesPerBlock));

        if (blockDist >= m_iTotalBlockCount) {
            // we must "jump" to the new cursor ...
            m_iFiffCursorBegin = std::min(absoluteLastSample() - m_iTotalBlockCount * m_iSamplesPerBlock, m_iFiffCursorBegin + blockDist * m_iSamplesPerBlock);

            // ... and load the whole model anew
            //startBackgroundOperation(&FiffRawViewModel::loadLaterBlocks, m_iTotalBlockCount);
            postBlockLoad(loadLaterBlocks(m_iTotalBlockCount));
            updateHorizontalScrollPosition(newScrollPosition);
        } else {
            // there are some blocks in the intersection of the old and the new window that can stay in the buffer:
            // simply load later blocks
            //startBackgroundOperation(&FiffRawViewModel::loadLaterBlocks, blockDist);
            postBlockLoad(loadLaterBlocks(blockDist));
        }
    }
}

//=============================================================================================================

QSharedPointer<FIFFLIB::FiffInfo> FiffRawViewModel::getFiffInfo() const
{
    return m_pFiffInfo;
}

//=============================================================================================================

void FiffRawViewModel::setScaling(const QMap< qint32,float >& p_qMapChScaling)
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
    updateDisplayData();

    //Update m_dDx based on new size
    setDataColumnWidth(iColWidth);

    endResetModel();
}

//=============================================================================================================

void FiffRawViewModel::distanceTimeSpacerChanged(int iNewValue)
{
    if(iNewValue <= 0) {
        m_iDistanceTimerSpacer = 1000;
    } else {
        m_iDistanceTimerSpacer = iNewValue;
    }
}

//=============================================================================================================

float FiffRawViewModel::getNumberOfTimeSpacers() const
{
    return (float)(1000 / m_iDistanceTimerSpacer);
}

//=============================================================================================================

int FiffRawViewModel::getTimeMarks(int iIndex) const
{
    return m_pAnnotationModel->getAnnotation(iIndex);
}

//=============================================================================================================

int FiffRawViewModel::getTimeListSize() const
{
    return m_pAnnotationModel->getNumberOfAnnotations();
}

//=============================================================================================================

int FiffRawViewModel::getSampleScrollPos() const
{
    return m_iScrollPos;
}

//=============================================================================================================

int FiffRawViewModel::getWindowSizeBlocks() const
{
    return m_iVisibleWindowSize + m_iPreloadBufferSize;
}

//=============================================================================================================

void FiffRawViewModel::toggleDispAnnotation(int iToggleDisp)
{
    m_bDispAnnotation = (iToggleDisp ? true : false);
}

//=============================================================================================================

bool FiffRawViewModel::shouldDisplayAnnotation() const
{
    return (m_bDispAnnotation && getTimeListSize());
}

//=============================================================================================================

QSharedPointer<AnnotationModel> FiffRawViewModel::getAnnotationModel() const
{
    return m_pAnnotationModel;
}

//=============================================================================================================

void FiffRawViewModel::addTimeMark(int iLastClicked)
{
    m_pAnnotationModel->setSamplePos(iLastClicked);
    m_pAnnotationModel->insertRow(0, QModelIndex());
}

//=============================================================================================================

void FiffRawViewModel::setFilter(const QList<FilterKernel>& filterData)
{
    m_filterKernel = filterData;

     //Filter all visible data channels at once
    filterAllDataBlocks();

    emit dataChanged(createIndex(0,0), createIndex(rowCount(), columnCount()));
}

//=============================================================================================================

void FiffRawViewModel::setFilterActive(bool bState)
{
    m_bPerformFiltering = bState;

    //Filter all data channels at once
    filterAllDataBlocks();

    emit dataChanged(createIndex(0,0), createIndex(rowCount(), columnCount()));
}

//=============================================================================================================

void FiffRawViewModel::setFilterChannelType(const QString& channelType)
{
    if(!m_pFiffInfo) {
        return;
    }

    m_sFilterChannelType = channelType;

    //This version is for when all channels of a type are to be filtered (not only the visible ones).
    //Create channel filter list independent from channelNames
    m_lFilterChannelList.resize(0);

    for(int i = 0; i < m_pFiffInfo->chs.size(); ++i) {
        if((m_pFiffInfo->chs.at(i).kind == FIFFV_MEG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_EEG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EOG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_ECG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EMG_CH)/* && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)*/) {

            if(m_sFilterChannelType == "All") {
                m_lFilterChannelList.conservativeResize(m_lFilterChannelList.cols() + 1);
                m_lFilterChannelList[m_lFilterChannelList.cols()-1] = i;
            } else if(m_pFiffInfo->chs.at(i).ch_name.contains(m_sFilterChannelType)) {
                m_lFilterChannelList.conservativeResize(m_lFilterChannelList.cols() + 1);
                m_lFilterChannelList[m_lFilterChannelList.cols()-1] = i;
            }
        }
    }

    //Filter all data channels at once
    filterAllDataBlocks();

    emit dataChanged(createIndex(0,0), createIndex(rowCount(), columnCount()));
}

//=============================================================================================================

bool FiffRawViewModel::isFilterActive() const
{
    return m_bPerformFiltering;
}

//=============================================================================================================

int FiffRawViewModel::getFilterLength() const
{
    if(!m_filterKernel.isEmpty()) {
        return m_filterKernel.first().getFilterOrder();
    }
}

//=============================================================================================================

void FiffRawViewModel::filterDataBlock(MatrixXd& matData,
                                       bool bFilterEnd)
{
    if(!m_bPerformFiltering) {
        return;
    }

    if(m_lFilterChannelList.cols() == 0) {
        qWarning() << "[FiffRawViewModel::filterDataBlock] No channels to filter specified.";
        return;
    }

    if(m_filterKernel.isEmpty()) {
        qWarning() << "[FiffRawViewModel::filterDataBlock] Filter is unspecified.";
        return;
    }

    // In WASM mode do not use multithreading for filtering
    #ifdef WASMBUILD
    matData = m_pRtFilter->filterData(matData,
                                      m_filterKernel,
                                      m_lFilterChannelList,
                                      bFilterEnd,
                                      false);
    #else
    // We keep the overhead becuase this excludes extra copying in the filterData function which leads to a smoother scrolling
    matData = m_pRtFilter->filterData(matData,
                                      m_filterKernel,
                                      m_lFilterChannelList,
                                      bFilterEnd,
                                      true,
                                      true);
    #endif
}

//=============================================================================================================

void FiffRawViewModel::filterAllDataBlocks()
{
    if(!m_bPerformFiltering) {
        return;
    }

    if(m_lFilterChannelList.cols() == 0) {
        qWarning() << "[FiffRawViewModel::filterAllDataBlocks] No channels to filter specified.";
        return;
    }

    if(m_filterKernel.isEmpty()) {
        qWarning() << "[FiffRawViewModel::filterAllDataBlocks] Filter is unspecified.";
        return;
    }

    m_lFilteredData.clear();

    QSharedPointer<QPair<MatrixXd, MatrixXd> > pPair;
    std::list<QSharedPointer<QPair<MatrixXd, MatrixXd> > >::const_iterator itr;

    for(int i = 0; i < m_lData.size(); ++i) {
        itr = std::next(m_lData.begin(), i);

        #ifdef WASMBUILD
        pPair = QSharedPointer<QPair<MatrixXd, MatrixXd> >::create(qMakePair(m_pRtFilter->filterData((*itr)->first,
                                                                                                     m_filterKernel,
                                                                                                     m_lFilterChannelList,
                                                                                                     false,
                                                                                                     false),
                                                                             (*itr)->second));
        #else
        pPair = QSharedPointer<QPair<MatrixXd, MatrixXd> >::create(qMakePair(m_pRtFilter->filterData((*itr)->first,
                                                                                                     m_filterKernel,
                                                                                                     m_lFilterChannelList,
                                                                                                     true,
                                                                                                     true,
                                                                                                     true),
                                                                             (*itr)->second));
        #endif

        m_lFilteredData.push_back(pPair);
    }
}

//=============================================================================================================

void FiffRawViewModel::updateEndStartFlags()
{
    m_bStartOfFileReached = m_iFiffCursorBegin == absoluteFirstSample();
    m_bEndOfFileReached = (m_iFiffCursorBegin + m_iTotalBlockCount * m_iSamplesPerBlock) == absoluteLastSample();
}

//=============================================================================================================

void FiffRawViewModel::startBackgroundOperation(int (FiffRawViewModel::*loadFunction)(int), int iBlocksToLoad)
{
    m_bCurrentlyLoading = true;
    QFuture<int> future = QtConcurrent::run(this, loadFunction, iBlocksToLoad);
    m_blockLoadFutureWatcher.setFuture(future);
}

//=============================================================================================================

int FiffRawViewModel::loadEarlierBlocks(qint32 numBlocks)
{
    // check if start of file was reached:
    int leftSamples = (m_iFiffCursorBegin - numBlocks * m_iSamplesPerBlock) - absoluteFirstSample();
    if (leftSamples <= 0) {
        //qInfo() << "[FiffRawViewModel::loadEarlierBlocks] Reached start of file !";
        // see how many blocks we still can load
        int maxNumBlocks = (m_iFiffCursorBegin - absoluteFirstSample()) / m_iSamplesPerBlock;
        //qInfo() << "[FiffRawViewModel::loadEarlierBlocks] Loading " << maxNumBlocks << " earlier blocks instead of requested " << numBlocks;
        if (maxNumBlocks != 0) {
            numBlocks = maxNumBlocks;
        } else {
            // nothing to be done, cant load any more blocks
            // return 0, meaning that this was a loading of earlier blocks
            return 0;
        }
    }

    // we expect m_lNewData and m_lFilteredNewData to be empty:
    if (m_lNewData.empty() == false &&
        m_lFilteredNewData.empty() == false) {
        qInfo() << "[FiffRawViewModel::loadEarlierBlocks] FATAL, temporary data storage non empty !";
        return -1;
    }

    // build data structures to be filled from file
    MatrixXd data, times;

    // initialize start and end indices
    int start = m_iFiffCursorBegin - (numBlocks * m_iSamplesPerBlock);
    int end = m_iFiffCursorBegin - 1;

    // read data, use the already prepared list m_lNewData
    if(m_pFiffIO->m_qlistRaw[0]->read_raw_segment(data, times, start, end)) {
        // qDebug() << "[FiffRawViewModel::loadFiffData] Successfully read a block ";
    } else {
        qWarning() << "[FiffRawViewModel::loadEarlierBlocks] Could not read block ";
        return -1;
    }

    for(int i = 0; i < numBlocks; ++i) {
        m_lNewData.push_front(QSharedPointer<QPair<MatrixXd, MatrixXd> >::create(qMakePair(data.block(0, i*m_iSamplesPerBlock, data.rows(), m_iSamplesPerBlock),
                                                                                           times.block(0, i*m_iSamplesPerBlock, times.rows(), m_iSamplesPerBlock))));
    }

    // Filter data
    filterDataBlock(data, false);
    for(int i = 0; i < numBlocks; ++i) {
        m_lFilteredNewData.push_front(QSharedPointer<QPair<MatrixXd, MatrixXd> >::create(qMakePair(data.block(0, i*m_iSamplesPerBlock, data.rows(), m_iSamplesPerBlock),
                                                                                                   times.block(0, i*m_iSamplesPerBlock, times.rows(), m_iSamplesPerBlock))));
    }

    // adjust fiff cursor
    m_iFiffCursorBegin = start;

    // return 0, meaning that this was a loading of earlier blocks
    return 0;
}

//=============================================================================================================

int FiffRawViewModel::loadLaterBlocks(qint32 numBlocks)
{
    // check if end of file is reached:
    int leftSamples = absoluteLastSample() - (m_iFiffCursorBegin + (m_iTotalBlockCount + numBlocks) * m_iSamplesPerBlock);
    if (leftSamples < 0) {
        //qInfo() << "[FiffRawViewModel::loadLaterBlocks] Reached end of file !";
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
    if (m_lNewData.empty() == false &&
        m_lFilteredNewData.empty() == false) {
        qCritical() << "[FiffRawViewModel::loadLaterBlocks] FATAL, temporary data storage non empty !";
        return -1;
    }

    // build data structures to be filled from file
    MatrixXd data, times;

    // initialize start and end indices
    int start = m_iFiffCursorBegin + m_iTotalBlockCount * m_iSamplesPerBlock;
    int end = start + (m_iSamplesPerBlock * numBlocks) - 1;

    // read data, use the already prepaired list m_lNewData
    if(m_pFiffIO->m_qlistRaw[0]->read_raw_segment(data, times, start, end)) {
        // qDebug() << "[FiffRawViewModel::loadFiffData] Successfully read a block ";
    } else {
        qWarning() << "[FiffRawViewModel::loadLaterBlocks] Could not read block ";
        return -1;
    }

    for(int i = 0; i < numBlocks; ++i) {
        m_lNewData.push_back(QSharedPointer<QPair<MatrixXd, MatrixXd> >::create(qMakePair(data.block(0, i*m_iSamplesPerBlock, data.rows(), m_iSamplesPerBlock),
                                                                                          times.block(0, i*m_iSamplesPerBlock, times.rows(), m_iSamplesPerBlock))));
    }

    // Filter data
    filterDataBlock(data, true);
    for(int i = 0; i < numBlocks; ++i) {
        m_lFilteredNewData.push_back(QSharedPointer<QPair<MatrixXd, MatrixXd> >::create(qMakePair(data.block(0, i*m_iSamplesPerBlock, data.rows(), m_iSamplesPerBlock),
                                                                                                  times.block(0, i*m_iSamplesPerBlock, times.rows(), m_iSamplesPerBlock))));
    }

    // adjust fiff cursor
    m_iFiffCursorBegin += numBlocks * m_iSamplesPerBlock;

    // return 1, meaning that this was a loading of later blocks
    return 1;
}

//=============================================================================================================

void FiffRawViewModel::postBlockLoad(int result)
{
    switch(result){
    case -1:
        qWarning() << "[FiffRawViewModel::postBlockLoad] QFuture returned an error: " << result;
        break;
    case 0:
    {
        // insertion of earlier blocks
        int iNewBlocks = m_lNewData.size();

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
    case 1:
    {
        // insertion of later blocks
        int iNewBlocks = m_lNewData.size();

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
        qCritical() << "[FiffRawViewModel::postBlockLoad] FATAL Non-intended return value: " << result;
    }

    updateEndStartFlags();
    m_bCurrentlyLoading = false;
    emit dataChanged(createIndex(0,0), createIndex(rowCount(), columnCount()));
}

//=============================================================================================================

void FiffRawViewModel::updateDisplayData()
{
    m_lData.clear();
    m_lFilteredData.clear();

    MatrixXd data, times;

    //m_iFiffCursorBegin = m_pFiffIO->m_qlistRaw[0]->first_samp;

    int start = m_iFiffCursorBegin;
    m_iSamplesPerBlock = m_pFiffInfo->sfreq;

    // for some reason the read_raw_segment function works with inclusive upper bound
    int end = start + (m_iSamplesPerBlock * m_iTotalBlockCount) - 1;

    // read in all blocks, use the already prepared list m_lData
    if(m_pFiffIO->m_qlistRaw[0]->read_raw_segment(data,
                                                  times,
                                                  start,
                                                  end)) {
        // qDebug() << "[FiffRawmodel::loadFiffData] Successfully read a block ";
    } else {
        qWarning() << "[FiffRawViewModel::loadFiffData] Could not read samples " << start << " to " << end;
        return;
    }

    // append a matrix pair for each block
    for(int i = 0; i < m_iTotalBlockCount; ++i) {
        m_lData.push_back(QSharedPointer<QPair<MatrixXd, MatrixXd> >::create(qMakePair(data.block(0, i*m_iSamplesPerBlock, data.rows(), m_iSamplesPerBlock),
                                                                                       times.block(0, i*m_iSamplesPerBlock, times.rows(), m_iSamplesPerBlock))));
    }

    // Filtered data
    filterDataBlock(data, true);
    for(int i = 0; i < m_iTotalBlockCount; ++i) {
        m_lFilteredData.push_back(QSharedPointer<QPair<MatrixXd, MatrixXd> >::create(qMakePair(data.block(0, i*m_iSamplesPerBlock, data.rows(), m_iSamplesPerBlock),
                                                                                               times.block(0, i*m_iSamplesPerBlock, times.rows(), m_iSamplesPerBlock))));
    }
}

//=============================================================================================================

QSharedPointer<FIFFLIB::FiffIO> FiffRawViewModel::getFiffIO() const
{
    return m_pFiffIO;
}
