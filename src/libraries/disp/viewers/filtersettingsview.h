//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file filtersettingsview.h
 * @since July 2018
 * @brief Compact filter on/off + bandwidth panel that pops up the full @ref FilterDesignView on demand.
 *
 * FilterSettingsView is the small, always-visible front-end of the
 * filtering subsystem: an activation @c QCheckBox, a from / to
 * frequency display and an @c Edit button that brings up the heavier
 * @ref FilterDesignView. The two widgets are kept in sync through
 * @c filterChannelTypeChanged and @c filterChanged signals.
 */

#ifndef FILTERSETTINGSVIEW_H
#define FILTERSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMap>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class FilterSettingsViewWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class FilterDesignView;

//=============================================================================================================
/**
 * @brief Compact filter on / off + bandwidth panel that opens the full @ref FilterDesignView on demand.
 *
 * Houses an activation @c QCheckBox, from / to frequency labels and
 * an @c Edit button; the two widgets stay in sync via
 * @c filterChannelTypeChanged and @c filterChanged signals.
 */
class DISPSHARED_EXPORT FilterSettingsView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<FilterSettingsView> SPtr;              /**< Shared pointer type for FilterSettingsView. */
    typedef QSharedPointer<const FilterSettingsView> ConstSPtr;   /**< Const shared pointer type for FilterSettingsView. */

    //=========================================================================================================
    /**
     * Constructs a FilterSettingsView which is a child of parent.
     *
     * @param[in] parent        parent of widget.
     */
    FilterSettingsView(const QString& sSettingsPath = "",
                       QWidget *parent = 0,
                       Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the FilterSettingsView.
     */
    ~FilterSettingsView();

    //=========================================================================================================
    /**
     * Returns the filter design view used to design filters.
     */
    QSharedPointer<FilterDesignView> getFilterView();

    //=========================================================================================================
    /**
     * Returns true if the filters a set as active.
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
     * This function is called whenever the filter activation changed
     */
    void onFilterActivationChanged();

    //=========================================================================================================
    /**
     * This function is called whenever the filter parameters From changed
     */
    void onFilterFromChanged();

    //=========================================================================================================
    /**
     * This function is called whenever the filter parameters To changed
     */
    void onFilterToChanged();

    //=========================================================================================================
    /**
     * This function is called whenever the channel type changed
     *
     * @param[in] sType        the channel type.
     */
    void onFilterChannelTypeChanged(const QString& sType);

    QString                                 m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

    QSharedPointer<FilterDesignView>        m_pFilterView;                  /**< The filter view. */

    Ui::FilterSettingsViewWidget*           m_pUi;                          /**< The filter settings GUI view. */
};
} // NAMESPACE

#endif // FILTERSETTINGSVIEW_H
