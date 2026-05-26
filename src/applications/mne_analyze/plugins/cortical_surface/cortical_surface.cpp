//=============================================================================================================
/**
 * @file     cortical_surface.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    CorticalSurface plugin definition (TASK 4.1).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "cortical_surface.h"

#include <anShared/Management/communicator.h>

#include <disp3D/scene/multimodalscene.h>

#include <fs/fs_surface.h>

#include <inv/inv_source_estimate.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <algorithm>
#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CORTICALSURFACEPLUGIN;
using namespace ANSHAREDLIB;
using namespace DISP3DLIB;
using namespace FSLIB;
using namespace INVLIB;

//=============================================================================================================

CorticalSurface::CorticalSurface()
    : m_pScene(new MultimodalScene())
{
}

//=============================================================================================================

CorticalSurface::~CorticalSurface() = default;

//=============================================================================================================

QSharedPointer<AbstractPlugin> CorticalSurface::clone() const
{
    return QSharedPointer<CorticalSurface>::create();
}

//=============================================================================================================

void CorticalSurface::init()
{
    m_pCommu = new Communicator(this);
    buildControlDock();
}

//=============================================================================================================

void CorticalSurface::unload()
{
    pause();
    m_stc = InvSourceEstimate();
    m_currentSample = -1;
    m_pSurfaceLh.reset();
    m_pSurfaceRh.reset();
    if (m_pScene) {
        m_pScene->clear();
    }
}

//=============================================================================================================

QString CorticalSurface::getName() const
{
    return QStringLiteral("Cortical Surface");
}

//=============================================================================================================

QString CorticalSurface::getBuildInfo()
{
    return QStringLiteral("%1 - %2").arg(buildHash()).arg(buildDateTime());
}

//=============================================================================================================

QMenu* CorticalSurface::getMenu()
{
    if (!m_pMenu) {
        m_pMenu = new QMenu(getName());
        m_pLoadSurfaceAction = m_pMenu->addAction(QStringLiteral("Load Surface…"));
        connect(m_pLoadSurfaceAction.data(), &QAction::triggered,
                this, &CorticalSurface::onLoadSurfaceTriggered);
        m_pLoadStcAction = m_pMenu->addAction(QStringLiteral("Load STC Overlay…"));
        connect(m_pLoadStcAction.data(), &QAction::triggered,
                this, &CorticalSurface::onLoadStcTriggered);
    }
    return m_pMenu;
}

//=============================================================================================================

QDockWidget* CorticalSurface::getControl()
{
    return m_pControlDock;
}

//=============================================================================================================

QWidget* CorticalSurface::getView()
{
    // The 3-D rendering itself is owned by the existing View3D plugin in
    // v2.3.0; this plugin only contributes data layers to the shared
    // MultimodalScene. Returning nullptr keeps the host from creating an
    // empty central widget.
    return nullptr;
}

//=============================================================================================================

void CorticalSurface::handleEvent(QSharedPointer<Event> /*e*/)
{
    // No subscriptions yet. STC overlay events (TASK 4.2) and vertex-pick
    // requests (TASK 4.3) plug in here.
}

//=============================================================================================================

QVector<EVENT_TYPE> CorticalSurface::getEventSubscriptions() const
{
    return {};
}

//=============================================================================================================

MultimodalScene* CorticalSurface::scene() const
{
    return m_pScene.data();
}

//=============================================================================================================

