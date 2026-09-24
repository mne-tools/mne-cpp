//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     noisereductionwindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     December, 2014
 * @version  2.1.0
 * @brief    Contains the declaration of the NoiseReductionWindow class.
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

#include <memory>


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

    //=========================================================================================================
    /**
     * Toggle all projectors on or off.
     */
    void toggleAllProjectors();

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

    std::unique_ptr<Ui::NoiseReductionWindow> ui;           /**< Pointer to the qt designer generated ui class.*/

    QList<QCheckBox*>   m_qListProjCheckBox;            /**< List of projection CheckBox. */
    QList<QCheckBox*>   m_qListCompCheckBox;            /**< List of compensator CheckBox. */
    QCheckBox *         m_enableDisableProjectors;      /**< Holds the enable disable all check box. */

    QSignalMapper*      m_pCompSignalMapper;

    FiffInfo::SPtr      m_pFiffInfo;                    /**< Connected fiff info. */
};

} // NAMESPACE MNEBROWSE

#endif // NOISEREDUCTIONWINDOW_H
