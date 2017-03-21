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
    if(m_pView) {
        qDebug() << "TODO Make sure that the old view is dosconnected and destroyed later on.";
    }

    m_pView = v;
    createLayout();
}


//*************************************************************************************************************

void Controls::createCommonControls(QWidget* parent)
{
    m_pCapGroup = new QGroupBox(parent);
    m_pCapGroup->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    QRadioButton *flatCap = new QRadioButton(m_pCapGroup);
    QRadioButton *squareCap = new QRadioButton(m_pCapGroup);
    QRadioButton *roundCap = new QRadioButton(m_pCapGroup);
    m_pCapGroup->setTitle(tr("Cap Style"));
    flatCap->setText(tr("Flat"));
    squareCap->setText(tr("Square"));
    roundCap->setText(tr("Round"));
    flatCap->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    squareCap->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    roundCap->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_pJoinGroup = new QGroupBox(parent);
    m_pJoinGroup->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    QRadioButton *bevelJoin = new QRadioButton(m_pJoinGroup);
    QRadioButton *miterJoin = new QRadioButton(m_pJoinGroup);
    QRadioButton *roundJoin = new QRadioButton(m_pJoinGroup);
    m_pJoinGroup->setTitle(tr("Join Style"));
    bevelJoin->setText(tr("Bevel"));
    miterJoin->setText(tr("Miter"));
    roundJoin->setText(tr("Round"));

    m_pStyleGroup = new QGroupBox(parent);
    m_pStyleGroup->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    QRadioButton *solidLine = new QRadioButton(m_pStyleGroup);
    QRadioButton *dashLine = new QRadioButton(m_pStyleGroup);
    QRadioButton *dotLine = new QRadioButton(m_pStyleGroup);
    QRadioButton *dashDotLine = new QRadioButton(m_pStyleGroup);
    QRadioButton *dashDotDotLine = new QRadioButton(m_pStyleGroup);
    QRadioButton *customDashLine = new QRadioButton(m_pStyleGroup);
    m_pStyleGroup->setTitle(tr("Pen Style"));

    QPixmap line_solid(":res/images/line_solid.png");
    solidLine->setIcon(line_solid);
    solidLine->setIconSize(line_solid.size());
    QPixmap line_dashed(":res/images/line_dashed.png");
    dashLine->setIcon(line_dashed);
    dashLine->setIconSize(line_dashed.size());
    QPixmap line_dotted(":res/images/line_dotted.png");
    dotLine->setIcon(line_dotted);
    dotLine->setIconSize(line_dotted.size());
    QPixmap line_dash_dot(":res/images/line_dash_dot.png");
    dashDotLine->setIcon(line_dash_dot);
    dashDotLine->setIconSize(line_dash_dot.size());
    QPixmap line_dash_dot_dot(":res/images/line_dash_dot_dot.png");
    dashDotDotLine->setIcon(line_dash_dot_dot);
    dashDotDotLine->setIconSize(line_dash_dot_dot.size());
    customDashLine->setText(tr("Custom"));

    int fixedHeight = bevelJoin->sizeHint().height();
    solidLine->setFixedHeight(fixedHeight);
    dashLine->setFixedHeight(fixedHeight);
    dotLine->setFixedHeight(fixedHeight);
    dashDotLine->setFixedHeight(fixedHeight);
    dashDotDotLine->setFixedHeight(fixedHeight);

    m_pPathModeGroup = new QGroupBox(parent);
    m_pPathModeGroup->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    QRadioButton *curveMode = new QRadioButton(m_pPathModeGroup);
    QRadioButton *lineMode = new QRadioButton(m_pPathModeGroup);
    m_pPathModeGroup->setTitle(tr("Line Style"));
    curveMode->setText(tr("Curves"));
    lineMode->setText(tr("Lines"));


    // Layouts
    QVBoxLayout *capGroupLayout = new QVBoxLayout(m_pCapGroup);
    capGroupLayout->addWidget(flatCap);
    capGroupLayout->addWidget(squareCap);
    capGroupLayout->addWidget(roundCap);

    QVBoxLayout *joinGroupLayout = new QVBoxLayout(m_pJoinGroup);
    joinGroupLayout->addWidget(bevelJoin);
    joinGroupLayout->addWidget(miterJoin);
    joinGroupLayout->addWidget(roundJoin);

    QVBoxLayout *styleGroupLayout = new QVBoxLayout(m_pStyleGroup);
    styleGroupLayout->addWidget(solidLine);
    styleGroupLayout->addWidget(dashLine);
    styleGroupLayout->addWidget(dotLine);
    styleGroupLayout->addWidget(dashDotLine);
    styleGroupLayout->addWidget(dashDotDotLine);
    styleGroupLayout->addWidget(customDashLine);

    QVBoxLayout *pathModeGroupLayout = new QVBoxLayout(m_pPathModeGroup);
    pathModeGroupLayout->addWidget(curveMode);
    pathModeGroupLayout->addWidget(lineMode);


//    // Connections
//    connect(flatCap, SIGNAL(clicked()), m_renderer, SLOT(setFlatCap()));
//    connect(squareCap, SIGNAL(clicked()), m_renderer, SLOT(setSquareCap()));
//    connect(roundCap, SIGNAL(clicked()), m_renderer, SLOT(setRoundCap()));

//    connect(bevelJoin, SIGNAL(clicked()), m_renderer, SLOT(setBevelJoin()));
//    connect(miterJoin, SIGNAL(clicked()), m_renderer, SLOT(setMiterJoin()));
//    connect(roundJoin, SIGNAL(clicked()), m_renderer, SLOT(setRoundJoin()));

//    connect(curveMode, SIGNAL(clicked()), m_renderer, SLOT(setCurveMode()));
//    connect(lineMode, SIGNAL(clicked()), m_renderer, SLOT(setLineMode()));

//    connect(solidLine, SIGNAL(clicked()), m_renderer, SLOT(setSolidLine()));
//    connect(dashLine, SIGNAL(clicked()), m_renderer, SLOT(setDashLine()));
//    connect(dotLine, SIGNAL(clicked()), m_renderer, SLOT(setDotLine()));
//    connect(dashDotLine, SIGNAL(clicked()), m_renderer, SLOT(setDashDotLine()));
//    connect(dashDotDotLine, SIGNAL(clicked()), m_renderer, SLOT(setDashDotDotLine()));
//    connect(customDashLine, SIGNAL(clicked()), m_renderer, SLOT(setCustomDashLine()));

    // Set the defaults:
    flatCap->setChecked(true);
    bevelJoin->setChecked(true);
    curveMode->setChecked(true);
    solidLine->setChecked(true);
}


