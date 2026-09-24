//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     gusbampsetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     November, 2015
 * @brief    Contains the declaration of the GUSBAmpSetupWidget class.
 */

#ifndef GUSBAMPSETUPWIDGET_H
#define GUSBAMPSETUPWIDGET_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include "ui_gusbampsetupwidget.h"

//=============================================================================================================
// DEFINE NAMESPACE GUSBAMPPLUGIN
//=============================================================================================================

namespace GUSBAMPPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class GUSBAmp;

//=============================================================================================================
/**
 * DECLARE CLASS GUSBAmpSetupWidget
 *
 * @brief The GUSBAmpSetupWidget class provides the GUSBAmp configuration window.
 */
class GUSBAmpSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a GUSBAmpSetupWidget which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new GUSBAmpSetupWidget becomes a window. If parent is another widget, GUSBAmpSetupWidget becomes a child window inside parent. GUSBAmpSetupWidget is deleted when its parent is deleted.
     * @param[in] pGUSBAmp a pointer to the corresponding ECGSimulator.
     */
    GUSBAmpSetupWidget(GUSBAmp* pGUSBAmp, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the GUSBAmpSetupWidget.
     * All GUSBAmpSetupWidget's children are deleted first. The application exits if GUSBAmpSetupWidget is the main widget.
     */
    ~GUSBAmpSetupWidget();

    //=========================================================================================================
    /**
     * Initializes the Connector's GUI properties.
     *
     */
    void initGui();

    //=========================================================================================================
    /**
     * checks all the checkBoxes for their corresponding channels and sets the the vector for the
     * cannel-selection
     *
     */
    void checkBoxes();

private slots:
    //=========================================================================================================
    /**
     * push-button-click for setting the serial adresses for master and slaves
     *
     */
    void setSerialAdresses();

    //=========================================================================================================
    /**
     * activate or deactivate groupbox for singel-channel-select
     *
     */
    void activateChannelSelect(bool checked);

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel1();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel2();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel3();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel4();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel5();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel6();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel7();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel8();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel9();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel10();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel11();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel12();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel13();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel14();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel15();

    //=========================================================================================================
    /**
     * grid of checkboxes for the channel select
     */
    void activateChannel16();

    //=========================================================================================================
    /**
     * combo box for adjusting the sample rate
     */
    void on_comboBox_currentIndexChanged(const QString &arg1);

private:
    //=========================================================================================================
    /**
     * Shows the About Dialog
     *
     */
    void showAboutDialog();

    GUSBAmp*               m_pGUSBAmp;              /**< a pointer to corresponding GUSBAmp.*/
    Ui::GUSBAmpSetupClass  ui;                      /**< the user interface for the GUSBAmpSetupWidget.*/
};
} // NAMESPACE

#endif // GUSBAMPSETUPWIDGET_H
