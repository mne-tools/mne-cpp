//=============================================================================================================
/**
 * @file     datawindow.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     August, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the DataWindow class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "datawindow.h"
#include "../Models/virtualchannelmodel.h"
#include <disp/viewers/helpers/channeldatamodel.h>
#include <fiff/fiff_constants.h>

#include <algorithm>
#include <QHash>

#if !defined(NO_QOPENGLWIDGET)
#include <QOpenGLWidget>
#endif

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;

namespace {

int resolveStimChannelIndex(const FIFFLIB::FiffInfo& info)
{
    int fallbackStim = -1;

    for (int ch = 0; ch < info.nchan; ++ch) {
        if (info.chs[ch].kind != FIFFV_STIM_CH)
            continue;

        if (fallbackStim < 0)
            fallbackStim = ch;

        QString normalizedName = info.ch_names[ch].trimmed().toUpper();
        normalizedName.remove(QLatin1Char(' '));
        if (normalizedName == QLatin1String("STI014") ||
            normalizedName == QLatin1String("STI101")) {
            return ch;
        }
    }

    return fallbackStim;
}

quint64 makeStimEventKey(int sample, int type)
{
    return (static_cast<quint64>(static_cast<quint32>(sample)) << 32)
           | static_cast<quint32>(type);
}

} // namespace


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DataWindow::DataWindow(QWidget *parent)
: QWidget(parent)
, ui(new Ui::DataWindowDockWidget)
, m_pMainWindow(static_cast<MainWindow*>(parent))
, m_pDataMarker(new DataMarker(this))
, m_pCurrentDataMarkerLabel(new QLabel(this))
, m_iCurrentMarkerSample(0)
, m_bHideBadChannels(false)
, m_pRawDelegate(Q_NULLPTR)
, m_pKineticScroller(Q_NULLPTR)
{
    ui->setupUi(this);

    //------------------------
    //--- Setup data model ---
    //------------------------
    if(m_pMainWindow->rawFile().exists())
        m_pRawModel = new RawModel(m_pMainWindow->rawFile(), this);
    else
        m_pRawModel = new RawModel(this);
}


//*************************************************************************************************************

DataWindow::~DataWindow()
{
}


//*************************************************************************************************************

void DataWindow::init()
{
    initMVCSettings();
    initMarker();
    initLabels();
}


//*************************************************************************************************************

QTableView* DataWindow::getDataTableView()
{
    return ui->m_tableView_rawTableView;
}


//*************************************************************************************************************

RawModel* DataWindow::getDataModel()
{
    return m_pRawModel;
}


//*************************************************************************************************************

RawDelegate* DataWindow::getDataDelegate()
{
    return m_pRawDelegate;
}


//*************************************************************************************************************

void DataWindow::scaleData(const QMap<QString,double> &scaleMap)
{
    m_pRawDelegate->setScaleMap(scaleMap);
    updateDataTableViews();
}


//*************************************************************************************************************

void DataWindow::updateDataTableViews()
{
    ui->m_tableView_rawTableView->viewport()->update();
}


//*************************************************************************************************************

void DataWindow::showSelectedChannelsOnly(QStringList selectedChannels)
{
    m_slSelectedChannels = selectedChannels;

    //Hide non selected channels/rows in the data views
    for(int i = 0; i<m_pRawModel->rowCount(); i++) {
        QString channel = m_pRawModel->data(m_pRawModel->index(i, 0), Qt::DisplayRole).toString();
        QVariant v = m_pRawModel->data(m_pRawModel->index(i,1), Qt::BackgroundRole);

        if(!selectedChannels.contains(channel))
            ui->m_tableView_rawTableView->hideRow(i);
        else
            ui->m_tableView_rawTableView->showRow(i);

        //if channel is a bad channel and bad channels are to be hidden -> do not show
        if(v.canConvert<QBrush>() && m_bHideBadChannels)
            ui->m_tableView_rawTableView->hideRow(i);
    }

    // Also apply the filter to the GPU-accelerated ChannelDataView
    if (m_pChannelDataView)
        m_pChannelDataView->setChannelFilter(selectedChannels);

    updateDataTableViews();
}


//*************************************************************************************************************

void DataWindow::changeRowHeight(int height)
{
    for(int i = 0; i<ui->m_tableView_rawTableView->verticalHeader()->count(); i++)
        ui->m_tableView_rawTableView->setRowHeight(i, height);

    updateDataTableViews();
}


//*************************************************************************************************************

void DataWindow::hideBadChannels(bool hideChannels)
{
    m_bHideBadChannels = hideChannels;

    //Hide non selected channels/rows in the data views
    for(int i = 0; i<m_pRawModel->rowCount(); i++) {
        QVariant v = m_pRawModel->data(m_pRawModel->index(i,1),Qt::BackgroundRole);
        QString chName = m_pRawModel->data(m_pRawModel->index(i,0),Qt::DisplayRole).toString();

        //Check if channel is marked as bad
        if(v.canConvert<QBrush>() && m_bHideBadChannels)
            ui->m_tableView_rawTableView->hideRow(i);
        else if(m_slSelectedChannels.contains(chName))
            ui->m_tableView_rawTableView->showRow(i);
    }

    if (m_pChannelDataView)
        m_pChannelDataView->hideBadChannels(hideChannels);

    updateDataTableViews();
}

//*************************************************************************************************************

void DataWindow::setAnnotations(const QVector<DISPLIB::ChannelRhiView::AnnotationSpan> &annotations)
{
    if(m_pChannelDataView) {
        m_pChannelDataView->setAnnotations(annotations);
    }
}

//*************************************************************************************************************

void DataWindow::setAnnotationSelectionEnabled(bool enabled)
{
    if(m_pChannelDataView) {
        m_pChannelDataView->setAnnotationSelectionEnabled(enabled);
    }
}

//*************************************************************************************************************

void DataWindow::setVirtualChannels(const QVector<VirtualChannelDefinition> &virtualChannels,
                                    bool reloadIfOpen)
{
    m_virtualChannelDefinitions = virtualChannels;
    m_resolvedVirtualChannels.clear();

    if(!m_pChannelDataView || !m_pChannelDataView->model()) {
        return;
    }

    if(!reloadIfOpen) {
        m_pChannelDataView->model()->setVirtualChannels({});
        return;
    }

    if(!m_pFiffReader || !m_pFiffReader->isOpen()) {
        return;
    }

    rebuildVirtualChannels();

    const int targetSample = qBound(m_pFiffReader->firstSample(),
                                    m_pChannelDataView->firstVisibleSample(),
                                    m_pFiffReader->lastSample());
    restartChannelView(targetSample, false);
}


//*************************************************************************************************************

void DataWindow::initMVCSettings()
{
    //-----------------------------------
    //------ Init data window view ------
    //-----------------------------------

    // ── Zero out layout margins so the view fills the dock edge-to-edge ─
    ui->verticalLayout->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout->setSpacing(0);
    ui->m_gridLayout->setContentsMargins(0, 0, 0, 0);
    ui->m_gridLayout->setSpacing(0);

    // ── ChannelDataView (GPU-accelerated, demand-paged) ───────────────
    m_pFiffReader = new FiffBlockReader(this);
    connect(m_pFiffReader, &FiffBlockReader::blockLoaded,
            this, &DataWindow::onBlockLoaded);

    m_pChannelDataView = new DISPLIB::ChannelDataView("DataWindow", this);
    m_pChannelDataView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(m_pChannelDataView, &DISPLIB::ChannelDataView::scrollPositionChanged,
            this, &DataWindow::onChannelViewScrollChanged);
    connect(m_pChannelDataView, &DISPLIB::ChannelDataView::sampleRangeSelected,
            this, &DataWindow::annotationRangeSelected);

    // Insert ChannelDataView into the grid at the same slot as the legacy QTableView
    // (row 1, col 0, rowspan 2, colspan 4) then hide the old widget.
    ui->m_gridLayout->addWidget(m_pChannelDataView, 1, 0, 2, 4);
    ui->m_tableView_rawTableView->hide();

    // ── Legacy RawModel / QTableView (kept for filter / event sub-systems) ──
    //Set MVC model
    ui->m_tableView_rawTableView->setModel(m_pRawModel);

    //Set MVC delegate
    m_pRawDelegate = new RawDelegate(this);
    ui->m_tableView_rawTableView->setItemDelegate(m_pRawDelegate);

    //set some settings for m_pRawTableView
    ui->m_tableView_rawTableView->verticalHeader()->setDefaultSectionSize(m_pRawDelegate->defaultPlotHeight());
    ui->m_tableView_rawTableView->setColumnHidden(0,true);
    ui->m_tableView_rawTableView->setColumnHidden(2,true);
    ui->m_tableView_rawTableView->resizeColumnsToContents();

    //connect QScrollBar with model in order to reload data samples (keeps filter/event logic alive)
    connect(ui->m_tableView_rawTableView->horizontalScrollBar(), &QScrollBar::valueChanged,
            m_pRawModel, &RawModel::updateScrollPos);

    //Set MVC in delegate
    m_pRawDelegate->setModelView(m_pMainWindow->eventWindow()->getEventModel(),
                                 m_pMainWindow->eventWindow()->getEventTableView(),
                                 ui->m_tableView_rawTableView);
}

//=============================================================================================================

bool DataWindow::isFiffFileLoaded() const
{
    return m_pFiffReader && m_pFiffReader->isOpen();
}

//=============================================================================================================

double DataWindow::fiffFileDurationSeconds() const
{
    if (!m_pFiffReader || !m_pFiffReader->isOpen() || !m_pFiffReader->fiffInfo())
        return 0.0;
    int nSamples = m_pFiffReader->lastSample() - m_pFiffReader->firstSample() + 1;
    return static_cast<double>(nSamples) / m_pFiffReader->fiffInfo()->sfreq;
}

//=============================================================================================================

QSharedPointer<FIFFLIB::FiffInfo> DataWindow::fiffInfo() const
{
    return (m_pFiffReader && m_pFiffReader->isOpen()) ? m_pFiffReader->fiffInfo()
                                                      : QSharedPointer<FIFFLIB::FiffInfo>();
}

//=============================================================================================================

int DataWindow::firstSample() const
{
    return (m_pFiffReader && m_pFiffReader->isOpen()) ? m_pFiffReader->firstSample() : 0;
}

//=============================================================================================================

int DataWindow::lastSample() const
{
    return (m_pFiffReader && m_pFiffReader->isOpen()) ? m_pFiffReader->lastSample() : 0;
}

//=============================================================================================================

QString DataWindow::fiffFileName() const
{
    if (!m_pFiffReader || !m_pFiffReader->isOpen())
        return QString();
    int idx = m_sFiffFilePath.lastIndexOf('/');
    return (idx >= 0) ? m_sFiffFilePath.mid(idx + 1) : m_sFiffFilePath;
}

//=============================================================================================================

bool DataWindow::loadFiffFile(const QString &path)
{
    if (!m_pFiffReader->open(path)) {
        return false;
    }
    m_sFiffFilePath = path;

    auto fiffInfo = m_pFiffReader->fiffInfo();
    m_iStimChannel = fiffInfo ? resolveStimChannelIndex(*fiffInfo) : -1;
    rebuildVirtualChannels();
    restartChannelView(m_pFiffReader->firstSample(), true);

    return true;
}

//=============================================================================================================

void DataWindow::onBlockLoaded(const Eigen::MatrixXd &data, int firstSample)
{
    m_bLoadingBlock = false;

    if (data.rows() == 0 || data.cols() == 0)
        return;

    const Eigen::MatrixXd displayData = appendVirtualChannels(data);
    auto *model = m_pChannelDataView->model();

    // Determine whether this block is contiguous with existing model data.
    // If not (e.g. jump happened while the block was in-flight), start fresh
    // at this position so the ring buffer stays coherent.
    bool useSetData = (model->totalSamples() == 0);
    if (!useSetData) {
        int modelEnd = model->firstSample() + model->totalSamples();
        // Allow a 1-sample gap for rounding
        useSetData = (firstSample < model->firstSample() - 1 ||
                      firstSample > modelEnd + 1);
    }

    if (useSetData)
        m_pChannelDataView->setData(displayData, firstSample);
    else
        m_pChannelDataView->addData(displayData);

    // ── Scan STIM channels for rising-edge events ─────────────────────
    auto fiffInfo = m_pFiffReader->fiffInfo();
    if (fiffInfo && m_iStimChannel >= 0) {
        // Fixed colour palette for event types (cycled by type index)
        static const QColor kEventPalette[] = {
            QColor(220, 50,  50),   // red
            QColor( 50, 140, 220),  // blue
            QColor( 50, 180,  80),  // green
            QColor(210, 130,  30),  // orange
            QColor(140,  60, 200),  // purple
            QColor( 30, 180, 180),  // teal
            QColor(200,  50, 160),  // pink
            QColor(100, 160,  40),  // olive
        };
        constexpr int kPaletteSize = static_cast<int>(sizeof(kEventPalette) / sizeof(kEventPalette[0]));

        // Prefer the combined trigger channel ("STI 014" for Neuromag Vectorview).
        // The remaining edge case is block boundaries and revisit loads: if we
        // always assume the previous value is zero, we can invent a false event
        // at the start of a block or duplicate an event when a region is loaded twice.
        if (m_iStimChannel < data.rows()) {
            int prevValue = 0;
            if (firstSample > m_pFiffReader->firstSample()) {
                if (m_iStimLastSample == firstSample - 1) {
                    prevValue = m_iStimLastValue;
                } else {
                    const Eigen::MatrixXd prevSampleData =
                        m_pFiffReader->readBlockSync(firstSample - 1, firstSample - 1);
                    if (m_iStimChannel < prevSampleData.rows() && prevSampleData.cols() > 0)
                        prevValue = qRound(prevSampleData(m_iStimChannel, 0));
                }
            }

            bool eventsChanged = false;
            for (int s = 0; s < data.cols(); ++s) {
                const int currValue = qRound(data(m_iStimChannel, s));

                if (currValue > 0 && prevValue <= 0) {
                    const int type      = currValue;
                    const int absSample = firstSample + s;
                    const quint64 key   = makeStimEventKey(absSample, type);

                    if (!m_seenStimEventKeys.contains(key)) {
                        if (!m_eventTypeColors.contains(type)) {
                            const int idx = m_eventTypeColors.size() % kPaletteSize;
                            m_eventTypeColors[type] = kEventPalette[idx];
                        }

                        DISPLIB::ChannelRhiView::EventMarker ev;
                        ev.sample = absSample;
                        ev.type   = type;
                        ev.color  = m_eventTypeColors[type];
                        ev.label  = QString::number(type);

                        m_seenStimEventKeys.insert(key);
                        m_stimEvents.append(ev);
                        eventsChanged = true;
                    }
                }

                prevValue = currValue;
            }

            m_iStimLastSample = firstSample + data.cols() - 1;
            m_iStimLastValue  = prevValue;

            if (eventsChanged && m_pChannelDataView) {
                std::sort(m_stimEvents.begin(), m_stimEvents.end(),
                          [](const DISPLIB::ChannelRhiView::EventMarker &a,
                             const DISPLIB::ChannelRhiView::EventMarker &b) {
                              return a.sample < b.sample;
                          });
                m_pChannelDataView->setEvents(m_stimEvents);
            }
        }
    }

    // Advance the frontier only if this block moved it forward — never regress.
    // This preserves a jump-redirect written by onChannelViewScrollChanged()
    // that may have been set while this block was in-flight.
    int newFrontier = firstSample + static_cast<int>(data.cols());
    if (m_iNextLoadSample < newFrontier)
        m_iNextLoadSample = newFrontier;

    scheduleNextLoad();
}

//=============================================================================================================

void DataWindow::rebuildVirtualChannels()
{
    m_resolvedVirtualChannels.clear();

    if(!m_pFiffReader || !m_pFiffReader->isOpen() || !m_pChannelDataView || !m_pChannelDataView->model()) {
        return;
    }

    const auto fiffInfo = m_pFiffReader->fiffInfo();
    if(!fiffInfo) {
        return;
    }

    QHash<QString, int> channelIndexByName;
    for(int channelIndex = 0; channelIndex < fiffInfo->ch_names.size(); ++channelIndex) {
        channelIndexByName.insert(fiffInfo->ch_names.at(channelIndex).trimmed(), channelIndex);
    }

    QVector<DISPLIB::ChannelDisplayInfo> virtualDisplayInfo;
    virtualDisplayInfo.reserve(m_virtualChannelDefinitions.size());

    for(const VirtualChannelDefinition& definition : std::as_const(m_virtualChannelDefinitions)) {
        const int positiveChannel = channelIndexByName.value(definition.positiveChannel.trimmed(), -1);
        const int negativeChannel = channelIndexByName.value(definition.negativeChannel.trimmed(), -1);

        if(positiveChannel < 0 || negativeChannel < 0 || positiveChannel == negativeChannel) {
            qWarning() << "DataWindow: could not resolve virtual channel"
                       << definition.name
                       << definition.positiveChannel
                       << definition.negativeChannel;
            continue;
        }

        const auto& positiveChannelInfo = fiffInfo->chs.at(positiveChannel);
        const auto& negativeChannelInfo = fiffInfo->chs.at(negativeChannel);

        const auto amplitudeForChannel = [](const FIFFLIB::FiffChInfo& channelInfo) -> float {
            switch(channelInfo.kind) {
                case FIFFV_MEG_CH:
                    return channelInfo.unit == FIFF_UNIT_T_M ? 400e-13f : 1.2e-12f;
                case FIFFV_EEG_CH:
                    return 30e-6f;
                case FIFFV_EOG_CH:
                    return 150e-6f;
                case FIFFV_EMG_CH:
                case FIFFV_ECG_CH:
                    return 1e-3f;
                case FIFFV_STIM_CH:
                    return 5.0f;
                default:
                    return 1.0f;
            }
        };

        const auto colorForKind = [](qint32 kind) -> QColor {
            switch(kind) {
                case FIFFV_MEG_CH:
                    return QColor(20, 90, 180);
                case FIFFV_EEG_CH:
                    return QColor(170, 55, 10);
                case FIFFV_EOG_CH:
                    return QColor(130, 0, 130);
                case FIFFV_ECG_CH:
                    return QColor(190, 15, 45);
                case FIFFV_EMG_CH:
                    return QColor(20, 110, 20);
                case FIFFV_STIM_CH:
                    return QColor(180, 100, 0);
                default:
                    return QColor(Qt::darkGreen);
            }
        };

        const auto typeLabelForKind = [](qint32 kind) -> QString {
            switch(kind) {
                case FIFFV_MEG_CH:
                    return QStringLiteral("MEG");
                case FIFFV_EEG_CH:
                    return QStringLiteral("EEG");
                case FIFFV_EOG_CH:
                    return QStringLiteral("EOG");
                case FIFFV_ECG_CH:
                    return QStringLiteral("ECG");
                case FIFFV_EMG_CH:
                    return QStringLiteral("EMG");
                case FIFFV_STIM_CH:
                    return QStringLiteral("STIM");
                default:
                    return QStringLiteral("MISC");
            }
        };

        ResolvedVirtualChannel resolvedChannel;
        resolvedChannel.name = definition.name;
        resolvedChannel.positiveChannel = positiveChannel;
        resolvedChannel.negativeChannel = negativeChannel;
        resolvedChannel.displayInfo.name = definition.name;
        resolvedChannel.displayInfo.typeLabel =
            (positiveChannelInfo.kind == negativeChannelInfo.kind)
            ? typeLabelForKind(positiveChannelInfo.kind)
            : QStringLiteral("MISC");
        resolvedChannel.displayInfo.color = colorForKind(positiveChannelInfo.kind).darker(115);
        resolvedChannel.displayInfo.amplitudeMax =
            qMax(amplitudeForChannel(positiveChannelInfo), amplitudeForChannel(negativeChannelInfo));
        if(resolvedChannel.displayInfo.amplitudeMax <= 0.f) {
            resolvedChannel.displayInfo.amplitudeMax = 1.f;
        }
        resolvedChannel.displayInfo.bad = false;
        resolvedChannel.displayInfo.isVirtualChannel = true;

        m_resolvedVirtualChannels.append(resolvedChannel);
        virtualDisplayInfo.append(resolvedChannel.displayInfo);
    }

    m_pChannelDataView->model()->setVirtualChannels(virtualDisplayInfo);
}

//=============================================================================================================

Eigen::MatrixXd DataWindow::appendVirtualChannels(const Eigen::MatrixXd &data) const
{
    if(m_resolvedVirtualChannels.isEmpty()) {
        return data;
    }

    Eigen::MatrixXd displayData(data.rows() + m_resolvedVirtualChannels.size(), data.cols());
    displayData.topRows(data.rows()) = data;

    for(int row = 0; row < m_resolvedVirtualChannels.size(); ++row) {
        const ResolvedVirtualChannel& virtualChannel = m_resolvedVirtualChannels.at(row);
        if(virtualChannel.positiveChannel < 0 || virtualChannel.negativeChannel < 0
           || virtualChannel.positiveChannel >= data.rows()
           || virtualChannel.negativeChannel >= data.rows()) {
            displayData.row(data.rows() + row).setZero();
            continue;
        }

        displayData.row(data.rows() + row) =
            data.row(virtualChannel.positiveChannel) - data.row(virtualChannel.negativeChannel);
    }

    return displayData;
}

//=============================================================================================================

void DataWindow::restartChannelView(int initialSample, bool clearAnnotations)
{
    if(!m_pFiffReader || !m_pFiffReader->isOpen() || !m_pChannelDataView) {
        return;
    }

    auto fiffInfo = m_pFiffReader->fiffInfo();
    if(!fiffInfo) {
        return;
    }

    m_bLoadingBlock = true;

    m_stimEvents.clear();
    m_eventTypeColors.clear();
    m_seenStimEventKeys.clear();
    m_iStimLastSample = std::numeric_limits<int>::min();
    m_iStimLastValue = 0;

    m_pChannelDataView->setEvents({});
    if(clearAnnotations) {
        m_pChannelDataView->setAnnotations({});
    }
    m_pChannelDataView->clearView();
    m_pChannelDataView->init(fiffInfo);
    m_pChannelDataView->setFileBounds(m_pFiffReader->firstSample(),
                                      m_pFiffReader->lastSample());
    m_pChannelDataView->hideBadChannels(m_bHideBadChannels);
    m_pChannelDataView->setChannelFilter(m_slSelectedChannels);

    const int blockSamples = static_cast<int>(kBlockSeconds * fiffInfo->sfreq);
    m_pChannelDataView->model()->setMaxStoredSamples(kMaxBlocks * blockSamples);

    m_iCurrentScrollSample = qBound(m_pFiffReader->firstSample(),
                                    initialSample,
                                    m_pFiffReader->lastSample());
    m_iNextLoadSample = m_iCurrentScrollSample;

    m_pChannelDataView->scrollToSample(m_iCurrentScrollSample, false);

    m_bLoadingBlock = false;
    scheduleNextLoad();
}

//=============================================================================================================

void DataWindow::onChannelViewScrollChanged(int sample)
{
    m_iCurrentScrollSample = sample;

    // If the scroll jumped outside the currently loaded data range, redirect
    // the load frontier so the data at the jump target loads immediately.
    // We update m_iNextLoadSample even when a load is in-flight; the in-flight
    // block will complete, onBlockLoaded will detect non-contiguity and call
    // setData, and scheduleNextLoad will then load from the redirected position.
    if (m_pFiffReader && m_pFiffReader->isOpen()) {
        auto *model = m_pChannelDataView->model();
        if (model && model->totalSamples() > 0) {
            int modelFirst = model->firstSample();
            int modelEnd   = modelFirst + model->totalSamples();
            // Jump detected when scroll is outside the loaded range
            if (sample < modelFirst || sample >= modelEnd) {
                m_iNextLoadSample = qBound(m_pFiffReader->firstSample(),
                                           sample,
                                           m_pFiffReader->lastSample());
            }
        }
    }

    scheduleNextLoad();
}

//=============================================================================================================

void DataWindow::scheduleNextLoad()
{
    if (m_bLoadingBlock || !m_pFiffReader || !m_pFiffReader->isOpen())
        return;
    if (m_iNextLoadSample > m_pFiffReader->lastSample())
        return; // entire file already loaded into the ring buffer

    const float sfreq        = static_cast<float>(m_pFiffReader->fiffInfo()->sfreq);
    const int   blockSamples = static_cast<int>(kBlockSeconds * sfreq);
    const int   lookahead    = static_cast<int>(kLookaheadBlocks * blockSamples);

    // Fire a load whenever the unloaded frontier is inside our lookahead window.
    // kLookaheadBlocks × kBlockSeconds = 3 × 60 s = 3 min ahead — enough headroom
    // for any realistic inertial scroll speed.
    if (m_iNextLoadSample < m_iCurrentScrollSample + lookahead) {
        m_bLoadingBlock  = true;
        const int blockEnd = qMin(m_iNextLoadSample + blockSamples - 1,
                                  m_pFiffReader->lastSample());
        m_pFiffReader->loadBlockAsync(m_iNextLoadSample, blockEnd);
    }
}


//*************************************************************************************************************

void DataWindow::initMarker()
{
    //Set marker as front top-most widget
    m_pDataMarker->raise();

    //Get boundary rect coordinates for table view
    double boundingLeft = ui->m_tableView_rawTableView->verticalHeader()->geometry().right() + ui->m_tableView_rawTableView->geometry().left();
    double boundingRight = ui->m_tableView_rawTableView->geometry().right() - ui->m_tableView_rawTableView->verticalScrollBar()->width() + 1;
    QRect boundingRect;
    boundingRect.setLeft(boundingLeft);
    boundingRect.setRight(boundingRight);

    //Inital position of the marker
    m_pDataMarker->move(boundingRect.x(), boundingRect.y() + 1);

    //Create Region from bounding rect - this region is used to restrain the marker inside the data view
    QRegion region(boundingRect);
    m_pDataMarker->setMovementBoundary(region);

    //Set marker size to table view size minus horizontal scroll bar height
    m_pDataMarker->resize(DATA_MARKER_WIDTH,
                          boundingRect.height() - ui->m_tableView_rawTableView->horizontalScrollBar()->height()-1);

    //Connect current marker to marker move signal
    connect(m_pDataMarker,&DataMarker::markerMoved,
            this,&DataWindow::updateMarkerPosition);

    //If no file has been loaded yet dont show the marker and its label
    if(!m_pRawModel->isFileLoaded()) {
        m_pDataMarker->hide();
        m_pCurrentDataMarkerLabel->hide();
        ui->m_label_sampleMin->hide();
        ui->m_label_sampleMax->hide();
    }

    //Connect current marker to loading a fiff file - no loaded file - no visible marker
    connect(m_pRawModel, &RawModel::fileLoaded, this, [this](){
        bool state = m_pRawModel->isFileLoaded();
        if(state) {
            //Inital position of the marker
            m_pDataMarker->move(DATA_MARKER_INITIAL_X, m_pDataMarker->y());
            m_pCurrentDataMarkerLabel->move(m_pDataMarker->geometry().left() + (DATA_MARKER_WIDTH/2) - (m_pCurrentDataMarkerLabel->width()/2) + 1, m_pDataMarker->geometry().top() - DATA_MARKER_LABEL_V_OFFSET);

            m_pDataMarker->show();
            m_pCurrentDataMarkerLabel->show();

            ui->m_label_sampleMin->show();
            ui->m_label_sampleMax->show();
        }
        else {
            m_pDataMarker->hide();
            m_pCurrentDataMarkerLabel->hide();

            ui->m_label_sampleMin->hide();
            ui->m_label_sampleMax->hide();
        }
    });
}


//*************************************************************************************************************

void DataWindow::initLabels()
{
    //Setup range samples
    //Connect range sample labels to horizontal sroll bar changes
    connect(ui->m_tableView_rawTableView->horizontalScrollBar(),&QScrollBar::valueChanged,
            this,&DataWindow::setRangeSampleLabels);

    //Setup marker label
    //Set current marker sample label to vertical spacer position and initalize text
    m_pCurrentDataMarkerLabel->resize(150, m_pCurrentDataMarkerLabel->height());
    m_pCurrentDataMarkerLabel->setAlignment(Qt::AlignHCenter);
    m_pCurrentDataMarkerLabel->move(m_pDataMarker->geometry().left(), m_pDataMarker->geometry().top() + 5);
    QString numberString = QString().number(m_iCurrentMarkerSample);
    m_pCurrentDataMarkerLabel->setText(numberString.append(QString(" / %1").arg("0 sec")));

    //Set color
    QPalette palette;
    QColor textColor = m_qSettings.value("DataMarker/data_marker_color", QColor(93,177,47)).value<QColor>();
    //textColor.setAlpha(DATA_MARKER_OPACITY);
    palette.setColor(QPalette::WindowText, textColor);

    QColor windowColor;
    windowColor.setAlpha(0); //make qlabel bounding rect fully transparent
    palette.setColor(QPalette::Window, windowColor);

    m_pCurrentDataMarkerLabel->setAutoFillBackground(true);
    m_pCurrentDataMarkerLabel->setPalette(palette);

    //Connect current marker sample label to marker move signal
    connect(m_pDataMarker,&DataMarker::markerMoved,
            this,&DataWindow::setMarkerSampleLabel);

    //Connect current marker sample label to horizontal scroll bar changes
    connect(ui->m_tableView_rawTableView->horizontalScrollBar(),&QScrollBar::valueChanged,
            this,&DataWindow::setMarkerSampleLabel);
}


//*************************************************************************************************************

void DataWindow::resizeEvent(QResizeEvent * event)
{
    //On every resize update marker position
    updateMarkerPosition();

    //On every resize set sample informaiton
    setRangeSampleLabels();

    return QWidget::resizeEvent(event);
}


//*************************************************************************************************************

void DataWindow::keyPressEvent(QKeyEvent* event)
{
    QScrollBar* horizontalScrollBar = ui->m_tableView_rawTableView->horizontalScrollBar();

    switch(event->key()) {
    case Qt::Key_Left:
        horizontalScrollBar->setValue(horizontalScrollBar->value() - RAWVIEW_KEYBOARD_SCROLL_STEP);
        break;
    case Qt::Key_Right:
        horizontalScrollBar->setValue(horizontalScrollBar->value() + RAWVIEW_KEYBOARD_SCROLL_STEP);
        break;
    }

    if((event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_D))
        ui->m_tableView_rawTableView->clearSelection();

    return QWidget::keyPressEvent(event);
}


//*************************************************************************************************************

bool DataWindow::eventFilter(QObject *object, QEvent *event)
{    
    //Detect double mouse clicks and move data marker to current mouse position
    if (object == ui->m_tableView_rawTableView->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouseEventCast = static_cast<QMouseEvent*>(event);
        if(mouseEventCast->button() == Qt::LeftButton) {
            m_pDataMarker->move(mouseEventCast->position().x() + ui->m_tableView_rawTableView->verticalHeader()->width() + ui->m_tableView_rawTableView->x(), m_pDataMarker->y());

            //Deselect channel which was selected through the double click -> dirty hack
            //ui->m_tableView_rawTableView->selectionModel()->select(ui->m_tableView_rawTableView->selectionModel()->currentIndex(), QItemSelectionModel::Deselect);
        }
    }

    //Deactivate grabbing gesture when scrollbars or vertical header are selected
    if ((object == ui->m_tableView_rawTableView->horizontalScrollBar() ||
         object == ui->m_tableView_rawTableView->verticalScrollBar() ||
         object == ui->m_tableView_rawTableView->verticalHeader())
        && event->type() == QEvent::Enter) {
        QScroller::ungrabGesture(ui->m_tableView_rawTableView);
        return true;
    }

    //Activate grabbing gesture when scrollbars or vertical header are deselected
    if ((object == ui->m_tableView_rawTableView->horizontalScrollBar() ||
         object == ui->m_tableView_rawTableView->verticalScrollBar() ||
         object == ui->m_tableView_rawTableView->verticalHeader())
        && event->type() == QEvent::Leave) {
        QScroller::grabGesture(ui->m_tableView_rawTableView, QScroller::LeftMouseButtonGesture);
        return true;
    }

    //Look for swipe gesture in order to scale the channels
    if (object == ui->m_tableView_rawTableView && event->type() == QEvent::Gesture) {
        QGestureEvent* gestureEventCast = static_cast<QGestureEvent*>(event);
        return gestureEvent(static_cast<QGestureEvent*>(gestureEventCast));
    }

    return false;
}

//*************************************************************************************************************

void DataWindow::customContextMenuRequested(QPoint pos)
{
    //obtain index where index was clicked
    //QModelIndex index = m_pRawTableView->indexAt(pos);

    //get selected items
    QModelIndexList selected = ui->m_tableView_rawTableView->selectionModel()->selectedIndexes();

    //create custom context menu and actions
    QMenu *menu = new QMenu(this);

    //**************** Marking ****************
    QMenu *markingSubMenu = new QMenu("Mark channels",menu);

    QAction* doMarkChBad = markingSubMenu->addAction(tr("Mark as bad"));
    connect(doMarkChBad,&QAction::triggered, [this, selected](){
        m_pRawModel->markChBad(selected,1);
    });

    QAction* doMarkChGood = markingSubMenu->addAction(tr("Mark as good"));
    connect(doMarkChGood,&QAction::triggered, [this, selected](){
        m_pRawModel->markChBad(selected,0);
    });

    //**************** FilterOperators ****************
    //selected channels
    QMenu *filtOpSubMenu = new QMenu("Apply FilterOperator to selected channel",menu);
    QMutableMapIterator<QString,QSharedPointer<MNEOperator> > it(m_pRawModel->operators());
    while(it.hasNext()) {
        it.next();
        QAction* doApplyFilter = filtOpSubMenu->addAction(tr("%1").arg(it.key()));

        connect(doApplyFilter,&QAction::triggered, [this, selected, it](){
            m_pRawModel->applyOperator(selected,it.value());
        });
    }

    //all channels
    QMenu *filtOpAllSubMenu = new QMenu("Apply FilterOperator to all channels",menu);
    it.toFront();
    while(it.hasNext()) {
        it.next();
        QAction* doApplyFilter = filtOpAllSubMenu->addAction(tr("%1").arg(it.key()));

        connect(doApplyFilter,&QAction::triggered, [this, it](){
            m_pRawModel->applyOperator(QModelIndexList(),it.value());
        });
    }

    //undoing filtering
    QMenu *undoFiltOpSubMenu = new QMenu("Undo filtering",menu);
    QMenu *undoFiltOpSelSubMenu = new QMenu("to selected channels",undoFiltOpSubMenu);

    //undo certain FilterOperators to selected channels
    it.toFront();
    while(it.hasNext()) {
        it.next();
        QAction* undoApplyFilter = undoFiltOpSelSubMenu->addAction(tr("%1").arg(it.key()));

        connect(undoApplyFilter,&QAction::triggered, [this, selected, it](){
            m_pRawModel->undoFilter(selected,it.value());
        });
    }

    undoFiltOpSubMenu->addMenu(undoFiltOpSelSubMenu);

    //undo all filterting to selected channels
    QAction* undoApplyFilterSel = undoFiltOpSubMenu->addAction(tr("Undo FilterOperators to selected channels"));
    connect(undoApplyFilterSel,&QAction::triggered, [this, selected](){
        m_pRawModel->undoFilter(selected);
    });

    //undo all filtering to all channels
    QAction* undoApplyFilterAll = undoFiltOpSubMenu->addAction(tr("Undo FilterOperators to all channels"));
    connect(undoApplyFilterAll,&QAction::triggered, [this](){
        m_pRawModel->undoFilter();
    });

    //add everything to main contextmenu
    menu->addMenu(markingSubMenu);
    menu->addMenu(filtOpSubMenu);
    menu->addMenu(filtOpAllSubMenu);
    menu->addMenu(undoFiltOpSubMenu);

    //show context menu
    menu->popup(ui->m_tableView_rawTableView->viewport()->mapToGlobal(pos));
}


//*************************************************************************************************************

void DataWindow::setRangeSampleLabels()
{
    //Set sapce width so that min sample and max sample are in line with the data plot
    ui->m_horizontalSpacer_Min->setFixedWidth(ui->m_tableView_rawTableView->verticalHeader()->width());
    ui->m_horizontalSpacer_Max->setFixedWidth(ui->m_tableView_rawTableView->verticalScrollBar()->width());

    //calculate sample range which is currently displayed in the view
    //Note: the viewport holds the width of the area which is changed through scrolling
    int minSampleRange = ui->m_tableView_rawTableView->horizontalScrollBar()->value()/* + m_pMainWindow->m_pRawModel->firstSample()*/;
    int maxSampleRange = minSampleRange + ui->m_tableView_rawTableView->viewport()->width();

    //Set values as string
    auto fiffInfo = m_pRawModel->fiffInfo();
    if (!fiffInfo)
        return;
    QString stringTemp;
    int minSampleRangeSec = (minSampleRange/fiffInfo->sfreq)*1000;
    ui->m_label_sampleMin->setText(QString("%1 / %2 sec").arg(stringTemp.number(minSampleRange)).arg(stringTemp.number((double)minSampleRangeSec/1000,'g')));
    int maxSampleRangeSec = (maxSampleRange/fiffInfo->sfreq)*1000;
    ui->m_label_sampleMax->setText(QString("%1 / %2 sec").arg(stringTemp.number(maxSampleRange)).arg(stringTemp.number((double)maxSampleRangeSec/1000,'g')));
}


