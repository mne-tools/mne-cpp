//=============================================================================================================
/**
 * @file     processdurationmessagebox.cpp
 * @author   Daniel Knobl <Daniel.Knobl@tu-ilmenau.de>;
 *           Martin Henfling <Martin.Henfling@tu-ilmenau.de>
 * @version  dev
 * @date     July, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Daniel Knobl, Martin Henfling. All rights reserved.
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
 * @brief    Implemenation of ProcessDurationMessagebox class.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "processdurationmessagebox.h"
#include "ui_processdurationmessagebox.h"

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include "QtGui"

//=============================================================================================================
//constructor
processdurationmessagebox::processdurationmessagebox(QWidget *parent):QDialog(parent),ui(new Ui::processdurationmessagebox)
{
    ui->setupUi(this);
}

//*****************************************************************************************************************

processdurationmessagebox::~processdurationmessagebox()
{
    delete ui;
}

//*****************************************************************************************************************

void processdurationmessagebox::on_chb_NoMessageBox_toggled(bool checked)
{
   QSettings settings;
   settings.setValue("show_warnings", !checked);
}

//*****************************************************************************************************************

void processdurationmessagebox::on_pushButton_clicked()
{
    setResult(1);
    hide();
}

//*****************************************************************************************************************
