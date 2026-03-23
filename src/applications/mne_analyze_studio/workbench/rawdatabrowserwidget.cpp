//=============================================================================================================
/**
 * @file     rawdatabrowserwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  dev
 * @date     March, 2026
 *
 * @brief    Implements the studio raw FIFF browser widget.
 */

#include "rawdatabrowserwidget.h"

#include "mnebrowserrawdelegate.h"

#include <QEvent>
#include <QHeaderView>
#include <QDebug>
#include <QLabel>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPushButton>
#include <QScroller>
#include <QScrollBar>
#include <QTableView>
#include <QVBoxLayout>

using namespace MNEANALYZESTUDIO;

RawDataBrowserWidget::RawDataBrowserWidget(QWidget* parent)
: QWidget(parent)
, m_file(new QFile(this))
, m_zoomOutButton(new QPushButton("-"))
, m_zoomResetButton(new QPushButton("1:1"))
, m_zoomInButton(new QPushButton("+"))
, m_tableView(new QTableView)
, m_markerLine(new QFrame)
, m_rawModel(new MNEBROWSE::RawModel(this))
, m_delegate(new MneBrowseRawDelegate(this))
, m_markerSample(-1)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    QHBoxLayout* controlLayout = new QHBoxLayout;
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->setSpacing(8);

    m_zoomOutButton->setFixedWidth(34);
    m_zoomResetButton->setFixedWidth(56);
    m_zoomInButton->setFixedWidth(34);
    m_tableView->setModel(m_rawModel);
    m_tableView->setItemDelegate(m_delegate);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->verticalHeader()->setDefaultSectionSize(m_delegate->plotHeight());
    m_tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_tableView->setColumnHidden(2, true);
    m_tableView->setWordWrap(false);
    m_tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_tableView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    QScroller::grabGesture(m_tableView, QScroller::LeftMouseButtonGesture);
    m_tableView->viewport()->installEventFilter(this);
    m_tableView->viewport()->setMouseTracking(true);
    connect(m_tableView->horizontalScrollBar(), &QScrollBar::valueChanged,
            m_rawModel, &MNEBROWSE::RawModel::updateScrollPos);
    connect(m_tableView->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, [this]() {
                updateMarkerOverlay();
                publishBrowserState(false);
            });
    connect(m_tableView->verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this]() { updateMarkerOverlay(); });
    connect(m_zoomOutButton, &QPushButton::clicked, this, [this]() { adjustZoom(0.8); });
    connect(m_zoomResetButton, &QPushButton::clicked, this, [this]() {
        m_delegate->setPixelsPerSample(1.0);
        updateScrollConfiguration();
    });
    connect(m_zoomInButton, &QPushButton::clicked, this, [this]() { adjustZoom(1.25); });

    m_markerLine->setParent(m_tableView->viewport());
    m_markerLine->setObjectName("rawDataBrowserMarkerLine");
    m_markerLine->setFrameShape(QFrame::VLine);
    m_markerLine->setLineWidth(2);
    m_markerLine->setStyleSheet("background: #f78166;");
    m_markerLine->hide();

    controlLayout->addStretch(1);
    controlLayout->addWidget(m_zoomOutButton);
    controlLayout->addWidget(m_zoomResetButton);
    controlLayout->addWidget(m_zoomInButton);

    layout->addLayout(controlLayout);
    layout->addWidget(m_tableView, 1);
}

bool RawDataBrowserWidget::loadFile(const QString& filePath)
{
    m_filePath = filePath;
    if(m_file->isOpen()) {
        m_file->close();
    }

    m_file->setFileName(filePath);

    if(!m_rawModel->loadFiffData(m_file)) {
        qWarning() << "[RawDataBrowserWidget] Failed to load raw model for" << filePath;
        emit outputMessage(QString("Raw browser failed to load %1").arg(filePath));
        return false;
    }

    updateScrollConfiguration();
    if(m_tableView->selectionModel() && m_rawModel->rowCount() > 0) {
        m_tableView->selectRow(0);
    }
    setMarkerSample(m_rawModel->firstSample());
    publishBrowserState(true);

    const auto info = m_rawModel->m_pFiffInfo;
    qInfo() << "[RawDataBrowserWidget] Loaded raw browser for" << filePath
            << "with" << (info ? info->nchan : 0) << "channels";

    return true;
}

