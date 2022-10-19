//=============================================================================================================
/**
 * @file     fwdsettingsview.h
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 *           Gabriel B Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.1
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel, Gabriel B Motta. All rights reserved.
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
 * @brief     FwdSettingsView class declaration.
 *
 */

#ifndef FWDSETTINGSVIEW_H
#define FWDSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

#include <fiff/fiff_types.h>
#include <fs/annotationset.h>

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
    class AnnotationSet;
}

namespace FWDLIB{
    class ComputeFwdSettings;
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
 * The FwdSettingsView class provides a QWidget for the real-time Forward Solution controls.
 *
 * @brief The FwdSettingsView class provides a QWidget for the real-time Forward Solution controls.
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

    void setSettings(QSharedPointer<FWDLIB::ComputeFwdSettings>  pSettings);

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

    QSharedPointer<FWDLIB::ComputeFwdSettings>  m_pFwdSettings;

private:
    //=========================================================================================================
    /**
     * Shows forward solution directory selection dialog
     */
    void showFwdDirDialog();

    //=========================================================================================================
    /**
     * change name of solution file
     */
    void onSolNameChanged();

    //=========================================================================================================
    /**
     * Shows measurement selection dialog
     */
    void showMeasFileDialog();

    //=========================================================================================================
    /**
     * Shows source space selection dialog
     */
    void showSourceFileDialog();

    //=========================================================================================================
    /**
     * Shows Bem model selection dialog
     */
    void showBemFileDialog();

    //=========================================================================================================
    /**
     * Shows Mri->Head transformation selection dialog
     */
    void showMriFileDialog();

    //=========================================================================================================
    /**
     * Shows output file selection dialog
     */
    void showMinDistDirDialog();

    QString     m_sMinDistDir;

    //=========================================================================================================
    /**
     * Change name of MinDistDir output
     */
    void onMinDistNameChanged();

    //=========================================================================================================
    /**
     * Shows EEG model selection dialog
     */
    void showEEGModelFileDialog();

    //=========================================================================================================
    /**
     * Shows EEG model name selection selection dialog
     */
    void onEEGModelNameChanged();

    //=========================================================================================================
    /**
     * Change value of minimum distance skull - source
     */
    void onMinDistChanged();

    //=========================================================================================================
    /**
     * Change EEG sphere radius
     */
    void onEEGSphereRadChanged();

    //=========================================================================================================
    /**
     * Change EEG sphere origin
     */
    void onEEGSphereOriginChanged();

    //=========================================================================================================
    /**
     * Change settings from checkboxes
     */
    void onCheckStateChanged();

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
     * @param[in] pAnnotationSet        The Annotation set.
     */
    void atlasDirChanged(const QString& sDirPath,
                         const FSLIB::AnnotationSet::SPtr pAnnotationSet);

    //=========================================================================================================
    /**
     * Emit this signal whenever a forward computation is supposed to be triggered.
     */
    void doForwardComputation();

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // FWDSETTINGSVIEW_H

