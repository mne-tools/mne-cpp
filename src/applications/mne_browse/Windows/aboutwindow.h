//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     aboutwindow.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     August, 2014
 * @version  2.1.0
 * @brief    Contains the declaration of the AboutWindow class.
 */

#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_aboutwindow.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>

#include <memory>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

/**
 * DECLARE CLASS AboutWindow
 *
 * @brief The AboutWindow class provides the about window.
 */
class AboutWindow : public QDockWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a AboutWindow dialog which is a child of parent.
     *
     * @param [in] parent pointer to parent widget; If parent is 0, the new AboutWindow becomes a window. If parent is another widget, AboutWindow becomes a child window inside parent. AboutWindow is deleted when its parent is deleted.
     */
    AboutWindow(QWidget *parent = 0);


    //=========================================================================================================
    /**
     * Destroys the AboutWindow.
     * All AboutWindow's children are deleted first. The application exits if AboutWindow is the main widget.
     */
    ~AboutWindow();

private:
    std::unique_ptr<Ui::AboutWindow> ui;                    /**< Pointer to the qt designer generated ui class.*/
};

} // NAMESPACE MNEBROWSE

#endif // ABOUTWINDOW_H
