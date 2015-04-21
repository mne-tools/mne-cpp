//=============================================================================================================
/**
* @file     filterwindow.h
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
* @brief    Contains the declaration of the FilterWindow class.
*
*/

#ifndef FILTERWINDOW_H
#define FILTERWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp_global.h"
#include "utils/mnemath.h"
#include "utils/filterTools/filterdata.h"
#include "filterplotscene.h"
#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSettings>
#include <QGraphicsScene>
#include <QSvgGenerator>
#include <QDate>
#include <QFileDialog>
#include <QStandardPaths>
#include <QKeyEvent>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace Ui {class FilterWindowWidget;} //This must be defined outside of the DISPLIB namespace

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================


/**
* DECLARE CLASS FilterWindow
*
* @brief The FilterWindow class provides the filter window.
*/
class DISPSHARED_EXPORT FilterWindow : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a FilterWindow dialog which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new FilterWindow becomes a window. If parent is another widget, FilterWindow becomes a child window inside parent. FilterWindow is deleted when its parent is deleted.
    */
    FilterWindow(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the FilterWindow.
    * All FilterWindow's children are deleted first. The application exits if FilterWindow is the main widget.
    */
    ~FilterWindow();

    //=========================================================================================================
    /**
    * Sets the new fiff info.
    *
    * @param[in] fiffInfo the new fiffInfo
    */
    void setFiffInfo(const FiffInfo &fiffInfo);

    //=========================================================================================================
    /**
    * Sets the new window size for the filter.
    *
    * @param[in] iWindowSize length of the data which is to be filtered
    */
    void setWindowSize(int iWindowSize);

    //=========================================================================================================
    /**
    * Returns the current filter.
    */
    FilterData& getCurrentFilter();

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

    Ui::FilterWindowWidget *ui;                 /**< Pointer to the qt designer generated ui class.*/

    FilterData          m_filterData;           /**< The current filter operator.*/

    int                 m_iWindowSize;          /**< The current window size of the loaded fiff data in the DataWindow class.*/
    int                 m_iFilterTaps;          /**< The current number of filter taps.*/

    FiffInfo            m_fiffInfo;             /**< The current fiffInfo.*/

    QSettings           m_qSettings;            /**< QSettings variable used to write or read from independent application sessions.*/

    FilterPlotScene*    m_pFilterPlotScene;     /**< Pointer to the QGraphicsScene which holds the filter plotting.*/

signals:
    void filterChanged(UTILSLIB::FilterData& filterData);

    void applyFilter(QString channelType);

    void activateFilter(bool active);

protected slots:
    //=========================================================================================================
    /**
    * This function gets called whenever the combo box is altered by the user via the gui.
    *
    * @param currentIndex holds the current index of the combo box
    */
    void changeStateSpinBoxes(int currentIndex);

    //=========================================================================================================
    /**
    * This function gets called whenever default filter combo box is altered by the user via the gui.
    *
    * @param currentIndex holds the current index of the combo box
    */
    void changeDefaultFilter(int currentIndex);

    //=========================================================================================================
    /**
    * This function gets called whenever the filter parameters are altered by the user via the gui.
    */
    void filterParametersChanged();

    //=========================================================================================================
    /**
    * This function activates the filter functionality.
    */
    void onBtnActivateFilter();

    //=========================================================================================================
    /**
    * This function applies the user defined filter to all channels.
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
};

} // NAMESPACE DISPLIB

Q_DECLARE_METATYPE(UTILSLIB::FilterData);

#endif // FILTERWINDOW_H
