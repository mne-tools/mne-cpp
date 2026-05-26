//=============================================================================================================
/**
 * @file     cortical_surface.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    CorticalSurface plugin definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "cortical_surface.h"

#include <anShared/Management/communicator.h>

#include <disp3D/scene/multimodalscene.h>

#include <fs/fs_annotation.h>
#include <fs/fs_label.h>
#include <fs/fs_label_utils.h>
#include <fs/fs_surface.h>

#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked.h>

#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>

#include <inv/inv_label_time_course.h>
#include <inv/inv_source_estimate.h>
#include <inv/minimum_norm/inv_cmne.h>
#include <inv/minimum_norm/inv_cmne_settings.h>
#include <inv/minimum_norm/inv_minimum_norm.h>
#include <inv/sparse/inv_gamma_map.h>
#include <inv/sparse/inv_mxne.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CORTICALSURFACEPLUGIN;
using namespace ANSHAREDLIB;
using namespace DISP3DLIB;
using namespace FIFFLIB;
using namespace FSLIB;
using namespace INVLIB;
using namespace MNELIB;

//=============================================================================================================
// Inner widget: TimeCoursePlotter
//=============================================================================================================

namespace CORTICALSURFACEPLUGIN {

class TimeCoursePlotter : public QWidget
{
public:
    explicit TimeCoursePlotter(QWidget* parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(140);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    struct Series { QString name; QColor color; QVector<double> data; };

    void setSeries(const QVector<Series>& series, int currentSample)
    {
        m_series = series;
        m_currentSample = currentSample;
        update();
    }

    void setCurrentSample(int sample)
    {
        if (sample == m_currentSample) {
            return;
        }
        m_currentSample = sample;
        update();
    }

protected:
    void paintEvent(QPaintEvent* /*ev*/) override
    {
        QPainter p(this);
        p.fillRect(rect(), palette().base());

        const int margin = 10;
        const QRect plot = rect().adjusted(margin, margin, -margin, -margin);
        p.setPen(palette().mid().color());
        p.drawRect(plot);

        if (m_series.isEmpty()) {
            p.setPen(palette().text().color());
            p.drawText(plot, Qt::AlignCenter,
                       QStringLiteral("No picks yet — click a vertex to plot."));
            return;
        }

        int nSamples = 0;
        double dMin =  std::numeric_limits<double>::infinity();
        double dMax = -std::numeric_limits<double>::infinity();
        for (const Series& s : m_series) {
            nSamples = std::max(nSamples, static_cast<int>(s.data.size()));
            for (double v : s.data) {
                dMin = std::min(dMin, v);
                dMax = std::max(dMax, v);
            }
        }
        if (nSamples <= 1 || !std::isfinite(dMin) || !std::isfinite(dMax)) {
            return;
        }
        if (dMax == dMin) {
            dMax = dMin + 1.0;
        }

        const auto xFor = [&](int sample) {
            const double t = static_cast<double>(sample) / static_cast<double>(nSamples - 1);
            return plot.left() + static_cast<int>(t * plot.width());
        };
        const auto yFor = [&](double v) {
            const double t = (v - dMin) / (dMax - dMin);
            return plot.bottom() - static_cast<int>(t * plot.height());
        };

        for (const Series& s : m_series) {
            if (s.data.size() < 2) {
                continue;
            }
            QPainterPath path;
            path.moveTo(xFor(0), yFor(s.data.front()));
            for (int i = 1; i < s.data.size(); ++i) {
                path.lineTo(xFor(i), yFor(s.data[i]));
            }
            p.setPen(QPen(s.color, 1.5));
            p.drawPath(path);
        }

        if (m_currentSample >= 0 && m_currentSample < nSamples) {
            p.setPen(QPen(QColor(220, 60, 60), 1, Qt::DashLine));
            const int xc = xFor(m_currentSample);
            p.drawLine(xc, plot.top(), xc, plot.bottom());
        }
    }

