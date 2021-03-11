//=============================================================================================================
/**
 * @file     artifactsettingsview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the ArtifactSettingsView class.
 *
 */

#ifndef ARTIFACTSETTINGSVIEW_H
#define ARTIFACTSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>
#include <QMap>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QCheckBox;
class QDoubleSpinBox;
class QSpinBox;
class QGridLayout;

namespace Ui {
    class AverageSettingsViewWidget;
}

namespace FIFFLIB {
    class FiffEvokedSet;
    class FiffChInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

/**
 * DECLARE CLASS ArtifactSettingsView
 *
 * @brief The ArtifactSettingsView class provides an artifact rejection settings view.
 */
class DISPSHARED_EXPORT ArtifactSettingsView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<ArtifactSettingsView> SPtr;         /**< Shared pointer type for AveragingAdjustmentWidget. */
    typedef QSharedPointer<ArtifactSettingsView> ConstSPtr;    /**< Const shared pointer type for AveragingAdjustmentWidget. */

    explicit ArtifactSettingsView(const QString& sSettingsPath = "",
                                  const QList<FIFFLIB::FiffChInfo>& fiffChInfoList = QList<FIFFLIB::FiffChInfo>(),
                                  QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destroys the ArtifactSettingsView.
     */
    ~ArtifactSettingsView();

    void setChInfo(const QList<FIFFLIB::FiffChInfo>& fiffChInfoList);

    QMap<QString,double> getThresholdMap();

    void setThresholdMap(const QMap<QString,double>& mapThresholds);

    bool getDoArtifactThresholdRejection();

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

    //=========================================================================================================
    /**
     * Redraw the GUI.
     */
    void redrawGUI();

    void onChangeArtifactThreshold();

    QMap<QString,QDoubleSpinBox*>   m_mapChThresholdsDoubleSpinBoxes;
    QMap<QString,QSpinBox*>         m_mapChThresholdsSpinBoxes;

    QMap<QString,double>            m_mapThresholdsFirst;
    QMap<QString,int>               m_mapThresholdsSecond;
    QMap<QString,double>            m_mapThresholds;

    QList<FIFFLIB::FiffChInfo>      m_fiffChInfoList;

    bool                            m_bDoArtifactThresholdReduction;

    QPointer<QCheckBox>             m_pArtifactRejectionCheckBox;

signals:
    void changeArtifactThreshold(const QMap<QString,double>& mapThresholds);
};
} // NAMESPACE

#endif // ARTIFACTSETTINGSVIEW_H
