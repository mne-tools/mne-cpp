//=============================================================================================================
/**
 * @file     noisereductionwindow.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     December, 2014
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
 * @brief    Contains the declaration of the NoiseReductionWindow class.
 *
 */

#ifndef NOISEREDUCTIONWINDOW_H
#define NOISEREDUCTIONWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_noisereductionwindow.h"

#include "fiff/fiff_info.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>
#include <QCheckBox>
#include <QSignalMapper>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================

/**
 * DECLARE CLASS NoiseReductionWindow
 *
 * @brief The NoiseReductionWindow class provides a dock window for managing SSP operator projcetions and compensators. Both for purposes of noise reduction.
 */
class NoiseReductionWindow : public QDockWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a NoiseReductionWindow which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new NoiseReductionWindow becomes a window. If parent is another widget, NoiseReductionWindow becomes a child window inside parent. NoiseReductionWindow is deleted when its parent is deleted.
    */
    NoiseReductionWindow(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Constructs a NoiseReductionWindow which is a child of parent.
    *
    * @param [in] parent        pointer to parent widget; If parent is 0, the new NoiseReductionWindow becomes a window. If parent is another widget, NoiseReductionWindow becomes a child window inside parent. NoiseReductionWindow is deleted when its parent is deleted.
    * @param [in] pFiffInfo     fiff info with the projectors and compensators.
    */
    NoiseReductionWindow(QWidget *parent, FiffInfo* pFiffInfo);

    //=========================================================================================================
    /**
    * Set new fiff info
    */
    void setFiffInfo(FiffInfo::SPtr& pFiffInfo);

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the user changes the projections.
    */
    void projSelectionChanged();

    //=========================================================================================================
    /**
    * Emit this signal whenever the user changes the compensator.
    */
    void compSelectionChanged(int to);

    //=========================================================================================================
    /**
    * Signal mapper signal for compensator changes.
    */
    void compClicked(const QString &text);

private:
    //=========================================================================================================
    /**
    * Create the widgets used in the projector group
    */
    void createProjectorGroup();

    //=========================================================================================================
    /**
    * Create the widgets used in the compensator group
    */
    void createCompensatorGroup();

    //=========================================================================================================
    /**
    * Slot called when user enables/disables all projectors
    */
    void enableDisableAllProj(bool status);

    //=========================================================================================================
    /**
    * Slot called when the projector check state changes
    */
    void checkProjStatusChanged(bool state);

    //=========================================================================================================
    /**
    * Slot called when the compensator check state changes
    */
    void checkCompStatusChanged(const QString & compName);

    //=========================================================================================================
    /**
    * Function to remove all children from a layout
    */
    void remove(QLayout* layout);

    Ui::NoiseReductionWindow *ui;                       /**< Pointer to the qt designer generated ui class.*/

    QList<QCheckBox*>   m_qListProjCheckBox;            /**< List of projection CheckBox. */
    QList<QCheckBox*>   m_qListCompCheckBox;            /**< List of compensator CheckBox. */
    QCheckBox *         m_enableDisableProjectors;      /**< Holds the enable disable all check box. */

    QSignalMapper*      m_pCompSignalMapper;

    FiffInfo::SPtr      m_pFiffInfo;                    /**< Connected fiff info. */
};

} // NAMESPACE MNEBROWSE

#endif // NOISEREDUCTIONWINDOW_H