private:
    QVector<Series> m_series;
    int             m_currentSample = -1;
};

} // namespace CORTICALSURFACEPLUGIN

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
        m_pComputeSourceEstimateAction = m_pMenu->addAction(QStringLiteral("Compute Source Estimate…"));
        connect(m_pComputeSourceEstimateAction.data(), &QAction::triggered,
                this, &CorticalSurface::onComputeSourceEstimateTriggered);
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
    // No subscriptions yet. STC overlay events and vertex-pick requests
    // from other mne_analyze plugins plug in here.
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
    if (m_pPlotter) {
        m_pPlotter->setCurrentSample(m_currentSample);
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

//=============================================================================================================
// Vertex picking + Time Course dock
//=============================================================================================================

QDockWidget* CorticalSurface::getTimeCourseDock()
{
    buildTimeCourseDock();
    return m_pTimeCourseDock;
}

//=============================================================================================================

void CorticalSurface::buildTimeCourseDock()
{
    if (m_pTimeCourseDock) {
        return;
    }
    m_pTimeCourseDock = new QDockWidget(QStringLiteral("Source Time Course"));

    auto* container = new QWidget(m_pTimeCourseDock);
    auto* layout = new QVBoxLayout(container);

    m_pPlotter = new TimeCoursePlotter(container);
    layout->addWidget(m_pPlotter);

    m_pTimeCourseList = new QListWidget(container);
    m_pTimeCourseList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_pTimeCourseList.data(), &QListWidget::itemSelectionChanged,
            this, &CorticalSurface::onTimeCourseManagerSelectionChanged);
    layout->addWidget(m_pTimeCourseList);

    auto* btnRow = new QHBoxLayout();
    m_pTcRenameBtn = new QPushButton(QStringLiteral("Rename"), container);
    m_pTcColorBtn  = new QPushButton(QStringLiteral("Colour..."), container);
    m_pTcRemoveBtn = new QPushButton(QStringLiteral("Remove"), container);
    m_pTcExportBtn = new QPushButton(QStringLiteral("Export CSV..."), container);
    connect(m_pTcRenameBtn.data(), &QPushButton::clicked, this, &CorticalSurface::onTimeCourseRenameClicked);
    connect(m_pTcColorBtn.data(),  &QPushButton::clicked, this, &CorticalSurface::onTimeCourseColorClicked);
    connect(m_pTcRemoveBtn.data(), &QPushButton::clicked, this, &CorticalSurface::onTimeCourseRemoveClicked);
    connect(m_pTcExportBtn.data(), &QPushButton::clicked, this, &CorticalSurface::onTimeCourseExportClicked);
    btnRow->addWidget(m_pTcRenameBtn);
    btnRow->addWidget(m_pTcColorBtn);
    btnRow->addWidget(m_pTcRemoveBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_pTcExportBtn);
    layout->addLayout(btnRow);

    m_pTimeCourseDock->setWidget(container);
}

//=============================================================================================================

bool CorticalSurface::pickVertex(const QVector3D& worldPoint)
{
    int bestHemi = -1;
    int bestVertex = -1;
    double bestDistSq = std::numeric_limits<double>::infinity();

    auto scan = [&](const QSharedPointer<FsSurface>& surf, int hemiCode) {
        if (!surf) {
            return;
        }
        const Eigen::MatrixX3f& rr = surf->rr();
        for (int i = 0; i < rr.rows(); ++i) {
            const double dx = static_cast<double>(rr(i, 0)) - static_cast<double>(worldPoint.x());
            const double dy = static_cast<double>(rr(i, 1)) - static_cast<double>(worldPoint.y());
            const double dz = static_cast<double>(rr(i, 2)) - static_cast<double>(worldPoint.z());
            const double d2 = dx * dx + dy * dy + dz * dz;
            if (d2 < bestDistSq) {
                bestDistSq = d2;
                bestHemi   = hemiCode;
                bestVertex = i;
            }
        }
    };
    scan(m_pSurfaceLh, 0);
    scan(m_pSurfaceRh, 1);

    if (bestVertex < 0) {
        return false;
    }

    static const QColor palette[] = {
        QColor("#1f77b4"), QColor("#ff7f0e"), QColor("#2ca02c"),
        QColor("#d62728"), QColor("#9467bd"), QColor("#8c564b"),
        QColor("#e377c2"), QColor("#7f7f7f"), QColor("#bcbd22"),
        QColor("#17becf"),
    };
    const int idx = m_traces.size();
    const QColor color = palette[idx % (sizeof(palette) / sizeof(palette[0]))];
    const QString hemiStr = (bestHemi == 0) ? QStringLiteral("lh") : QStringLiteral("rh");
    const QString name = QStringLiteral("Pick %1 - %2 - vtx%3")
                             .arg(idx + 1).arg(hemiStr).arg(bestVertex);

    QVector<double> trace = timeCourseAt(bestHemi, bestVertex);
    appendTrace(name, color, trace, bestHemi, bestVertex);
    if (!m_traces.isEmpty()) {
        m_traces.last().isPick = true;
    }

    if (m_pScene) {
        PickResult pr;
        pr.kind        = PickKind::CorticalVertex;
        pr.world       = worldPoint;
        pr.objectId    = bestVertex;
        pr.hemisphere  = bestHemi;
        pr.sourceId    = (bestHemi == 0) ? QStringLiteral("cortex_lh") : QStringLiteral("cortex_rh");
        m_pScene->reportPick(pr);
    }

    emit vertexPicked(bestHemi, bestVertex);
    return true;
}

//=============================================================================================================

