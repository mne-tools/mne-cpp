//=============================================================================================================
/**
* @file     controls.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017 Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Controls class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "controls.h"
#include "view.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QRadioButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QSlider>
#include <QPushButton>

#ifndef QT_NO_OPENGL
#include <QtOpenGL>
#else
#include <QtWidgets>
#endif


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Controls::Controls(QWidget* parent)
: QWidget(parent)
, m_pView(Q_NULLPTR)
{

}


//*************************************************************************************************************

Controls::Controls(View* v, QWidget* parent)
: QWidget(parent)
, m_pView(v)
{
    createLayout();
}


//*************************************************************************************************************

void Controls::setView(View *v)
{
    if(!v){
        fprintf(stderr,"Error: The view to set is empty!");
        return;
    }
    if( m_pView ) {
        qDebug() << "TODO Make sure that the old view is disconnected and destroyed later on.";
    }

    m_pView = v;
    createLayout();
}


//*************************************************************************************************************

void Controls::createNetworkControls(QWidget *parent)
{
    //
    // Create Controls
    //
    QPushButton *trainButton = new QPushButton(parent);
    trainButton->setText(tr("Train"));

    //
    // Layouts
    //
    QVBoxLayout *networkGroupLayout = new QVBoxLayout(parent);
    networkGroupLayout->addWidget(trainButton);

    //
    // Set up connections
    //
//    connect(trainButton, SIGNAL(clicked(bool)), m_renderer, SLOT(setDescriptionEnabled(bool)));

}


//*************************************************************************************************************

void Controls::createAppearanceControls(QWidget* parent)
{
    //
    // Create Controls
    //
    QGroupBox* penStyleGroup = new QGroupBox(parent);
    penStyleGroup->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    QRadioButton *solidLine = new QRadioButton(penStyleGroup);
    QRadioButton *dashLine = new QRadioButton(penStyleGroup);
    QRadioButton *dotLine = new QRadioButton(penStyleGroup);
    penStyleGroup->setTitle(tr("Pen Style"));
    solidLine->setText(tr("Solid Line"));
    dashLine->setText(tr("Dash Line"));
    dotLine->setText(tr("Dot Line"));
    solidLine->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    dashLine->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    dotLine->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QGroupBox* penWidthGroup = new QGroupBox(parent);
    QSlider *penWidth = new QSlider(Qt::Horizontal, penWidthGroup);
    penWidth->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    penWidthGroup->setTitle(tr("Pen Width"));
    penWidth->setRange(0, 500);

    QPushButton *antialiasButton = new QPushButton(parent);
    antialiasButton->setText(tr("Antialiasing"));
    antialiasButton->setCheckable(true);

    QPushButton *openGlButton = new QPushButton(parent);
    openGlButton->setText(tr("Use OpenGL"));
    openGlButton->setCheckable(true);

#ifndef QT_NO_OPENGL
    openGlButton->setEnabled(QGLFormat::hasOpenGL());
#else
    openGlButton->setEnabled(false);
#endif

    //
    // Layouts
    //
    QVBoxLayout *penStyleGroupLayout = new QVBoxLayout(penStyleGroup);
    penStyleGroupLayout->addWidget(solidLine);
    penStyleGroupLayout->addWidget(dashLine);
    penStyleGroupLayout->addWidget(dotLine);

    // Appearance Group
    QVBoxLayout *appearanceGroupLayout = new QVBoxLayout(parent);
    appearanceGroupLayout->setMargin(3);
    appearanceGroupLayout->addWidget(penStyleGroup);
    appearanceGroupLayout->addWidget(penWidthGroup);
    appearanceGroupLayout->addWidget(antialiasButton);
#ifndef QT_NO_OPENGL
    appearanceGroupLayout->addWidget(openGlButton);
#endif
    appearanceGroupLayout->addStretch(1);

    QVBoxLayout *penWidthLayout = new QVBoxLayout(penWidthGroup);
    penWidthLayout->addWidget(penWidth);


    //
    // Set up connections
    //
//    connect(solidLine, SIGNAL(clicked()), m_renderer, SLOT(setSolidLine()));
//    connect(dashLine, SIGNAL(clicked()), m_renderer, SLOT(setDashLine()));
//    connect(dotLine, SIGNAL(clicked()), m_renderer, SLOT(setDotLine()));
//    connect(penWidth, SIGNAL(valueChanged(int)), m_renderer, SLOT(setPenWidth(int)));

    connect(antialiasButton, &QPushButton::toggled, m_pView, &View::enableAntialiasing);
#ifndef QT_NO_OPENGL
    connect(openGlButton, &QPushButton::clicked, m_pView, &View::enableOpenGL);
#endif

    //
    // Set the defaults
    //
    solidLine->setChecked(true);

    antialiasButton->setChecked(m_pView->usesAntialiasing());
#ifndef QT_NO_OPENGL
    openGlButton->setChecked(m_pView->usesOpenGL());
#endif
    penWidth->setValue(50);
}


//*************************************************************************************************************

void Controls::createViewControls(QWidget *parent)
{
    QToolButton * selectModeButton = new QToolButton;//Pointer Mode
    selectModeButton->setText(tr("Select"));
    selectModeButton->setCheckable(true);
    selectModeButton->setChecked(true);
    QToolButton * dragModeButton = new QToolButton;
    dragModeButton->setText(tr("Drag"));
    dragModeButton->setCheckable(true);
    dragModeButton->setChecked(false);

    // TODO print button should go into the menu
    QPushButton * printButton = new QPushButton;
    printButton->setText(tr("Print"));
    printButton->setIcon(QIcon(QPixmap(":/fileprint.png")));

    QButtonGroup *pointerModeGroup = new QButtonGroup(parent);
    pointerModeGroup->setExclusive(true);
    pointerModeGroup->addButton(selectModeButton);
    pointerModeGroup->addButton(dragModeButton);

    QHBoxLayout *labelLayout = new QHBoxLayout;
    labelLayout->addWidget(selectModeButton);
    labelLayout->addWidget(dragModeButton);

    QVBoxLayout *viewLayout = new QVBoxLayout(parent);

    viewLayout->addLayout(labelLayout);
    viewLayout->addWidget(printButton);

    //
    // Set up connections
    //
    connect(selectModeButton, &QToolButton::toggled, m_pView, &View::enableSelectMode);
    connect(printButton, &QPushButton::clicked, m_pView, &View::print);

    //
    // Set the defaults
    //
}


//*************************************************************************************************************

void Controls::createLayout()
{
    //
    // Group configuration
    //

    // Network
    QGroupBox *networkGroup = new QGroupBox(this);
    networkGroup->setFixedWidth(180);
    networkGroup->setTitle(tr("Network"));

    createNetworkControls(networkGroup);

    // Appearance
    QGroupBox *appearanceGroup = new QGroupBox(this);
    appearanceGroup->setFixedWidth(180);
    appearanceGroup->setTitle(tr("Appearance"));

    createAppearanceControls(appearanceGroup);

    // View
    QGroupBox *viewGroup = new QGroupBox(this);
    viewGroup->setFixedWidth(180);
    viewGroup->setTitle(tr("View"));

    createViewControls(viewGroup);

    //
    // Layouts
    //
    QVBoxLayout * mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->addWidget(networkGroup);
    mainLayout->addWidget(appearanceGroup);
    mainLayout->addWidget(viewGroup);

    //
    // Set up connections
    //

    //
    // Set the defaults:
    //
}