QString RawDataBrowserWidget::filePath() const
{
    return m_filePath;
}

QString RawDataBrowserWidget::summaryText() const
{
    if(!m_rawModel->m_pFiffInfo) {
        return "No raw browser loaded.";
    }

    const int totalSamples = std::max<qint32>(0, m_rawModel->lastSample() - m_rawModel->firstSample() + 1);
    return QString("Raw Data Browser: %1 | Channels: %2 | Sampling rate: %3 Hz | Samples: %4 | First sample: %5 | Last sample: %6")
        .arg(m_filePath)
        .arg(m_rawModel->m_pFiffInfo->nchan)
        .arg(m_rawModel->m_pFiffInfo->sfreq, 0, 'f', 2)
        .arg(totalSamples)
        .arg(m_rawModel->firstSample())
        .arg(m_rawModel->lastSample());
}

QString RawDataBrowserWidget::stateText() const
{
    return QString("%1 | %2 | Zoom: %3 px/sample")
        .arg(currentVisibleRangeText(), currentCursorText())
        .arg(QString::number(m_delegate->pixelsPerSample(), 'f', 2));
}

bool RawDataBrowserWidget::gotoSample(int sample)
{
    if(!m_rawModel->m_pFiffInfo || m_rawModel->m_pFiffInfo->sfreq <= 0.0) {
        return false;
    }

    const int boundedSample = qBound(m_rawModel->firstSample(), sample, m_rawModel->lastSample());
    const int scrollValue = static_cast<int>((boundedSample - m_rawModel->firstSample()) * m_delegate->pixelsPerSample());
    m_tableView->horizontalScrollBar()->setValue(scrollValue);
    setMarkerSample(boundedSample);
    return true;
}

bool RawDataBrowserWidget::setZoomPixelsPerSample(double pixelsPerSample)
{
    if(pixelsPerSample <= 0.0) {
        return false;
    }

    m_delegate->setPixelsPerSample(pixelsPerSample);
    updateScrollConfiguration();
    emit outputMessage(QString("Raw browser zoom set to %1 px/sample")
                       .arg(QString::number(m_delegate->pixelsPerSample(), 'f', 2)));
    return true;
}

double RawDataBrowserWidget::pixelsPerSample() const
{
    return m_delegate->pixelsPerSample();
}

int RawDataBrowserWidget::cursorSample() const
{
    return m_markerSample;
}

void RawDataBrowserWidget::updateScrollConfiguration()
{
    m_tableView->resizeColumnToContents(0);

    const int visibleSampleWidth = qMax(m_tableView->viewport()->width(), 800);
    const int totalSamples = std::max<qint32>(0, m_rawModel->lastSample() - m_rawModel->firstSample() + 1);
    const int plotWidth = qMax(visibleSampleWidth * 2,
                               static_cast<int>(totalSamples * m_delegate->pixelsPerSample()));

    m_tableView->setColumnWidth(1, plotWidth);
    m_tableView->horizontalScrollBar()->setSingleStep(25);
    m_tableView->horizontalScrollBar()->setPageStep(visibleSampleWidth / 2);
    updateMarkerOverlay();
    publishBrowserState(false);
}

QString RawDataBrowserWidget::formatSeconds(int sample) const
{
    const double seconds = sample / m_rawModel->m_pFiffInfo->sfreq;
    return QString::number(seconds, 'f', 3);
}

bool RawDataBrowserWidget::eventFilter(QObject* watched, QEvent* event)
{
    if(watched == m_tableView->viewport()) {
        if(event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            const int dataColumnX = m_tableView->columnViewportPosition(1);
            if(mouseEvent->position().x() >= dataColumnX) {
                const double pixelsPerSample = m_delegate->pixelsPerSample();
                const int relativeX = static_cast<int>(mouseEvent->position().x()) - dataColumnX;
                const int visibleStart = m_rawModel->firstSample()
                                         + static_cast<int>(m_tableView->horizontalScrollBar()->value() / pixelsPerSample);
                setMarkerSample(visibleStart + static_cast<int>(relativeX / pixelsPerSample));
                return true;
            }
        } else if(event->type() == QEvent::Resize) {
            updateMarkerOverlay();
        }
    }

    return QWidget::eventFilter(watched, event);
}

