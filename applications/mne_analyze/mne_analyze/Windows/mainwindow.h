//=============================================================================================================
/**
* @file     mainwindow.h
* @author   Franco Polo <Franco-Joel.Polo@tu-ilmenau.de>;
*			Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     January, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Franco Polo, Lorenz Esch, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
* @brief
*
*
*@file
*       mainwindow.cpp
*       mainwindow.ui
*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include "viewerwidget.h"
#include "aboutwindow.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QMainWindow>
#include <QString>
#include <QFileDialog>
#include <QDockWidget>
#include <QtWidgets/QGridLayout>

//*************************************************************************************************************
//=============================================================================================================
// DECLARE NAMESPACE Ui
//=============================================================================================================

namespace Ui {
class MainWindow;
}

//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================

class MainWindow : public QMainWindow
{
    Q_OBJECT

//=============================================================================================================

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

//=============================================================================================================

private slots:

    //AboutWindow
    void on_actionAbout_triggered();

    //MdiArea subwindows
    void on_actionCascade_triggered();
    void on_actionTile_triggered();

    //Open a FIFF file
    void on_actionOpen_data_file_triggered();

    //Docks
    void CreateDockWindows();
    void on_actionReload_surfaces_triggered();

//=============================================================================================================

private:

    //Ui setup
    Ui::MainWindow          *ui;

    //ViewerWIdget
    ViewerWidget            *m_viewerWidget;

    //AboutWindow
    AboutWindow             *m_about;

    //FIFF File management
    QString                 m_fiffFileName;

    //QFile m_fiffFile;

    //Dock Widgets
    QDockWidget             *m_layersDock;
    QDockWidget             *m_informationDock;

};

#endif // MAINWINDOW_H
