//=============================================================================================================
/**
 * @file     filterdesignview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the FilterDesignView class.
 *
 */

#ifndef FILTERDESIGNVIEW_H
#define FILTERDESIGNVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

#include <rtprocessing/helpers/filterkernel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QCheckBox;

namespace Ui {
    class FilterDesignViewWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class FilterPlotScene;

/**
 * DECLARE CLASS FilterDesignView
 *
 * @brief The FilterDesignView class provides the a manager for temporal filtering.
 */
class DISPSHARED_EXPORT FilterDesignView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<FilterDesignView> SPtr;              /**< Shared pointer type for FilterDesignView. */
    typedef QSharedPointer<const FilterDesignView> ConstSPtr;   /**< Const shared pointer type for FilterDesignView. */

    //=========================================================================================================
    /**
     * Constructs a FilterDesignView dialog which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new FilterDesignView becomes a window.
     *             If parent is another widget, FilterDesignView becomes a child window inside parent. FilterDesignView
     *             is deleted when its parent is deleted.
     */
    FilterDesignView(const QString& sSettingsPath,
                     QWidget *parent = 0,
                     Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Destroys the FilterDesignView.
     * All FilterDesignView's children are deleted first. The application exits if FilterDesignView is the main widget.
     */
    ~FilterDesignView();

    //=========================================================================================================
    /**
     * Sets the max number of allowed filter taps.
     *
     * @param[in] iMaxNumberFilterTaps  number of max allowed filter taps.
     */
    void setMaxAllowedFilterTaps(int iMaxNumberFilterTaps);

    //=========================================================================================================
    /**
     * Returns the currently set number of filter taps.
     *
     * @return returns the currently set number of filter taps.
     */
    int getFilterTaps();

    //=========================================================================================================
    /**
     * Sets the new samplingRate.
     *
     * @param[in] dSamplingRate the new sampling rate.
     */
    void setSamplingRate(double dSamplingRate);

    //=========================================================================================================
    /**
     * Sets filter 'From' value
     *
     * @param[in] dFrom    set filter 'From' value.
     */
    void setFrom(double dFrom);

    //=========================================================================================================
    /**
     * Sets filter 'To' value
     *
     * @param[in] dTo      set filter 'To' value.
     */
    void setTo(double dTo);

    //=========================================================================================================
    /**
     * Get filter 'From' value
     *
     * @return filter 'From' value.
     */
    double getFrom();

    //=========================================================================================================
    /**
     * Get filter 'To' value
     *
     * @return filter 'To' value.
     */
    double getTo();

    //=========================================================================================================
    /**
     * Returns the current filter.
     *
     * @return returns the current filter.
     */
    RTPROCESSINGLIB::FilterKernel getCurrentFilter();

    //=========================================================================================================
    /**
     * Returns the current channel type which is to be filtered.
     *
     * @return returns the channel type.
     */
    QString getChannelType();

    //=========================================================================================================
    /**
     * Sets the current channel type which is to be filtered.
     *
     * @param[in] sType               The new channel type to be filtered.
     */
    void setChannelType(const QString& sType);

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

    //=========================================================================================================
    /**
     * updates the filter plot scene with the newly generated filter
     */
    void updateFilterPlot();

    //=========================================================================================================
    /**
     * Process the event of style mode change in an upper class.
     */
    void guiStyleChanged(DISPLIB::AbstractView::StyleMode style);

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
     * inits all spin boxes.
     */
    void initSpinBoxes();

    //=========================================================================================================
    /**
     * inits all buttons.
     */
    void initButtons();

    //=========================================================================================================
    /**
     * inits the QComboBoxes.
     */
    void initComboBoxes();

    //=========================================================================================================
    /**
     * inits the filter plot.
     */
    void initFilterPlot();

    //=========================================================================================================
    /**
     * resizeEvent reimplemented virtual function to handle resize events of the filter window
     */
    void resizeEvent(QResizeEvent * event);

    //=========================================================================================================
    /**
     * keyPressEvent reimplemented virtual function to handle key events
     */
    virtual void keyPressEvent(QKeyEvent * event);

    //=========================================================================================================
    /**
     * This function gets called whenever the combo box is altered by the user via the gui.
     *
     * @param[in] currentIndex holds the current index of the combo box.
     */
    void changeStateSpinBoxes(int currentIndex);

    //=========================================================================================================
    /**
     * This function gets called whenever the filter parameters are altered by the user via the gui.
     */
    void filterParametersChanged();

    //=========================================================================================================
    /**
     * This function applies the user defined filter to all channels.
     *
     * @param[in] channelType holds the current text of the connected spin box.
     */
    void onSpinBoxFilterChannelType(const QString &channelType);

    //=========================================================================================================
    /**
     * Saves an svg graphic of the scene if wanted by the user.
     */
    void onBtnExportFilterPlot();

    //=========================================================================================================
    /**
     * This function exports the filter coefficients to a txt file.
     */
    void onBtnExportFilterCoefficients();

    //=========================================================================================================
    /**
     * This function loads a filter from a txt file.
     */
    void onBtnLoadFilter();

    //=========================================================================================================
    /**
     * Update Gui and filter based on an input filter parameters
     *
     * @param[in] filter   filter which parameters will be used to update gui and stored filter.
     */
    void updateGuiFromFilter(const RTPROCESSINGLIB::FilterKernel& filter);

    Ui::FilterDesignViewWidget*         m_pUi;                      /**< Pointer to the qt designer generated ui class.*/

    QPointer<FilterPlotScene>           m_pFilterPlotScene;         /**< Pointer to the QGraphicsScene which holds the filter plotting.*/

    RTPROCESSINGLIB::FilterKernel       m_filterKernel;             /**< The current filter operator.*/

    QString                             m_sSettingsPath;            /**< The settings path to store the GUI settings to. */

    int                                 m_iFilterTaps;              /**< The current number of filter taps.*/
    double                              m_dSFreq;                   /**< The current sampling frequency.*/

signals:
    //=========================================================================================================
    /**
     * Emitted when the filter changes.
     *
     * @param[in] activeFilter  The currently active filters.
     */
    void filterChanged(const RTPROCESSINGLIB::FilterKernel& activeFilter);

    //=========================================================================================================
    /**
     * Emitted when the filter channel type changed.
     *
     * @param[in] channelType  The channel type on which the filter should be performed on.
     */
    void filterChannelTypeChanged(const QString& channelType);

    //=========================================================================================================
    /**
     * Update to simple filter control 'From'
     *
     * @param[in] dFrom     change in filter 'From' value.
     */
    void updateFilterFrom(double dFrom);

    //=========================================================================================================
    /**
     * Update to simple filter control 'To'
     *
     * @param[in] dTo       change in filter 'To' value.
     */
    void updateFilterTo(double dTo);
};
} // NAMESPACE DISPLIB

#endif // FILTERDESIGNVIEW_H
