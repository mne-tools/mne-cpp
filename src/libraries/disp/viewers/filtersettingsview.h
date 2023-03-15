//=============================================================================================================
/**
 * @file     filtersettingsview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
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
 * @brief    Declaration of the FilterSettingsView Class.
 *
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
 * DECLARE CLASS FilterSettingsView
 *
 * @brief The FilterSettingsView class provides a view to select between different modalities
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