int CorticalSurface::pickedVertexCount() const
{
    int n = 0;
    for (const TraceEntry& t : m_traces) {
        if (t.isPick) {
            ++n;
        }
    }
    return n;
}

//=============================================================================================================

QVector<CorticalPickedVertex> CorticalSurface::pickedVertices() const
{
    QVector<CorticalPickedVertex> out;
    for (const TraceEntry& t : m_traces) {
        if (!t.isPick) {
            continue;
        }
        CorticalPickedVertex p;
        p.hemi   = t.hemi;
        p.vertex = t.vertex;
        p.name   = t.name;
        p.color  = t.color;
        out.append(p);
    }
    return out;
}

//=============================================================================================================

int CorticalSurface::findStcRowFor(int hemi, int vertex) const
{
    if (m_stc.isEmpty() || m_stc.vertices.size() == 0) {
        return -1;
    }
    // Convention used across mne-cpp / mne-python single-VectorXi stacking:
    //   rh vertex i is stored as (nLhVertices + i). When we do not know the
    //   lh vertex count, also accept a direct match (single-hemi STC).
    const int target = (hemi == 1 && m_pSurfaceLh) ?
        (vertex + static_cast<int>(m_pSurfaceLh->rr().rows())) : vertex;
    for (int i = 0; i < m_stc.vertices.size(); ++i) {
        if (m_stc.vertices[i] == target || (hemi == 1 && m_stc.vertices[i] == vertex)) {
            return i;
        }
    }
    return -1;
}

//=============================================================================================================

QVector<double> CorticalSurface::timeCourseAt(int hemi, int vertex) const
{
    QVector<double> out;
    const int row = findStcRowFor(hemi, vertex);
    if (row < 0) {
        return out;
    }
    out.reserve(static_cast<int>(m_stc.data.cols()));
    for (int t = 0; t < m_stc.data.cols(); ++t) {
        out.append(m_stc.data(row, t));
    }
    return out;
}

//=============================================================================================================

void CorticalSurface::appendTrace(const QString& name,
                                  const QColor& color,
                                  const QVector<double>& trace,
                                  int hemi,
                                  int vertex)
{
    TraceEntry e;
    e.name = name;
    e.color = color;
    e.data = trace;
    e.hemi = hemi;
    e.vertex = vertex;
    m_traces.append(e);
    refreshTimeCourseManager();
    refreshPlotter();
}

//=============================================================================================================

void CorticalSurface::refreshTimeCourseManager()
{
    if (!m_pTimeCourseList) {
        return;
    }
    QSignalBlocker block(m_pTimeCourseList.data());
    m_pTimeCourseList->clear();
    for (const TraceEntry& t : m_traces) {
        auto* item = new QListWidgetItem(t.name);
        item->setForeground(QBrush(t.color));
        m_pTimeCourseList->addItem(item);
    }
}

//=============================================================================================================

void CorticalSurface::refreshPlotter()
{
    if (!m_pPlotter) {
        return;
    }
    QVector<TimeCoursePlotter::Series> ss;
    ss.reserve(m_traces.size());
    for (const TraceEntry& t : m_traces) {
        ss.append({ t.name, t.color, t.data });
    }
    m_pPlotter->setSeries(ss, m_currentSample);
}

//=============================================================================================================

CORTICALSURFACEPLUGIN::TimeCoursePlotter* CorticalSurface::plotter() const
{
    return m_pPlotter;
}

//=============================================================================================================

bool CorticalSurface::exportTimeCoursesCsv(const QString& path) const
{
    if (m_traces.isEmpty()) {
        return false;
    }
    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream ts(&out);
    ts << QStringLiteral("sample,t");
    int maxLen = 0;
    for (const TraceEntry& t : m_traces) {
        ts << ',' << t.name;
        maxLen = std::max(maxLen, static_cast<int>(t.data.size()));
    }
    ts << '\n';
    const double tmin = m_stc.tmin;
    const double tstep = m_stc.tstep;
    for (int i = 0; i < maxLen; ++i) {
        ts << i << ',' << (tmin + i * tstep);
        for (const TraceEntry& t : m_traces) {
            ts << ',';
            if (i < t.data.size()) {
                ts << t.data[i];
            }
        }
        ts << '\n';
    }
    out.close();
    emit const_cast<CorticalSurface*>(this)->timeCoursesExported(path);
    return true;
}

//=============================================================================================================

void CorticalSurface::onTimeCourseManagerSelectionChanged()
{
    const bool any = m_pTimeCourseList && m_pTimeCourseList->currentRow() >= 0;
    if (m_pTcRenameBtn) m_pTcRenameBtn->setEnabled(any);
    if (m_pTcColorBtn)  m_pTcColorBtn->setEnabled(any);
    if (m_pTcRemoveBtn) m_pTcRemoveBtn->setEnabled(any);
}

