//=============================================================================================================
/**
* @file     mainwindow.cpp
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
* MNE Analyze allows the user to perform different interactive analysis. Still in development.
*
* @file
*       mainwindow.h
*       mainwindow.ui
*/
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "viewerwidget.h"

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================


MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowState(Qt::WindowMaximized);
    //Instance of ViewerWIdget
    m_viewerWidget = new ViewerWidget(this);
    this->setCentralWidget(m_viewerWidget);
    //Dock windows
    CreateDockWindows();
}

//*************************************************************************************************************

MainWindow::~MainWindow()
{
    delete ui;
}

//*************************************************************************************************************

void MainWindow::on_actionAbout_triggered()
{
    //AboutWindow pops up with info about the software
    m_about = new AboutWindow();
    m_about->show();
}

//*************************************************************************************************************

void MainWindow::on_actionCascade_triggered()
{
    //Since we need to acces some private attributes from ViewerWIdget, we need a method to do it
    //Used to arrange the subwindows that contains the surfaces and 2D plots, in a Cascade mode
    this->m_viewerWidget->CascadeSubWindows();
}

//*************************************************************************************************************

void MainWindow::on_actionTile_triggered()
{
    //Since we need to acces some private attributes from ViewerWIdget, we need a method to do it
    //Used to arrange the subwindows that contains the surfaces and 2D plots, in a Tile mode
    this->m_viewerWidget->TileSubWindows();
}

//*************************************************************************************************************

void MainWindow::on_actionOpen_data_file_triggered()
{
    //Open a FIFF file

    //Get the path
    m_fiffFileName = QFileDialog::getOpenFileName(this,
                                                    ("Open File"),
                                                    "C:/",
                                                    ("fiff File(*.fiff)"));
    //Open file
    QFile m_fiffFile(m_fiffFileName);
}

//*************************************************************************************************************

void MainWindow::CreateDockWindows()
{
    //
    //Layers DockWidget
    //
    m_layersDock = new QDockWidget(tr("Layers"), this);
    m_layersDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea,m_layersDock);
    m_layersDock->setMinimumWidth(128);
    m_layersDock->setMinimumHeight(128);

    //
    //Information DockWidget
    //
    m_informationDock = new QDockWidget(tr("Information"), this);
    m_informationDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea,m_informationDock);
    m_informationDock->setMinimumWidth(128);

}

//*************************************************************************************************************

void MainWindow::on_actionReload_surfaces_triggered()
{
        //m_viewerWidget->ReloadSurfaces();
}
