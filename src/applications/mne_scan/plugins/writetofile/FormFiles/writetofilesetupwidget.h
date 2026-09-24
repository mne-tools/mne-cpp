//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     writetofilesetupwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2020
 * @brief    Contains the declaration of the WriteToFileSetupWidget class.
 */

#ifndef WRITETOFILESETUPWIDGET_H
#define WRITETOFILESETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_writetofilesetup.h"
#include "../writetofile.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE WRITETOFILEPLUGIN
//=============================================================================================================

namespace WRITETOFILEPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class WriteToFile;

//=============================================================================================================
/**
 * DECLARE CLASS WriteToFileSetupWidget
 *
 * @brief The WriteToFileSetupWidget class provides the WriteToFile configuration window.
 */
class WriteToFileSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a WriteToFileSetupWidget which is a child of parent.
     *
     * @param[in] toolbox a pointer to the corresponding WriteToFile.
     * @param[in] parent pointer to parent widget; If parent is 0, the new WriteToFileSetupWidget becomes a window. If parent is another widget, WriteToFileSetupWidget becomes a child window inside parent. WriteToFileSetupWidget is deleted when its parent is deleted.
     */
    WriteToFileSetupWidget(WriteToFile* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the WriteToFileSetupWidget.
     * All WriteToFileSetupWidget's children are deleted first. The application exits if WriteToFileSetupWidget is the main widget.
     */
    ~WriteToFileSetupWidget();

private:

    WriteToFile* m_pWriteToFile;	/**< Holds a pointer to corresponding WriteToFile.*/

    Ui::WriteToFileSetupWidgetClass ui;	/**< Holds the user interface for the WriteToFileSetupWidget.*/
};
} // NAMESPACE

#endif // WRITETOFILESETUPWIDGET_H