//=============================================================================================================

void CorticalSurface::onTimeCourseRenameClicked()
{
    if (!m_pTimeCourseList) return;
    const int row = m_pTimeCourseList->currentRow();
    if (row < 0 || row >= m_traces.size()) return;
    bool ok = false;
    const QString name = QInputDialog::getText(nullptr,
        QStringLiteral("Rename trace"),
        QStringLiteral("Name"),
        QLineEdit::Normal,
        m_traces[row].name, &ok);
    if (!ok || name.isEmpty()) return;
    m_traces[row].name = name;
    refreshTimeCourseManager();
    refreshPlotter();
}

//=============================================================================================================

void CorticalSurface::onTimeCourseColorClicked()
{
    if (!m_pTimeCourseList) return;
    const int row = m_pTimeCourseList->currentRow();
    if (row < 0 || row >= m_traces.size()) return;
    const QColor c = QColorDialog::getColor(m_traces[row].color, nullptr,
                                            QStringLiteral("Trace colour"));
    if (!c.isValid()) return;
    m_traces[row].color = c;
    refreshTimeCourseManager();
    refreshPlotter();
}

//=============================================================================================================

void CorticalSurface::onTimeCourseRemoveClicked()
{
    if (!m_pTimeCourseList) return;
    const int row = m_pTimeCourseList->currentRow();
    if (row < 0 || row >= m_traces.size()) return;
    m_traces.remove(row);
    refreshTimeCourseManager();
    refreshPlotter();
}

//=============================================================================================================

void CorticalSurface::onTimeCourseExportClicked()
{
    const QString path = QFileDialog::getSaveFileName(nullptr,
        QStringLiteral("Export time courses"),
        QString(),
        QStringLiteral("CSV (*.csv)"));
    if (path.isEmpty()) return;
    if (!exportTimeCoursesCsv(path)) {
        QMessageBox::warning(nullptr, getName(),
            QStringLiteral("Failed to write CSV to '%1'.").arg(path));
    }
}

//=============================================================================================================
// Inverse computation (Compute Source Estimate dialog)
//=============================================================================================================

bool CorticalSurface::runComputeSourceEstimate(const QString& fwdPath,
                                               const QString& covPath,
                                               const QString& evokedPath,
                                               const ComputeSourceEstimateOptions& opts)
{
    QFile fwdFile(fwdPath);
    if (!QFileInfo::exists(fwdPath)) {
        qWarning() << "[CorticalSurface] Cannot open forward file" << fwdPath;
        return false;
    }
    MNEForwardSolution fwd;
    if (!MNEForwardSolution::read(fwdFile, fwd)) {
        qWarning() << "[CorticalSurface] read forward failed";
        return false;
    }
    if (fwdFile.isOpen()) {
        fwdFile.close();
    }

    QFile covFile(covPath);
    if (!QFileInfo::exists(covPath)) {
        qWarning() << "[CorticalSurface] Cannot open cov file" << covPath;
        return false;
    }
    FiffCov noiseCov;
    try {
        noiseCov = FiffCov(covFile);
    } catch (const std::exception& e) {
        qWarning() << "[CorticalSurface] read cov failed:" << e.what();
        return false;
    }
    if (covFile.isOpen()) {
        covFile.close();
    }

    QFile aveFile(evokedPath);
    if (!QFileInfo::exists(evokedPath)) {
        qWarning() << "[CorticalSurface] Cannot open evoked file" << evokedPath;
        return false;
    }
    FiffEvoked evoked;
    if (!FiffEvoked::read(aveFile, evoked)) {
        qWarning() << "[CorticalSurface] read evoked failed";
        return false;
    }
    if (aveFile.isOpen()) {
        aveFile.close();
    }

    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    const float lambda2 = (opts.snr > 0.0) ? static_cast<float>(1.0 / (opts.snr * opts.snr)) : 0.111111f;

    bool ok = false;
    InvSourceEstimate stc;
    const QString m = opts.method;

    if (m == QStringLiteral("MNE")    || m == QStringLiteral("dSPM") ||
        m == QStringLiteral("sLORETA") || m == QStringLiteral("eLORETA")) {
        MNEInverseOperator invOp = MNEInverseOperator::make_inverse_operator(
            evoked.info, fwd, noiseCov,
            static_cast<float>(opts.loose), static_cast<float>(opts.depth),
            /*fixed=*/false);
        InvMinimumNorm mnEst(invOp, lambda2, m);
        if (m == QStringLiteral("eLORETA")) {
            mnEst.setELoretaOptions();
        }
        stc = mnEst.calculateInverse(evoked);
        ok = !stc.isEmpty();
    } else if (m == QStringLiteral("MxNE")) {
        const Eigen::MatrixXd& gain = fwd.sol->data;
        InvMxneResult res = InvMxne::compute(gain, evoked.data, opts.alpha);
        stc = res.stc;
        ok = !stc.isEmpty();
    } else if (m == QStringLiteral("Gamma-MAP")) {
        const Eigen::MatrixXd& gain = fwd.sol->data;
        InvGammaMapResult res = InvGammaMap::compute(gain, evoked.data, noiseCov.data);
        stc = res.stc;
        ok = !stc.isEmpty();
    } else if (m == QStringLiteral("CMNE")) {
        InvCMNESettings settings;
        settings.onnxModelPath = opts.onnxPath;
        settings.lambda2 = 1.0 / (opts.snr * opts.snr);
        const Eigen::MatrixXd& gain = fwd.sol->data;
        const Eigen::MatrixXd srcCov = Eigen::MatrixXd::Identity(gain.cols(), gain.cols());
        InvCMNEResult res = InvCMNE::compute(evoked.data, gain, noiseCov.data, srcCov, settings);
        stc = res.stcCmne;
        ok = !stc.isEmpty();
    } else {
        qWarning() << "[CorticalSurface] Unknown inverse method" << m;
    }

    QGuiApplication::restoreOverrideCursor();

    if (ok) {
        setSourceEstimate(stc);
        emit sourceEstimateComputed(m);
    }
    return ok;
}

