//=============================================================================================================
/**
* @file     viewerwidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017 Christoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    MdiView class implementation.
*
*/
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mdiview.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPainter>

#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
#include <QPrinter>
#include <QPrintDialog>
#endif

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANALYZE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MdiView::MdiView(QWidget *parent)
: QWidget(parent)
{
    //QGridLayout is used so the viewer and MdiArea can fit always the size of MainWindow
    m_gridLayout = new QGridLayout(this);
    //Multiple Display Area, created inside ViewerWidget
    m_mdiArea = new QMdiArea(this);
    m_gridLayout->addWidget(m_mdiArea);
}


//*************************************************************************************************************

MdiView::~MdiView()
{
}


//*************************************************************************************************************

QMdiSubWindow *MdiView::addSubWindow(QWidget *widget, Qt::WindowFlags flags)
{
    return m_mdiArea->addSubWindow(widget, flags);
}


//*************************************************************************************************************

void MdiView::removeSubWindow(QWidget *widget)
{
    m_mdiArea->removeSubWindow(widget);
}


//*************************************************************************************************************

void MdiView::cascadeSubWindows()
{
    //Arrange subwindows in a Tile mode    //Arrange subwindows in a Tile mode
    this->m_mdiArea->cascadeSubWindows();
}


//*************************************************************************************************************

void MdiView::tileSubWindows()
{
    //Arrange subwindows in a Tile mode
    this->m_mdiArea->tileSubWindows();
}


//*************************************************************************************************************

void MdiView::printCurrentSubWindow()
{
    qDebug() << "TODO Interface the central view widget to have to implement a specific paint function.";
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() == QDialog::Accepted) {
        QPainter painter(&printer);
        m_mdiArea->currentSubWindow()->render(&painter);
    }
#endif

}

