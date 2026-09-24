//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     gusbampsetupprojectwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March 2016
 * @brief    Contains the declaration of the GUSBAmpSetupProjectWidget class.
 */

#ifndef GUSBAMPSETUPPROJECTWIDGET_H
#define GUSBAMPSETUPPROJECTWIDGET_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
class GUSBAmpSetupProjectWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE GUSBAMPPLUGIN
//=============================================================================================================

namespace GUSBAMPPLUGIN
{

//=============================================================================================================
// GUSBAMPPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class GUSBAmp;

//=============================================================================================================
/**
 * DECLARE CLASS GUSBAmpSetupProjectWidget
 *
 * @brief The GUSBAmpSetupProjectWidget class provides the GUSBAmpSetupProjectWidget configuration window.
 */
class GUSBAmpSetupProjectWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a TMSISetupProjectWidget which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new TMSISetupProjectWidget becomes a window. If parent is another widget, TMSISetupWidget becomes a child window inside parent. TMSISetupWidget is deleted when its parent is deleted.
     * @param[in] pTMSI a pointer to the corresponding ECGSimulator.
     */
    explicit GUSBAmpSetupProjectWidget(GUSBAmp* pGUSBAmp,
                                       QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destructs a which is a child of parent.
     *
     */
    ~GUSBAmpSetupProjectWidget();

    //=========================================================================================================
    /**
     * Inits the GUI
     *
     */
    void initGui();

private:
    //=========================================================================================================
    /**
     * Sets the dir where the eeg cap file is located
     *
     */
    void changeCap();

    //=========================================================================================================
    /**
     * Changes the EEG cap and file path variables in the EEGoSports class
     *
     */
    void changeQLineEdits();

    GUSBAmp*                        m_pGUSBAmp;         /**< a pointer to corresponding GUSBAmp.*/
    Ui::GUSBAmpSetupProjectWidget*  ui;                 /**< the user interface for the GUSBAmpSetupProjectWidget.*/
};
} // NAMESPACE

#endif // GUSBAMPSETUPPROJECTWIDGET.H