//=============================================================================================================

void CorticalSurface::onComputeSourceEstimateTriggered()
{
    QDialog dlg;
    dlg.setWindowTitle(QStringLiteral("Compute Source Estimate"));
    auto* form = new QFormLayout(&dlg);

    auto makePathRow = [&](const QString& label, const QString& filter, QString* lastDir) {
        auto* row = new QWidget(&dlg);
        auto* hb = new QHBoxLayout(row);
        hb->setContentsMargins(0, 0, 0, 0);
        auto* edit = new QLineEdit(row);
        auto* btn = new QPushButton(QStringLiteral("..."), row);
        hb->addWidget(edit);
        hb->addWidget(btn);
        QObject::connect(btn, &QPushButton::clicked, &dlg, [edit, label, filter, lastDir]() {
            const QString p = QFileDialog::getOpenFileName(nullptr, label, *lastDir, filter);
            if (!p.isEmpty()) {
                edit->setText(p);
                *lastDir = QFileInfo(p).absolutePath();
            }
        });
        form->addRow(label, row);
        return edit;
    };

    QLineEdit* fwdEdit  = makePathRow(QStringLiteral("Forward (-fwd.fif)"),  QStringLiteral("FIFF (*.fif)"), &m_lastFwdDir);
    QLineEdit* covEdit  = makePathRow(QStringLiteral("Noise cov (-cov.fif)"), QStringLiteral("FIFF (*.fif)"), &m_lastCovDir);
    QLineEdit* aveEdit  = makePathRow(QStringLiteral("Evoked (-ave.fif)"),    QStringLiteral("FIFF (*.fif)"), &m_lastAveDir);
    QLineEdit* onnxEdit = makePathRow(QStringLiteral("CMNE ONNX (optional)"),  QStringLiteral("ONNX (*.onnx)"), &m_lastAveDir);

    auto* methodCombo = new QComboBox(&dlg);
    methodCombo->addItems({ QStringLiteral("MNE"), QStringLiteral("dSPM"),
                            QStringLiteral("sLORETA"), QStringLiteral("eLORETA"),
                            QStringLiteral("MxNE"), QStringLiteral("Gamma-MAP"),
                            QStringLiteral("CMNE") });
    form->addRow(QStringLiteral("Method"), methodCombo);

    auto* snrSpin = new QDoubleSpinBox(&dlg);
    snrSpin->setRange(0.01, 100.0);
    snrSpin->setValue(3.0);
    form->addRow(QStringLiteral("SNR"), snrSpin);

    auto* alphaSpin = new QDoubleSpinBox(&dlg);
    alphaSpin->setRange(0.0, 1e6);
    alphaSpin->setValue(1.0);
    form->addRow(QStringLiteral("Alpha (sparse)"), alphaSpin);

    auto* looseSpin = new QDoubleSpinBox(&dlg);
    looseSpin->setRange(0.0, 1.0);
    looseSpin->setValue(0.2);
    looseSpin->setSingleStep(0.1);
    form->addRow(QStringLiteral("Loose"), looseSpin);

    auto* depthSpin = new QDoubleSpinBox(&dlg);
    depthSpin->setRange(0.0, 1.0);
    depthSpin->setValue(0.8);
    depthSpin->setSingleStep(0.1);
    form->addRow(QStringLiteral("Depth"), depthSpin);

    auto setSparseVisible = [&](const QString& method) {
        const bool sparse = (method == QStringLiteral("MxNE") || method == QStringLiteral("Gamma-MAP"));
        alphaSpin->setEnabled(sparse);
    };
    setSparseVisible(methodCombo->currentText());
    QObject::connect(methodCombo, &QComboBox::currentTextChanged, &dlg, setSparseVisible);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    ComputeSourceEstimateOptions opts;
    opts.method   = methodCombo->currentText();
    opts.snr      = snrSpin->value();
    opts.alpha    = alphaSpin->value();
    opts.loose    = looseSpin->value();
    opts.depth    = depthSpin->value();
    opts.onnxPath = onnxEdit->text();

    if (!runComputeSourceEstimate(fwdEdit->text(), covEdit->text(), aveEdit->text(), opts)) {
        QMessageBox::warning(nullptr, getName(),
            QStringLiteral("Compute Source Estimate failed (see log)."));
    }
}

