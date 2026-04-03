//=============================================================================================================
/**
 * @file     datawindow.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  2.1.0
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
#include <cmath>
#include <QHash>
#include <QSignalBlocker>
#include <utility>

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

QColor markerColorForIndex(int index)
{
    static const QVector<QColor> kMarkerColors = {
        QColor(93, 177, 47),
        QColor(52, 152, 219),
        QColor(231, 126, 35),
        QColor(155, 89, 182),
        QColor(231, 76, 60),
        QColor(26, 188, 156)
    };

    if (kMarkerColors.isEmpty()) {
        return QColor(93, 177, 47);
    }

    return kMarkerColors.at(index % kMarkerColors.size());
}

QMultiMap<int, QSharedPointer<MNEOperator> > browserAssignedOperators(const QSharedPointer<FIFFLIB::FiffInfo>& info,
                                                                      const QSharedPointer<SessionFilter>& filterDefinition)
{
    QMultiMap<int, QSharedPointer<MNEOperator> > assignedOperators;
    if (!info || filterDefinition.isNull()) {
        return assignedOperators;
    }

    const QSharedPointer<MNEOperator> operatorBase = QSharedPointer<MNEOperator>::create(MNEOperator::FILTER);
    operatorBase->m_sName = filterDefinition->displayName();
    for (int channelIndex = 0; channelIndex < info->nchan; ++channelIndex) {
        if (filterDefinition->appliesToChannel(*info, channelIndex)) {
            assignedOperators.insert(channelIndex, operatorBase);
        }
    }

    return assignedOperators;
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
, m_bHideBadChannels(false)
, m_pRawDelegate(Q_NULLPTR)
, m_pKineticScroller(Q_NULLPTR)
{
    ui->setupUi(this);
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

    // Forward to the GPU-based channel data view
    if (m_pChannelDataView && m_pChannelDataView->model())
        m_pChannelDataView->model()->setScaleMapFromStrings(scaleMap);
}


//*************************************************************************************************************

void DataWindow::updateProjections()
{
    qWarning() << "[DataWindow] updateProjections called.";
    if (m_pFiffReader)
        m_pFiffReader->updateProjections();

    // Reload all visible data with the updated projection
    restartChannelView(m_iCurrentScrollSample, false);
    qWarning() << "[DataWindow] updateProjections: restartChannelView done.";
}


//*************************************************************************************************************

void DataWindow::computeAutoScale()
{
    if (!m_pFiffReader || !m_pFiffReader->isOpen())
        return;
    auto fiffInfo = m_pFiffReader->fiffInfo();
    if (!fiffInfo)
        return;

    // Read a short sample (up to 2 seconds) synchronously from the file start
    const int first = m_pFiffReader->firstSample();
    const int last  = m_pFiffReader->lastSample();
    const int sampleCount = qMin(static_cast<int>(2.0 * fiffInfo->sfreq),
                                 last - first + 1);
    if (sampleCount <= 0)
        return;

    Eigen::MatrixXd block = m_pFiffReader->readBlockSync(first, first + sampleCount - 1);
    if (block.rows() == 0 || block.cols() == 0)
        return;

    // Accumulate per-channel-type RMS
    // Channel types: MEG_grad, MEG_mag, EEG, EOG, EMG, ECG, STIM, MISC
    struct TypeStats {
        double sumSq = 0.0;
        int    count = 0;         // number of (channel × sample) values
    };

    QMap<qint32, TypeStats> typeStats;

    for (int ch = 0; ch < fiffInfo->nchan && ch < block.rows(); ++ch) {
        if (fiffInfo->bads.contains(fiffInfo->ch_names[ch]))
            continue;

        const auto &chInfo = fiffInfo->chs[ch];
        qint32 typeKey;
        if (chInfo.kind == FIFFV_MEG_CH) {
            typeKey = (chInfo.unit == FIFF_UNIT_T_M) ? -1 : -2; // grad vs mag
        } else if (chInfo.kind == FIFFV_STIM_CH) {
            continue; // skip stim channels for auto-scale
        } else {
            typeKey = chInfo.kind;
        }

        double rowSumSq = 0.0;
        for (int s = 0; s < block.cols(); ++s) {
            double v = block(ch, s);
            rowSumSq += v * v;
        }
        typeStats[typeKey].sumSq += rowSumSq;
        typeStats[typeKey].count += static_cast<int>(block.cols());
    }

    // Compute auto-scale: use 3× RMS as the half-range so ~99% of signal fits
    QMap<qint32, float> autoScaleMap;
    for (auto it = typeStats.constBegin(); it != typeStats.constEnd(); ++it) {
        if (it.value().count > 0) {
            double rms = std::sqrt(it.value().sumSq / it.value().count);
            float halfRange = static_cast<float>(3.0 * rms);
            if (halfRange > 0.0f)
                autoScaleMap[it.key()] = halfRange;
        }
    }

    // Apply to the GPU model (and cache in the view for restartChannelView)
    if (m_pChannelDataView && !autoScaleMap.isEmpty()) {
        m_pChannelDataView->setScalingMap(autoScaleMap);
        qWarning() << "[DataWindow] Auto-scale applied:" << autoScaleMap.size() << "channel types.";
        for (auto it = autoScaleMap.constBegin(); it != autoScaleMap.constEnd(); ++it)
            qWarning() << "  typeKey=" << it.key() << "scale=" << it.value();
    }
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

void DataWindow::addMarkerAtSample(int sample)
{
    const QRect viewportRect = markerViewportRect();
    if (viewportRect.isEmpty()) {
        return;
    }

    int clampedSample = sample;
    if (m_pFiffReader && m_pFiffReader->isOpen()) {
        clampedSample = qBound(m_pFiffReader->firstSample(), sample, m_pFiffReader->lastSample());
    }

    PersistentMarker marker;
    marker.id = m_iNextMarkerId++;
    marker.sample = clampedSample;
    marker.color = markerColorForIndex(m_dataMarkers.size());
    marker.line = new DataMarker(this);
    marker.line->setMarkerColor(marker.color);
    marker.line->resize(DATA_MARKER_WIDTH, viewportRect.height());
    marker.line->raise();

    marker.label = new QLabel(this);
    marker.label->setMinimumWidth(90);
    marker.label->setMargin(4);
    marker.label->setAlignment(Qt::AlignHCenter);
    marker.label->setAttribute(Qt::WA_TransparentForMouseEvents);

    const int markerId = marker.id;
    connect(marker.line, &DataMarker::markerMoved, this, [this, markerId]() {
        PersistentMarker *m = findMarker(markerId);
        if (!m || !m->line || !m_pChannelDataView) {
            return;
        }

        const QRect viewportRect = markerViewportRect();
        if (viewportRect.isEmpty()) {
            return;
        }

        const int markerCenterX = m->line->geometry().left() + (DATA_MARKER_WIDTH / 2);
        const int localMarkerX = m_pChannelDataView->mapFrom(this, QPoint(markerCenterX, viewportRect.center().y())).x();
        m->sample = m_pChannelDataView->viewportXToSample(localMarkerX);
        if (m_pFiffReader && m_pFiffReader->isOpen()) {
            m->sample = qBound(m_pFiffReader->firstSample(), m->sample, m_pFiffReader->lastSample());
        }

        setActiveMarker(markerId);
        setMarkerSampleLabel();
        updateTimeRulerMarkers();
    });
    connect(marker.line, &DataMarker::markerPressed, this, [this, markerId]() {
        setActiveMarker(markerId);
    });
    connect(marker.line, &DataMarker::removeRequested, this, [this, markerId]() {
        removeMarker(markerId);
    });

    m_dataMarkers.append(marker);
    setActiveMarker(marker.id);
    syncMarkersToViewport();
}

//*************************************************************************************************************

void DataWindow::removeMarker(int markerId)
{
    for (int i = 0; i < m_dataMarkers.size(); ++i) {
        if (m_dataMarkers.at(i).id != markerId) {
            continue;
        }

        PersistentMarker marker = m_dataMarkers.takeAt(i);
        if (marker.line) {
            marker.line->deleteLater();
        }
        if (marker.label) {
            marker.label->deleteLater();
        }
        break;
    }

    if (m_iActiveMarkerId == markerId) {
        m_iActiveMarkerId = m_dataMarkers.isEmpty() ? -1 : m_dataMarkers.last().id;
    }

    setMarkerSampleLabel();
    updateTimeRulerMarkers();
}

//*************************************************************************************************************

void DataWindow::removeMarkerNearSample(int sample)
{
    if (m_dataMarkers.isEmpty()) {
        return;
    }

    int nearestMarkerId = -1;
    int nearestDistance = std::numeric_limits<int>::max();
    for (const PersistentMarker &marker : std::as_const(m_dataMarkers)) {
        const int distance = qAbs(marker.sample - sample);
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearestMarkerId = marker.id;
        }
    }

    removeMarker(nearestMarkerId);
}

//*************************************************************************************************************

void DataWindow::clearMarkers()
{
    while (!m_dataMarkers.isEmpty()) {
        PersistentMarker marker = m_dataMarkers.takeLast();
        if (marker.line) {
            marker.line->deleteLater();
        }
        if (marker.label) {
            marker.label->deleteLater();
        }
    }

    m_iActiveMarkerId = -1;
    updateTimeRulerMarkers();
    m_pMainWindow->eventWindow()->getEventModel()->setCurrentMarkerPos(m_iCurrentScrollSample);
}

//*************************************************************************************************************

void DataWindow::setActiveMarker(int markerId)
{
    m_iActiveMarkerId = markerId;
    setMarkerSampleLabel();
}

//*************************************************************************************************************

int DataWindow::activeMarkerSample() const
{
    const PersistentMarker *marker = findMarker(m_iActiveMarkerId);
    if (marker) {
        return marker->sample;
    }

    return m_pChannelDataView ? m_pChannelDataView->firstVisibleSample() : m_iCurrentScrollSample;
}

//*************************************************************************************************************

void DataWindow::syncMarkersToViewport()
{
    const QRect viewportRect = markerViewportRect();
    if (viewportRect.isEmpty()) {
        return;
    }

    for (PersistentMarker &marker : m_dataMarkers) {
        if (!marker.line || !marker.label) {
            continue;
        }

        QRegion region(viewportRect);
        marker.line->setMovementBoundary(region);

        bool visible = true;
        int markerX = viewportRect.left();
        if (m_pChannelDataView) {
            const int localViewportX = m_pChannelDataView->sampleToViewportX(marker.sample);
            markerX = m_pChannelDataView->mapTo(this, QPoint(localViewportX, 0)).x() - (DATA_MARKER_WIDTH / 2);
            visible = marker.sample >= m_pChannelDataView->firstVisibleSample()
                   && marker.sample <= (m_pChannelDataView->firstVisibleSample() + m_pChannelDataView->visibleSampleCount());
        }

        const int maxMarkerX = qMax(viewportRect.left(), viewportRect.right() - DATA_MARKER_WIDTH + 1);
        markerX = qBound(viewportRect.left(), markerX, maxMarkerX);

        {
            QSignalBlocker blocker(marker.line);
            marker.line->setGeometry(markerX,
                                     viewportRect.y(),
                                     DATA_MARKER_WIDTH,
                                     viewportRect.height());
        }

        marker.line->setVisible(visible);
        marker.line->raise();
        marker.label->setVisible(visible);
    }

    setMarkerSampleLabel();
    updateTimeRulerMarkers();
}

//*************************************************************************************************************

void DataWindow::updateTimeRulerMarkers()
{
    if (!m_pChannelDataView) {
        return;
    }

    QVector<DISPLIB::TimeRulerReferenceMark> marks;
    marks.reserve(m_dataMarkers.size());
    for (const PersistentMarker &marker : std::as_const(m_dataMarkers)) {
        QString label = QStringLiteral("M%1").arg(marker.id);
        if (marker.id == m_iActiveMarkerId) {
            label.append(QLatin1Char('*'));
        }
        marks.append({marker.sample, marker.color, label});
    }

    m_pChannelDataView->setReferenceMarkers(marks);
}

//*************************************************************************************************************

DataWindow::PersistentMarker* DataWindow::findMarker(int markerId)
{
    for (PersistentMarker &marker : m_dataMarkers) {
        if (marker.id == markerId) {
            return &marker;
        }
    }

    return nullptr;
}

//*************************************************************************************************************

const DataWindow::PersistentMarker* DataWindow::findMarker(int markerId) const
{
    for (const PersistentMarker &marker : m_dataMarkers) {
        if (marker.id == markerId) {
            return &marker;
        }
    }

    return nullptr;
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

void DataWindow::setUserDefinedFilter(const QSharedPointer<SessionFilter>& filterDefinition)
{
    m_pUserDefinedFilter = filterDefinition;

    if (m_pMainWindow && m_pMainWindow->chInfoWindow() && m_pMainWindow->chInfoWindow()->getDataModel()) {
        m_pMainWindow->chInfoWindow()->getDataModel()->assignedOperatorsChanged(
            browserAssignedOperators(fiffInfo(), m_pUserDefinedFilter));
    }

    if (m_pFiffReader && m_pFiffReader->isOpen()) {
        restartChannelView(m_iCurrentScrollSample, false);
    }
}

//*************************************************************************************************************

void DataWindow::clearUserDefinedFilter()
{
    m_pUserDefinedFilter.clear();

    if (m_pMainWindow && m_pMainWindow->chInfoWindow() && m_pMainWindow->chInfoWindow()->getDataModel()) {
        m_pMainWindow->chInfoWindow()->getDataModel()->assignedOperatorsChanged({});
    }

    if (m_pFiffReader && m_pFiffReader->isOpen()) {
        restartChannelView(m_iCurrentScrollSample, false);
    }
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

    m_pChannelDataView = new RawView("DataWindow", this);
    m_pChannelDataView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(m_pChannelDataView, &DISPLIB::ChannelDataView::scrollPositionChanged,
            this, &DataWindow::onChannelViewScrollChanged);
    connect(m_pChannelDataView, &DISPLIB::ChannelDataView::sampleRangeSelected,
            this, &DataWindow::annotationRangeSelected);
    connect(m_pChannelDataView, &DISPLIB::ChannelDataView::annotationBoundaryMoved,
            this, &DataWindow::annotationBoundaryMoved);
    connect(m_pChannelDataView, &DISPLIB::ChannelDataView::referenceMarkerAddRequested,
            this, &DataWindow::addMarkerAtSample);
    connect(m_pChannelDataView, &DISPLIB::ChannelDataView::referenceMarkerRemoveRequested,
            this, &DataWindow::removeMarkerNearSample);
    connect(m_pChannelDataView, &DISPLIB::ChannelDataView::referenceMarkersClearRequested,
            this, &DataWindow::clearMarkers);

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
    computeAutoScale();
    restartChannelView(m_pFiffReader->firstSample(), true);

    return true;
}

//=============================================================================================================

bool DataWindow::loadFiffBuffer(const QByteArray &data, const QString &displayName)
{
    if (!m_pFiffReader->openBuffer(data, displayName)) {
        return false;
    }

    m_sFiffFilePath = displayName;

    auto fiffInfo = m_pFiffReader->fiffInfo();
    m_iStimChannel = fiffInfo ? resolveStimChannelIndex(*fiffInfo) : -1;
    rebuildVirtualChannels();
    computeAutoScale();
    restartChannelView(m_pFiffReader->firstSample(), true);

    return true;
}

//=============================================================================================================

void DataWindow::onBlockLoaded(const Eigen::MatrixXd &data, int firstSample)
{
    m_bLoadingBlock = false;

    if (data.rows() == 0 || data.cols() == 0)
        return;

    const Eigen::MatrixXd filteredData = applyUserDefinedFilter(data);
    const Eigen::MatrixXd displayData = appendVirtualChannels(filteredData);
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
        const int primaryChannel = channelIndexByName.value(definition.primaryChannel.trimmed(), -1);
        QVector<int> referenceChannels;
        QVector<double> referenceWeights;
        referenceChannels.reserve(definition.referenceChannels.size());
        referenceWeights.reserve(definition.referenceChannels.size());
        for(int referenceIndex = 0; referenceIndex < definition.referenceChannels.size(); ++referenceIndex) {
            const QString& referenceName = definition.referenceChannels.at(referenceIndex);
            const int referenceChannel = channelIndexByName.value(referenceName.trimmed(), -1);
            if(referenceChannel >= 0 && referenceChannel != primaryChannel && !referenceChannels.contains(referenceChannel)) {
                referenceChannels.append(referenceChannel);
                referenceWeights.append(referenceIndex < definition.referenceWeights.size()
                                        ? definition.referenceWeights.at(referenceIndex)
                                        : 1.0);
            }
        }

        const bool invalidDefinition =
            primaryChannel < 0
            || referenceChannels.isEmpty()
            || (definition.kind == VirtualChannelKind::Bipolar && referenceChannels.size() != 1);

        if(invalidDefinition) {
            qWarning() << "DataWindow: could not resolve virtual channel"
                       << definition.name
                       << definition.primaryChannel
                       << definition.referenceChannels;
            continue;
        }

        const auto& primaryChannelInfo = fiffInfo->chs.at(primaryChannel);

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
        resolvedChannel.kind = definition.kind;
        resolvedChannel.primaryChannel = primaryChannel;
        resolvedChannel.referenceChannels = referenceChannels;
        resolvedChannel.referenceWeights = referenceWeights;
        resolvedChannel.displayInfo.name = definition.name;
        bool sameKind = true;
        float amplitudeMax = amplitudeForChannel(primaryChannelInfo);
        for(int referenceChannel : referenceChannels) {
            const auto& referenceChannelInfo = fiffInfo->chs.at(referenceChannel);
            sameKind = sameKind && (referenceChannelInfo.kind == primaryChannelInfo.kind);
            amplitudeMax = qMax(amplitudeMax, amplitudeForChannel(referenceChannelInfo));
        }
        resolvedChannel.displayInfo.typeLabel = sameKind
            ? typeLabelForKind(primaryChannelInfo.kind)
            : QStringLiteral("MISC");
        resolvedChannel.displayInfo.color = colorForKind(primaryChannelInfo.kind).darker(115);
        resolvedChannel.displayInfo.amplitudeMax = amplitudeMax;
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
        if(virtualChannel.primaryChannel < 0
           || virtualChannel.primaryChannel >= data.rows()
           || virtualChannel.referenceChannels.isEmpty()) {
            displayData.row(data.rows() + row).setZero();
            continue;
        }

        Eigen::RowVectorXd referenceSignal = Eigen::RowVectorXd::Zero(data.cols());
        bool validReferences = true;
        for(int index = 0; index < virtualChannel.referenceChannels.size(); ++index) {
            const int referenceChannel = virtualChannel.referenceChannels.at(index);
            if(referenceChannel < 0 || referenceChannel >= data.rows()) {
                validReferences = false;
                break;
            }

            const double weight = virtualChannel.kind == VirtualChannelKind::WeightedReference
                ? virtualChannel.referenceWeights.value(index, 1.0)
                : 1.0;
            referenceSignal += weight * data.row(referenceChannel);
        }

        if(!validReferences) {
            displayData.row(data.rows() + row).setZero();
            continue;
        }

        if(virtualChannel.kind == VirtualChannelKind::AverageReference) {
            referenceSignal /= static_cast<double>(virtualChannel.referenceChannels.size());
        }

        displayData.row(data.rows() + row) =
            data.row(virtualChannel.primaryChannel) - referenceSignal;
    }

    return displayData;
}

//=============================================================================================================

Eigen::MatrixXd DataWindow::applyUserDefinedFilter(const Eigen::MatrixXd &data) const
{
    if (m_pUserDefinedFilter.isNull() || !m_pFiffReader || !m_pFiffReader->fiffInfo()) {
        return data;
    }

    const auto info = m_pFiffReader->fiffInfo();
    return m_pUserDefinedFilter->applyToMatrix(data, *info);
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
        clearMarkers();
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
    syncMarkersToViewport();
    setRangeSampleLabels();
    setMarkerSampleLabel();
    ui->m_label_sampleMin->show();
    ui->m_label_sampleMax->show();

    m_bLoadingBlock = false;
    scheduleNextLoad();
}

//=============================================================================================================

void DataWindow::onChannelViewScrollChanged(int sample)
{
    m_iCurrentScrollSample = sample;
    setRangeSampleLabels();
    syncMarkersToViewport();
    if(m_dataMarkers.isEmpty()) {
        m_pMainWindow->eventWindow()->getEventModel()->setCurrentMarkerPos(sample);
    }

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
    if(!isFiffFileLoaded()) {
        ui->m_label_sampleMin->hide();
        ui->m_label_sampleMax->hide();
    }
}


//*************************************************************************************************************

void DataWindow::initLabels()
{
    //Setup range samples
    if (m_pChannelDataView) {
        connect(m_pChannelDataView, &DISPLIB::ChannelDataView::scrollPositionChanged,
                this, &DataWindow::setRangeSampleLabels);
        connect(m_pChannelDataView, &DISPLIB::ChannelDataView::scrollPositionChanged,
                this, &DataWindow::setMarkerSampleLabel);
    }
}


//*************************************************************************************************************

void DataWindow::resizeEvent(QResizeEvent * event)
{
    //On every resize update marker position
    syncMarkersToViewport();

    //On every resize set sample informaiton
    setRangeSampleLabels();

    return QWidget::resizeEvent(event);
}


//*************************************************************************************************************

void DataWindow::keyPressEvent(QKeyEvent* event)
{
    if (m_pChannelDataView) {
        switch(event->key()) {
        case Qt::Key_Left:
            if (event->modifiers() & Qt::ShiftModifier)
                m_pChannelDataView->scrollToSample(m_pChannelDataView->firstVisibleSample() - m_pChannelDataView->visibleSampleCount(), false);
            else
                m_pChannelDataView->scrollToSample(m_pChannelDataView->firstVisibleSample() - static_cast<int>(RAWVIEW_KEYBOARD_SCROLL_STEP * m_pChannelDataView->scrollSpeedFactor()), false);
            break;
        case Qt::Key_Right:
            if (event->modifiers() & Qt::ShiftModifier)
                m_pChannelDataView->scrollToSample(m_pChannelDataView->firstVisibleSample() + m_pChannelDataView->visibleSampleCount(), false);
            else
                m_pChannelDataView->scrollToSample(m_pChannelDataView->firstVisibleSample() + static_cast<int>(RAWVIEW_KEYBOARD_SCROLL_STEP * m_pChannelDataView->scrollSpeedFactor()), false);
            break;
        case Qt::Key_Plus:
        case Qt::Key_Equal:
            // Increase amplitude scale (zoom in vertically)
            m_pChannelDataView->setZoom(m_pChannelDataView->zoom() * 1.25);
            break;
        case Qt::Key_Minus:
            // Decrease amplitude scale (zoom out vertically)
            m_pChannelDataView->setZoom(m_pChannelDataView->zoom() / 1.25);
            break;
        case Qt::Key_Home:
            // Decrease visible time window (show less time = zoom in)
            m_pChannelDataView->setWindowSize(m_pChannelDataView->windowSize() / 1.25f);
            break;
        case Qt::Key_End:
            // Increase visible time window (show more time = zoom out)
            m_pChannelDataView->setWindowSize(m_pChannelDataView->windowSize() * 1.25f);
            break;
        case Qt::Key_B:
            // Toggle butterfly mode
            m_pChannelDataView->setButterflyMode(!m_pChannelDataView->butterflyMode());
            break;
        case Qt::Key_D:
            if (event->modifiers() & Qt::ControlModifier) {
                ui->m_tableView_rawTableView->clearSelection();
            } else {
                // Toggle DC removal
                m_pChannelDataView->setRemoveDC(!m_pChannelDataView->model()->removeDC());
            }
            break;
        case Qt::Key_S:
            // Toggle scalebars
            m_pChannelDataView->setScalebarsVisible(!m_pChannelDataView->scalebarsVisible());
            break;
        case Qt::Key_X:
            // Toggle crosshair cursor
            m_pChannelDataView->setCrosshairEnabled(!m_pChannelDataView->crosshairEnabled());
            break;
        case Qt::Key_E:
            // Toggle event marker visibility
            m_pChannelDataView->setEventsVisible(!m_pChannelDataView->eventsVisible());
            break;
        case Qt::Key_G:
            // Toggle epoch grid lines
            m_pChannelDataView->setEpochMarkersVisible(!m_pChannelDataView->epochMarkersVisible());
            break;
        case Qt::Key_A:
            if (event->modifiers() & Qt::ShiftModifier) {
                // Toggle annotation span visibility
                m_pChannelDataView->setAnnotationsVisible(!m_pChannelDataView->annotationsVisible());
            }
            break;
        case Qt::Key_O:
            // Toggle overview bar visibility
            m_pChannelDataView->setOverviewBarVisible(!m_pChannelDataView->overviewBarVisible());
            break;
        case Qt::Key_BracketRight:
            // Increase scroll speed
            m_pChannelDataView->setScrollSpeedFactor(m_pChannelDataView->scrollSpeedFactor() * 1.5f);
            break;
        case Qt::Key_BracketLeft:
            // Decrease scroll speed
            m_pChannelDataView->setScrollSpeedFactor(m_pChannelDataView->scrollSpeedFactor() / 1.5f);
            break;
        case Qt::Key_T:
            // Toggle time format
            if (m_pChannelDataView) {
                emit timeFormatToggleRequested();
            }
            break;
        case Qt::Key_Question:
            // Show keyboard shortcut help
            {
                QMessageBox::information(this, tr("Keyboard Shortcuts"),
                    tr("<b>Navigation:</b><br>"
                       "← / → — Scroll left/right (¼ page)<br>"
                       "Shift+← / → — Scroll left/right (full page)<br>"
                       "Home / End — Decrease/increase time window<br>"
                       "+/= — Increase amplitude scale<br>"
                       "- — Decrease amplitude scale<br>"
                       "<br>"
                       "<b>Display:</b><br>"
                       "B — Toggle butterfly mode<br>"
                       "D — Toggle DC removal<br>"
                       "E — Toggle event markers<br>"
                       "G — Toggle epoch grid lines<br>"
                       "O — Toggle overview bar<br>"
                       "S — Toggle scalebars<br>"
                       "T — Toggle time format (seconds / clock)<br>"
                       "X — Toggle crosshair cursor<br>"
                       "Shift+A — Toggle annotation spans<br>"
                       "[ / ] — Decrease/increase scroll speed<br>"
                       "Ctrl+D — Clear channel selection<br>"
                       "? — Show this help<br>"
                       "<br>"
                       "<b>Mouse (Data View):</b><br>"
                       "Left-drag — Pan through time<br>"
                       "Right-drag — Ruler measurement<br>"
                       "Alt+Left-drag — Pan (alternative)<br>"
                       "Double-click — Toggle channel bad/good<br>"
                       "Scroll wheel — Scroll through channels<br>"
                       "<br>"
                       "<b>Annotations:</b><br>"
                       "Enable Annotation Mode from toolbar<br>"
                       "Right-drag — Select time range for annotation<br>"
                       "Drag boundary — Resize annotation span<br>"
                       "<br>"
                       "<b>File:</b><br>"
                       "Ctrl+O — Open file<br>"
                       "Ctrl+S — Save file"));
            }
            break;
        default:
            break;
        }
    }

    return QWidget::keyPressEvent(event);
}


//*************************************************************************************************************

bool DataWindow::eventFilter(QObject *object, QEvent *event)
{    
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
    if(!m_pMainWindow->ensureLegacyRawModelLoaded(QStringLiteral("Raw Browser Context Menu"))) {
        return;
    }

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
    ui->m_horizontalSpacer_Min->setFixedWidth(0);
    ui->m_horizontalSpacer_Max->setFixedWidth(0);

    const int minSampleRange = m_pChannelDataView ? m_pChannelDataView->firstVisibleSample() : 0;
    const int maxSampleRange = minSampleRange
        + (m_pChannelDataView ? m_pChannelDataView->visibleSampleCount() : 0);

    //Set values as string
    auto fiffInfo = m_pFiffReader ? m_pFiffReader->fiffInfo() : QSharedPointer<FIFFLIB::FiffInfo>();
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
    const QRect viewportRect = markerViewportRect();
    if(viewportRect.isEmpty()) {
        return;
    }

    auto fiffInfo = m_pFiffReader ? m_pFiffReader->fiffInfo() : QSharedPointer<FIFFLIB::FiffInfo>();
    if (!fiffInfo)
        return;

    for (int i = 0; i < m_dataMarkers.size(); ++i) {
        PersistentMarker &marker = m_dataMarkers[i];
        if (!marker.line || !marker.label) {
            continue;
        }

        const bool visible = marker.line->isVisible();
        marker.label->setVisible(visible);
        if (!visible) {
            continue;
        }

        const int markerCenterX = marker.line->geometry().left() + (DATA_MARKER_WIDTH / 2);
        const int currentMilliseconds = static_cast<int>((static_cast<double>(marker.sample) / fiffInfo->sfreq) * 1000.0);
        QString markerText = QString("%1 / %2 sec")
                                 .arg(QString::number(marker.sample))
                                 .arg(QString::number(static_cast<double>(currentMilliseconds) / 1000.0, 'g'));
        if (marker.id == m_iActiveMarkerId) {
            markerText.prepend(QStringLiteral("Active "));
        }

        marker.label->setText(markerText);
        marker.label->adjustSize();
        marker.label->raise();

        const int labelX = qBound(viewportRect.left(),
                                  markerCenterX - (marker.label->width() / 2),
                                  qMax(viewportRect.left(),
                                       viewportRect.right() - marker.label->width() + 1));
        const int labelStack = i % 2;
        const int labelY = qMax(0,
                                viewportRect.top() - marker.label->height() - 4 - labelStack * (marker.label->height() + 2));
        marker.label->move(labelX, labelY);

        marker.label->setStyleSheet(QStringLiteral(
            "QLabel {"
            " color: %1;"
            " background-color: rgba(255, 255, 255, 220);"
            " border: 1px solid %1;"
            " border-radius: 3px;"
            " font-weight: %2;"
            " }")
            .arg(marker.color.name())
            .arg(marker.id == m_iActiveMarkerId ? QStringLiteral("700")
                                                : QStringLiteral("500")));
    }

    m_pMainWindow->eventWindow()->getEventModel()->setCurrentMarkerPos(activeMarkerSample());
}


//*************************************************************************************************************

void DataWindow::moveMarkerToSample(int sample)
{
    if (m_dataMarkers.isEmpty()) {
        addMarkerAtSample(sample);
        return;
    }

    PersistentMarker *marker = findMarker(m_iActiveMarkerId);
    if (!marker) {
        addMarkerAtSample(sample);
        return;
    }

    int clampedSample = sample;
    if (m_pFiffReader && m_pFiffReader->isOpen()) {
        clampedSample = qBound(m_pFiffReader->firstSample(), sample, m_pFiffReader->lastSample());
    }
    marker->sample = clampedSample;
    syncMarkersToViewport();
}

//*************************************************************************************************************

void DataWindow::updateMarkerPosition()
{
    syncMarkersToViewport();
}


//*************************************************************************************************************

QRect DataWindow::markerViewportRect() const
{
    if(m_pChannelDataView) {
        const QRect localViewportRect = m_pChannelDataView->signalViewportRect();
        if(!localViewportRect.isEmpty()) {
            return QRect(m_pChannelDataView->mapTo(const_cast<DataWindow*>(this), localViewportRect.topLeft()),
                         localViewportRect.size());
        }
    }

    return ui->m_tableView_rawTableView->geometry();
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
