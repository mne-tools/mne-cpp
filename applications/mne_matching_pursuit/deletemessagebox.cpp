//=============================================================================================================
/**
* @file     deletemessagebox.cpp
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>;
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>;
*           Sebastian Krause <sebastian.krause@tu-ilmenau.de>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Martin Henfling, Daniel Knobl and Sebastian Krause. All rights reserved.
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
* @brief    Definition of the DeleteMesssageBox Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "deletemessagebox.h"
#include "ui_deletemessagebox.h"
#include "QtGui"

//=============================================================================================================

QString parentName;

//*************************************************************************************************************************************

// CONSTRUCTOR
DeleteMessageBox::DeleteMessageBox(QWidget *parent) :    QDialog(parent),    ui(new Ui::DeleteMessageBox)
{
    parentName =  parent->accessibleName();
    ui->setupUi(this);
}

//*************************************************************************************************************************************

DeleteMessageBox::DeleteMessageBox(QString msg_text, QString caption, QString btt_left_text, QString btt_right_text, QWidget *parent)
    :    QDialog(parent),    ui(new Ui::DeleteMessageBox)
{
    ui->setupUi(this);
    ui->lb_MessageText->setText(msg_text);
    ui->btt_yes->setText(btt_left_text);
    ui->btt_No->setText(btt_right_text);
    this->setWindowTitle(caption);
}

//*************************************************************************************************************************************

DeleteMessageBox::~DeleteMessageBox()
{
    delete ui;
}

//*************************************************************************************************************************************

void DeleteMessageBox::on_btt_yes_clicked()
{
    setResult(1);
    hide();
}

//*************************************************************************************************************************************

void DeleteMessageBox::on_btt_No_clicked()
{
    setResult(0);
    hide();
}

//*************************************************************************************************************************************

void DeleteMessageBox::on_chb_NoMessageBox_toggled(bool checked)
{
    QSettings settings;
    settings.setValue("show_warnings", !checked);
}

//*************************************************************************************************************************************