//=============================================================================================================
// Labels dock (FreeSurfer annotation / atlas browser)
//=============================================================================================================

QDockWidget* CorticalSurface::getLabelsDock()
{
    buildLabelsDock();
    return m_pLabelsDock;
}

//=============================================================================================================

void CorticalSurface::buildLabelsDock()
{
    if (m_pLabelsDock) {
        return;
    }
    m_pLabelsDock = new QDockWidget(QStringLiteral("Labels"));

    auto* container = new QWidget(m_pLabelsDock);
    auto* layout = new QVBoxLayout(container);

    auto* toolbar = new QToolBar(container);
    m_pLoadLabelAction   = toolbar->addAction(QStringLiteral("Load .label..."));
    m_pLoadAnnotAction   = toolbar->addAction(QStringLiteral("Load .annot..."));
    m_pSaveLabelAction   = toolbar->addAction(QStringLiteral("Save Label..."));
    m_pCreateLabelAction = toolbar->addAction(QStringLiteral("Create from Marked Vertices..."));
    connect(m_pLoadLabelAction.data(),   &QAction::triggered, this, &CorticalSurface::onLoadLabelTriggered);
    connect(m_pLoadAnnotAction.data(),   &QAction::triggered, this, &CorticalSurface::onLoadAnnotTriggered);
    connect(m_pSaveLabelAction.data(),   &QAction::triggered, this, &CorticalSurface::onSaveLabelTriggered);
    connect(m_pCreateLabelAction.data(), &QAction::triggered, this, &CorticalSurface::onCreateLabelFromMarkedTriggered);
    layout->addWidget(toolbar);

    m_pLabelTree = new QTreeWidget(container);
    m_pLabelTree->setHeaderLabels({ QStringLiteral("Label"), QStringLiteral("#verts") });
    m_pLabelTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_pLabelTree.data(), &QTreeWidget::customContextMenuRequested,
            this, &CorticalSurface::onLabelTreeContextMenu);
    connect(m_pLabelTree.data(), &QTreeWidget::itemChanged,
            this, &CorticalSurface::onLabelItemChanged);

    m_pLabelRootLh = new QTreeWidgetItem(m_pLabelTree.data(), { QStringLiteral("Left hemisphere") });
    m_pLabelRootRh = new QTreeWidgetItem(m_pLabelTree.data(), { QStringLiteral("Right hemisphere") });
    m_pLabelRootLh->setExpanded(true);
    m_pLabelRootRh->setExpanded(true);

    layout->addWidget(m_pLabelTree);
    m_pLabelsDock->setWidget(container);
}

//=============================================================================================================

QTreeWidgetItem* CorticalSurface::addLabelItem(const FsLabel& label)
{
    buildLabelsDock();
    QTreeWidgetItem* root = (label.hemi == 1) ? m_pLabelRootRh : m_pLabelRootLh;
    auto* item = new QTreeWidgetItem(root,
        { label.name.isEmpty() ? QStringLiteral("(unnamed)") : label.name,
          QString::number(label.vertices.size()) });
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(0, Qt::Checked);
    item->setData(0, Qt::UserRole, QVariant::fromValue(label));
    root->setExpanded(true);
    emit labelsChanged();
    return item;
}

//=============================================================================================================

FsLabel CorticalSurface::labelFromItem(QTreeWidgetItem* item)
{
    if (!item) {
        return FsLabel();
    }
    const QVariant v = item->data(0, Qt::UserRole);
    if (v.canConvert<FsLabel>()) {
        return v.value<FsLabel>();
    }
    return FsLabel();
}

//=============================================================================================================

QList<FsLabel> CorticalSurface::labels() const
{
    QList<FsLabel> out;
    if (!m_pLabelTree) {
        return out;
    }
    for (QTreeWidgetItem* root : { m_pLabelRootLh, m_pLabelRootRh }) {
        if (!root) continue;
        for (int i = 0; i < root->childCount(); ++i) {
            FsLabel l = labelFromItem(root->child(i));
            if (!l.isEmpty()) {
                out.append(l);
            }
        }
    }
    return out;
}

