//=============================================================================================================
/**
 * @file     filterwindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  2.1.0
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
#include "../Utils/sessionfilter.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSettings>
#include <QGraphicsScene>
#include <QSvgGenerator>

#include <memory>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{
    using namespace FIFFLIB;


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

    //=========================================================================================================
    /**
     * setFrequencies configures the filter type and corner frequencies.
     * Pass -1 for a frequency to leave it as a passthrough (e.g., only highpass: setFrequencies(1.0, -1)).
     *
     * @param [in] highpass  High-pass corner frequency in Hz, or -1 to skip.
     * @param [in] lowpass   Low-pass corner frequency in Hz, or -1 to skip.
     */
    void setFrequencies(double highpass, double lowpass);

    //=========================================================================================================
    /**
     * applyFilter applies the currently configured filter to the data.
     * Also callable programmatically (e.g. from command-line option handling).
     */
    void applyFilter();

    //=========================================================================================================
    /**
     * Return the currently configured preview filter shown in the dock.
     *
     * @return Shared session-filter definition, or null if the current UI state is invalid.
     */
    QSharedPointer<SessionFilter> currentPreviewFilter() const { return m_pUserDefinedFilter; }

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

    //=========================================================================================================
    /**
     * Rebuild the currently configured session filter from the UI state.
     *
     * @return Shared filter operator or null when no raw metadata is available yet.
     */
    QSharedPointer<SessionFilter> buildUserDefinedFilter() const;

    std::unique_ptr<Ui::FilterWindowDockWidget> ui;         /**< Pointer to the qt designer generated ui class.*/

    MainWindow*         m_pMainWindow;          /**< Pointer to the parent, the MainWindow class.*/

    QSettings           m_qSettings;            /**< QSettings variable used to write or read from independent application sessions.*/

    std::unique_ptr<FilterPlotScene> m_pFilterPlotScene;    /**< Owns the QGraphicsScene which holds the filter plotting.*/
    QSharedPointer<SessionFilter>    m_pUserDefinedFilter;  /**< Current shared session-filter definition. */

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
