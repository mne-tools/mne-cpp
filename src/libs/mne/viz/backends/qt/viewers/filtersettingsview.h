//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     filtersettingsview.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Compact filter on/off + bandwidth panel that pops up the full @ref mne::viz::qt::FilterDesignView on demand.
 *
 * FilterSettingsView is the small, always-visible front-end of the
 * filtering subsystem: an activation @c QCheckBox, a from / to
 * frequency display and an @c Edit button that brings up the heavier
 * @ref mne::viz::qt::FilterDesignView. The two widgets are kept in sync through
 * @c filterChannelTypeChanged and @c filterChanged signals.
 */

#ifndef FILTERSETTINGSVIEW_H
#define FILTERSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_qt_global.h"
#include "abstractview.h"
#include <mne/viz/forms/settings_form.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE mne::viz::qt
//=============================================================================================================

namespace mne::viz::qt
{

//=============================================================================================================
// mne::viz::qt FORWARD DECLARATIONS
//=============================================================================================================

class FilterDesignView;
class SettingsPanel;

//=============================================================================================================
/**
 * @brief Compact filter on / off + bandwidth panel that opens the full @ref FilterDesignView on demand.
 *
 * Houses an activation @c QCheckBox, from / to frequency labels and
 * an @c Edit button; the two widgets stay in sync via
 * @c filterChannelTypeChanged and @c filterChanged signals.
 */
class VIZQTSHARED_EXPORT FilterSettingsView : public AbstractView
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a FilterSettingsView which is a child of parent.
     *
     * @param[in] sSettingsPath Settings path under which the widget persists its state (empty = no persistence).
     * @param[in] parent        parent of widget.
     * @param[in] f Qt window flags passed to the QWidget constructor.
     */
    FilterSettingsView(const QString& sSettingsPath = "",
                       QWidget* parent = 0,
                       Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the FilterSettingsView.
     */
    ~FilterSettingsView();

    //=========================================================================================================
    /**
     * Returns the filter design view used to design filters.
     *
     * @return Shared pointer to the owned FilterDesignView.
     */
    std::shared_ptr<FilterDesignView> getFilterView();

    //=========================================================================================================
    /**
     * Returns true if the filters a set as active.
     *
     * @return True if the activate-filter check box is checked.
     */
    bool getFilterActive();

    //=========================================================================================================
    /**
     * Sets the sampling frequency and setups this view accrodingly.
     *
     * @param[in] dSFreq the new sampling frequency.
     */
    void setSamplingRate(double dSFreq);

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

signals:
    //=========================================================================================================
    /**
     * Signal emited when the filter activation box's state is changed.
     */
    void filterActivationChanged(bool activated);

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
     * Show the filter option screen to the user.
     */
    void onShowFilterView();

    //=========================================================================================================
    /**
     * Reacts to a changed field or a triggered action of the form.
     *
     * @param[in] key   Key of the field or action.
     */
    void onFormChanged(const std::string& key);

    QString m_sSettingsPath; /**< The settings path to store the GUI settings to. */

    std::shared_ptr<FilterDesignView> m_pFilterView; /**< The filter view. */

    std::shared_ptr<viz::SettingsForm> m_pForm; /**< Activation, channel type and pass band, built by m_pPanel. */
    QPointer<SettingsPanel> m_pPanel;           /**< Qt Widgets of the form. */
    int m_listener = 0;                         /**< Id of the view's listener on the form. */
    bool m_bSilent = false;                     /**< Whether form changes are applied without reactions, e.g. while loading. */
};
} // NAMESPACE

#endif // FILTERSETTINGSVIEW_H