void CorticalSurface::buildControlDock()
{
    if (m_pControlDock) {
        return;
    }
    m_pControlDock = new QDockWidget(getName());
    auto* container = new QWidget(m_pControlDock);
    auto* form = new QFormLayout(container);

    m_pHemiCombo = new QComboBox(container);
    m_pHemiCombo->addItem(QStringLiteral("Left only"),  static_cast<int>(HemisphereChoice::LeftOnly));
    m_pHemiCombo->addItem(QStringLiteral("Right only"), static_cast<int>(HemisphereChoice::RightOnly));
    m_pHemiCombo->addItem(QStringLiteral("Both"),       static_cast<int>(HemisphereChoice::Both));
    m_pHemiCombo->setCurrentIndex(2);
    connect(m_pHemiCombo.data(), QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CorticalSurface::onHemisphereChoiceChanged);
    form->addRow(QStringLiteral("Hemispheres"), m_pHemiCombo);

    m_pSurfaceTypeCombo = new QComboBox(container);
    m_pSurfaceTypeCombo->addItem(QStringLiteral("Inflated"), static_cast<int>(CorticalSurfaceType::Inflated));
    m_pSurfaceTypeCombo->addItem(QStringLiteral("Pial"),     static_cast<int>(CorticalSurfaceType::Pial));
    m_pSurfaceTypeCombo->addItem(QStringLiteral("White"),    static_cast<int>(CorticalSurfaceType::White));
    connect(m_pSurfaceTypeCombo.data(), QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CorticalSurface::onSurfaceTypeChanged);
    form->addRow(QStringLiteral("Surface type"), m_pSurfaceTypeCombo);

    auto* loadButton = new QPushButton(QStringLiteral("Load Surface…"), container);
    connect(loadButton, &QPushButton::clicked, this, &CorticalSurface::onLoadSurfaceTriggered);
    form->addRow(loadButton);

    // --- STC overlay controls -------------------------------------------------
    auto* overlayGroup = new QGroupBox(QStringLiteral("STC Overlay"), container);
    auto* overlayForm  = new QFormLayout(overlayGroup);

    auto* loadStcButton = new QPushButton(QStringLiteral("Load STC…"), overlayGroup);
    connect(loadStcButton, &QPushButton::clicked,
            this, &CorticalSurface::onLoadStcTriggered);
    overlayForm->addRow(loadStcButton);

    auto makeThresholdSpin = [&](float initial) {
        auto* spin = new QDoubleSpinBox(overlayGroup);
        spin->setDecimals(4);
        spin->setRange(-1.0e9, 1.0e9);
        spin->setSingleStep(0.1);
        spin->setValue(initial);
        return spin;
    };

    m_pFThreshSpin = makeThresholdSpin(m_fThresh);
    connect(m_pFThreshSpin.data(),
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CorticalSurface::onFThreshSpinChanged);
    overlayForm->addRow(QStringLiteral("fthresh"), m_pFThreshSpin);

    m_pFMidSpin = makeThresholdSpin(m_fMid);
    connect(m_pFMidSpin.data(),
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CorticalSurface::onFMidSpinChanged);
    overlayForm->addRow(QStringLiteral("fmid"), m_pFMidSpin);

    m_pFMaxSpin = makeThresholdSpin(m_fMax);
    connect(m_pFMaxSpin.data(),
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CorticalSurface::onFMaxSpinChanged);
    overlayForm->addRow(QStringLiteral("fmax"), m_pFMaxSpin);

    m_pTimeSlider = new QSlider(Qt::Horizontal, overlayGroup);
    m_pTimeSlider->setRange(0, 0);
    m_pTimeSlider->setEnabled(false);
    connect(m_pTimeSlider.data(), &QSlider::valueChanged,
            this, &CorticalSurface::onTimeSliderChanged);
    overlayForm->addRow(QStringLiteral("time"), m_pTimeSlider);

    m_pTimeReadout = new QLabel(QStringLiteral("—"), overlayGroup);
    overlayForm->addRow(QStringLiteral("sample"), m_pTimeReadout);

    auto* playRow = new QWidget(overlayGroup);
    auto* playLayout = new QHBoxLayout(playRow);
    playLayout->setContentsMargins(0, 0, 0, 0);
    m_pPlayButton = new QPushButton(QStringLiteral("Play"), playRow);
    m_pPlayButton->setEnabled(false);
    connect(m_pPlayButton.data(), &QPushButton::clicked,
            this, &CorticalSurface::onPlayPauseClicked);
    playLayout->addWidget(m_pPlayButton);

    m_pFpsSpin = new QDoubleSpinBox(playRow);
    m_pFpsSpin->setRange(0.1, 240.0);
    m_pFpsSpin->setDecimals(2);
    m_pFpsSpin->setSuffix(QStringLiteral(" fps"));
    m_pFpsSpin->setValue(m_fps);
    connect(m_pFpsSpin.data(),
            QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CorticalSurface::onPlaybackFpsChanged);
    playLayout->addWidget(m_pFpsSpin);
    overlayForm->addRow(QStringLiteral("playback"), playRow);

    form->addRow(overlayGroup);

    if (!m_pPlaybackTimer) {
        m_pPlaybackTimer = new QTimer(this);
        connect(m_pPlaybackTimer.data(), &QTimer::timeout,
                this, &CorticalSurface::onPlaybackTick);
    }

    container->setLayout(form);
    m_pControlDock->setWidget(container);
}

