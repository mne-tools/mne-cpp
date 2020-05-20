//=============================================================================================================
/**
 * @file     annotationview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Christoph Dinh, Lorenz Esch, Gabriel Motta. All rights reserved.
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
 * @brief    Declaration of the AnnotationSettingsView Class.
 *
 */

#ifndef ANNOTATIONSETTINGSVIEW_H
#define ANNOTATIONSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_ch_info.h>
#include <anShared/Model/annotationmodel.h>
#include "annotationdelegate.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QColorDialog>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


namespace Ui {
    class EventWindowDockWidget;
}

//=============================================================================================================
/**
 * AnnotationSettingsView
 *
 * @brief The AnnotationSettingsView class provides the GUI for adding and removing annotation.
 */
class AnnotationSettingsView : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor
     */
    AnnotationSettingsView();

    //=========================================================================================================
    /**
     * Updates the GUI combo box with a new item type, given by the input parameter.
     *
     * @param [in] currentAnnotationType    New annotation type to be added to the combo box
     */
    void updateComboBox(const QString &currentAnnotationType);

    //=========================================================================================================
    /**
     * Passes a shared pointer to the annotation model triggers all the relevant init functions
     *
     * @param [in] pAnnModel    Pointer to the annotation model of the current loaded file
     */
    void setModel(QSharedPointer<ANSHAREDLIB::AnnotationModel> pAnnModel);

    //=========================================================================================================
    /**
     * Used to pass parameters of the currently loaded fif file to the annotation model.
     *
     * @param [in] iFirst   Sample number of the first sample in the file
     * @param [in] iLast    Sample number of the last sample in the file
     * @param [in] fFreq    Sample frequency for the data in the file
     */
    void passFiffParams(int iFirst,
                        int iLast,
                        float fFreq);

    //=========================================================================================================
    /**
     * Disconnects from current model for switching between files
     */
    void disconnectFromModel();

public slots:
    //=========================================================================================================
    /**
     * Adds input parameter to annotation model as a new annotation.
     *
     * @param [in] iSample   Sample number to be added to annotation model
     */
    void addAnnotationToModel(const int iSample);

signals:
    //=========================================================================================================
    /**
     * Emits state of the Activate Annotations checkbox
     *
     * @param [in] iCheckBoxState   0 for unchecked, 1 for checked
     */
    void activeEventsChecked(const int& iCheckBoxState);

    //=========================================================================================================
    /**
     * Used to force the fiffrawviewer to redraw the data plot.
     */
    void triggerRedraw();

    void jumpToSelected();

protected slots:

    //=========================================================================================================
    /**
     * Removes currently selected annotation from model.
     */
    void removeAnnotationfromModel();

    //=========================================================================================================
    /**
     * Creates new annotation of the type currently on the Spin Box widget.
     */
    void addNewAnnotationType();

    //=========================================================================================================
    /**
     * Transmits the checkbox state of the 'Activate annotations' checkbox
     *
     * @param [in] iCheckBoxState    0 for unchecked, 2 for checked
     */
    void onActiveEventsChecked(int iCheckBoxState);

    //=========================================================================================================
    /**
     * Transmits the checkbox state of the 'Show selected only' checkbox
     *
     * @param [in] iCheckBoxState    0 for unchecked, 2 for checked
     */
    void onSelectedEventsChecked(int iCheckBoxState);

    //=========================================================================================================
    /**
     * Sets the current selected annotation in the gui and passes it to the model
     */
    void onCurrentSelectedChanged();

    //=========================================================================================================
    /**
     * Used to handle delete key to remove annotations.
     *
     * @param [in] event    a key press event
     */
    void keyPressEvent(QKeyEvent* event);

private slots:
    //=========================================================================================================
    /**
     * Forces the dock widget to be redrawn.
     */
    void onDataChanged();

    //=========================================================================================================
    /**
     * Triggers the save file dialog. Calls the save file function in the Annotation Model
     */
    void onSaveButton();

    //=========================================================================================================
    /**
     * Update selected annotation with new sample value iValue
     *
     * @param [in] iValue   new sample value for selected annotation
     */
    void realTimeDataSample(int iValue);

    //=========================================================================================================
    /**
     * Update selected annotation with new time value dValue
     *
     * @param [in] dValue   new time value for selected annotation
     */
    void realTimeDataTime(double dValue);

private:
    //=========================================================================================================
    /**
     * Creates and connects GUI items to work with view class.
     */
    void initGUIFunctionality();

    //=========================================================================================================
    /**
     * Links delegate, model and view.
     */
    void initMSVCSettings();

    Ui::EventWindowDockWidget*                      m_pUi;                          /** < Pointer to GUI elements */

    int                                             m_iCheckState;                  /** < State of show annotations checkbox (0 unchecked, 2 checked) */
    int                                             m_iCheckSelectedState;          /** < State of the show selected checkbox (0 unchecked, 2 checked) */
    int                                             m_iLastSampClicked;             /** < Number of the last sample clicked */

    QSharedPointer<AnnotationDelegate>              m_pAnnDelegate;                 /** < Pointer to associated delegate */
    QSharedPointer<ANSHAREDLIB::AnnotationModel>    m_pAnnModel;                    /** < Pointer to associated model. Points to currently loaded. */

    QColorDialog*                                   m_pColordialog;                 /** < USed for Prompting users for annotation type colors */
};

#endif // ANNOTATIONSETTINGSVIEW_H
