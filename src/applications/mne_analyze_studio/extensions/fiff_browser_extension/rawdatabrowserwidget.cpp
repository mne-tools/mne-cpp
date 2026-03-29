//=============================================================================================================
/**
 * @file     rawdatabrowserwidget.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  2.1.0
 * @date     March, 2026
 *
 * @brief    Implements the embedded raw FIFF browser widget for the FIFF extension.
 */

#include "rawdatabrowserwidget.h"

#include "mnebrowserrawdelegate.h"

#include <extensionviewfactoryregistry.h>
#include <iextensionviewfactory.h>

#include <QEvent>
#include <QFileInfo>
#include <QHeaderView>
#include <QDebug>
#include <QLabel>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QPushButton>
#include <QScroller>
#include <QScrollBar>
#include <QTableView>
#include <QVBoxLayout>

using namespace MNEANALYZESTUDIO;

namespace
{

class FiffBrowserExtensionViewFactory final : public IExtensionViewFactory
{
public:
    QString widgetType() const override
    {
        return "embedded_raw_browser";
    }

    QWidget* createView(const QJsonObject& sessionDescriptor, QWidget* parent) const override
    {
        RawDataBrowserWidget* rawBrowser = new RawDataBrowserWidget(parent);
        if(!rawBrowser->loadFile(sessionDescriptor.value("file").toString())) {
            delete rawBrowser;
            return nullptr;
        }

        return rawBrowser;
    }
};

class FiffBrowserExtensionViewFactoryRegistration
{
public:
    FiffBrowserExtensionViewFactoryRegistration()
    {
        ExtensionViewFactoryRegistry::instance().registerFactory(&m_factory);
    }

private:
    FiffBrowserExtensionViewFactory m_factory;
};

FiffBrowserExtensionViewFactoryRegistration s_fiffBrowserFactoryRegistration;

} // anonymous namespace

// =============================================================================

/**
 * @brief Horizontal time ruler drawn below the signal traces.
 *
 * Displays evenly-spaced time ticks and labels in seconds, automatically
 * choosing a tick interval that keeps labels readable at any zoom level.
 */
class MNEANALYZESTUDIO::TimeRulerWidget : public QWidget
{
public:
    explicit TimeRulerWidget(QWidget* parent = nullptr)
    : QWidget(parent)
    {
        setFixedHeight(22);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void update(double sfreq, int firstSample, double pixelsPerSample,
                int scrollValue, int labelColumnWidth)
    {
        m_sfreq = sfreq;
        m_firstSample = firstSample;
        m_pixelsPerSample = pixelsPerSample;
        m_scrollValue = scrollValue;
        m_labelColumnWidth = labelColumnWidth;
        QWidget::update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)
        if(m_sfreq <= 0.0 || m_pixelsPerSample <= 0.0) {
            return;
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);

        const QColor tickColor = palette().color(QPalette::Mid);
        const QColor textColor = palette().color(QPalette::Text);
        painter.setPen(tickColor);

        // Draw top border line
        painter.drawLine(0, 0, width(), 0);

        const int dataAreaLeft = m_labelColumnWidth;
        const int dataAreaWidth = width() - dataAreaLeft;
        if(dataAreaWidth <= 0) {
            return;
        }

        // Determine visible time range
        const double firstVisibleSample = m_firstSample + m_scrollValue / m_pixelsPerSample;
        const double lastVisibleSample  = firstVisibleSample + dataAreaWidth / m_pixelsPerSample;
        const double firstVisibleSec    = firstVisibleSample / m_sfreq;
        const double lastVisibleSec     = lastVisibleSample  / m_sfreq;
        const double visibleDuration    = lastVisibleSec - firstVisibleSec;

        // Choose a tick interval so we get ~5–10 ticks in view
        const double rawInterval = visibleDuration / 7.0;
        const double magnitude   = std::pow(10.0, std::floor(std::log10(rawInterval)));
        double tickInterval = magnitude;
        if(rawInterval / magnitude >= 5.0)      tickInterval = magnitude * 5.0;
        else if(rawInterval / magnitude >= 2.0) tickInterval = magnitude * 2.0;

        if(tickInterval <= 0.0) {
            return;
        }

        // Decide label precision
        int decimals = 0;
        if(tickInterval < 0.1)       decimals = 3;
        else if(tickInterval < 1.0)  decimals = 2;
        else if(tickInterval < 10.0) decimals = 1;

        const double firstTick = std::ceil(firstVisibleSec / tickInterval) * tickInterval;

        painter.setPen(textColor);
        QFont rulerFont = painter.font();
        rulerFont.setPointSizeF(7.5);
        painter.setFont(rulerFont);

        for(double t = firstTick; t <= lastVisibleSec + tickInterval * 0.5; t += tickInterval) {
            const double samplePos = t * m_sfreq;
            const int x = dataAreaLeft + static_cast<int>((samplePos - m_firstSample) * m_pixelsPerSample) - m_scrollValue;

            if(x < dataAreaLeft || x > width()) {
                continue;
            }

            // Major tick
            painter.setPen(tickColor);
            painter.drawLine(x, 0, x, 6);

            // Label
            const QString label = QString::number(t, 'f', decimals) + "s";
            painter.setPen(textColor);
            painter.drawText(x + 3, 1, 60, height() - 1, Qt::AlignLeft | Qt::AlignVCenter, label);
        }
    }

private:
    double m_sfreq           = 0.0;
    int    m_firstSample     = 0;
    double m_pixelsPerSample = 1.0;
    int    m_scrollValue     = 0;
    int    m_labelColumnWidth = 0;
};