//=============================================================================================================

void CorticalSurface::onLoadSurfaceTriggered()
{
    const QString subjectsDir = QFileDialog::getExistingDirectory(
        nullptr,
        QStringLiteral("Select FreeSurfer SUBJECTS_DIR"),
        m_lastSubjectsDir);
    if (subjectsDir.isEmpty()) {
        return;
    }

    const QStringList subjects = enumerateSubjects(subjectsDir);
    if (subjects.isEmpty()) {
        QMessageBox::warning(nullptr, getName(),
                             QStringLiteral("No FreeSurfer subjects found in '%1' "
                                            "(expected sub-directories with surf/lh.* files).")
                                 .arg(subjectsDir));
        return;
    }

    bool ok = false;
    const QString subjectId = QInputDialog::getItem(
        nullptr,
        QStringLiteral("Choose FreeSurfer subject"),
        QStringLiteral("Subject"),
        subjects,
        0, /*editable*/ false, &ok);
    if (!ok || subjectId.isEmpty()) {
        return;
    }

    if (!loadSurfaces(subjectsDir, subjectId, m_hemi, m_surfaceType)) {
        QMessageBox::warning(nullptr, getName(),
                             QStringLiteral("Failed to load surfaces for subject '%1'.")
                                 .arg(subjectId));
    }
}

//=============================================================================================================

void CorticalSurface::onHemisphereChoiceChanged(int index)
{
    if (!m_pHemiCombo) {
        return;
    }
    m_hemi = static_cast<HemisphereChoice>(m_pHemiCombo->itemData(index).toInt());
    refreshSceneLayers();
}

//=============================================================================================================

void CorticalSurface::onSurfaceTypeChanged(int index)
{
    if (!m_pSurfaceTypeCombo) {
        return;
    }
    const auto newType = static_cast<CorticalSurfaceType>(m_pSurfaceTypeCombo->itemData(index).toInt());
    if (newType == m_surfaceType) {
        return;
    }
    m_surfaceType = newType;
    if (!m_lastSubjectsDir.isEmpty() && !m_lastSubjectId.isEmpty()) {
        loadSurfaces(m_lastSubjectsDir, m_lastSubjectId, m_hemi, m_surfaceType);
    }
}

//=============================================================================================================

