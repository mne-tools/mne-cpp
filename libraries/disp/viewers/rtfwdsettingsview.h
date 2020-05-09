//=============================================================================================================
/**
 * @file     rtfwdsettingsview.h
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief     RtFwdSettingsView class declaration.
 *
 */

#ifndef RTFWDSETTINGSVIEW_H
#define RTFWDSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include <fiff/fiff_types.h>
#include <fs/annotationset.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QWidget>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FSLIB {
    class AnnotationSet;
}

namespace Ui {
    class RtFwdSettingsViewWidget;
}
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
/**
 * The RtFwdSettingsView class provides a QWidget for the real-time Forward Solution controls.
 *
 * @brief The RtFwdSettingsView class provides a QWidget for the real-time Forward Solution controls.
 */
class DISPSHARED_EXPORT RtFwdSettingsView : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a RtFwdSettingsView object.
    */
    explicit RtFwdSettingsView(const QString& sSettingsPath = "",
                               QWidget *parent = 0,
                               Qt::WindowFlags f = Qt::Widget);

    ~RtFwdSettingsView();

    //=========================================================================================================
    /**
     * Get status of recomputation.
     *
     * @return  The icurrent recomputation status checked.
     */
    bool getRecomputationStatusChanged();

    //=========================================================================================================
    /**
     * Updates the recomputation status
     *
     * @param[in] bRecomputationStatus            if recomputation finished.
     */
    void setRecomputationStatus(bool bRecomputationStatus);

    //=========================================================================================================
    /**
     * Get status of clustering.
     *
     * @return  Wheter clustering is checked or not.
     */
    bool getClusteringStatusChanged();

    //=========================================================================================================
    /**
     * Updates the clustering status
     *
     * @param[in] bRecomputationStatus            if recomputation finished.
     */
    void setClusteringStatus(bool bRecomputationStatus);

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
     * @param[in] iNSource      Number of source dipoles.
     */

    void setClusteredInformation(int iNSource);

protected:

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     *
     * @param[in] settingsPath        the path to store the settings to.
     */
    void saveSettings(const QString& settingsPath);

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     *
     * @param[in] settingsPath        the path to load the settings from.
     */
    void loadSettings(const QString& settingsPath);

    Ui::RtFwdSettingsViewWidget*                m_ui;                   /**< The rtFwd dialog. */

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
     * @param[in] pAnnotationSet        The Annotation set.
     */
    void atlasDirChanged(const QString& sDirPath,
                         const FSLIB::AnnotationSet::SPtr pAnnotationSet);

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // RTFWDSETTINGSVIEW_H

