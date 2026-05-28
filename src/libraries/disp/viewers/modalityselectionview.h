//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file modalityselectionview.h
 * @since 2022
 * @date  February 2026
 * @brief Per-modality visibility / activation checkbox panel (MEG-MAG, MEG-GRAD, EEG, EOG, …).
 *
 * ModalitySelectionView builds one @c QCheckBox per channel-modality
 * present in the active @c FiffInfo and emits a @c modalitiesChanged
 * list whenever the user toggles one. It is consumed primarily by
 * @ref ButterflyView and @ref AverageLayoutView to hide entire
 * modalities at once.
 */

#ifndef MODALITYSELECTIONVIEW_H
#define MODALITYSELECTIONVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QCheckBox;

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
    typedef QSharedPointer<ModalitySelectionView> SPtr;              /**< Shared pointer type for ModalitySelectionView. */
    typedef QSharedPointer<const ModalitySelectionView> ConstSPtr;   /**< Const shared pointer type for ModalitySelectionView. */

    //=========================================================================================================
    /**
     * Constructs a ModalitySelectionView which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    ModalitySelectionView(const QList<FIFFLIB::FiffChInfo> &lChannelList,
                          const QString& sSettingsPath = "",
                          QWidget *parent = 0,
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
    void setModalityMap(const QMap<QString, bool>& modalityMap);

    //=========================================================================================================
    /**
     * Get the activation of the already created modality check boxes.
     *
     * @return The current modality map.
     */
    QMap<QString, bool> getModalityMap();

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
     */
    void onUpdateModalityCheckbox(Qt::CheckState state);

    QMap<QString, bool>                 m_modalityMap;                  /**< Map of different modalities. */
    QList<QCheckBox*>                   m_qListModalityCheckBox;        /**< List of modality checkboxes. */

    QStringList                         m_lChannelTypeList;             /**< Channel type list. */

    QString                             m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

signals:
    //=========================================================================================================
    /**
     * Emit this signal whenever the user changed the modality.
     */
    void modalitiesChanged(const QMap<QString, bool>& modalityMap);
};
} // NAMESPACE

#endif // MODALITYSELECTIONVIEW_H