bool CorticalSurface::loadSurfaces(const QString& subjectsDir,
                                   const QString& subjectId,
                                   HemisphereChoice hemi,
                                   CorticalSurfaceType type)
{
    if (subjectsDir.isEmpty() || subjectId.isEmpty()) {
        return false;
    }

    m_lastSubjectsDir = subjectsDir;
    m_lastSubjectId = subjectId;
    m_hemi = hemi;
    m_surfaceType = type;

    bool anyLoaded = false;

    auto loadOne = [&](int hemiCode, QSharedPointer<FsSurface>& slot) {
        const QString path = surfaceFilePath(subjectsDir, subjectId, hemiCode, type);
        if (!QFileInfo::exists(path)) {
            slot.reset();
            qWarning() << "[CorticalSurface] Surface file not found:" << path;
            return;
        }
        auto pSurf = QSharedPointer<FsSurface>::create();
        if (!FsSurface::read(path, *pSurf, /*loadCurvature*/ true)) {
            slot.reset();
            qWarning() << "[CorticalSurface] FsSurface::read failed for" << path;
            return;
        }
        slot = pSurf;
        anyLoaded = true;
    };

    const bool wantLh = (hemi != HemisphereChoice::RightOnly);
    const bool wantRh = (hemi != HemisphereChoice::LeftOnly);

    if (wantLh) {
        loadOne(/*lh*/ 0, m_pSurfaceLh);
    } else {
        m_pSurfaceLh.reset();
    }
    if (wantRh) {
        loadOne(/*rh*/ 1, m_pSurfaceRh);
    } else {
        m_pSurfaceRh.reset();
    }

    refreshSceneLayers();
    return anyLoaded;
}

//=============================================================================================================

QStringList CorticalSurface::enumerateSubjects(const QString& subjectsDir)
{
    QStringList result;
    QDir root(subjectsDir);
    if (!root.exists()) {
        return result;
    }
    const QStringList candidates = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString& candidate : candidates) {
        // A FreeSurfer subject directory must contain at least one of
        // surf/lh.inflated or surf/rh.inflated for us to consider it.
        const QString surfDir = root.filePath(candidate) + QStringLiteral("/surf");
        QFileInfo lh(surfDir + QStringLiteral("/lh.inflated"));
        QFileInfo rh(surfDir + QStringLiteral("/rh.inflated"));
        if (lh.exists() || rh.exists()) {
            result.append(candidate);
        }
    }
    return result;
}

//=============================================================================================================

QString CorticalSurface::surfaceFilePath(const QString& subjectsDir,
                                         const QString& subjectId,
                                         int hemiCode,
                                         CorticalSurfaceType type)
{
    const QString hemiPrefix = (hemiCode == 0) ? QStringLiteral("lh") : QStringLiteral("rh");
    const QString suffix = surfaceTypeSuffix(type);
    return QDir::cleanPath(subjectsDir + QStringLiteral("/") + subjectId
                           + QStringLiteral("/surf/") + hemiPrefix + QStringLiteral(".") + suffix);
}

//=============================================================================================================

QString CorticalSurface::surfaceTypeSuffix(CorticalSurfaceType type)
{
    switch (type) {
        case CorticalSurfaceType::Inflated: return QStringLiteral("inflated");
        case CorticalSurfaceType::Pial:     return QStringLiteral("pial");
        case CorticalSurfaceType::White:    return QStringLiteral("white");
    }
    return QStringLiteral("inflated");
}

//=============================================================================================================

void CorticalSurface::refreshSceneLayers()
{
    if (!m_pScene) {
        return;
    }
    // Remove any prior cortex layers.
    m_pScene->removeLayer(QStringLiteral("cortex_lh"));
    m_pScene->removeLayer(QStringLiteral("cortex_rh"));

    auto registerHemi = [&](const QString& id, const QString& display,
                            const QSharedPointer<FsSurface>& surf) {
        if (!surf) {
            return;
        }
        SceneLayer layer;
        layer.id = id;
        layer.displayName = display;
        layer.kind = SceneLayerKind::BrainSurface;
        layer.opacity = 1.0f;
        layer.visible = true;
        // The renderer downcasts payload back to FsSurface via the kind.
        // We hand the scene a std::shared_ptr<void> whose deleter captures
        // the QSharedPointer; this keeps the FsSurface alive for as long
        // as the layer is registered, without depending on QSharedPointer
        // <-> std::shared_ptr interop helpers (Qt 6.5+).
        QSharedPointer<FsSurface> keepAlive = surf;
        layer.payload = std::shared_ptr<void>(static_cast<void*>(surf.data()),
                                              [keepAlive](void*) mutable {
                                                  keepAlive.reset();
                                              });
        m_pScene->addLayer(std::move(layer));
    };

    registerHemi(QStringLiteral("cortex_lh"), QStringLiteral("Cortex (left)"),  m_pSurfaceLh);
    registerHemi(QStringLiteral("cortex_rh"), QStringLiteral("Cortex (right)"), m_pSurfaceRh);
}