RawDataBrowserWidget::RawDataBrowserWidget(QWidget* parent)
: QWidget(parent)
, m_file(new QFile(this))
, m_zoomOutButton(new QPushButton("-"))
, m_zoomResetButton(new QPushButton("1:1"))
, m_zoomInButton(new QPushButton("+"))
, m_dcButton(new QPushButton("DC"))
, m_tableView(new QTableView)
, m_markerLine(new QFrame)
, m_timeRuler(new TimeRulerWidget)
, m_rawModel(new MNEBROWSE::RawModel(this))
, m_eventModel(new MNEBROWSE::EventModel(this))
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
    m_dcButton->setFixedWidth(40);
    m_dcButton->setCheckable(true);
    m_dcButton->setToolTip("Remove DC offset (d)");
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
    m_tableView->installEventFilter(this);
    m_tableView->setFocusPolicy(Qt::StrongFocus);
    connect(m_tableView->horizontalScrollBar(), &QScrollBar::valueChanged,
            m_rawModel, &MNEBROWSE::RawModel::updateScrollPos);
    connect(m_tableView->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, [this]() {
                updateMarkerOverlay();
                updateTimeRuler();
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
    connect(m_dcButton, &QPushButton::toggled, this, [this](bool checked) {
        m_delegate->setRemoveDC(checked);
        m_tableView->viewport()->update();
    });

    m_delegate->setEventModel(m_eventModel);

    m_markerLine->setParent(m_tableView->viewport());
    m_markerLine->setObjectName("rawDataBrowserMarkerLine");
    m_markerLine->setFrameShape(QFrame::VLine);
    m_markerLine->setLineWidth(2);
    m_markerLine->setStyleSheet("background: #f78166;");
    m_markerLine->hide();

    controlLayout->addWidget(m_dcButton);
    controlLayout->addStretch(1);
    controlLayout->addWidget(m_zoomOutButton);
    controlLayout->addWidget(m_zoomResetButton);
    controlLayout->addWidget(m_zoomInButton);

    layout->addLayout(controlLayout);
    layout->addWidget(m_tableView, 1);
    layout->addWidget(m_timeRuler);
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

    // Auto-load companion event file (e.g. sample_audvis_raw-eve.fif)
    m_eventModel->clearModel();
    const QFileInfo fi(filePath);
    const QString stem = fi.completeBaseName();
    const QString eveCandidate = fi.absolutePath() + "/" + stem + "-eve.fif";
    QFile eveFile(eveCandidate);
    if(eveFile.exists()) {
        if(m_eventModel->loadEventData(eveFile)) {
            m_eventModel->setFiffInfo(m_rawModel->fiffInfo());
            m_eventModel->setFirstLastSample(m_rawModel->firstSample(), m_rawModel->lastSample());
            qInfo() << "[RawDataBrowserWidget] Loaded event file:" << eveCandidate
                    << "(" << m_eventModel->rowCount() << "events)";
            emit outputMessage(QString("Events loaded: %1 (%2 events)")
                               .arg(eveCandidate).arg(m_eventModel->rowCount()));
        }
    }

    publishBrowserState(true);

    const auto info = m_rawModel->fiffInfo();
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
    if(!m_rawModel->fiffInfo()) {
        return "No raw browser loaded.";
    }

    const int totalSamples = std::max<qint32>(0, m_rawModel->lastSample() - m_rawModel->firstSample() + 1);
    return QString("Raw Data Browser: %1 | Channels: %2 | Sampling rate: %3 Hz | Samples: %4 | First sample: %5 | Last sample: %6")
        .arg(m_filePath)
        .arg(m_rawModel->fiffInfo()->nchan)
        .arg(m_rawModel->fiffInfo()->sfreq, 0, 'f', 2)
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
    if(!m_rawModel->fiffInfo() || m_rawModel->fiffInfo()->sfreq <= 0.0) {
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
    updateTimeRuler();
    publishBrowserState(false);
}

void RawDataBrowserWidget::updateTimeRuler()
{
    if(!m_rawModel->fiffInfo() || m_rawModel->fiffInfo()->sfreq <= 0.0) {
        return;
    }

    const int labelColumnWidth = m_tableView->columnViewportPosition(1)
                                 + m_tableView->frameWidth();
    m_timeRuler->update(m_rawModel->fiffInfo()->sfreq,
                        m_rawModel->firstSample(),
                        m_delegate->pixelsPerSample(),
                        m_tableView->horizontalScrollBar()->value(),
                        labelColumnWidth);
}

QString RawDataBrowserWidget::formatSeconds(int sample) const
{
    const double seconds = sample / m_rawModel->fiffInfo()->sfreq;
    return QString::number(seconds, 'f', 3);
}

bool RawDataBrowserWidget::eventFilter(QObject* watched, QEvent* event)
{
    if(watched == m_tableView->viewport()) {
        if(event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if(mouseEvent->button() == Qt::LeftButton) {
                const int dataColumnX = m_tableView->columnViewportPosition(1);
                if(mouseEvent->position().x() < dataColumnX) {
                    // Click in label column — toggle bad channel
                    const QModelIndex idx = m_tableView->indexAt(mouseEvent->pos());
                    if(idx.isValid() && !m_rawModel->channelInfoList().isEmpty()) {
                        const QString& chName = m_rawModel->channelInfoList().at(idx.row()).ch_name;
                        const bool isBad = m_rawModel->fiffInfo()->bads.contains(chName);
                        QModelIndexList colOneList;
                        colOneList << m_rawModel->index(idx.row(), 1);
                        m_rawModel->markChBad(colOneList, !isBad);
                        m_tableView->viewport()->update();
                        return true;
                    }
                }
            }
        } else if(event->type() == QEvent::MouseButtonDblClick) {
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
            updateTimeRuler();
        } else if(event->type() == QEvent::Wheel) {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            if(wheelEvent->modifiers() & Qt::ControlModifier) {
                const double factor = wheelEvent->angleDelta().y() > 0 ? 1.25 : 0.8;
                adjustAmplitude(factor);
                return true;
            }
        }
    } else if(watched == m_tableView) {
        if(event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if(keyEvent->key() == Qt::Key_Plus || keyEvent->key() == Qt::Key_Equal) {
                adjustAmplitude(1.25);
                return true;
            } else if(keyEvent->key() == Qt::Key_Minus) {
                adjustAmplitude(0.8);
                return true;
            } else if(keyEvent->key() == Qt::Key_D) {
                m_dcButton->toggle();
                return true;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void RawDataBrowserWidget::setMarkerSample(int sample)
{
    if(!m_rawModel->fiffInfo() || m_rawModel->fiffInfo()->sfreq <= 0.0) {
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

void RawDataBrowserWidget::adjustAmplitude(double factor)
{
    m_delegate->setAmplitudeScale(m_delegate->amplitudeScale() * factor);
    m_tableView->viewport()->update();
}

QString RawDataBrowserWidget::currentVisibleRangeText() const
{
    if(!m_rawModel->fiffInfo() || m_rawModel->fiffInfo()->sfreq <= 0.0) {
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
    if(m_markerSample < 0 || !m_rawModel->fiffInfo() || m_rawModel->fiffInfo()->sfreq <= 0.0) {
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

    if(appendToOutput && m_rawModel->fiffInfo()) {
        const int totalSamples = std::max<qint32>(0, m_rawModel->lastSample() - m_rawModel->firstSample() + 1);
        emit outputMessage(QString("Raw Data Browser: %1").arg(m_filePath));
        emit outputMessage(QString("Channels: %1 | Sampling rate: %2 Hz | Samples: %3 | First sample: %4 | Last sample: %5")
                           .arg(m_rawModel->fiffInfo()->nchan)
                           .arg(m_rawModel->fiffInfo()->sfreq, 0, 'f', 2)
                           .arg(totalSamples)
                           .arg(m_rawModel->firstSample())
                           .arg(m_rawModel->lastSample()));
    }
}
