//=============================================================================================================
/**
* @file     processdurationmessagebox.cpp
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
processdurationmessagebox::processdurationmessagebox(QWidget *parent) :    QDialog(parent),    ui(new Ui::processdurationmessagebox)
{
    ui->setupUi(this);
}

processdurationmessagebox::~processdurationmessagebox()
{
    delete ui;
}

void processdurationmessagebox::on_chb_NoMessageBox_toggled(bool checked)
{
    bool mustEdit = false;
    QString contents;
    QFile configFile("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox.config");

    if(checked)
    {
        if (configFile.open(QIODevice::ReadWrite | QIODevice::Text))
        {
            while(!configFile.atEnd())
            {
                contents = configFile.readLine(0).constData();
                if(QString::compare("ShowProcessDurationMessageBox=true;\n", contents) == 0)
                    mustEdit = true;
            }
        }
        configFile.close();


        if(mustEdit)
        {
            QFile configTempFile("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox-Temp.config");

            if(!configTempFile.exists())
            {
                if (configTempFile.open(QIODevice::ReadWrite | QIODevice::Text))
                configTempFile.close();
            }


            QTextStream stream( &configTempFile );
            if (configFile.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                if (configTempFile.open (QIODevice::WriteOnly| QIODevice::Append))
                {
                    while(!configFile.atEnd())
                    {
                        contents = configFile.readLine(0).constData();
                        if(QString::compare("ShowProcessDurationMessageBox=true;\n", contents) == 0)
                            stream << QString("ShowProcessDurationMessageBox=false;");
                        else
                            stream << contents;
                    }
                }
            }

            configFile.close();
            configTempFile.close();

            configFile.remove();
            configTempFile.rename("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox.config");
        }
    }
    else
    {
        if (configFile.open(QIODevice::ReadWrite | QIODevice::Text))
        {
            while(!configFile.atEnd())
            {
                contents = configFile.readLine(0).constData();
                if(QString::compare("ShowProcessDurationMessageBox=false;\n", contents) == 0)
                    mustEdit = true;
            }
        }
        configFile.close();


        if(mustEdit)
        {
            QFile configTempFile("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox-Temp.config");

            if(!configTempFile.exists())
            {
                if (configTempFile.open(QIODevice::ReadWrite | QIODevice::Text))
                configTempFile.close();
            }


            QTextStream stream( &configTempFile );
            if (configFile.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                if (configTempFile.open (QIODevice::WriteOnly| QIODevice::Append))
                {
                    while(!configFile.atEnd())
                    {
                        contents = configFile.readLine(0).constData();
                        if(QString::compare("ShowProcessDurationMessageBox=false;\n", contents) == 0)
                            stream << QString("ShowProcessDurationMessageBox=true;");
                        else
                            stream << contents;
                    }
                }
            }

            configFile.close();
            configTempFile.close();

            configFile.remove();
            configTempFile.rename("Matching-Pursuit-Toolbox/Matching-Pursuit-Toolbox.config");
        }
    }

}
void processdurationmessagebox::on_pushButton_clicked()
{
    setResult(1);
    hide();
}