//=============================================================================================================
// STC overlay
//=============================================================================================================

bool CorticalSurface::loadSourceEstimate(const QString& path)
{
    QFile file(path);
    if (!QFileInfo::exists(path)) {
        qWarning() << "[CorticalSurface] STC file not found:" << path;
        return false;
    }
    InvSourceEstimate stc;
    if (!InvSourceEstimate::read(file, stc)) {
        qWarning() << "[CorticalSurface] InvSourceEstimate::read failed for" << path;
        return false;
    }
    if (stc.isEmpty() || stc.data.rows() == 0) {
        qWarning() << "[CorticalSurface] STC produced an empty estimate:" << path;
        return false;
    }
    setSourceEstimate(stc);
    m_lastStcDir = QFileInfo(path).absolutePath();
    return true;
}

//=============================================================================================================

void CorticalSurface::setSourceEstimate(const InvSourceEstimate& stc)
{
    m_stc = stc;
    const int nTimes = totalTimeSamples();

    if (nTimes <= 0) {
        m_currentSample = -1;
    } else {
        m_currentSample = 0;
    }

    if (m_pTimeSlider) {
        QSignalBlocker block(m_pTimeSlider.data());
        m_pTimeSlider->setRange(0, std::max(0, nTimes - 1));
        m_pTimeSlider->setValue(std::max(0, m_currentSample));
        m_pTimeSlider->setEnabled(nTimes > 1);
    }
    if (m_pPlayButton) {
        m_pPlayButton->setEnabled(nTimes > 1);
    }

    // Initialise thresholds from data range if user has not yet customised
    // them (i.e. still at the default 0/0.5/1 triple).
    if (m_fThresh == 0.0f && m_fMid == 0.5f && m_fMax == 1.0f && m_stc.data.size() > 0) {
        const double absMax = m_stc.data.cwiseAbs().maxCoeff();
        if (absMax > 0.0) {
            m_fThresh = static_cast<float>(absMax * 0.25);
            m_fMid    = static_cast<float>(absMax * 0.5);
            m_fMax    = static_cast<float>(absMax);
            syncThresholdSpins();
            emit colormapThresholdsChanged(m_fThresh, m_fMid, m_fMax);
        }
    }

    refreshOverlayLayer();
    updateTimeReadout();
    emit sourceEstimateLoaded(static_cast<int>(m_stc.data.rows()), nTimes);
    if (nTimes > 0) {
        emit timeSampleChanged(m_currentSample);
    }
}

//=============================================================================================================

const InvSourceEstimate& CorticalSurface::sourceEstimate() const
{
    return m_stc;
}

//=============================================================================================================

int CorticalSurface::overlayRowCount() const
{
    return static_cast<int>(m_stc.data.rows());
}

//=============================================================================================================

int CorticalSurface::totalTimeSamples() const
{
    return static_cast<int>(m_stc.data.cols());
}

//=============================================================================================================

int CorticalSurface::surfaceVertexCount() const
{
    int n = 0;
    if (m_pSurfaceLh) {
        n += static_cast<int>(m_pSurfaceLh->rr().rows());
    }
    if (m_pSurfaceRh) {
        n += static_cast<int>(m_pSurfaceRh->rr().rows());
    }
    return n;
}

//=============================================================================================================