//*************************************************************************************************************

void DataWindow::setMarkerSampleLabel()
{
    m_pCurrentDataMarkerLabel->raise();

    //Update the text and position in the current sample marker label
    m_iCurrentMarkerSample = ui->m_tableView_rawTableView->horizontalScrollBar()->value() +
            (m_pDataMarker->geometry().x() - ui->m_tableView_rawTableView->geometry().x() - ui->m_tableView_rawTableView->verticalHeader()->width());

    auto fiffInfo = m_pRawModel->fiffInfo();
    if (!fiffInfo)
        return;
    int currentSeconds = (m_iCurrentMarkerSample/fiffInfo->sfreq)*1000;

    QString numberString = QString("%1 / %2 sec").arg(QString().number(m_iCurrentMarkerSample)).arg(QString().number((double)currentSeconds/1000,'g'));
    m_pCurrentDataMarkerLabel->setText(numberString);

    m_pCurrentDataMarkerLabel->move(m_pDataMarker->geometry().left() + (DATA_MARKER_WIDTH/2) - (m_pCurrentDataMarkerLabel->width()/2) + 1, m_pDataMarker->geometry().top() - DATA_MARKER_LABEL_V_OFFSET);

    //Set current marker posisiton in event model
    m_pMainWindow->eventWindow()->getEventModel()->setCurrentMarkerPos(m_iCurrentMarkerSample);
}