//*************************************************************************************************************

void Controls::createLayout()
{
    QGroupBox *mainGroup = new QGroupBox(this);
    mainGroup->setFixedWidth(180);
    mainGroup->setTitle(tr("Deep Model Viewer"));

    createCommonControls(mainGroup);

    QGroupBox* penWidthGroup = new QGroupBox(mainGroup);
    QSlider *penWidth = new QSlider(Qt::Horizontal, penWidthGroup);
    penWidth->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    penWidthGroup->setTitle(tr("Pen Width"));
    penWidth->setRange(0, 500);

    QPushButton *antialiasButton = new QPushButton(mainGroup);
    antialiasButton->setText(tr("Antialiasing"));
    antialiasButton->setCheckable(true);


    QPushButton *openGlButton = new QPushButton(mainGroup);
    openGlButton->setText(tr("Use OpenGL"));
    openGlButton->setCheckable(true);

#ifndef QT_NO_OPENGL
    openGlButton->setEnabled(QGLFormat::hasOpenGL());
#else
    openGlButton->setEnabled(false);
#endif

    QPushButton *aboutButton = new QPushButton(mainGroup);
    aboutButton->setText(tr("About"));
    aboutButton->setCheckable(true);


    // Layouts:
    QVBoxLayout *penWidthLayout = new QVBoxLayout(penWidthGroup);
    penWidthLayout->addWidget(penWidth);

    QVBoxLayout * mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->addWidget(mainGroup);

    QVBoxLayout *mainGroupLayout = new QVBoxLayout(mainGroup);
    mainGroupLayout->setMargin(3);
    mainGroupLayout->addWidget(m_pCapGroup);
    mainGroupLayout->addWidget(m_pJoinGroup);
    mainGroupLayout->addWidget(m_pStyleGroup);
    mainGroupLayout->addWidget(penWidthGroup);
    mainGroupLayout->addWidget(m_pPathModeGroup);
    mainGroupLayout->addWidget(antialiasButton);
#ifndef QT_NO_OPENGL
    mainGroupLayout->addWidget(openGlButton);
#endif
    mainGroupLayout->addStretch(1);
    mainGroupLayout->addWidget(aboutButton);


    // Set up connections
    connect(antialiasButton, SIGNAL(toggled(bool)), m_pView, SLOT(enableAntialiasing(bool)));

//    connect(penWidth, SIGNAL(valueChanged(int)), m_renderer, SLOT(setPenWidth(int)));

#ifndef QT_NO_OPENGL
    connect(openGlButton, SIGNAL(clicked(bool)), m_pView, SLOT(enableOpenGL(bool)));
#endif
//    connect(aboutButton, SIGNAL(clicked(bool)), m_renderer, SLOT(setDescriptionEnabled(bool)));
//    connect(m_renderer, SIGNAL(descriptionEnabledChanged(bool)),
//            aboutButton, SLOT(setChecked(bool)));


    // Set the defaults
    antialiasButton->setChecked(m_pView->usesAntialiasing());
#ifndef QT_NO_OPENGL
    openGlButton->setChecked(m_pView->usesOpenGL());
#endif
    penWidth->setValue(50);
}