bool CorticalSurface::overlayMatchesSurface() const
{
    const int surfN = surfaceVertexCount();
    if (surfN == 0) {
        return true;
    }
    return overlayRowCount() == surfN;
}

//=============================================================================================================

int CorticalSurface::currentTimeSample() const
{
    return m_currentSample;
}

//=============================================================================================================

void CorticalSurface::setCurrentTimeSample(int sample)
{
    const int nTimes = totalTimeSamples();
    int clamped = -1;
    if (nTimes > 0) {
        clamped = std::clamp(sample, 0, nTimes - 1);
    }
    if (clamped == m_currentSample) {
        return;
    }
    m_currentSample = clamped;
    if (m_pTimeSlider) {
        QSignalBlocker block(m_pTimeSlider.data());
        m_pTimeSlider->setValue(std::max(0, m_currentSample));
    }
    if (m_pScene) {
        m_pScene->setCurrentTimeSample(m_currentSample);
    }
    updateTimeReadout();
    emit timeSampleChanged(m_currentSample);
}

//=============================================================================================================

void CorticalSurface::setColormapThresholds(float fthresh, float fmid, float fmax)
{
    float vals[3] = { fthresh, fmid, fmax };
    std::sort(std::begin(vals), std::end(vals));
    if (vals[0] == m_fThresh && vals[1] == m_fMid && vals[2] == m_fMax) {
        return;
    }
    m_fThresh = vals[0];
    m_fMid    = vals[1];
    m_fMax    = vals[2];
    syncThresholdSpins();
    refreshOverlayLayer();
    emit colormapThresholdsChanged(m_fThresh, m_fMid, m_fMax);
}

//=============================================================================================================

float CorticalSurface::fThresh() const { return m_fThresh; }
float CorticalSurface::fMid()    const { return m_fMid; }
float CorticalSurface::fMax()    const { return m_fMax; }

//=============================================================================================================

bool CorticalSurface::isPlaying() const
{
    return m_playing;
}

//=============================================================================================================

void CorticalSurface::play()
{
    if (m_playing) {
        return;
    }
    if (totalTimeSamples() <= 1) {
        return;
    }
    m_playing = true;
    if (m_pPlayButton) {
        m_pPlayButton->setText(QStringLiteral("Pause"));
    }
    if (m_pPlaybackTimer) {
        const int interval = std::max(1, static_cast<int>(1000.0 / m_fps));
        m_pPlaybackTimer->start(interval);
    }
    emit playStateChanged(true);
}

//=============================================================================================================

void CorticalSurface::pause()
{
    if (!m_playing) {
        return;
    }
    m_playing = false;
    if (m_pPlaybackTimer) {
        m_pPlaybackTimer->stop();
    }
    if (m_pPlayButton) {
        m_pPlayButton->setText(QStringLiteral("Play"));
    }
    emit playStateChanged(false);
}

//=============================================================================================================

void CorticalSurface::togglePlayPause()
{
    if (m_playing) {
        pause();
    } else {
        play();
    }
}

//=============================================================================================================

double CorticalSurface::playbackFps() const
{
    return m_fps;
}

//=============================================================================================================

void CorticalSurface::setPlaybackFps(double fps)
{
    if (fps <= 0.0) {
        fps = 1.0;
    }
    if (fps == m_fps) {
        return;
    }
    m_fps = fps;
    if (m_pFpsSpin) {
        QSignalBlocker block(m_pFpsSpin.data());
        m_pFpsSpin->setValue(m_fps);
    }
    if (m_playing && m_pPlaybackTimer) {
        const int interval = std::max(1, static_cast<int>(1000.0 / m_fps));
        m_pPlaybackTimer->start(interval);
    }
}

//=============================================================================================================
// Slot implementations
//=============================================================================================================