void RawDataBrowserWidget::setMarkerSample(int sample)
{
    if(!m_rawModel->m_pFiffInfo || m_rawModel->m_pFiffInfo->sfreq <= 0.0) {
        return;
    }

    const int maxSample = std::max<qint32>(m_rawModel->firstSample(), m_rawModel->lastSample());
    m_markerSample = qBound(m_rawModel->firstSample(), sample, maxSample);
    updateMarkerOverlay();
    publishBrowserState(false);
}

void RawDataBrowserWidget::updateMarkerOverlay()
{
    if(m_markerSample < 0) {
        m_markerLine->hide();
        return;
    }

    const double pixelsPerSample = m_delegate->pixelsPerSample();
    const int dataColumnX = m_tableView->columnViewportPosition(1);
    const int scrollValue = m_tableView->horizontalScrollBar()->value();
    const int xPosition = dataColumnX
                          + static_cast<int>((m_markerSample - m_rawModel->firstSample()) * pixelsPerSample)
                          - scrollValue;

    if(xPosition < dataColumnX || xPosition > m_tableView->viewport()->width()) {
        m_markerLine->hide();
        return;
    }

    m_markerLine->setGeometry(xPosition, 0, 2, m_tableView->viewport()->height());
    m_markerLine->show();
    m_markerLine->raise();
}

void RawDataBrowserWidget::adjustZoom(double factor)
{
    setZoomPixelsPerSample(m_delegate->pixelsPerSample() * factor);
}

QString RawDataBrowserWidget::currentVisibleRangeText() const
{
    if(!m_rawModel->m_pFiffInfo || m_rawModel->m_pFiffInfo->sfreq <= 0.0) {
        return "Visible range unavailable";
    }

    const double pixelsPerSample = m_delegate->pixelsPerSample();
    const int minSample = m_rawModel->firstSample() + static_cast<int>(m_tableView->horizontalScrollBar()->value() / pixelsPerSample);
    const int maxSample = minSample + static_cast<int>(m_tableView->viewport()->width() / pixelsPerSample);

    return QString("Visible range: %1 to %2 samples | %3 s to %4 s")
        .arg(minSample)
        .arg(maxSample)
        .arg(formatSeconds(minSample))
        .arg(formatSeconds(maxSample));
}

QString RawDataBrowserWidget::currentCursorText() const
{
    if(m_markerSample < 0 || !m_rawModel->m_pFiffInfo || m_rawModel->m_pFiffInfo->sfreq <= 0.0) {
        return "Cursor unavailable";
    }

    return QString("Cursor: %1 | %2 s")
        .arg(m_markerSample)
        .arg(formatSeconds(m_markerSample));
}

void RawDataBrowserWidget::publishBrowserState(bool appendToOutput)
{
    const QString stateMessage = QString("%1 | %2")
                                     .arg(currentVisibleRangeText(), currentCursorText());
    emit statusMessage(stateMessage);

    if(appendToOutput && m_rawModel->m_pFiffInfo) {
        const int totalSamples = std::max<qint32>(0, m_rawModel->lastSample() - m_rawModel->firstSample() + 1);
        emit outputMessage(QString("Raw Data Browser: %1").arg(m_filePath));
        emit outputMessage(QString("Channels: %1 | Sampling rate: %2 Hz | Samples: %3 | First sample: %4 | Last sample: %5")
                           .arg(m_rawModel->m_pFiffInfo->nchan)
                           .arg(m_rawModel->m_pFiffInfo->sfreq, 0, 'f', 2)
                           .arg(totalSamples)
                           .arg(m_rawModel->firstSample())
                           .arg(m_rawModel->lastSample()));
    }
}
