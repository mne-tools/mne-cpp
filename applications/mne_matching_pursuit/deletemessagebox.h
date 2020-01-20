//=============================================================================================================
/**
 * @file     deletemessagebox.h
 * @author   Daniel Knobl <Daniel.Knobl@tu-ilmenau.de>
 * @version  dev
 * @date     July, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Daniel Knobl. All rights reserved.
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
 * @brief    DeleteMessageBox class declaration, which asked for acknowledgment to delete dictionaries or
 *           formulas.
 *
 */

#ifndef DELETEMESSAGEBOX_H
#define DELETEMESSAGEBOX_H

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDialog>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE Ui
//=============================================================================================================

namespace Ui
{

//=============================================================================================================

class DeleteMessageBox;
}

class DeleteMessageBox : public QDialog
{
    Q_OBJECT
    
public:

    //*********************************************************************************************************
    //constructor
    explicit DeleteMessageBox(QWidget *parent = 0);
    DeleteMessageBox(QString msg_text, QString caption, QString btt_left_text, QString btt_right_text, QWidget *parent = 0);
    //*********************************************************************************************************

    ~DeleteMessageBox();

private slots:
    void on_btt_yes_clicked();
    void on_btt_No_clicked();
    void on_chb_NoMessageBox_toggled(bool checked);

private:
    Ui::DeleteMessageBox *ui;
};

#endif // DELETEMESSAGEBOX_H
