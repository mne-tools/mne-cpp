//=============================================================================================================
/**
 * @file     mdiview.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @version  dev
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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

#include <anShared/Interfaces/IStandardView.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QMdiSubWindow>
#include <QPainter>
#include <QListView>

#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
#include <QPrinter>
#include <QPrintDialog>
#endif


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANALYZE;
using namespace ANSHAREDLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MdiView::MdiView(QWidget *parent)
: QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout;
    splitterHorizontal = new QSplitter(this);
    splitterHorizontal->setOrientation(Qt::Horizontal);
    splitterVertical = new QSplitter(this);
    splitterVertical->setOrientation(Qt::Vertical);
    splitterVertical->addWidget(splitterHorizontal);
    layout->addWidget(splitterVertical);
    QListView *listview = new QListView;
    splitterHorizontal->addWidget(listview);
    QListView *listviewa = new QListView;
    splitterHorizontal->addWidget(listviewa);
    QListView *listviewv = new QListView;
    splitterHorizontal->addWidget(listviewv);
    QListView *listviewvx = new QListView;
    splitterHorizontal->addWidget(listviewvx);
    this->setLayout(layout);
}


//*************************************************************************************************************

MdiView::~MdiView()
{

}


//*************************************************************************************************************

void MdiView::printCurrentSubWindow()
{
//    if(! currentSubWindow())
//        return;

//#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
//    IStandardView *view = qobject_cast<IStandardView *>(currentSubWindow());
//    // if no standard view -> render widget to printer otherwise call print function
//    if(! view){
//        QPrinter printer;
//        QPrintDialog dialog(&printer, this);
//        if (dialog.exec() == QDialog::Accepted) {
//            QPainter painter(&printer);
//            currentSubWindow()->render(&painter);
//        }
//    }
//    else {
//        view->print();
//    }
//#endif

}
