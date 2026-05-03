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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <QComboBox>
#include <QDebug>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CORTICALSURFACEPLUGIN;
using namespace ANSHAREDLIB;
using namespace DISP3DLIB;
using namespace FSLIB;

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
