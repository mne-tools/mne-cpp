//=============================================================================================================
/**
 * @file     mainwindow.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     January, 2014
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
 * @brief    mne_browse is the QT equivalent of the already existing C-version of mne_browse_raw. It is pursued
 *           to reimplement the full feature set of mne_browse_raw and even extend these.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainwindow.h"

#include <QBuffer>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QInputDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QFileInfo>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;

namespace
{

QString defaultEvokedFilePath(const QString& rawFilePath)
{
    if(rawFilePath.isEmpty()) {
        return QString();
    }

    if(rawFilePath.endsWith(".fif", Qt::CaseInsensitive)) {
        return rawFilePath.left(rawFilePath.size() - 4) + "-ave.fif";
    }

    return rawFilePath + "-ave.fif";
}

QString defaultCovFilePath(const QString& rawFilePath)
{
    if(rawFilePath.isEmpty()) {
        return QString();
    }

    if(rawFilePath.endsWith(".fif", Qt::CaseInsensitive)) {
        return rawFilePath.left(rawFilePath.size() - 4) + "-cov.fif";
    }

    return rawFilePath + "-cov.fif";
}

QString defaultAnnotationFilePath(const QString& rawFilePath)
{
    if(rawFilePath.isEmpty()) {
        return QString();
    }

    if(rawFilePath.endsWith(".fif", Qt::CaseInsensitive)) {
        return rawFilePath.left(rawFilePath.size() - 4) + "-annot.json";
    }

    return rawFilePath + "-annot.json";
}

QString defaultVirtualChannelFilePath(const QString& rawFilePath)
{
    if(rawFilePath.isEmpty()) {
        return QString();
    }

    if(rawFilePath.endsWith(".fif", Qt::CaseInsensitive)) {
        return rawFilePath.left(rawFilePath.size() - 4) + "-virtchan.json";
    }

    return rawFilePath + "-virtchan.json";
}

QMap<int, int> countEventsByType(const MatrixXi& events)
{
    QMap<int, int> counts;

    if(events.cols() < 3) {
        return counts;
    }

    for(int row = 0; row < events.rows(); ++row) {
        const int eventCode = events(row, 2);
        if(eventCode != 0) {
            counts[eventCode] = counts.value(eventCode) + 1;
        }
    }

    return counts;
}

QString normalizeStimChannelName(const QString& channelName)
{
    QString normalizedName = channelName.trimmed().toUpper();
    normalizedName.remove(QLatin1Char(' '));
    return normalizedName;
}

bool detectFallbackStimEvents(const FIFFLIB::FiffRawData& raw,
                              MatrixXi& events,
                              QString& sourceDescription)
{
    QStringList preferredStimChannels;
    QStringList fallbackStimChannels;

    for(int channelIndex = 0; channelIndex < raw.info.nchan; ++channelIndex) {
        if(raw.info.chs[channelIndex].kind != FIFFV_STIM_CH) {
            continue;
        }

        const QString stimChannel = raw.info.ch_names.value(channelIndex);
        const QString normalizedName = normalizeStimChannelName(stimChannel);

        if(normalizedName == QLatin1String("STI014") ||
           normalizedName == QLatin1String("STI101")) {
            preferredStimChannels.append(stimChannel);
        } else {
            fallbackStimChannels.append(stimChannel);
        }
    }

    const QStringList stimChannels = preferredStimChannels + fallbackStimChannels;
    for(const QString& stimChannel : stimChannels) {
        FIFFLIB::FiffEvents detectedEvents;
        if(FIFFLIB::FiffEvents::detect_from_raw(raw, detectedEvents, stimChannel)) {
            events = detectedEvents.events;
            sourceDescription = QString("Using trigger events detected from %1.").arg(stimChannel);
            return true;
        }
    }

    return false;
}

bool showComputeEvokedDialog(QWidget* parent,
                             QSettings& settings,
                             const QMap<int, int>& eventCounts,
                             const QString& sourceDescription,
                             QList<int>& selectedEventCodes,
                             float& tmin,
                             float& tmax,
                             bool& applyBaseline,
                             float& baselineFrom,
                             float& baselineTo,
                             bool& dropRejected)
{
    QDialog dialog(parent);
    dialog.setWindowTitle("Compute Evoked");

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    QLabel* sourceLabel = new QLabel(sourceDescription, &dialog);
    sourceLabel->setWordWrap(true);
    mainLayout->addWidget(sourceLabel);

    QGroupBox* timingGroup = new QGroupBox("Epoch Settings", &dialog);
    QFormLayout* timingLayout = new QFormLayout(timingGroup);

    QSpinBox* preStimSpinBox = new QSpinBox(timingGroup);
    preStimSpinBox->setRange(0, 10000);
    preStimSpinBox->setSingleStep(10);
    preStimSpinBox->setPrefix("-");
    preStimSpinBox->setValue(settings.value("MainWindow/Averaging/preStimMs", 100).toInt());

    QSpinBox* postStimSpinBox = new QSpinBox(timingGroup);
    postStimSpinBox->setRange(10, 10000);
    postStimSpinBox->setSingleStep(10);
    postStimSpinBox->setValue(settings.value("MainWindow/Averaging/postStimMs", 400).toInt());

    QCheckBox* baselineCheckBox = new QCheckBox("Apply baseline correction", timingGroup);
    baselineCheckBox->setChecked(settings.value("MainWindow/Averaging/applyBaseline", true).toBool());

    QSpinBox* baselineFromSpinBox = new QSpinBox(timingGroup);
    baselineFromSpinBox->setSingleStep(10);
    baselineFromSpinBox->setRange(-preStimSpinBox->value(), postStimSpinBox->value());
    baselineFromSpinBox->setValue(settings.value("MainWindow/Averaging/baselineFromMs",
                                                 -preStimSpinBox->value()).toInt());

    QSpinBox* baselineToSpinBox = new QSpinBox(timingGroup);
    baselineToSpinBox->setSingleStep(10);
    baselineToSpinBox->setRange(-preStimSpinBox->value(), postStimSpinBox->value());
    baselineToSpinBox->setValue(settings.value("MainWindow/Averaging/baselineToMs", 0).toInt());

    QCheckBox* rejectCheckBox = new QCheckBox("Drop rejected epochs", timingGroup);
    rejectCheckBox->setChecked(settings.value("MainWindow/Averaging/dropRejected", true).toBool());

    timingLayout->addRow("Pre-stimulus (ms)", preStimSpinBox);
    timingLayout->addRow("Post-stimulus (ms)", postStimSpinBox);
    timingLayout->addRow(baselineCheckBox);
    timingLayout->addRow("Baseline from (ms)", baselineFromSpinBox);
    timingLayout->addRow("Baseline to (ms)", baselineToSpinBox);
    timingLayout->addRow(rejectCheckBox);
    mainLayout->addWidget(timingGroup);

    auto syncSpinBoxBounds = [=]() {
        const int minValue = -preStimSpinBox->value();
        const int maxValue = postStimSpinBox->value();

        baselineFromSpinBox->setRange(minValue, maxValue);
        baselineToSpinBox->setRange(minValue, maxValue);

        if(baselineFromSpinBox->value() > baselineToSpinBox->value()) {
            baselineFromSpinBox->setValue(minValue);
            baselineToSpinBox->setValue(0);
        }
    };

    syncSpinBoxBounds();

    QObject::connect(preStimSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                     &dialog, [=](int) { syncSpinBoxBounds(); });
    QObject::connect(postStimSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                     &dialog, [=](int) { syncSpinBoxBounds(); });
    QObject::connect(baselineCheckBox, &QCheckBox::toggled,
                     baselineFromSpinBox, &QSpinBox::setEnabled);
    QObject::connect(baselineCheckBox, &QCheckBox::toggled,
                     baselineToSpinBox, &QSpinBox::setEnabled);
    baselineFromSpinBox->setEnabled(baselineCheckBox->isChecked());
    baselineToSpinBox->setEnabled(baselineCheckBox->isChecked());

    QGroupBox* eventGroup = new QGroupBox("Event Types", &dialog);
    QVBoxLayout* eventLayout = new QVBoxLayout(eventGroup);
    QLabel* eventHint = new QLabel("Select one or more event codes to average.", eventGroup);
    eventHint->setWordWrap(true);
    eventLayout->addWidget(eventHint);

    QListWidget* eventListWidget = new QListWidget(eventGroup);
    eventListWidget->setSelectionMode(QAbstractItemView::MultiSelection);

    for(auto it = eventCounts.constBegin(); it != eventCounts.constEnd(); ++it) {
        QListWidgetItem* item = new QListWidgetItem(QString("%1 (%2 epochs)").arg(it.key()).arg(it.value()),
                                                    eventListWidget);
        item->setData(Qt::UserRole, it.key());
        item->setSelected(true);
    }

    eventLayout->addWidget(eventListWidget);
    mainLayout->addWidget(eventGroup);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal,
                                                       &dialog);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if(dialog.exec() != QDialog::Accepted) {
        return false;
    }

    selectedEventCodes.clear();
    for(int row = 0; row < eventListWidget->count(); ++row) {
        QListWidgetItem* item = eventListWidget->item(row);
        if(item->isSelected()) {
            selectedEventCodes.append(item->data(Qt::UserRole).toInt());
        }
    }

    if(selectedEventCodes.isEmpty()) {
        QMessageBox::warning(parent,
                             "Compute Evoked",
                             "Select at least one event type to compute an evoked response.");
        return false;
    }

    settings.setValue("MainWindow/Averaging/preStimMs", preStimSpinBox->value());
    settings.setValue("MainWindow/Averaging/postStimMs", postStimSpinBox->value());
    settings.setValue("MainWindow/Averaging/applyBaseline", baselineCheckBox->isChecked());
    settings.setValue("MainWindow/Averaging/baselineFromMs", baselineFromSpinBox->value());
    settings.setValue("MainWindow/Averaging/baselineToMs", baselineToSpinBox->value());
    settings.setValue("MainWindow/Averaging/dropRejected", rejectCheckBox->isChecked());

    tmin = -static_cast<float>(preStimSpinBox->value()) / 1000.0f;
    tmax = static_cast<float>(postStimSpinBox->value()) / 1000.0f;
    applyBaseline = baselineCheckBox->isChecked();
    baselineFrom = static_cast<float>(baselineFromSpinBox->value()) / 1000.0f;
    baselineTo = static_cast<float>(baselineToSpinBox->value()) / 1000.0f;
    dropRejected = rejectCheckBox->isChecked();

    return true;
}

bool showComputeCovarianceDialog(QWidget* parent,
                                 QSettings& settings,
                                 const QMap<int, int>& eventCounts,
                                 const QString& sourceDescription,
                                 QList<int>& selectedEventCodes,
                                 float& tmin,
                                 float& tmax,
                                 bool& applyBaseline,
                                 float& baselineFrom,
                                 float& baselineTo,
                                 bool& removeMean)
{
    QDialog dialog(parent);
    dialog.setWindowTitle("Compute Covariance");

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    QLabel* sourceLabel = new QLabel(sourceDescription, &dialog);
    sourceLabel->setWordWrap(true);
    mainLayout->addWidget(sourceLabel);

    QGroupBox* timingGroup = new QGroupBox("Covariance Window", &dialog);
    QFormLayout* timingLayout = new QFormLayout(timingGroup);

    QSpinBox* fromSpinBox = new QSpinBox(timingGroup);
    fromSpinBox->setRange(-10000, 10000);
    fromSpinBox->setSingleStep(10);
    fromSpinBox->setValue(settings.value("MainWindow/Covariance/tminMs", -200).toInt());

    QSpinBox* toSpinBox = new QSpinBox(timingGroup);
    toSpinBox->setRange(-10000, 10000);
    toSpinBox->setSingleStep(10);
    toSpinBox->setValue(settings.value("MainWindow/Covariance/tmaxMs", 0).toInt());

    QCheckBox* baselineCheckBox = new QCheckBox("Apply baseline correction", timingGroup);
    baselineCheckBox->setChecked(settings.value("MainWindow/Covariance/applyBaseline", false).toBool());

    QSpinBox* baselineFromSpinBox = new QSpinBox(timingGroup);
    baselineFromSpinBox->setRange(-10000, 10000);
    baselineFromSpinBox->setSingleStep(10);
    baselineFromSpinBox->setValue(settings.value("MainWindow/Covariance/baselineFromMs", -200).toInt());

    QSpinBox* baselineToSpinBox = new QSpinBox(timingGroup);
    baselineToSpinBox->setRange(-10000, 10000);
    baselineToSpinBox->setSingleStep(10);
    baselineToSpinBox->setValue(settings.value("MainWindow/Covariance/baselineToMs", 0).toInt());

    QCheckBox* removeMeanCheckBox = new QCheckBox("Remove sample mean", timingGroup);
    removeMeanCheckBox->setChecked(settings.value("MainWindow/Covariance/removeMean", true).toBool());

    timingLayout->addRow("Window from (ms)", fromSpinBox);
    timingLayout->addRow("Window to (ms)", toSpinBox);
    timingLayout->addRow(baselineCheckBox);
    timingLayout->addRow("Baseline from (ms)", baselineFromSpinBox);
    timingLayout->addRow("Baseline to (ms)", baselineToSpinBox);
    timingLayout->addRow(removeMeanCheckBox);
    mainLayout->addWidget(timingGroup);

    auto syncRanges = [=]() {
        if(fromSpinBox->value() > toSpinBox->value()) {
            toSpinBox->setValue(fromSpinBox->value());
        }

        baselineFromSpinBox->setMinimum(fromSpinBox->value());
        baselineFromSpinBox->setMaximum(toSpinBox->value());
        baselineToSpinBox->setMinimum(fromSpinBox->value());
        baselineToSpinBox->setMaximum(toSpinBox->value());

        if(baselineFromSpinBox->value() > baselineToSpinBox->value()) {
            baselineFromSpinBox->setValue(fromSpinBox->value());
            baselineToSpinBox->setValue(toSpinBox->value());
        }
    };

    syncRanges();

    QObject::connect(fromSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                     &dialog, [=](int) { syncRanges(); });
    QObject::connect(toSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                     &dialog, [=](int) { syncRanges(); });
    QObject::connect(baselineCheckBox, &QCheckBox::toggled,
                     baselineFromSpinBox, &QSpinBox::setEnabled);
    QObject::connect(baselineCheckBox, &QCheckBox::toggled,
                     baselineToSpinBox, &QSpinBox::setEnabled);
    baselineFromSpinBox->setEnabled(baselineCheckBox->isChecked());
    baselineToSpinBox->setEnabled(baselineCheckBox->isChecked());

    QGroupBox* eventGroup = new QGroupBox("Event Types", &dialog);
    QVBoxLayout* eventLayout = new QVBoxLayout(eventGroup);
    QLabel* eventHint = new QLabel("Select one or more event codes whose epochs should contribute to the covariance estimate.",
                                   eventGroup);
    eventHint->setWordWrap(true);
    eventLayout->addWidget(eventHint);

    QListWidget* eventListWidget = new QListWidget(eventGroup);
    eventListWidget->setSelectionMode(QAbstractItemView::MultiSelection);

    for(auto it = eventCounts.constBegin(); it != eventCounts.constEnd(); ++it) {
        QListWidgetItem* item = new QListWidgetItem(QString("%1 (%2 epochs)").arg(it.key()).arg(it.value()),
                                                    eventListWidget);
        item->setData(Qt::UserRole, it.key());
        item->setSelected(true);
    }

    eventLayout->addWidget(eventListWidget);
    mainLayout->addWidget(eventGroup);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal,
                                                       &dialog);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    if(dialog.exec() != QDialog::Accepted) {
        return false;
    }

    selectedEventCodes.clear();
    for(int row = 0; row < eventListWidget->count(); ++row) {
        QListWidgetItem* item = eventListWidget->item(row);
        if(item->isSelected()) {
            selectedEventCodes.append(item->data(Qt::UserRole).toInt());
        }
    }

    if(selectedEventCodes.isEmpty()) {
        QMessageBox::warning(parent,
                             "Compute Covariance",
                             "Select at least one event type to compute a covariance matrix.");
        return false;
    }

    settings.setValue("MainWindow/Covariance/tminMs", fromSpinBox->value());
    settings.setValue("MainWindow/Covariance/tmaxMs", toSpinBox->value());
    settings.setValue("MainWindow/Covariance/applyBaseline", baselineCheckBox->isChecked());
    settings.setValue("MainWindow/Covariance/baselineFromMs", baselineFromSpinBox->value());
    settings.setValue("MainWindow/Covariance/baselineToMs", baselineToSpinBox->value());
    settings.setValue("MainWindow/Covariance/removeMean", removeMeanCheckBox->isChecked());

    tmin = static_cast<float>(fromSpinBox->value()) / 1000.0f;
    tmax = static_cast<float>(toSpinBox->value()) / 1000.0f;
    applyBaseline = baselineCheckBox->isChecked();
    baselineFrom = static_cast<float>(baselineFromSpinBox->value()) / 1000.0f;
    baselineTo = static_cast<float>(baselineToSpinBox->value()) / 1000.0f;
    removeMean = removeMeanCheckBox->isChecked();

    return true;
}

} // namespace


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
//, m_qFileRaw(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif")
//, m_qEventFile(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw-eve.fif")
//, m_qEvokedFile(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-ave.fif")
, m_qSettings()
, m_rawSettings()
, ui(new Ui::MainWindowWidget)
, m_pStatusLabel(nullptr)
, m_pWhitenButterflyAction(nullptr)
, m_pAnnotationModeAction(nullptr)
{
    ui->setupUi(this);

    //The following functions and their point of calling should NOT be changed
    //Setup the windows first - this NEEDS to be done here because important pointers (window pointers) which are needed for further processing are generated in this function
    setupWindowWidgets();
    setupMainWindow();

    // Setup rest of the GUI
    connectMenus();
    setWindowStatus();

    //Set standard LogLevel
    setLogLevel(_LogLvMax);
}


//*************************************************************************************************************

MainWindow::~MainWindow()
{
}


//*************************************************************************************************************

RawModel* MainWindow::rawModel() const
{
    return m_pDataWindow ? m_pDataWindow->getDataModel() : nullptr;
}


//*************************************************************************************************************

void MainWindow::setupWindowWidgets()
{
    //Create data window
    m_pDataWindow = new DataWindow(this);

    //Create dockble event window - QTDesigner used - see / FormFiles
    m_pEventWindow = new EventWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pEventWindow);
    m_pEventWindow->hide();

    //Create dockable annotation window
    m_pAnnotationWindow = new AnnotationWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pAnnotationWindow);
    m_pAnnotationWindow->hide();

    //Create dockable virtual-channel window
    m_pVirtualChannelWindow = new VirtualChannelWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pVirtualChannelWindow);
    m_pVirtualChannelWindow->hide();

    //Create dockable information window - QTDesigner used - see / FormFiles
    m_pInformationWindow = new InformationWindow(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pInformationWindow);
    m_pInformationWindow->hide();

    //Create about window - QTDesigner used - see / FormFiles
    m_pAboutWindow = new AboutWindow(this);
    m_pAboutWindow->hide();

    //Create channel info window - QTDesigner used - see / FormFiles
    m_pChInfoWindow = new ChInfoWindow(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pChInfoWindow);
    m_pChInfoWindow->hide();

    //Create selection manager window - QTDesigner used - see / FormFiles
    m_pChannelSelectionViewDock = new QDockWidget(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pChannelSelectionViewDock);
    m_pChannelSelectionView = new ChannelSelectionView(QString("MNEBrowse"),
                                                       m_pChannelSelectionViewDock,
                                                       m_pChInfoWindow->getDataModel(),
                                                       Qt::Widget);
    m_pChannelSelectionViewDock->setWidget(m_pChannelSelectionView);
    m_pChannelSelectionViewDock->hide();

    //Create average manager window - QTDesigner used - see / FormFiles
    m_pAverageWindow = new AverageWindow(this, m_qEvokedFile);
    addDockWidget(Qt::BottomDockWidgetArea, m_pAverageWindow);
    m_pAverageWindow->hide();

    //Create scale window - QTDesigner used - see / FormFiles
    m_pScaleWindow = new ScaleWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pScaleWindow);
    m_pScaleWindow->hide();

    //Create filter window - QTDesigner used - see / FormFiles
    m_pFilterWindow = new FilterWindow(this, this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pFilterWindow);
    m_pFilterWindow->hide();

    //Create noise reduction window - QTDesigner used - see / FormFiles
    m_pNoiseReductionWindow = new NoiseReductionWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pNoiseReductionWindow);
    m_pNoiseReductionWindow->hide();

    //Init windows - TODO: get rid of this here, do this inside the window classes
    m_pDataWindow->init();
    m_pEventWindow->init();
    m_pAnnotationWindow->init();
    m_pVirtualChannelWindow->init();
    m_pScaleWindow->init();

    //Create the toolbar after all indows have been initiliased
    createToolBar();

    //Connect window signals
    //Change scaling of the data and averaged data whenever a spinbox value changed or the user performs a pinch gesture on the view
    connect(m_pScaleWindow, &ScaleWindow::scalingChannelValueChanged,
            m_pDataWindow, &DataWindow::scaleData);

    connect(m_pScaleWindow, &ScaleWindow::scalingViewValueChanged,
            m_pDataWindow, &DataWindow::changeRowHeight);

    connect(m_pDataWindow, &DataWindow::scaleChannels,
            m_pScaleWindow, &ScaleWindow::scaleAllChannels);

    connect(m_pScaleWindow, &ScaleWindow::scalingChannelValueChanged,
            m_pAverageWindow, &AverageWindow::scaleAveragedData);

    //Hide non selected channels in the data view
    connect(m_pChannelSelectionView, &ChannelSelectionView::showSelectedChannelsOnly,
            m_pDataWindow, &DataWindow::showSelectedChannelsOnly);

    //Connect selection manager with average manager
    connect(m_pChannelSelectionView, &ChannelSelectionView::selectionChanged,
            m_pAverageWindow, &AverageWindow::channelSelectionManagerChanged);

    connect(m_pAverageWindow, &AverageWindow::addAverageRequested,
            this, &MainWindow::computeEvoked);

    connect(m_pAnnotationWindow->getAnnotationModel(), &AnnotationModel::annotationsChanged,
            this, [this]() {
                QVector<DISPLIB::ChannelRhiView::AnnotationSpan> spans;
                const QVector<AnnotationSpanData> modelSpans =
                    m_pAnnotationWindow->getAnnotationModel()->getAnnotationSpans();
                spans.reserve(modelSpans.size());
                for(const AnnotationSpanData& modelSpan : modelSpans) {
                    DISPLIB::ChannelRhiView::AnnotationSpan span;
                    span.startSample = modelSpan.startSample;
                    span.endSample = modelSpan.endSample;
                    span.color = modelSpan.color;
                    span.label = modelSpan.label;
                    spans.append(span);
                }

                m_pDataWindow->setAnnotations(spans);
                setWindowStatus();
            });

    connect(m_pDataWindow, &DataWindow::annotationRangeSelected,
            this, &MainWindow::handleAnnotationRangeSelected);

    connect(m_pVirtualChannelWindow->getVirtualChannelModel(), &VirtualChannelModel::virtualChannelsChanged,
            this, [this]() {
                m_pDataWindow->setVirtualChannels(
                    m_pVirtualChannelWindow->getVirtualChannelModel()->virtualChannels());
                setWindowStatus();
            });

    //Connect channel info window with raw data model, layout manager, average manager and the data window
    connect(rawModel(), &RawModel::fileLoaded,
            m_pChInfoWindow->getDataModel().data(), &ChannelInfoModel::setFiffInfo);

    connect(rawModel(), &RawModel::assignedOperatorsChanged,
            m_pChInfoWindow->getDataModel().data(), &ChannelInfoModel::assignedOperatorsChanged);

    connect(m_pChannelSelectionView, &ChannelSelectionView::loadedLayoutMap,
            m_pChInfoWindow->getDataModel().data(), &ChannelInfoModel::layoutChanged);

    connect(m_pChInfoWindow->getDataModel().data(), &ChannelInfoModel::channelsMappedToLayout,
            m_pChannelSelectionView, &ChannelSelectionView::setCurrentlyMappedFiffChannels);

    connect(m_pChInfoWindow->getDataModel().data(), &ChannelInfoModel::channelsMappedToLayout,
            m_pAverageWindow, &AverageWindow::setMappedChannelNames);

    //Connect selection manager with a new file loaded signal
    connect(rawModel(), &RawModel::fileLoaded,
            m_pChannelSelectionView, &ChannelSelectionView::newFiffFileLoaded);

    //Connect filter window with new file loaded signal
    connect(rawModel(), &RawModel::fileLoaded,
            m_pFilterWindow, &FilterWindow::newFileLoaded);

    //Connect noise reduction manager with fif file loading
    connect(rawModel(), &RawModel::fileLoaded,
            m_pNoiseReductionWindow, &NoiseReductionWindow::setFiffInfo);

    connect(m_pNoiseReductionWindow, &NoiseReductionWindow::projSelectionChanged,
            rawModel(), &RawModel::updateProjections);

    connect(m_pNoiseReductionWindow, &NoiseReductionWindow::compSelectionChanged,
            rawModel(), &RawModel::updateCompensator);

    //If a default file has been specified on startup -> call hideSpinBoxes and set laoded fiff channels - TODO: dirty move get rid of this here
    if(rawModel()->isFileLoaded()) {
        auto fiffInfo = rawModel()->fiffInfo();
        m_pScaleWindow->hideSpinBoxes(fiffInfo);
        m_pChInfoWindow->getDataModel()->setFiffInfo(fiffInfo);
        m_pChInfoWindow->getDataModel()->layoutChanged(m_pChannelSelectionView->getLayoutMap());
        m_pChannelSelectionView->setCurrentlyMappedFiffChannels(m_pChInfoWindow->getDataModel()->getMappedChannelsList());
        m_pChannelSelectionView->newFiffFileLoaded(fiffInfo);
        m_pFilterWindow->newFileLoaded(fiffInfo);
        m_pNoiseReductionWindow->setFiffInfo(fiffInfo);
    }
}


//*************************************************************************************************************

void MainWindow::createToolBar()
{
    //Create toolbar
    QToolBar *toolBar = new QToolBar(this);
    toolBar->setOrientation(Qt::Vertical);
    toolBar->setMovable(true);

    //Add DC removal action
    m_pRemoveDCAction = new QAction(QIcon(":/Resources/Images/removeDC.png"),tr("Remove DC component"), this);
    m_pRemoveDCAction->setStatusTip(tr("Remove the DC component by subtracting the channel mean"));
    connect(m_pRemoveDCAction,&QAction::triggered, [this](){
        if(m_pDataWindow->getDataDelegate()->isRemoveDC()) {
            m_pRemoveDCAction->setIcon(QIcon(":/Resources/Images/removeDC.png"));
            m_pRemoveDCAction->setToolTip("Remove DC component");
            m_pDataWindow->setStatusTip(tr("Remove the DC component by subtracting the channel mean"));
            m_pDataWindow->getDataDelegate()->setRemoveDC(false);
            if (m_pDataWindow->getChannelDataView())
                m_pDataWindow->getChannelDataView()->setRemoveDC(false);
        }
        else {
            m_pRemoveDCAction->setIcon(QIcon(":/Resources/Images/addDC.png"));
            m_pRemoveDCAction->setToolTip("Add DC component");
            m_pRemoveDCAction->setStatusTip(tr("Add the DC component"));
            m_pDataWindow->getDataDelegate()->setRemoveDC(true);
            if (m_pDataWindow->getChannelDataView())
                m_pDataWindow->getChannelDataView()->setRemoveDC(true);
        }

        m_pDataWindow->updateDataTableViews();
    });
    toolBar->addAction(m_pRemoveDCAction);

    //Add show/hide bad channel button
    m_pHideBadAction = new QAction(QIcon(":/Resources/Images/hideBad.png"),tr("Hide all bad channels"), this);
    m_pHideBadAction->setStatusTip(tr("Hide all bad channels"));
    connect(m_pHideBadAction,&QAction::triggered, [this](){
        if(m_pHideBadAction->toolTip() == "Show all bad channels") {
            m_pHideBadAction->setIcon(QIcon(":/Resources/Images/hideBad.png"));
            m_pHideBadAction->setToolTip("Hide all bad channels");
            m_pDataWindow->setStatusTip(tr("Hide all bad channels"));
            m_pDataWindow->hideBadChannels(false);
        }
        else {
            m_pHideBadAction->setIcon(QIcon(":/Resources/Images/showBad.png"));
            m_pHideBadAction->setToolTip("Show all bad channels");
            m_pHideBadAction->setStatusTip(tr("Show all bad channels"));
            m_pDataWindow->hideBadChannels(true);
        }
    });
    toolBar->addAction(m_pHideBadAction);

    toolBar->addSeparator();

    //Toggle visibility of the event manager
    QAction* showEventManager = new QAction(QIcon(":/Resources/Images/showEventManager.png"),tr("Toggle event manager"), this);
    showEventManager->setStatusTip(tr("Toggle the event manager"));
    connect(showEventManager, &QAction::triggered, this, [this](){
        showWindow(m_pEventWindow);
    });
    toolBar->addAction(showEventManager);

    //Toggle visibility of the filter window
    QAction* showFilterWindow = new QAction(QIcon(":/Resources/Images/showFilterWindow.png"),tr("Toggle filter window"), this);
    showFilterWindow->setStatusTip(tr("Toggle filter window"));
    connect(showFilterWindow, &QAction::triggered, this, [this](){
        showWindow(m_pFilterWindow);
    });
    toolBar->addAction(showFilterWindow);

    //Toggle visibility of the Selection manager
    QAction* showSelectionManager = new QAction(QIcon(":/Resources/Images/showSelectionManager.png"),tr("Toggle selection manager"), this);
    showSelectionManager->setStatusTip(tr("Toggle the selection manager"));
    connect(showSelectionManager, &QAction::triggered, this, [this](){
        showWindow(m_pChannelSelectionViewDock);
    });
    toolBar->addAction(showSelectionManager);

    //Toggle visibility of the scaling window
    QAction* showScalingWindow = new QAction(QIcon(":/Resources/Images/showScalingWindow.png"),tr("Toggle scaling window"), this);
    showScalingWindow->setStatusTip(tr("Toggle the scaling window"));
    connect(showScalingWindow, &QAction::triggered, this, [this](){
        showWindow(m_pScaleWindow);
    });
    toolBar->addAction(showScalingWindow);

    //Toggle visibility of the average manager
    QAction* showAverageManager = new QAction(QIcon(":/Resources/Images/showAverageManager.png"),tr("Toggle average manager"), this);
    showAverageManager->setStatusTip(tr("Toggle the average manager"));
    connect(showAverageManager, &QAction::triggered, this, [this](){
        showWindow(m_pAverageWindow);
    });
    toolBar->addAction(showAverageManager);

    //Toggle visibility of the noise reduction manager
    QAction* showNoiseReductionManager = new QAction(QIcon(":/Resources/Images/showNoiseReductionWindow.png"),tr("Toggle noise reduction manager"), this);
    showNoiseReductionManager->setStatusTip(tr("Toggle the noise reduction manager"));
    connect(showNoiseReductionManager, &QAction::triggered, this, [this](){
        showWindow(m_pNoiseReductionWindow);
    });
    toolBar->addAction(showNoiseReductionManager);

    toolBar->addSeparator();

    //Toggle visibility of the channel information window manager
    QAction* showChInfo = new QAction(QIcon(":/Resources/Images/showChInformationWindow.png"),tr("Channel info"), this);
    showChInfo->setStatusTip(tr("Toggle channel info window"));
    connect(showChInfo, &QAction::triggered, this, [this](){
        showWindow(m_pChInfoWindow);
    });
    toolBar->addAction(showChInfo);

    //Toggle visibility of the information window
//    QAction* showInformationWindow = new QAction(QIcon(":/Resources/Images/showInformationWindow.png"),tr("Toggle information window"), this);
//    showInformationWindow->setStatusTip(tr("Toggle the information window"));
//    connect(showInformationWindow, &QAction::triggered, this, [=](){
//        showWindow(m_pInformationWindow);
//    });
//    toolBar->addAction(showInformationWindow);

    this->addToolBar(Qt::RightToolBarArea,toolBar);
}


//*************************************************************************************************************

void MainWindow::connectMenus()
{
    QAction* loadCovarianceAction = new QAction(tr("Load Covariance..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, loadCovarianceAction);

    QAction* computeCovarianceAction = new QAction(tr("Compute Covariance..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, computeCovarianceAction);

    QAction* saveCovarianceAction = new QAction(tr("Save Covariance (fif)..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, saveCovarianceAction);

    QAction* computeEvokedAction = new QAction(tr("Compute Evoked..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, computeEvokedAction);

    QAction* saveEvokedAction = new QAction(tr("Save Evoked (fif)..."), this);
    ui->menuTest->insertAction(ui->m_quitAction, saveEvokedAction);

    QAction* loadAnnotationsAction = new QAction(tr("Load Annotations..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, loadAnnotationsAction);

    QAction* saveAnnotationsAction = new QAction(tr("Save Annotations..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, saveAnnotationsAction);

    QAction* loadVirtualChannelsAction = new QAction(tr("Load Virtual Channels..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, loadVirtualChannelsAction);

    QAction* saveVirtualChannelsAction = new QAction(tr("Save Virtual Channels..."), this);
    ui->menuTest->insertAction(ui->m_loadEvokedAction, saveVirtualChannelsAction);

    m_pWhitenButterflyAction = new QAction(tr("Whiten Butterfly Plot"), this);
    m_pWhitenButterflyAction->setCheckable(true);
    ui->menuTest->insertAction(ui->m_quitAction, m_pWhitenButterflyAction);

    QAction* annotationManagerAction = new QAction(tr("Annotation manager..."), this);
    ui->menuWindows->insertAction(ui->m_informationAction, annotationManagerAction);

    QAction* virtualChannelManagerAction = new QAction(tr("Virtual channel manager..."), this);
    ui->menuWindows->insertAction(ui->m_informationAction, virtualChannelManagerAction);

    m_pAnnotationModeAction = new QAction(tr("Annotation Mode"), this);
    m_pAnnotationModeAction->setCheckable(true);
    m_pAnnotationModeAction->setStatusTip(tr("When enabled, Shift-drag in the raw browser creates annotation spans."));
    ui->menuAdjust->addAction(m_pAnnotationModeAction);

    //File
    connect(ui->m_openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->m_writeAction, &QAction::triggered, this, &MainWindow::writeFile);
    connect(ui->m_loadEvents, &QAction::triggered, this, &MainWindow::loadEvents);
    connect(ui->m_saveEvents, &QAction::triggered, this, &MainWindow::saveEvents);
    connect(loadAnnotationsAction, &QAction::triggered, this, &MainWindow::loadAnnotations);
    connect(saveAnnotationsAction, &QAction::triggered, this, &MainWindow::saveAnnotations);
    connect(loadVirtualChannelsAction, &QAction::triggered, this, &MainWindow::loadVirtualChannels);
    connect(saveVirtualChannelsAction, &QAction::triggered, this, &MainWindow::saveVirtualChannels);
    connect(loadCovarianceAction, &QAction::triggered, this, &MainWindow::loadCovariance);
    connect(computeCovarianceAction, &QAction::triggered, this, &MainWindow::computeCovariance);
    connect(saveCovarianceAction, &QAction::triggered, this, &MainWindow::saveCovariance);
    connect(computeEvokedAction, &QAction::triggered, this, &MainWindow::computeEvoked);
    connect(ui->m_loadEvokedAction, &QAction::triggered, this, &MainWindow::loadEvoked);
    connect(saveEvokedAction, &QAction::triggered, this, &MainWindow::saveEvoked);
    connect(m_pWhitenButterflyAction, &QAction::toggled, this, [this](bool checked){
        if(checked && m_covariance.isEmpty()) {
            QMessageBox::warning(this,
                                 "Whiten Butterfly Plot",
                                 "Load or compute a covariance matrix before enabling whitening.");
            m_pWhitenButterflyAction->setChecked(false);
            return;
        }

        m_pAverageWindow->setButterflyWhiteningEnabled(checked);

        if(checked && !m_pAverageWindow->isVisible()) {
            showWindow(m_pAverageWindow);
        }
    });
    connect(ui->m_quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    //Adjust
    connect(ui->m_filterAction, &QAction::triggered, this, [this](){
        showWindow(m_pFilterWindow);
    });
    connect(m_pAnnotationModeAction, &QAction::toggled, this, [this](bool checked) {
        m_pDataWindow->setAnnotationSelectionEnabled(checked);
        statusBar()->showMessage(checked
                                 ? QStringLiteral("Annotation mode enabled: Shift-drag in the raw browser to create spans.")
                                 : QStringLiteral("Annotation mode disabled."),
                                 4000);
    });

    //Windows
    connect(ui->m_eventAction, &QAction::triggered, this, [this](){
        showWindow(m_pEventWindow);
    });
    connect(annotationManagerAction, &QAction::triggered, this, [this](){
        showWindow(m_pAnnotationWindow);
    });
    connect(virtualChannelManagerAction, &QAction::triggered, this, [this](){
        showWindow(m_pVirtualChannelWindow);
    });
    connect(ui->m_informationAction, &QAction::triggered, this, [this](){
        showWindow(m_pInformationWindow);
    });
    connect(ui->m_channelSelectionManagerAction, &QAction::triggered, this, [this](){
        showWindow(m_pChannelSelectionViewDock);
    });
    connect(ui->m_averageWindowAction, &QAction::triggered, this, [this](){
        showWindow(m_pAverageWindow);
    });
    connect(ui->m_scalingAction, &QAction::triggered, this, [this](){
        showWindow(m_pScaleWindow);
    });
    connect(ui->m_ChInformationAction, &QAction::triggered, this, [this](){
        showWindow(m_pChInfoWindow);
    });
    connect(ui->m_noiseReductionManagerAction, &QAction::triggered, this, [this](){
        showWindow(m_pNoiseReductionWindow);
    });

    //Help
    connect(ui->m_aboutAction, &QAction::triggered, this, [this](){
        showWindow(m_pAboutWindow);
    });
}


//*************************************************************************************************************

void MainWindow::setupMainWindow()
{
    //set Window functions
    resize(m_qSettings.value("MainWindow/size", QSize(MAINWINDOW_WINDOW_SIZE_W, MAINWINDOW_WINDOW_SIZE_H)).toSize()); //Resize to predefined default size
    move(m_qSettings.value("MainWindow/position", QPoint(MAINWINDOW_WINDOW_POSITION_X, MAINWINDOW_WINDOW_POSITION_Y)).toPoint()); // Move this main window to position 50/50 on the screen

    //Set data window as central widget - This is needed because we are using QDockWidgets
    setCentralWidget(m_pDataWindow);
}


//*************************************************************************************************************

void MainWindow::setWindowStatus()
{
    //Set window title
    QString title;
    //title = QString("%1").arg(CInfo::AppNameShort());
    title = QString("Visualize and Process");
    setWindowTitle(title);

    //Set status bar
    //Set data file informations
    if (m_pDataWindow->isFiffFileLoaded()) {
        double dur = m_pDataWindow->fiffFileDurationSeconds();
        int durMin = static_cast<int>(dur) / 60;
        double durSec = dur - durMin * 60.0;
        QString durStr = durMin > 0
            ? QString("%1 min %2 s").arg(durMin).arg(durSec, 0, 'f', 1)
            : QString("%1 s").arg(dur, 0, 'f', 1);
        title = QString("Data file: %1  |  Duration: %2")
            .arg(m_pDataWindow->fiffFileName(), durStr);
    } else if (rawModel()->isFileLoaded() && rawModel()->fiffInfo()) {
        int idx = m_qFileRaw.fileName().lastIndexOf("/");
        QString filename = m_qFileRaw.fileName().remove(0,idx+1);
        title = QString("Data file: %1  /  First sample: %2  /  Sample frequency: %3Hz").arg(filename).arg(rawModel()->firstSample()).arg(rawModel()->fiffInfo()->sfreq);
    } else {
        title = QString("No data file");
    }

    //Set event file informations
    if(m_pEventWindow->getEventModel()->isFileLoaded()) {
        int idx = m_qEventFile.fileName().lastIndexOf("/");
        QString filename = m_qEventFile.fileName().remove(0,idx+1);

        title.append(QString("  -  Event file: %1").arg(filename));
    }
    else
        title.append("  -  No event file");

    if(m_pAnnotationWindow->getAnnotationModel()->rowCount() > 0) {
        if(m_qAnnotationFile.fileName().isEmpty()) {
            title.append("  -  Annotations: unsaved");
        } else {
            int idx = m_qAnnotationFile.fileName().lastIndexOf("/");
            QString filename = m_qAnnotationFile.fileName().remove(0,idx+1);
            title.append(QString("  -  Annotations: %1").arg(filename));
        }
    } else {
        title.append("  -  No annotations");
    }

    if(m_pVirtualChannelWindow->getVirtualChannelModel()->rowCount() > 0) {
        if(m_qVirtualChannelFile.fileName().isEmpty()) {
            title.append("  -  Virtual channels: unsaved");
        } else {
            int idx = m_qVirtualChannelFile.fileName().lastIndexOf("/");
            QString filename = m_qVirtualChannelFile.fileName().remove(0,idx+1);
            title.append(QString("  -  Virtual channels: %1").arg(filename));
        }
    } else {
        title.append("  -  No virtual channels");
    }

    //Set evoked file informations
    if(m_pAverageWindow->getAverageModel()->isFileLoaded()) {
        if(m_qEvokedFile.fileName().isEmpty()) {
            title.append("  -  Evoked data: unsaved");
        } else {
            int idx = m_qEvokedFile.fileName().lastIndexOf("/");
            QString filename = m_qEvokedFile.fileName().remove(0,idx+1);
            title.append(QString("  -  Evoked file: %1").arg(filename));
        }
    }
    else
        title.append("  -  No evoked file");

    if(!m_covariance.isEmpty()) {
        if(m_qCovFile.fileName().isEmpty()) {
            title.append("  -  Covariance: unsaved");
        } else {
            int idx = m_qCovFile.fileName().lastIndexOf("/");
            QString filename = m_qCovFile.fileName().remove(0,idx+1);
            title.append(QString("  -  Covariance file: %1").arg(filename));
        }
    } else {
        title.append("  -  No covariance");
    }

    if(!m_pStatusLabel) {
        m_pStatusLabel = new QLabel(this);
        statusBar()->addWidget(m_pStatusLabel, 1);
    }
    m_pStatusLabel->setText(title);
}

//*************************************************************************************************************

void MainWindow::syncAuxWindowsToFiffInfo(FiffInfo::SPtr fiffInfo,
                                          int firstSample,
                                          int lastSample)
{
    if (!fiffInfo)
        return;

    m_pEventWindow->getEventModel()->setFiffInfo(fiffInfo);
    m_pEventWindow->getEventModel()->setFirstLastSample(firstSample, lastSample);
    m_pAnnotationWindow->getAnnotationModel()->setFiffInfo(fiffInfo);
    m_pAnnotationWindow->getAnnotationModel()->setFirstLastSample(firstSample, lastSample);
    m_pVirtualChannelWindow->setAvailableChannelNames(fiffInfo->ch_names);

    m_pScaleWindow->hideSpinBoxes(fiffInfo);

    m_pChInfoWindow->getDataModel()->setFiffInfo(fiffInfo);
    m_pChInfoWindow->getDataModel()->layoutChanged(m_pChannelSelectionView->getLayoutMap());
    m_pChannelSelectionView->setCurrentlyMappedFiffChannels(m_pChInfoWindow->getDataModel()->getMappedChannelsList());
    m_pChannelSelectionView->newFiffFileLoaded(fiffInfo);

    m_pFilterWindow->newFileLoaded(fiffInfo);
    m_pNoiseReductionWindow->setFiffInfo(fiffInfo);
}


//*************************************************************************************************************
//Log
void MainWindow::writeToLog(const QString& logMsg, LogKind lgknd, LogLevel lglvl)
{
    m_pInformationWindow->writeToLog(logMsg, lgknd, lglvl);
}


//*************************************************************************************************************

void MainWindow::setLogLevel(LogLevel lvl)
{
    m_pInformationWindow->setLogLevel(lvl);
}


//*************************************************************************************************************
// SLOTS
#ifdef WASMBUILD
static QByteArray s_wasmByteArray;
#endif

void MainWindow::openFile()
{
#ifdef WASMBUILD
    auto fileContentReady = [&](const QString &fileName, const QByteArray &fileContent) {
        if (!fileName.isEmpty()) {
            m_qFileRaw.setFileName(fileName);

            //Clear event model
            m_pEventWindow->getEventModel()->clearModel();

            s_wasmByteArray = fileContent;
            QBuffer* buffer = new QBuffer(&s_wasmByteArray);
            if(rawModel()->loadFiffData(buffer))
                qInfo() << "Fiff data file" << fileName << "loaded.";
            else
                qWarning() << "ERROR loading fiff data file" << fileName;

            //set position of QScrollArea
            m_pDataWindow->getDataTableView()->horizontalScrollBar()->setValue(0);
            m_pDataWindow->initMVCSettings();

            //Set fiffInfo in event model
            m_pEventWindow->getEventModel()->setFiffInfo(rawModel()->fiffInfo());
            m_pEventWindow->getEventModel()->setFirstLastSample(rawModel()->firstSample(),
                                                                rawModel()->lastSample());

            //resize columns to contents - needs to be done because the new data file can be shorter than the old one
            m_pDataWindow->updateDataTableViews();
            m_pDataWindow->getDataTableView()->resizeColumnsToContents();

            //Update status bar
            setWindowStatus();

            //Hide not presented channel types and their spin boxes in the scale window
            m_pScaleWindow->hideSpinBoxes(rawModel()->fiffInfo());

            m_qFileRaw.close();
        }
    };
    QFileDialog::getOpenFileContent("Fiff File (*.fif *.fiff)",  fileContentReady);
#else
    QString filename = QFileDialog::getOpenFileName(this,
                                                    QString("Open fiff data file"),
                                                    QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),
                                                    tr("fif data files (*.fif)"));

    if(filename.isEmpty())
        return;

    loadRawFile(filename);
#endif
}


//*************************************************************************************************************

void MainWindow::writeFile()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    QString("Write fiff data file"),
                                                    QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),
                                                    tr("fif data files (*.fif)"));

    if(filename.isEmpty())
        return;

    if(filename == m_qFileRaw.fileName()) {
        QMessageBox msgBox;
        msgBox.setText("You are trying to write to the file you are currently loading the data from. Please choose another file to write to.");
        msgBox.exec();
        return;
    }

    //Create output file, progress dialog and future watcher
    QFile qFileOutput (filename);
    if(qFileOutput.isOpen())
        qFileOutput.close();

    QFutureWatcher<bool> writeFileFutureWatcher;
    QProgressDialog progressDialog("Writing to fif file...", QString(), 0, 0, this, Qt::Dialog);

    //Connect future watcher and dialog
    connect(&writeFileFutureWatcher, &QFutureWatcher<bool>::finished,
            &progressDialog, &QProgressDialog::reset);

    connect(&progressDialog, &QProgressDialog::canceled,
            &writeFileFutureWatcher, &QFutureWatcher<bool>::cancel);

    connect(rawModel(), &RawModel::writeProgressRangeChanged,
            &progressDialog, &QProgressDialog::setRange);

    connect(rawModel(), &RawModel::writeProgressChanged,
            &progressDialog, &QProgressDialog::setValue);

    //Run the file writing in seperate thread
    writeFileFutureWatcher.setFuture(QtConcurrent::run([this, &qFileOutput](){
        return rawModel()->writeFiffData(&qFileOutput);
    }));

    progressDialog.exec();

    writeFileFutureWatcher.waitForFinished();

    if(!writeFileFutureWatcher.future().result())
        qWarning() << "MainWindow: ERROR writing fiff data file" << qFileOutput.fileName();
    else
        qInfo() << "MainWindow: Successfully written to" << qFileOutput.fileName();
}


//*************************************************************************************************************

void MainWindow::loadEvents()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    QString("Open fiff event data file"),
                                                    QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),
                                                    tr("fif event data files (*-eve.fif);;fif data files (*.fif)"));

    if(filename.isEmpty())
        return;

    loadEventsFile(filename);
}


//*************************************************************************************************************

void MainWindow::saveEvents()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    QString("Save fiff event data file"),
                                                    QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),
                                                    tr("fif event data files (*-eve.fif);;fif data files (*.fif)"));
    if(filename.isEmpty())
        return;

    if(m_qEventFile.isOpen())
        m_qEventFile.close();
    m_qEventFile.setFileName(filename);

    if(m_pEventWindow->getEventModel()->saveEventData(m_qEventFile))
        qInfo() << "Fiff event data file" << filename << "saved.";
    else
        qWarning() << "ERROR saving fiff event data file" << filename;
}

//*************************************************************************************************************

void MainWindow::loadAnnotations()
{
    if(m_qFileRaw.fileName().isEmpty()) {
        QMessageBox::warning(this,
                             "Load Annotations",
                             "Load a raw FIF file before opening annotations.");
        return;
    }

    QString filename = QFileDialog::getOpenFileName(this,
                                                    QString("Open annotation file"),
                                                    QFileInfo(defaultAnnotationFilePath(m_qFileRaw.fileName())).absolutePath(),
                                                    tr("annotation files (*-annot.json *.json)"));

    if(filename.isEmpty()) {
        return;
    }

    loadAnnotationsFile(filename);
}

//*************************************************************************************************************

void MainWindow::saveAnnotations()
{
    if(m_pAnnotationWindow->getAnnotationModel()->rowCount() == 0) {
        QMessageBox::warning(this,
                             "Save Annotations",
                             "Create or load at least one annotation before saving.");
        return;
    }

    QString defaultPath = m_qAnnotationFile.fileName();
    if(defaultPath.isEmpty()) {
        defaultPath = defaultAnnotationFilePath(m_qFileRaw.fileName());
    }

    QString filename = QFileDialog::getSaveFileName(this,
                                                    QString("Save annotation file"),
                                                    defaultPath,
                                                    tr("annotation files (*-annot.json *.json)"));

    if(filename.isEmpty()) {
        return;
    }

    if(m_qAnnotationFile.isOpen()) {
        m_qAnnotationFile.close();
    }
    m_qAnnotationFile.setFileName(filename);

    if(m_pAnnotationWindow->getAnnotationModel()->saveAnnotationData(m_qAnnotationFile)) {
        qInfo() << "Annotation file" << filename << "saved.";
    } else {
        qWarning() << "ERROR saving annotation file" << filename;
    }

    setWindowStatus();
}

//*************************************************************************************************************

void MainWindow::loadVirtualChannels()
{
    if(m_qFileRaw.fileName().isEmpty()) {
        QMessageBox::warning(this,
                             "Load Virtual Channels",
                             "Load a raw FIF file before opening virtual channels.");
        return;
    }

    const QString filename = QFileDialog::getOpenFileName(this,
                                                          QString("Open virtual-channel file"),
                                                          QFileInfo(defaultVirtualChannelFilePath(m_qFileRaw.fileName())).absolutePath(),
                                                          tr("virtual channel files (*-virtchan.json *.json)"));

    if(filename.isEmpty()) {
        return;
    }

    loadVirtualChannelsFile(filename);
}

//*************************************************************************************************************

void MainWindow::saveVirtualChannels()
{
    if(m_pVirtualChannelWindow->getVirtualChannelModel()->rowCount() == 0) {
        QMessageBox::warning(this,
                             "Save Virtual Channels",
                             "Create or load at least one virtual channel before saving.");
        return;
    }

    QString defaultPath = m_qVirtualChannelFile.fileName();
    if(defaultPath.isEmpty()) {
        defaultPath = defaultVirtualChannelFilePath(m_qFileRaw.fileName());
    }

    const QString filename = QFileDialog::getSaveFileName(this,
                                                          QString("Save virtual-channel file"),
                                                          defaultPath,
                                                          tr("virtual channel files (*-virtchan.json *.json)"));

    if(filename.isEmpty()) {
        return;
    }

    if(m_qVirtualChannelFile.isOpen()) {
        m_qVirtualChannelFile.close();
    }
    m_qVirtualChannelFile.setFileName(filename);

    if(m_pVirtualChannelWindow->getVirtualChannelModel()->saveVirtualChannels(m_qVirtualChannelFile)) {
        qInfo() << "Virtual-channel file" << filename << "saved.";
        setWindowStatus();
    } else {
        qWarning() << "ERROR saving virtual-channel file" << filename;
    }
}


//*************************************************************************************************************

bool MainWindow::loadRawFile(const QString& filename)
{
    if(m_qFileRaw.isOpen())
        m_qFileRaw.close();

    m_qFileRaw.setFileName(filename);
    m_covariance.clear();
    if(m_qCovFile.isOpen())
        m_qCovFile.close();
    m_qCovFile.setFileName(QString());
    if(m_qAnnotationFile.isOpen())
        m_qAnnotationFile.close();
    m_qAnnotationFile.setFileName(QString());
    if(m_qVirtualChannelFile.isOpen())
        m_qVirtualChannelFile.close();
    m_qVirtualChannelFile.setFileName(QString());
    m_pAverageWindow->clearNoiseCovariance();
    if(m_pWhitenButterflyAction) {
        m_pWhitenButterflyAction->setChecked(false);
    }

    m_pEventWindow->getEventModel()->clearModel();
    m_pAnnotationWindow->getAnnotationModel()->clearModel();
    {
        QSignalBlocker blocker(m_pVirtualChannelWindow->getVirtualChannelModel());
        m_pVirtualChannelWindow->getVirtualChannelModel()->clearModel();
    }
    m_pVirtualChannelWindow->setAvailableChannelNames(QStringList());
    m_pDataWindow->setVirtualChannels({}, false);

    // ── ChannelDataView path: demand-paged, opens header only (fast) ──
    const bool ok = m_pDataWindow->loadFiffFile(filename);
    if(ok)
        qInfo() << "Fiff data file" << filename << "loaded (ChannelDataView).";
    else
        qWarning() << "ERROR loading fiff data file" << filename;

    if (ok)
        syncAuxWindowsToFiffInfo(m_pDataWindow->fiffInfo(),
                                 m_pDataWindow->firstSample(),
                                 m_pDataWindow->lastSample());

    if (ok) {
        const QString defaultAnnotationPath = defaultAnnotationFilePath(filename);
        if(QFileInfo::exists(defaultAnnotationPath)) {
            loadAnnotationsFile(defaultAnnotationPath, false);
        }

        const QString defaultVirtualChannelPath = defaultVirtualChannelFilePath(filename);
        if(QFileInfo::exists(defaultVirtualChannelPath)) {
            loadVirtualChannelsFile(defaultVirtualChannelPath, false);
        }
    }

    // ── Legacy RawModel path: kept for filter/event sub-systems ──────
    // Runs in a background thread so the ChannelDataView path (above) can
    // display data immediately while the full FIFF index is parsed.
    // Wait for any previous in-flight load to finish before starting a new one
    // to avoid concurrent access to RawModel internals.
    if (m_legacyLoadWatcher.isRunning())
        m_legacyLoadWatcher.waitForFinished();

    // Disconnect any stale finished connection from a prior load
    disconnect(&m_legacyLoadWatcher, &QFutureWatcher<bool>::finished, nullptr, nullptr);

    connect(&m_legacyLoadWatcher, &QFutureWatcher<bool>::finished, this, [this]() {
        auto fiffInfo = rawModel()->fiffInfo();
        syncAuxWindowsToFiffInfo(fiffInfo,
                                 rawModel()->firstSample(),
                                 rawModel()->lastSample());
        setWindowStatus();
        m_qFileRaw.close();
    });

    m_legacyLoadWatcher.setFuture(
        QtConcurrent::run([this]() -> bool {
            return rawModel()->loadFiffData(&m_qFileRaw);
        })
    );

    setWindowStatus();
    return ok;
}


//*************************************************************************************************************

bool MainWindow::loadEventsFile(const QString& filename)
{
    if(m_qEventFile.isOpen())
        m_qEventFile.close();

    m_qEventFile.setFileName(filename);

    const bool ok = m_pEventWindow->getEventModel()->loadEventData(m_qEventFile);
    if(ok)
        qInfo() << "Fiff event data file" << filename << "loaded.";
    else
        qWarning() << "ERROR loading fiff event data file" << filename;

    setWindowStatus();

    if(ok && !m_pEventWindow->isVisible())
        m_pEventWindow->show();

    return ok;
}

//*************************************************************************************************************

bool MainWindow::loadAnnotationsFile(const QString& filename, bool showWindow)
{
    if(m_qAnnotationFile.isOpen()) {
        m_qAnnotationFile.close();
    }

    m_qAnnotationFile.setFileName(filename);

    const bool ok = m_pAnnotationWindow->getAnnotationModel()->loadAnnotationData(m_qAnnotationFile);
    if(ok) {
        qInfo() << "Annotation file" << filename << "loaded.";
    } else {
        qWarning() << "ERROR loading annotation file" << filename;
    }

    setWindowStatus();

    if(ok && showWindow && !m_pAnnotationWindow->isVisible()) {
        m_pAnnotationWindow->show();
    }

    return ok;
}

//*************************************************************************************************************

bool MainWindow::loadVirtualChannelsFile(const QString& filename, bool showWindow)
{
    if(m_qVirtualChannelFile.isOpen()) {
        m_qVirtualChannelFile.close();
    }

    m_qVirtualChannelFile.setFileName(filename);

    const bool ok = m_pVirtualChannelWindow->getVirtualChannelModel()->loadVirtualChannels(m_qVirtualChannelFile);
    if(ok) {
        qInfo() << "Virtual-channel file" << filename << "loaded.";
    } else {
        qWarning() << "ERROR loading virtual-channel file" << filename;
    }

    setWindowStatus();

    if(ok && showWindow && !m_pVirtualChannelWindow->isVisible()) {
        m_pVirtualChannelWindow->show();
    }

    return ok;
}

//*************************************************************************************************************

void MainWindow::handleAnnotationRangeSelected(int startSample, int endSample)
{
    if(startSample > endSample) {
        std::swap(startSample, endSample);
    }

    if(startSample == endSample) {
        return;
    }

    const QString defaultLabel = m_qSettings.value("MainWindow/Annotations/defaultLabel",
                                                   QStringLiteral("BAD_manual")).toString();

    bool accepted = false;
    QString label = QInputDialog::getText(this,
                                          QStringLiteral("Add Annotation"),
                                          QStringLiteral("Annotation label"),
                                          QLineEdit::Normal,
                                          defaultLabel,
                                          &accepted).trimmed();

    if(!accepted) {
        return;
    }

    if(label.isEmpty()) {
        label = defaultLabel;
    }

    m_qSettings.setValue("MainWindow/Annotations/defaultLabel", label);

    m_pAnnotationWindow->addAnnotation(startSample, endSample, label);
    if(!m_pAnnotationWindow->isVisible()) {
        showWindow(m_pAnnotationWindow);
    }

    if(m_qAnnotationFile.fileName().isEmpty()) {
        setWindowStatus();
    }
}


//*************************************************************************************************************

void MainWindow::computeEvoked()
{
    if(m_qFileRaw.fileName().isEmpty() || !QFile::exists(m_qFileRaw.fileName())) {
        QMessageBox::warning(this,
                             "Compute Evoked",
                             "Load a raw FIF file before computing evoked responses.");
        return;
    }

    QFile rawFile(m_qFileRaw.fileName());
    FIFFLIB::FiffRawData raw(rawFile);

    if(raw.isEmpty()) {
        QMessageBox::warning(this,
                             "Compute Evoked",
                             QString("Could not open raw data from %1.").arg(m_qFileRaw.fileName()));
        return;
    }

    MatrixXi events = m_pEventWindow->getEventModel()->getEventMatrix();
    QString eventSourceDescription;

    if(events.rows() > 0) {
        eventSourceDescription = QString("Using %1 event(s) from the event manager.")
                                     .arg(events.rows());
    } else if(!detectFallbackStimEvents(raw, events, eventSourceDescription)) {
        QMessageBox::warning(this,
                             "Compute Evoked",
                             "No events were loaded, and no trigger events could be detected from STI 014 or STI 101.");
        return;
    }

    const QMap<int, int> eventCounts = countEventsByType(events);
    if(eventCounts.isEmpty()) {
        QMessageBox::warning(this,
                             "Compute Evoked",
                             "No non-zero event codes were found for averaging.");
        return;
    }

    QList<int> selectedEventCodes;
    float tmin = -0.1f;
    float tmax = 0.4f;
    bool applyBaseline = true;
    float baselineFrom = -0.1f;
    float baselineTo = 0.0f;
    bool dropRejected = true;

    if(!showComputeEvokedDialog(this,
                                m_qSettings,
                                eventCounts,
                                eventSourceDescription,
                                selectedEventCodes,
                                tmin,
                                tmax,
                                applyBaseline,
                                baselineFrom,
                                baselineTo,
                                dropRejected)) {
        return;
    }

    QStringList comments;
    for(int eventCode : selectedEventCodes) {
        comments << QString::number(eventCode);
    }

    QMap<QString,double> rejectMap;
    if(dropRejected) {
        rejectMap.insert("grad", 2000e-13);
        rejectMap.insert("mag", 3e-12);
        rejectMap.insert("eeg", 100e-6);
        rejectMap.insert("eog", 150e-6);
    }

    const QPair<float, float> baseline = applyBaseline
        ? QPair<float, float>(baselineFrom, baselineTo)
        : QPair<float, float>(0.0f, 0.0f);

    QApplication::setOverrideCursor(Qt::BusyCursor);
    qApp->processEvents();

    const FIFFLIB::FiffEvokedSet evokedSet = MNELIB::MNEEpochDataList::averageCategories(raw,
                                                                                           events,
                                                                                           selectedEventCodes,
                                                                                           comments,
                                                                                           tmin,
                                                                                           tmax,
                                                                                           rejectMap,
                                                                                           baseline);

    QApplication::restoreOverrideCursor();

    if(evokedSet.evoked.isEmpty()) {
        QMessageBox::warning(this,
                             "Compute Evoked",
                             "No evoked responses could be computed. The selected events may have been rejected or out of bounds.");
        return;
    }

    if(m_qEvokedFile.isOpen()) {
        m_qEvokedFile.close();
    }
    m_qEvokedFile.setFileName(QString());

    if(!m_pAverageWindow->getAverageModel()->setEvokedData(evokedSet)) {
        QMessageBox::warning(this,
                             "Compute Evoked",
                             "The computed evoked set could not be loaded into the average manager.");
        return;
    }

    setWindowStatus();
    m_pAverageWindow->show();
    m_pAverageWindow->raise();
}


//*************************************************************************************************************

void MainWindow::saveEvoked()
{
    if(!m_pAverageWindow->getAverageModel()->isFileLoaded()) {
        QMessageBox::warning(this,
                             "Save Evoked",
                             "There is no evoked data to save.");
        return;
    }

    const QString defaultPath = !m_qEvokedFile.fileName().isEmpty()
        ? m_qEvokedFile.fileName()
        : defaultEvokedFilePath(m_qFileRaw.fileName());

    const QString filename = QFileDialog::getSaveFileName(this,
                                                          QString("Save evoked fiff data file"),
                                                          defaultPath,
                                                          tr("fif evoked data files (*-ave.fif);;fif data files (*.fif)"));

    if(filename.isEmpty()) {
        return;
    }

    if(m_qEvokedFile.isOpen()) {
        m_qEvokedFile.close();
    }
    m_qEvokedFile.setFileName(filename);

    if(m_pAverageWindow->getAverageModel()->saveEvokedData(m_qEvokedFile)) {
        qInfo() << "Fiff evoked data file" << filename << "saved.";
        setWindowStatus();
    } else {
        qWarning() << "ERROR saving evoked data file" << filename;
        QMessageBox::warning(this,
                             "Save Evoked",
                             QString("Could not save evoked data to %1.").arg(filename));
    }
}


//*************************************************************************************************************

void MainWindow::computeCovariance()
{
    if(m_qFileRaw.fileName().isEmpty() || !QFile::exists(m_qFileRaw.fileName())) {
        QMessageBox::warning(this,
                             "Compute Covariance",
                             "Load a raw FIF file before computing a covariance matrix.");
        return;
    }

    QFile rawFile(m_qFileRaw.fileName());
    FIFFLIB::FiffRawData raw(rawFile);

    if(raw.isEmpty()) {
        QMessageBox::warning(this,
                             "Compute Covariance",
                             QString("Could not open raw data from %1.").arg(m_qFileRaw.fileName()));
        return;
    }

    MatrixXi events = m_pEventWindow->getEventModel()->getEventMatrix();
    QString eventSourceDescription;

    if(events.rows() > 0) {
        eventSourceDescription = QString("Using %1 event(s) from the event manager.")
                                     .arg(events.rows());
    } else if(!detectFallbackStimEvents(raw, events, eventSourceDescription)) {
        QMessageBox::warning(this,
                             "Compute Covariance",
                             "No events were loaded, and no trigger events could be detected from STI 014 or STI 101.");
        return;
    }

    const QMap<int, int> eventCounts = countEventsByType(events);
    if(eventCounts.isEmpty()) {
        QMessageBox::warning(this,
                             "Compute Covariance",
                             "No non-zero event codes were found for covariance computation.");
        return;
    }

    QList<int> selectedEventCodes;
    float tmin = -0.2f;
    float tmax = 0.0f;
    bool applyBaseline = false;
    float baselineFrom = -0.2f;
    float baselineTo = 0.0f;
    bool removeMean = true;

    if(!showComputeCovarianceDialog(this,
                                    m_qSettings,
                                    eventCounts,
                                    eventSourceDescription,
                                    selectedEventCodes,
                                    tmin,
                                    tmax,
                                    applyBaseline,
                                    baselineFrom,
                                    baselineTo,
                                    removeMean)) {
        return;
    }

    QApplication::setOverrideCursor(Qt::BusyCursor);
    qApp->processEvents();

    const FIFFLIB::FiffCov covariance = FIFFLIB::FiffCov::compute_from_epochs(raw,
                                                                               events,
                                                                               selectedEventCodes,
                                                                               tmin,
                                                                               tmax,
                                                                               baselineFrom,
                                                                               baselineTo,
                                                                               applyBaseline,
                                                                               removeMean);

    QApplication::restoreOverrideCursor();

    if(covariance.isEmpty()) {
        QMessageBox::warning(this,
                             "Compute Covariance",
                             "No covariance matrix could be computed. Check the selected events and time window.");
        return;
    }

    m_covariance = covariance;
    if(m_qCovFile.isOpen()) {
        m_qCovFile.close();
    }
    m_qCovFile.setFileName(QString());
    m_pAverageWindow->setNoiseCovariance(m_covariance);

    setWindowStatus();

    QMessageBox::information(this,
                             "Compute Covariance",
                             QString("Computed a %1 x %1 covariance matrix with %2 degrees of freedom.\nUse File > Save Covariance (fif)... to store it.")
                                 .arg(m_covariance.dim)
                                 .arg(m_covariance.nfree));
}


//*************************************************************************************************************

void MainWindow::loadCovariance()
{
    const QString filename = QFileDialog::getOpenFileName(this,
                                                          QString("Open covariance fiff data file"),
                                                          QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),
                                                          tr("fif covariance data files (*-cov.fif);;fif data files (*.fif)"));

    if(filename.isEmpty()) {
        return;
    }

    QFile covarianceFile(filename);
    FIFFLIB::FiffCov covariance(covarianceFile);

    if(covariance.isEmpty()) {
        QMessageBox::warning(this,
                             "Load Covariance",
                             QString("Could not load covariance data from %1.").arg(filename));
        return;
    }

    if(m_qCovFile.isOpen()) {
        m_qCovFile.close();
    }

    m_qCovFile.setFileName(filename);
    m_covariance = covariance;
    m_pAverageWindow->setNoiseCovariance(m_covariance);

    setWindowStatus();
}


//*************************************************************************************************************

void MainWindow::saveCovariance()
{
    if(m_covariance.isEmpty()) {
        QMessageBox::warning(this,
                             "Save Covariance",
                             "There is no covariance matrix to save.");
        return;
    }

    const QString defaultPath = !m_qCovFile.fileName().isEmpty()
        ? m_qCovFile.fileName()
        : defaultCovFilePath(m_qFileRaw.fileName());

    const QString filename = QFileDialog::getSaveFileName(this,
                                                          QString("Save covariance fiff data file"),
                                                          defaultPath,
                                                          tr("fif covariance data files (*-cov.fif);;fif data files (*.fif)"));

    if(filename.isEmpty()) {
        return;
    }

    if(m_qCovFile.isOpen()) {
        m_qCovFile.close();
    }
    m_qCovFile.setFileName(filename);

    if(m_covariance.save(filename)) {
        qInfo() << "Fiff covariance data file" << filename << "saved.";
        setWindowStatus();
    } else {
        qWarning() << "ERROR saving covariance data file" << filename;
        QMessageBox::warning(this,
                             "Save Covariance",
                             QString("Could not save covariance data to %1.").arg(filename));
    }
}


//*************************************************************************************************************

void MainWindow::applyCommandLineOptions(const QString& rawFile,
                                         const QString& eventsFile,
                                         double highpass,
                                         double lowpass)
{
    if(!rawFile.isEmpty()) {
        if(!QFile::exists(rawFile)) {
            qWarning() << "[mne_browse] --raw: file not found:" << rawFile;
        } else {
            loadRawFile(rawFile);
        }
    }

    if(!eventsFile.isEmpty()) {
        if(!QFile::exists(eventsFile)) {
            qWarning() << "[mne_browse] --events: file not found:" << eventsFile;
        } else {
            loadEventsFile(eventsFile);
        }
    }

    if(highpass >= 0.0 || lowpass >= 0.0) {
        m_pFilterWindow->setFrequencies(highpass, lowpass);
        m_pFilterWindow->applyFilter();
        if(!m_pFilterWindow->isVisible())
            m_pFilterWindow->show();
    }
}


//*************************************************************************************************************

void MainWindow::loadEvoked()
{
    QString filename = QFileDialog::getOpenFileName(this,QString("Open evoked fiff data file"),QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),tr("fif evoked data files (*-ave.fif);;fif data files (*.fif)"));

    if(filename.isEmpty())
        return;

    if(m_qEvokedFile.isOpen())
        m_qEvokedFile.close();

    m_qEvokedFile.setFileName(filename);

    if(m_pAverageWindow->getAverageModel()->loadEvokedData(m_qEvokedFile))
        qInfo() << "Fiff evoked data file" << filename << "loaded.";
    else
        qWarning() << "ERROR loading evoked data file" << filename;

    //Update status bar
    setWindowStatus();

    m_pAverageWindow->show();
    m_pAverageWindow->raise();
}


//*************************************************************************************************************

void MainWindow::showWindow(QWidget *window)
{
    //Note: A widget that happens to be obscured by other windows on the screen is considered to be visible.
    if(!window->isVisible())
    {
        window->show();
        window->raise();
    }
    else // if visible raise the widget to be sure that it is not obscured by other windows
        window->hide();
}
