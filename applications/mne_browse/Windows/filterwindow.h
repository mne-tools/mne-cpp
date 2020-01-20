//=============================================================================================================
/**
 * @file     filterwindow.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     August, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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

#include "ui_filterwindowdock.h"
#include "mainwindow.h"
#include "../Utils/filterplotscene.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSettings>
#include <QGraphicsScene>
#include <QSvgGenerator>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{


//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================

class MainWindow;

/**
 * DECLARE CLASS FilterWindow
 *
 * @brief The FilterWindow class provides the filter window.
 */
class FilterWindow : public QDockWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a FilterWindow dialog which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new FilterWindow becomes a window. If parent is another widget, FilterWindow becomes a child window inside parent. FilterWindow is deleted when its parent is deleted.
    */
    FilterWindow(MainWindow *mainWindow, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the FilterWindow.
    * All FilterWindow's children are deleted first. The application exits if FilterWindow is the main widget.
    */
    ~FilterWindow();

    //=========================================================================================================
    /**
    * On new file loaded.
    */
    void newFileLoaded(FiffInfo::SPtr& pFiffInfo);

private:
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
    * inits the table views.
    */
    void initTableViews();

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

    Ui::FilterWindowDockWidget *ui;             /**< Pointer to the qt designer generated ui class.*/

    MainWindow*         m_pMainWindow;          /**< Pointer to the parent, the MainWindow class.*/

    int                 m_iWindowSize;          /**< The current window size of the loaded fiff data in the DataWindow class.*/
    int                 m_iFilterTaps;          /**< The current number of filter taps.*/

    QSettings           m_qSettings;            /**< QSettings variable used to write or read from independent application sessions.*/

    FilterPlotScene*    m_pFilterPlotScene;     /**< Pointer to the QGraphicsScene which holds the filter plotting.*/

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
    * This function gets called whenever the filter parameters are altered by the user via the gui.
    */
    void filterParametersChanged();

    //=========================================================================================================
    /**
    * This function applies the user defined filter to all channels.
    */
    void applyFilter();

    //=========================================================================================================
    /**
    * This function undoes the user defined filter to all channels.
    */
    void undoFilter();

    //=========================================================================================================
    /**
    * Saves an svg graphic of the scene if wanted by the user.
    */
    void exportFilterPlot();

    //=========================================================================================================
    /**
    * This function exports the filter coefficients to a txt file.
    */
    void exportFilterCoefficients();
};

} // NAMESPACE MNEBROWSE

#endif // FILTERWINDOW_H