//*************************************************************************************************************

void DataWindow::updateMarkerPosition()
{
    //Get boundary rect coordinates for table view
    QRect boundingRect = ui->m_tableView_rawTableView->geometry();

    m_pDataMarker->move(m_pDataMarker->x(), boundingRect.y()+1);

    double boundingLeft = ui->m_tableView_rawTableView->verticalHeader()->geometry().right() + ui->m_tableView_rawTableView->geometry().left();
    double boundingRight = ui->m_tableView_rawTableView->geometry().right() - ui->m_tableView_rawTableView->verticalScrollBar()->width() + 1;
    boundingRect.setLeft(boundingLeft);
    boundingRect.setRight(boundingRight);

    //Create Region from bounding rect - this region is used to restrain the marker inside the data view
    QRegion region(boundingRect);
    m_pDataMarker->setMovementBoundary(region);

    //If marker is outside of the bounding rect move to edges of bounding rect
    if(m_pDataMarker->pos().x() < boundingRect.left()) {
        m_pDataMarker->move(boundingRect.left(), boundingRect.y()+1);
        setMarkerSampleLabel();
    }

    if(m_pDataMarker->pos().x() > boundingRect.right()) {
        m_pDataMarker->move(boundingRect.right()-2, boundingRect.y()+1);
        setMarkerSampleLabel();
    }

    //Set marker size to table view size minus horizontal scroll bar height
    m_pDataMarker->resize(DATA_MARKER_WIDTH,
                          boundingRect.height() - ui->m_tableView_rawTableView->horizontalScrollBar()->height()-1);

    //Update current marker sample lable
    m_pCurrentDataMarkerLabel->move(m_pDataMarker->geometry().left() - (m_pCurrentDataMarkerLabel->width()/2) + 1, m_pDataMarker->geometry().top() - DATA_MARKER_LABEL_V_OFFSET);
}


