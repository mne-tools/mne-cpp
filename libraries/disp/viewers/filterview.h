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
* @brief    Contains the declaration of the FilterView class.
*
*/

#ifndef FILTERVIEW_H
#define FILTERVIEW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "helpers/filterdatamodel.h"
#include "helpers/filterdatadelegate.h"
#include "helpers/filterplotscene.h"

#include "utils/mnemath.h"
#include "utils/filterTools/filterdata.h"
#include "utils/filterTools/filterio.h"

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSettings>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace Ui {class FilterViewWidget;} //This must be defined outside of the DISPLIB namespace

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================


/**
* DECLARE CLASS FilterView
*
* @brief The FilterView class provides the a manager for temporal filtering.
*/
class DISPSHARED_EXPORT FilterView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<FilterView> SPtr;              /**< Shared pointer type for FilterView. */
    typedef QSharedPointer<const FilterView> ConstSPtr;   /**< Const shared pointer type for FilterView. */

    //=========================================================================================================
    /**
    * Constructs a FilterView dialog which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new FilterView becomes a window. If parent is another widget, FilterView becomes a child window inside parent. FilterView is deleted when its parent is deleted.
    */
    FilterView(QWidget *parent = 0, Qt::WindowFlags type = 0);

    //=========================================================================================================
    /**
    * Destroys the FilterView.
    * All FilterView's children are deleted first. The application exits if FilterView is the main widget.
    */
    ~FilterView();

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
    * @param[in] activateFilter     The filter activation flag.
    * @param[in] channelType        the channel Type.
    */
    void setFilterParameters(double hp, double lp, int order, int type, int designMethod, double transition, bool activateFilter, const QString &sChannelType);

    //=========================================================================================================
    /**
    * Returns the current filter.
    *
    * @return returns the list with the currently active filters
    */
    QList<FilterData> getCurrentFilter();

    //=========================================================================================================
    /**
    * Returns the currently loaded filters.
    *
    * @return returns the list with the currently loaded filters
    */
    FilterData getUserDesignedFilter();

    //=========================================================================================================
    /**
    * Returns the current activation checkBox list.
    *
    * @return returns the current activation checkBox list.
    */
    QList<QCheckBox*> getActivationCheckBoxList();

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

private:
    //=========================================================================================================
    /**
    * inits all check boxes.
    */
    void initCheckBoxes();

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
    * inits the Model View Controller.
    */
    void initMVC();

    //=========================================================================================================
    /**
    * inits the default and current filter.
    */
    void initFilters();

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
    * eventFilter reimplemented virtual function to handle object specific events
    */
    bool eventFilter(QObject *obj, QEvent *event);

    //=========================================================================================================
    /**
    * updates the filter plot scene with the newly generated filter
    */
    void updateFilterPlot();

    Ui::FilterViewWidget*       ui;                         /**< Pointer to the qt designer generated ui class.*/

    FilterData                  m_filterData;               /**< The current filter operator.*/
    FilterDataModel::SPtr       m_pFilterDataModel;         /**< The model to hold current filters.*/
    FilterDataDelegate::SPtr    m_pFilterDataDelegate;      /**< The delegate to plot the activation check boxes in column one.*/

    QList<QCheckBox*>           m_lActivationCheckBoxList;  /**< List of all filter check boxes.*/
    QStringList                 m_lDefaultFilters;          /**< List with the names of all default filters.*/

    int                         m_iWindowSize;              /**< The current window size of the loaded fiff data in the DataWindow class.*/
    int                         m_iFilterTaps;              /**< The current number of filter taps.*/
    double                      m_dSFreq;                   /**< The current sampling frequency.*/

    QSettings                   m_qSettings;                /**< QSettings variable used to write or read from independent application sessions.*/

    FilterPlotScene::SPtr       m_pFilterPlotScene;         /**< Pointer to the QGraphicsScene which holds the filter plotting.*/

signals:
    void filterChanged(QList<UTILSLIB::FilterData> activeFilter);

    void applyFilter(QString channelType);

    void filterActivated(bool state);

    void activationCheckBoxListChanged(QList<QCheckBox*> list);

protected slots:
    //=========================================================================================================
    /**
    * updates the filter activation layout
    */
    void updateDefaultFiltersActivation(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);

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
    void onSpinBoxFilterChannelType(QString channelType);

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
    * This function connects the activation checkboxes to the filter data model.
    *
    * @param [in] state holds the current state of the connected check box
    */
    void onChkBoxFilterActivation(bool state);

    //=========================================================================================================
    /**
    * This function updates the filter window to the currently selected filter in view.
    *
    * @param current holds the current index of the model view
    * @param previous holds the previous index of the model view
    */
    void filterSelectionChanged(const QModelIndex &current, const QModelIndex &previous);

};

} // NAMESPACE DISPLIB

#endif // FILTERVIEW_H
