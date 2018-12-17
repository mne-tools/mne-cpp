//=============================================================================================================
/**
* @file     filterview.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

#include <utils/filterTools/filterdata.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QCheckBox;

namespace Ui {
    class FilterViewWidget;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class FilterPlotScene;


/**
* DECLARE CLASS FilterDesignView
*
* @brief The FilterDesignView class provides the a manager for temporal filtering.
*/
class DISPSHARED_EXPORT FilterDesignView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<FilterDesignView> SPtr;              /**< Shared pointer type for FilterDesignView. */
    typedef QSharedPointer<const FilterDesignView> ConstSPtr;   /**< Const shared pointer type for FilterDesignView. */

    //=========================================================================================================
    /**
    * Constructs a FilterDesignView dialog which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new FilterDesignView becomes a window. If parent is another widget, FilterDesignView becomes a child window inside parent. FilterDesignView is deleted when its parent is deleted.
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
    * Inits the filter window.
    *
    * @param[in] dSFreq the new sampling frequency
    */
    void init(double dSFreq);

    //=========================================================================================================
    /**
    * Sets the new window size for the filter.
    *
    * @param[in] iWindowSize length of the data which is to be filtered
    */
    void setWindowSize(int iWindowSize);

    //=========================================================================================================
    /**
    * Sets the max number of allowed filter taps depending on the current block size of the incoming data->
    *
    * @param[in] iMaxNumberFilterTaps  number of max allowed filter taps
    */
    void setMaxFilterTaps(int iMaxNumberFilterTaps);

    //=========================================================================================================
    /**
    * Sets the new samplingRate.
    *
    * @param[in] dSamplingRate the new sampling rate
    */
    void setSamplingRate(double dSamplingRate);

    //=========================================================================================================
    /**
    * Sets the new filter parameters.
    *
    * @param[in] hp                 Highpass frequency.
    * @param[in] lp                 Lowpass frequency.
    * @param[in] order              The order of the.
    * @param[in] type               The filter type.
    * @param[in] designMethod       The filter design method.
    * @param[in] transition         The transition frequency.
    * @param[in] channelType        the channel Type.
    */
    void setFilterParameters(double hp,
                             double lp,
                             int order,
                             int type,
                             int designMethod,
                             double transition,
                             const QString &sChannelType);

    //=========================================================================================================
    /**
    * Returns the current filter.
    *
    * @return returns the current filter
    */
    UTILSLIB::FilterData getCurrentFilter();

    //=========================================================================================================
    /**
    * Returns the current channel type which is to be filtered.
    *
    * @return returns the channel type.
    */
    QString getChannelType();

    //=========================================================================================================
    /**
    * Sets the new samplingRate.
    *
    * @return return true if user designed filter is active
    */
    bool userDesignedFiltersIsActive();

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
    * updates the filter plot scene with the newly generated filter
    */
    void updateFilterPlot();

    //=========================================================================================================
    /**
    * This function gets called whenever the combo box is altered by the user via the gui.
    *
    * @param currentIndex holds the current index of the combo box
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
    * @param channelType holds the current text of the connected spin box
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

    Ui::FilterViewWidget*               ui;                         /**< Pointer to the qt designer generated ui class.*/

    QSharedPointer<FilterPlotScene>     m_pFilterPlotScene;         /**< Pointer to the QGraphicsScene which holds the filter plotting.*/

    UTILSLIB::FilterData                m_filterData;               /**< The current filter operator.*/

    QString                             m_sSettingsPath;            /**< The settings path to store the GUI settings to. */

    int                                 m_iWindowSize;              /**< The current window size of the loaded fiff data in the DataWindow class.*/
    int                                 m_iFilterTaps;              /**< The current number of filter taps.*/
    double                              m_dSFreq;                   /**< The current sampling frequency.*/

signals:
    //=========================================================================================================
    /**
    * Emitted when the filter changes.
    *
    * @param activeFilter  The currently active filters.
    */
    void filterChanged(const UTILSLIB::FilterData& activeFilter);

    //=========================================================================================================
    /**
    * Emitted when the filter should be applied.
    *
    * @param channelType  The channel type on which the filter should be performed on.
    */
    void filterChannelTypeChanged(const QString& channelType);
};

} // NAMESPACE DISPLIB

#endif // FILTERDESIGNVIEW_H
