//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eegosportsaboutwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2014
 * @brief    Contains the declaration of the EEGoSportsAboutWidget class.
 */

#ifndef EEGOSPORTSABOUTWIDGET_H
#define EEGOSPORTSABOUTWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_eegosportsabout.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE EEGOSPORTSPLUGIN
//=============================================================================================================

namespace EEGOSPORTSPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS EEGoSportsAboutWidget
 *
 * @brief The EEGoSportsAboutWidget class provides the about dialog for the EEGoSports.
 */
class EEGoSportsAboutWidget : public QDialog
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a ECGAboutWidget dialog which is a child of parent.
     *
     * @param [in] parent pointer to parent widget; If parent is 0, the new EEGoSportsAboutWidget becomes a window.
     *             If parent is another widget, EEGoSportsAboutWidget becomes a child window inside parent.
     *             EEGoSportsAboutWidget is deleted when its parent is deleted.
     */
    EEGoSportsAboutWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the EEGoSportsAboutWidget.
     * All EEGoSportsAboutWidget's children are deleted first. The application exits if EEGoSportsAboutWidget is the main widget.
     */
    ~EEGoSportsAboutWidget();

private:
    Ui::EEGoSportsAboutWidgetClass m_ui;    /**< Holds the user interface for the EEGoSportsAboutWidgetClass.*/
};
} // NAMESPACE

#endif // EEGOSPORTSABOUTWIDGET_H