//=============================================================================================================

int CorticalSurface::loadLabel(const QString& path)
{
    FsLabel l;
    if (!FsLabel::read(path, l) || l.isEmpty()) {
        return 0;
    }
    addLabelItem(l);
    m_lastLabelDir = QFileInfo(path).absolutePath();
    return 1;
}

//=============================================================================================================

int CorticalSurface::loadAnnot(const QString& path)
{
    FsAnnotation annot;
    if (!FsAnnotation::read(path, annot)) {
        return 0;
    }
    int hemi = path.contains(QStringLiteral("/rh.")) ? 1 : 0;
    QSharedPointer<FsSurface> surf = (hemi == 0) ? m_pSurfaceLh : m_pSurfaceRh;
    if (!surf) {
        qWarning() << "[CorticalSurface] loadAnnot: matching hemisphere surface not loaded";
        return 0;
    }
    QList<FsLabel> labelList;
    QList<Eigen::RowVector4i> rgbas;
    if (!annot.toLabels(*surf, labelList, rgbas)) {
        return 0;
    }
    int added = 0;
    for (FsLabel& l : labelList) {
        if (l.hemi < 0) {
            l.hemi = hemi;
        }
        addLabelItem(l);
        ++added;
    }
    m_lastLabelDir = QFileInfo(path).absolutePath();
    return added;
}

//=============================================================================================================

bool CorticalSurface::extractLabelTimeCourse(const FsLabel& lbl, const QString& mode)
{
    if (lbl.isEmpty() || m_stc.isEmpty()) {
        return false;
    }
    QList<FsLabel> single; single.append(lbl);
    Eigen::MatrixXd tc = InvLabelTimeCourse::extract(m_stc, single, mode, /*bAllowEmpty=*/true);
    if (tc.rows() == 0) {
        return false;
    }
    QVector<double> trace; trace.reserve(static_cast<int>(tc.cols()));
    for (int i = 0; i < tc.cols(); ++i) {
        trace.append(tc(0, i));
    }
    const QColor color = QColor::fromHsv((m_traces.size() * 47) % 360, 200, 200);
    appendTrace(QStringLiteral("%1 [%2]").arg(lbl.name, mode), color, trace,
                /*hemi=*/lbl.hemi, /*vertex=*/-1);
    return true;
}

//=============================================================================================================

bool CorticalSurface::writeLabelFile(const FsLabel& label, const QString& path)
{
    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream ts(&out);
    const int n = static_cast<int>(label.vertices.size());
    ts << "#" << (label.comment.isEmpty() ? label.name : label.comment) << '\n';
    ts << n << '\n';
    for (int i = 0; i < n; ++i) {
        const int v = label.vertices[i];
        const float x = (i < label.pos.rows()) ? label.pos(i, 0) * 1000.0f : 0.0f;
        const float y = (i < label.pos.rows()) ? label.pos(i, 1) * 1000.0f : 0.0f;
        const float z = (i < label.pos.rows()) ? label.pos(i, 2) * 1000.0f : 0.0f;
        const double val = (i < label.values.size()) ? label.values[i] : 0.0;
        ts << v << ' ' << x << ' ' << y << ' ' << z << ' ' << val << '\n';
    }
    out.close();
    return true;
}

//=============================================================================================================

bool CorticalSurface::saveLabel(const FsLabel& lbl, const QString& path)
{
    return writeLabelFile(lbl, path);
}

//=============================================================================================================

void CorticalSurface::onLoadLabelTriggered()
{
    const QString p = QFileDialog::getOpenFileName(nullptr,
        QStringLiteral("Load FreeSurfer label"), m_lastLabelDir,
        QStringLiteral("Label (*.label)"));
    if (p.isEmpty()) return;
    if (loadLabel(p) == 0) {
        QMessageBox::warning(nullptr, getName(), QStringLiteral("Failed to read label '%1'.").arg(p));
    }
}

//=============================================================================================================

void CorticalSurface::onLoadAnnotTriggered()
{
    const QString p = QFileDialog::getOpenFileName(nullptr,
        QStringLiteral("Load FreeSurfer annotation"), m_lastLabelDir,
        QStringLiteral("Annot (*.annot)"));
    if (p.isEmpty()) return;
    if (loadAnnot(p) == 0) {
        QMessageBox::warning(nullptr, getName(), QStringLiteral("Failed to read annot '%1'.").arg(p));
    }
}

//=============================================================================================================