void CorticalSurface::onLoadStcTriggered()
{
    const QString path = QFileDialog::getOpenFileName(
        nullptr,
        QStringLiteral("Load STC overlay"),
        m_lastStcDir,
        QStringLiteral("Source estimate (*.stc);;All files (*)"));
    if (path.isEmpty()) {
        return;
    }
    if (!loadSourceEstimate(path)) {
        QMessageBox::warning(nullptr, getName(),
                             QStringLiteral("Failed to load STC from '%1'.").arg(path));
    }
}

//=============================================================================================================

void CorticalSurface::onFThreshSpinChanged(double value)
{
    setColormapThresholds(static_cast<float>(value), m_fMid, m_fMax);
}

void CorticalSurface::onFMidSpinChanged(double value)
{
    setColormapThresholds(m_fThresh, static_cast<float>(value), m_fMax);
}

void CorticalSurface::onFMaxSpinChanged(double value)
{
    setColormapThresholds(m_fThresh, m_fMid, static_cast<float>(value));
}

//=============================================================================================================

void CorticalSurface::onTimeSliderChanged(int sample)
{
    setCurrentTimeSample(sample);
}

//=============================================================================================================

void CorticalSurface::onPlayPauseClicked()
{
    togglePlayPause();
}

//=============================================================================================================

void CorticalSurface::onPlaybackFpsChanged(double fps)
{
    setPlaybackFps(fps);
}

//=============================================================================================================

void CorticalSurface::onPlaybackTick()
{
    const int nTimes = totalTimeSamples();
    if (nTimes <= 0) {
        pause();
        return;
    }
    int next = m_currentSample + 1;
    if (next >= nTimes) {
        next = 0; // loop
    }
    setCurrentTimeSample(next);
}

//=============================================================================================================

void CorticalSurface::refreshOverlayLayer()
{
    if (!m_pScene) {
        return;
    }
    const QString layerId = QStringLiteral("stc_overlay");
    if (m_stc.isEmpty() || m_stc.data.size() == 0) {
        m_pScene->removeLayer(layerId);
        return;
    }
    SceneLayer layer;
    layer.id = layerId;
    layer.displayName = QStringLiteral("Source estimate");
    layer.kind = SceneLayerKind::SourceOverlay;
    layer.opacity = 1.0f;
    layer.visible = true;

    // Pass the source estimate as the payload; the renderer downcasts by
    // kind. We copy into a std::shared_ptr<InvSourceEstimate> so the scene
    // owns its own snapshot and downstream consumers can outlive the
    // plugin's own m_stc field (e.g. after the next setSourceEstimate()).
    auto payload = std::make_shared<InvSourceEstimate>(m_stc);
    layer.payload = std::shared_ptr<void>(payload, payload.get());
    m_pScene->addLayer(std::move(layer));
}

//=============================================================================================================

void CorticalSurface::updateTimeReadout()
{
    if (!m_pTimeReadout) {
        return;
    }
    const int nTimes = totalTimeSamples();
    if (nTimes <= 0 || m_currentSample < 0) {
        m_pTimeReadout->setText(QStringLiteral("—"));
        return;
    }
    const float t = m_stc.tmin + m_currentSample * m_stc.tstep;
    m_pTimeReadout->setText(QStringLiteral("%1 / %2  (t = %3 s)")
                                .arg(m_currentSample)
                                .arg(nTimes - 1)
                                .arg(t, 0, 'f', 4));
}

//=============================================================================================================

void CorticalSurface::syncThresholdSpins()
{
    if (m_pFThreshSpin) {
        QSignalBlocker block(m_pFThreshSpin.data());
        m_pFThreshSpin->setValue(m_fThresh);
    }
    if (m_pFMidSpin) {
        QSignalBlocker block(m_pFMidSpin.data());
        m_pFMidSpin->setValue(m_fMid);
    }
    if (m_pFMaxSpin) {
        QSignalBlocker block(m_pFMaxSpin.data());
        m_pFMaxSpin->setValue(m_fMax);
    }
}
