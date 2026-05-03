//=============================================================================================================
/**
 * @file     cortical_surface.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 *       the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *       and the following disclaimer in the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used to endorse or
 *       promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    CorticalSurface plugin — implements TASK 4.1 (FreeSurfer cortical
 *           surface plugin) of the v2.3.0 plan.
 *
 *           This first slice ships the plugin shell, FreeSurfer subject
 *           browser, hemisphere/surface-type selection, and a `MultimodalScene`
 *           registration of the loaded surfaces. The QRhi rendering, vertex
 *           picking dock, and STC overlay loading land in subsequent slices
 *           (TASK 4.2 – 4.5).
 *
 */

#ifndef MNEANALYZE_CORTICAL_SURFACE_H
#define MNEANALYZE_CORTICAL_SURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "cortical_surface_global.h"

#include <anShared/Plugins/abstractplugin.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>
#include <QSharedPointer>
#include <QtCore/QtPlugin>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QAction;
class QComboBox;
class QDockWidget;
class QMenu;
class QWidget;

namespace ANSHAREDLIB {
    class Communicator;
    class Event;
}

namespace DISP3DLIB {
    class MultimodalScene;
}

namespace FSLIB {
    class FsSurface;
}

//=============================================================================================================
// DEFINE NAMESPACE CORTICALSURFACEPLUGIN
//=============================================================================================================

namespace CORTICALSURFACEPLUGIN
{

//=============================================================================================================
/**
 * @brief Hemisphere selection for the cortical surface plugin.
 */
enum class HemisphereChoice {
    LeftOnly = 0,
    RightOnly,
    Both
};

//=============================================================================================================
/**
 * @brief Cortical surface variant requested by the user.
 *
 * Maps directly to the FreeSurfer surface filename suffix
 * (`lh.<name>` / `rh.<name>`).
 */
enum class CorticalSurfaceType {
    Inflated = 0,    /**< `lh.inflated` / `rh.inflated` */
    Pial,            /**< `lh.pial` / `rh.pial` */
    White            /**< `lh.white` / `rh.white` */
};

//=============================================================================================================
/**
 * @brief mne_analyze plugin that loads and visualises FreeSurfer cortical surfaces.
 *
 * Workflow:
 *   1. User invokes "Load Surface…" from the plugin menu.
 *   2. The plugin asks for the FreeSurfer `SUBJECTS_DIR` and lists the
 *      subjects found there.
 *   3. The user picks subject + hemisphere set + surface type.
 *   4. The plugin loads the requested @ref FSLIB::FsSurface objects and
 *      registers them as @c BrainSurface layers on the shared
 *      @ref DISP3DLIB::MultimodalScene.
 *   5. The hemisphere toggle and surface-type combo box let the user
 *      switch what is shown without re-opening the dialog.
 *
 * The plugin emits the standard `MODEL_ADDED` event so other plugins
 * (TASK 4.2 STC overlay, TASK 4.3 vertex picking) can subscribe.
 */
class CORTICAL_SURFACE_SHARED_EXPORT CorticalSurface : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "cortical_surface.json")
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

public:
    CorticalSurface();
    ~CorticalSurface() override;

    // AbstractPlugin contract
    QSharedPointer<AbstractPlugin> clone() const override;
    void                           init() override;
    void                           unload() override;
    QString                        getName() const override;

    QMenu*                         getMenu() override;
    QDockWidget*                   getControl() override;
    QWidget*                       getView() override;
    QString                        getBuildInfo() override;

    void                           handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

    //=========================================================================================================
    /**
     * @return Pointer to the shared MultimodalScene used by this plugin.
     *         Owned by the plugin; consumers must not delete it.
     */
    DISP3DLIB::MultimodalScene* scene() const;

    //=========================================================================================================
    /**
     * Force-load the configured surfaces from a given FreeSurfer subject
     * directory. Public so unit tests can drive the loader without a GUI.
     *
     * @param[in] subjectsDir   Path to the SUBJECTS_DIR.
     * @param[in] subjectId     Subject id (sub-directory name in @p subjectsDir).
     * @param[in] hemi          Which hemispheres to load.
     * @param[in] type          Surface variant.
     *
     * @return true if at least one surface was loaded successfully.
     */
    bool loadSurfaces(const QString& subjectsDir,
                      const QString& subjectId,
                      HemisphereChoice hemi,
                      CorticalSurfaceType type);

    //=========================================================================================================
    /**
     * @return Map subject id -> (lh.inflated exists, rh.inflated exists)
     *         enumerated from @p subjectsDir. Used by the load dialog and
     *         by tests.
     */
    static QStringList enumerateSubjects(const QString& subjectsDir);

private slots:
    void onLoadSurfaceTriggered();
    void onHemisphereChoiceChanged(int index);
    void onSurfaceTypeChanged(int index);

private:
    /** Build the controls dock (hemisphere combo, surface-type combo,
     *  "Load Surface…" button, status label). Call once from @ref init. */
    void buildControlDock();

    /** Translate (subject_id, hemi, type) into a FreeSurfer file path. */
    static QString surfaceFilePath(const QString& subjectsDir,
                                   const QString& subjectId,
                                   int            hemiCode,    // 0 = lh, 1 = rh
                                   CorticalSurfaceType type);

    /** Map a plugin enum to the on-disk surface filename suffix. */
    static QString surfaceTypeSuffix(CorticalSurfaceType type);

    /** Re-register the loaded surfaces on the scene with current visibility. */
    void refreshSceneLayers();

    ANSHAREDLIB::Communicator*           m_pCommu = nullptr;
    QPointer<QMenu>                      m_pMenu;
    QPointer<QAction>                    m_pLoadSurfaceAction;
    QPointer<QDockWidget>                m_pControlDock;
    QPointer<QComboBox>                  m_pHemiCombo;
    QPointer<QComboBox>                  m_pSurfaceTypeCombo;

    HemisphereChoice                     m_hemi = HemisphereChoice::Both;
    CorticalSurfaceType                  m_surfaceType = CorticalSurfaceType::Inflated;

    /** Loaded surfaces keyed by hemisphere index (0 = lh, 1 = rh). */
    QSharedPointer<FSLIB::FsSurface>     m_pSurfaceLh;
    QSharedPointer<FSLIB::FsSurface>     m_pSurfaceRh;

    /** Per-plugin scene; v2.3.0 next slice will share this with TASK 10's
     *  MNE Inspect plugin via the AnalyzeData store. */
    QScopedPointer<DISP3DLIB::MultimodalScene> m_pScene;

    QString                              m_lastSubjectsDir;
    QString                              m_lastSubjectId;
};

} // namespace CORTICALSURFACEPLUGIN

#endif // MNEANALYZE_CORTICAL_SURFACE_H