void CorticalSurface::onSaveLabelTriggered()
{
    if (!m_pLabelTree) return;
    QTreeWidgetItem* item = m_pLabelTree->currentItem();
    FsLabel l = labelFromItem(item);
    if (l.isEmpty()) {
        QMessageBox::information(nullptr, getName(),
            QStringLiteral("Select a label to save."));
        return;
    }
    const QString p = QFileDialog::getSaveFileName(nullptr,
        QStringLiteral("Save FreeSurfer label"), m_lastLabelDir,
        QStringLiteral("Label (*.label)"));
    if (p.isEmpty()) return;
    if (!saveLabel(l, p)) {
        QMessageBox::warning(nullptr, getName(), QStringLiteral("Failed to write '%1'.").arg(p));
    }
}

//=============================================================================================================

void CorticalSurface::onCreateLabelFromMarkedTriggered()
{
    QVector<int> lhVerts;
    QVector<int> rhVerts;
    for (const TraceEntry& t : m_traces) {
        if (!t.isPick || t.vertex < 0) continue;
        if (t.hemi == 0) lhVerts.append(t.vertex);
        else if (t.hemi == 1) rhVerts.append(t.vertex);
    }
    if (lhVerts.isEmpty() && rhVerts.isEmpty()) {
        QMessageBox::information(nullptr, getName(),
            QStringLiteral("No picked vertices in the Time Course Manager."));
        return;
    }
    bool ok = false;
    const int nSteps = QInputDialog::getInt(nullptr,
        QStringLiteral("Grow label"),
        QStringLiteral("Surface steps:"), 3, 0, 200, 1, &ok);
    if (!ok) return;
    const QString name = QInputDialog::getText(nullptr,
        QStringLiteral("Label name"), QStringLiteral("Name"), QLineEdit::Normal,
        QStringLiteral("custom"), &ok);
    if (!ok || name.isEmpty()) return;

    auto buildSeed = [&](const QVector<int>& verts, int hemi,
                         const QSharedPointer<FsSurface>& surf) -> FsLabel {
        FsLabel seed;
        if (verts.isEmpty() || !surf) return seed;
        Eigen::VectorXi v(verts.size());
        Eigen::MatrixX3f pos(verts.size(), 3);
        Eigen::VectorXd vals = Eigen::VectorXd::Ones(verts.size());
        for (int i = 0; i < verts.size(); ++i) {
            v[i] = verts[i];
            if (verts[i] < surf->rr().rows()) {
                pos.row(i) = surf->rr().row(verts[i]);
            } else {
                pos.row(i).setZero();
            }
        }
        seed = FsLabel(v, pos, vals, hemi, name);
        return seed;
    };

    if (!lhVerts.isEmpty() && m_pSurfaceLh) {
        FsLabel seed = buildSeed(lhVerts, 0, m_pSurfaceLh);
        FsLabel grown = (nSteps > 0) ?
            FsLabelUtils::growLabel(seed, *m_pSurfaceLh, nSteps) : seed;
        grown.name = name;
        addLabelItem(grown);
    }
    if (!rhVerts.isEmpty() && m_pSurfaceRh) {
        FsLabel seed = buildSeed(rhVerts, 1, m_pSurfaceRh);
        FsLabel grown = (nSteps > 0) ?
            FsLabelUtils::growLabel(seed, *m_pSurfaceRh, nSteps) : seed;
        grown.name = name;
        addLabelItem(grown);
    }

    const QString outPath = QFileDialog::getSaveFileName(nullptr,
        QStringLiteral("Save new label"), m_lastLabelDir,
        QStringLiteral("Label (*.label)"));
    if (outPath.isEmpty()) return;
    const QList<FsLabel> all = labels();
    if (!all.isEmpty()) {
        saveLabel(all.last(), outPath);
    }
}

//=============================================================================================================

void CorticalSurface::onLabelItemChanged(QTreeWidgetItem* /*item*/, int /*column*/)
{
    emit labelsChanged();
}

//=============================================================================================================

void CorticalSurface::onLabelTreeContextMenu(const QPoint& pos)
{
    if (!m_pLabelTree) return;
    QTreeWidgetItem* item = m_pLabelTree->itemAt(pos);
    if (!item) return;
    const FsLabel lbl = labelFromItem(item);
    if (lbl.isEmpty()) return;

    QMenu menu;
    QMenu* extractMenu = menu.addMenu(QStringLiteral("Extract Time Course"));
    const QStringList modes = { QStringLiteral("mean"), QStringLiteral("mean_flip"),
                                QStringLiteral("pca_flip"), QStringLiteral("max"),
                                QStringLiteral("auto") };
    for (const QString& mode : modes) {
        QAction* a = extractMenu->addAction(mode);
        a->setData(mode);
    }
    QAction* picked = menu.exec(m_pLabelTree->viewport()->mapToGlobal(pos));
    if (!picked) return;
    extractLabelTimeCourse(lbl, picked->data().toString());
}
