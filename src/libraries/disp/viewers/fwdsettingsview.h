//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   johaenns <j.vorw01@gmail.com>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fwdsettingsview.h
 * @since 2022
 * @date  March 2026
 * @brief Forward-solution computation controls (BEM / source space / EEG sphere model selectors).
 *
 * FwdSettingsView exposes the parameters needed by the @c FWDLIB
 * forward-model recomputation: paths to BEM and source-space files,
 * EEG sphere / individual model choice, MEG / EEG enable flags and a
 * @c Recompute trigger. The panel also displays current model status
 * (loaded BEM, dipole count, sensor count) so the user can verify the
 * inputs before launching the heavy computation.
 */

#ifndef FWDSETTINGSVIEW_H
#define FWDSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

#include <fiff/fiff_types.h>
#include <fs/fs_annotationset.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FSLIB {
    class FsAnnotationSet;
}

namespace Ui {
    class FwdSettingsViewWidget;
}
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
/**
 * @brief Forward-solution computation control panel (BEM / source-space / EEG sphere model selectors).
 *
 * Exposes the parameters required by the @c FWDLIB recomputation job
 * and provides a @c Recompute trigger plus live status labels
 * showing the currently loaded BEM and dipole / sensor counts.
 */
class DISPSHARED_EXPORT FwdSettingsView : public AbstractView
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a FwdSettingsView object.
    */
    explicit FwdSettingsView(const QString& sSettingsPath = "",
                             QWidget *parent = 0,
                             Qt::WindowFlags f = Qt::Widget);

    ~FwdSettingsView();

    //=========================================================================================================
    /**
     * Get status of recomputation check box.
     *
     * @return  If recomputation status is checked.
     */
    bool getRecomputationStatusChanged();

    //=========================================================================================================
    /**
     * Updates the clustering status
     * (0 Initializing, 1 Computing, 2 Recomputing, 3 Clustering, 4 Not Computed, 5 Finished).
     *
     * @param[in] iStatus            status of recomputation.
     */
    void setRecomputationStatus(int iStatus);

    //=========================================================================================================
    /**
     * Get status of clustering check box.
     *
     * @return  Wheter clustering is checked or not.
     */
    bool getClusteringStatusChanged();

    //=========================================================================================================
    /**
     * Get status of cluster size spin box.
     *
     * @return  Desired number of sources in clustered source space.
     */
    int getClusterNumber();

    //=========================================================================================================
    /**
     * Shows atlas selection dialog
     */
    void showAtlasDirDialog();

    //=========================================================================================================
    /**
     * Updates forward solution information
     *
     * @param[in] iSourceOri    Source orientation: fixed or free.
     * @param[in] iCoordFrame   Coil coordinate system definition.
     * @param[in] iNSource      Number of source dipoles.
     * @param[in] iNChan        Number of channels.
     * @param[in] iNSpaces      Number of source spaces.
     */

    void setSolutionInformation(FIFFLIB::fiff_int_t iSourceOri,
                                FIFFLIB::fiff_int_t iCoordFrame,
                                int iNSource,
                                int iNChan,
                                int iNSpaces);

    //=========================================================================================================
    /**
     * Updates clustered forward solution information
     *
     * @param[in] iNSource      Number of clustered source dipoles.
     */

    void setClusteredInformation(int iNSource);

    //=========================================================================================================
    /**
     * Updates clustered forward solution information
     *
     * @param[in] bChecked      Whether the clustering check box is checked..
     */

    void onClusteringStatusChanged(bool bChecked);

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    void loadSettings();

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView();

protected:
    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param[in] mode     The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param[in] mode     The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode);

    bool                                        m_bAnnotaionsLoaded;    /**< If the annotationset is loaded. */

    Ui::FwdSettingsViewWidget*                  m_pUi;                  /**< The rtFwd dialog. */

    QString                                     m_sSettingsPath;        /**< The settings path to store the GUI settings to. */

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever recomputation checkbox changed.
     *
     * @param[in] bChecked    Whether the recomputation check box is checked.
     */
    void recompStatusChanged(bool bChecked);

    //=========================================================================================================
    /**
     * Emit this signal whenever clustering checkbox changed.
     *
     * @param[in] bChecked    Whether the clustering check box is checked.
     */
    void clusteringStatusChanged(bool bChecked);

    //=========================================================================================================
    /**
     * Emit this signal whenever the atlas directory is set.
     *
     * @param[in] sDirPath              The path to the atlas.
     * @param[in] pAnnotationSet        The FsAnnotation set.
     */
    void atlasDirChanged(const QString& sDirPath,
                         const FSLIB::FsAnnotationSet::SPtr pAnnotationSet);

    //=========================================================================================================
    /**
     * Emit this signal whenever a forward computation is supposed to be triggered.
     */
    void doForwardComputation();

    //=========================================================================================================
    /**
     * Emit this signal whenever a forward computation is supposed to be triggered.
     *
     * @param[in] iNCluster    Number of desired sources in the clustered source space.
     */
    void clusterNumberChanged(int iNCluster);

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // FWDSETTINGSVIEW_H

