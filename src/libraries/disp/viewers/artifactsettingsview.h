//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file artifactsettingsview.h
 * @since January 2019
 * @brief Per-modality artefact-rejection threshold panel (mag, grad, EEG, ECG, EOG).
 *
 * ArtifactSettingsView wires a @c QCheckBox + @c QDoubleSpinBox pair
 * for each enabled channel modality so the user can activate /
 * deactivate amplitude rejection and dial in a peak-to-peak limit per
 * channel type. Threshold edits emit @c changeArtifactThreshold(QMap)
 * which the averaging / pre-processing pipeline consumes to discard
 * epochs that exceed the limit on any participating channel.
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

//=============================================================================================================
/**
 * @brief Per-modality peak-to-peak artefact-rejection threshold panel.
 *
 * Builds one @c QCheckBox + @c QDoubleSpinBox row per supported
 * channel modality and emits @c changeArtifactThreshold(QMap) when
 * any threshold or activation flag changes.
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
