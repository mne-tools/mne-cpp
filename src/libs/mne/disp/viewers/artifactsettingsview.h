//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     artifactsettingsview.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January 2018
 * @brief    Per-modality artefact-rejection threshold panel (mag, grad, EEG, ECG, EOG).
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
#include <mne/fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <map>
#include <string>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QCheckBox;
class QDoubleSpinBox;
class QSpinBox;
class QGridLayout;

namespace Ui
{
class AverageSettingsViewWidget;
}

namespace mne
{
class FiffEvokedSet;
class FiffChInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE mne::disp
//=============================================================================================================

namespace mne::disp
{

//=============================================================================================================
// mne::disp FORWARD DECLARATIONS
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
    explicit ArtifactSettingsView(const QString& sSettingsPath = "",
                                  const std::vector<mne::FiffChInfo>& fiffChInfoList = std::vector<mne::FiffChInfo>(),
                                  QWidget* parent = nullptr);

    //=========================================================================================================
    /**
     * Destroys the ArtifactSettingsView.
     */
    ~ArtifactSettingsView();

    void setChInfo(const std::vector<mne::FiffChInfo>& fiffChInfoList);

    std::map<std::string, double> getThresholdMap();

    void setThresholdMap(const std::map<std::string, double>& mapThresholds);

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

    std::map<std::string, QDoubleSpinBox*> m_mapChThresholdsDoubleSpinBoxes;
    std::map<std::string, QSpinBox*> m_mapChThresholdsSpinBoxes;

    std::map<std::string, double> m_mapThresholdsFirst;
    std::map<std::string, int> m_mapThresholdsSecond;
    std::map<std::string, double> m_mapThresholds;

    std::vector<mne::FiffChInfo> m_fiffChInfoList;

    bool m_bDoArtifactThresholdReduction = false;

    QPointer<QCheckBox> m_pArtifactRejectionCheckBox;

signals:
    void changeArtifactThreshold(const std::map<std::string, double>& mapThresholds);
};
} // NAMESPACE

#endif // ARTIFACTSETTINGSVIEW_H
