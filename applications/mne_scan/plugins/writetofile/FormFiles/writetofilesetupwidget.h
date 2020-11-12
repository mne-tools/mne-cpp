//=============================================================================================================
/**
 * @file     writetofilesetupwidget.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the WriteToFileSetupWidget class.
 *
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
     * @param [in] toolbox a pointer to the corresponding WriteToFile.
     * @param [in] parent pointer to parent widget; If parent is 0, the new WriteToFileSetupWidget becomes a window. If parent is another widget, WriteToFileSetupWidget becomes a child window inside parent. WriteToFileSetupWidget is deleted when its parent is deleted.
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