//*************************************************************************************************************

void DataWindow::highlightChannelsInSelectionManager()
{
    if(m_pMainWindow->channelSelectionView()->isVisible()) {
        QModelIndexList selectedIndexes = ui->m_tableView_rawTableView->selectionModel()->selectedIndexes();

        m_pMainWindow->channelSelectionView()->highlightChannels(selectedIndexes);
    }
}


//*************************************************************************************************************

bool DataWindow::gestureEvent(QGestureEvent *event)
{
    //Pinch event
    if (QGesture *pinch = event->gesture(Qt::PinchGesture))
        pinchTriggered(static_cast<QPinchGesture *>(pinch));

    return true;
}


//*************************************************************************************************************

bool DataWindow::pinchTriggered(QPinchGesture *gesture)
{
    QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
    if (changeFlags & QPinchGesture::ScaleFactorChanged) {
        emit scaleChannels(gesture->scaleFactor());
        ui->m_tableView_rawTableView->setSelectionMode(QAbstractItemView::NoSelection);
        QScroller::ungrabGesture(ui->m_tableView_rawTableView);
    }

    if (gesture->state() == Qt::GestureFinished) {
        ui->m_tableView_rawTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        QScroller::grabGesture(ui->m_tableView_rawTableView, QScroller::LeftMouseButtonGesture);
    }

    return true;
}
