//=============================================================================================================
/**
* @file     deepmodelviewercontrol.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    DeepModelViewerControl class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "arthurwidgets.h"
#include "deepmodelviewercontrol.h"
#include "deepmodelviewerrenderer.h"

#include <stdio.h>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DeepModelViewerControls::DeepModelViewerControls(QWidget* parent, DeepModelViewerRenderer* renderer)
: QWidget(parent)
{
    m_renderer = renderer;

    createLayout();
}


//*************************************************************************************************************

void DeepModelViewerControls::createCommonControls(QWidget* parent)
{
    m_capGroup = new QGroupBox(parent);
    m_capGroup->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    QRadioButton *flatCap = new QRadioButton(m_capGroup);
    QRadioButton *squareCap = new QRadioButton(m_capGroup);
    QRadioButton *roundCap = new QRadioButton(m_capGroup);
    m_capGroup->setTitle(tr("Cap Style"));
    flatCap->setText(tr("Flat"));
    squareCap->setText(tr("Square"));
    roundCap->setText(tr("Round"));
    flatCap->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    squareCap->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    roundCap->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_joinGroup = new QGroupBox(parent);
    m_joinGroup->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    QRadioButton *bevelJoin = new QRadioButton(m_joinGroup);
    QRadioButton *miterJoin = new QRadioButton(m_joinGroup);
    QRadioButton *roundJoin = new QRadioButton(m_joinGroup);
    m_joinGroup->setTitle(tr("Join Style"));
    bevelJoin->setText(tr("Bevel"));
    miterJoin->setText(tr("Miter"));
    roundJoin->setText(tr("Round"));

    m_styleGroup = new QGroupBox(parent);
    m_styleGroup->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    QRadioButton *solidLine = new QRadioButton(m_styleGroup);
    QRadioButton *dashLine = new QRadioButton(m_styleGroup);
    QRadioButton *dotLine = new QRadioButton(m_styleGroup);
    QRadioButton *dashDotLine = new QRadioButton(m_styleGroup);
    QRadioButton *dashDotDotLine = new QRadioButton(m_styleGroup);
    QRadioButton *customDashLine = new QRadioButton(m_styleGroup);
    m_styleGroup->setTitle(tr("Pen Style"));

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

    m_pathModeGroup = new QGroupBox(parent);
    m_pathModeGroup->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    QRadioButton *curveMode = new QRadioButton(m_pathModeGroup);
    QRadioButton *lineMode = new QRadioButton(m_pathModeGroup);
    m_pathModeGroup->setTitle(tr("Line Style"));
    curveMode->setText(tr("Curves"));
    lineMode->setText(tr("Lines"));


    // Layouts
    QVBoxLayout *capGroupLayout = new QVBoxLayout(m_capGroup);
    capGroupLayout->addWidget(flatCap);
    capGroupLayout->addWidget(squareCap);
    capGroupLayout->addWidget(roundCap);

    QVBoxLayout *joinGroupLayout = new QVBoxLayout(m_joinGroup);
    joinGroupLayout->addWidget(bevelJoin);
    joinGroupLayout->addWidget(miterJoin);
    joinGroupLayout->addWidget(roundJoin);

    QVBoxLayout *styleGroupLayout = new QVBoxLayout(m_styleGroup);
    styleGroupLayout->addWidget(solidLine);
    styleGroupLayout->addWidget(dashLine);
    styleGroupLayout->addWidget(dotLine);
    styleGroupLayout->addWidget(dashDotLine);
    styleGroupLayout->addWidget(dashDotDotLine);
    styleGroupLayout->addWidget(customDashLine);

    QVBoxLayout *pathModeGroupLayout = new QVBoxLayout(m_pathModeGroup);
    pathModeGroupLayout->addWidget(curveMode);
    pathModeGroupLayout->addWidget(lineMode);


    // Connections
    connect(flatCap, SIGNAL(clicked()), m_renderer, SLOT(setFlatCap()));
    connect(squareCap, SIGNAL(clicked()), m_renderer, SLOT(setSquareCap()));
    connect(roundCap, SIGNAL(clicked()), m_renderer, SLOT(setRoundCap()));

    connect(bevelJoin, SIGNAL(clicked()), m_renderer, SLOT(setBevelJoin()));
    connect(miterJoin, SIGNAL(clicked()), m_renderer, SLOT(setMiterJoin()));
    connect(roundJoin, SIGNAL(clicked()), m_renderer, SLOT(setRoundJoin()));

    connect(curveMode, SIGNAL(clicked()), m_renderer, SLOT(setCurveMode()));
    connect(lineMode, SIGNAL(clicked()), m_renderer, SLOT(setLineMode()));

    connect(solidLine, SIGNAL(clicked()), m_renderer, SLOT(setSolidLine()));
    connect(dashLine, SIGNAL(clicked()), m_renderer, SLOT(setDashLine()));
    connect(dotLine, SIGNAL(clicked()), m_renderer, SLOT(setDotLine()));
    connect(dashDotLine, SIGNAL(clicked()), m_renderer, SLOT(setDashDotLine()));
    connect(dashDotDotLine, SIGNAL(clicked()), m_renderer, SLOT(setDashDotDotLine()));
    connect(customDashLine, SIGNAL(clicked()), m_renderer, SLOT(setCustomDashLine()));

    // Set the defaults:
    flatCap->setChecked(true);
    bevelJoin->setChecked(true);
    curveMode->setChecked(true);
    solidLine->setChecked(true);
}


//*************************************************************************************************************

void DeepModelViewerControls::createLayout()
{
    QGroupBox *mainGroup = new QGroupBox(this);
    mainGroup->setFixedWidth(180);
    mainGroup->setTitle(tr("Path Stroking"));

    createCommonControls(mainGroup);

    QGroupBox* penWidthGroup = new QGroupBox(mainGroup);
    QSlider *penWidth = new QSlider(Qt::Horizontal, penWidthGroup);
    penWidth->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    penWidthGroup->setTitle(tr("Pen Width"));
    penWidth->setRange(0, 500);

    QPushButton *animated = new QPushButton(mainGroup);
    animated->setText(tr("Animate"));
    animated->setCheckable(true);

#ifdef QT_OPENGL_SUPPORT
    QPushButton *enableOpenGLButton = new QPushButton(mainGroup);
    enableOpenGLButton->setText(tr("Use OpenGL"));
    enableOpenGLButton->setCheckable(true);
    enableOpenGLButton->setChecked(m_renderer->usesOpenGL());
    if (!QGLFormat::hasOpenGL())
        enableOpenGLButton->hide();
#endif
    QPushButton *whatsThisButton = new QPushButton(mainGroup);
    whatsThisButton->setText(tr("What's This?"));
    whatsThisButton->setCheckable(true);


    // Layouts:
    QVBoxLayout *penWidthLayout = new QVBoxLayout(penWidthGroup);
    penWidthLayout->addWidget(penWidth);

    QVBoxLayout * mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->addWidget(mainGroup);

    QVBoxLayout *mainGroupLayout = new QVBoxLayout(mainGroup);
    mainGroupLayout->setMargin(3);
    mainGroupLayout->addWidget(m_capGroup);
    mainGroupLayout->addWidget(m_joinGroup);
    mainGroupLayout->addWidget(m_styleGroup);
    mainGroupLayout->addWidget(penWidthGroup);
    mainGroupLayout->addWidget(m_pathModeGroup);
    mainGroupLayout->addWidget(animated);
    mainGroupLayout->addStretch(1);
#ifdef QT_OPENGL_SUPPORT
    mainGroupLayout->addWidget(enableOpenGLButton);
#endif
    mainGroupLayout->addWidget(whatsThisButton);


    // Set up connections
    connect(animated, SIGNAL(toggled(bool)), m_renderer, SLOT(setAnimation(bool)));

    connect(penWidth, SIGNAL(valueChanged(int)), m_renderer, SLOT(setPenWidth(int)));
#ifdef QT_OPENGL_SUPPORT
    connect(enableOpenGLButton, SIGNAL(clicked(bool)), m_renderer, SLOT(enableOpenGL(bool)));
#endif
    connect(whatsThisButton, SIGNAL(clicked(bool)), m_renderer, SLOT(setDescriptionEnabled(bool)));
    connect(m_renderer, SIGNAL(descriptionEnabledChanged(bool)),
            whatsThisButton, SLOT(setChecked(bool)));


    // Set the defaults
    animated->setChecked(false);
    penWidth->setValue(50);

}


//*************************************************************************************************************

void DeepModelViewerControls::emitQuitSignal()
{
    emit quitPressed();
}


//*************************************************************************************************************

void DeepModelViewerControls::emitOkSignal()
{
    emit okPressed();
}
