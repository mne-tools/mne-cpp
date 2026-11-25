//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     modalityselectionview.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     May 2014
 * @brief    Per-modality visibility / activation checkbox panel (MEG-MAG, MEG-GRAD, EEG, EOG, …).
 *
 * ModalitySelectionView builds one @c QCheckBox per channel-modality
 * present in the active @c FiffInfo and emits a @c modalitiesChanged
 * list whenever the user toggles one. It is consumed primarily by
 * @ref mne::disp::ButterflyView and @ref mne::disp::AverageLayoutView to hide entire
 * modalities at once.
 */

#ifndef MODALITYSELECTIONVIEW_H
#define MODALITYSELECTIONVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"
#include <mne/fiff/fiff_ch_info.h>

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
 * @brief Per-modality visibility toggle panel (one @c QCheckBox per channel modality).
 *
 * Builds itself from the active @c FiffInfo modalities and emits
 * @c modalitiesChanged so @ref ButterflyView and
 * @ref AverageLayoutView can hide entire modalities at once.
 */
class DISPSHARED_EXPORT ModalitySelectionView : public AbstractView
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a ModalitySelectionView which is a child of parent.
     *
     * @param[in] lChannelList Channels whose modalities are offered for selection.
     * @param[in] sSettingsPath Settings path under which the widget persists its state (empty = no persistence).
     * @param[in] parent        parent of widget.
     * @param[in] f Qt window flags passed to the QWidget constructor.
     */
    ModalitySelectionView(const std::vector<mne::FiffChInfo>& lChannelList,
                          const QString& sSettingsPath = "",
                          QWidget* parent = 0,
                          Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the ModalitySelectionView.
     */
    ~ModalitySelectionView();

    //=========================================================================================================
    /**
     * Set the modality checkboxes.
     *
     * @param[in] modalityMap    The modality map.
     */
    void setModalityMap(const std::map<std::string, bool>& modalityMap);

    //=========================================================================================================
    /**
     * Get the activation of the already created modality check boxes.
     *
     * @return The current modality map.
     */
    std::map<std::string, bool> getModalityMap();

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

    //=========================================================================================================
    /**
     * Slot called when modality check boxes were changed
     *
     * @param[in] state  New check state (unused; all check boxes are re-read).
     */
    void onUpdateModalityCheckbox(Qt::CheckState state);

    std::map<std::string, bool> m_modalityMap;       /**< Map of different modalities. */
    std::vector<QCheckBox*> m_qListModalityCheckBox; /**< List of modality checkboxes. */

    std::vector<std::string> m_lChannelTypeList; /**< Channel type list. */

    QString m_sSettingsPath; /**< The settings path to store the GUI settings to. */

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the user changed the modality.
     */
    void modalitiesChanged(const std::map<std::string, bool>& modalityMap);
};
} // NAMESPACE

#endif // MODALITYSELECTIONVIEW_H
