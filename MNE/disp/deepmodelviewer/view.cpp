//=============================================================================================================
/**
* @file     view.cpp
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
* @brief    View class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "view.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QPrinter>
#include <QPrintDialog>
#include <qmath.h>

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

#ifndef QT_NO_WHEELEVENT
void GraphicsView::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        if (e->delta() > 0)
            m_pView->zoomIn(6);
        else
            m_pView->zoomOut(6);
        e->accept();
    } else {
        QGraphicsView::wheelEvent(e);
    }
}
#endif


//*************************************************************************************************************

View::View(QWidget *parent)
: QWidget(parent)
, m_bUseAntialiasing(true)
#ifndef QT_NO_OPENGL
, m_bUseOpengl(false)
#endif
{
    m_pGgraphicsView = new GraphicsView(this);
    m_pGgraphicsView->setRenderHint(QPainter::Antialiasing, m_bUseAntialiasing);
    m_pGgraphicsView->setDragMode(QGraphicsView::RubberBandDrag);
    m_pGgraphicsView->setOptimizationFlags(QGraphicsView::DontSavePainterState);
    m_pGgraphicsView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    m_pGgraphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    int size = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
    QSize iconSize(size, size);

    QToolButton *zoomInIcon = new QToolButton;
    zoomInIcon->setAutoRepeat(true);
    zoomInIcon->setAutoRepeatInterval(33);
    zoomInIcon->setAutoRepeatDelay(0);
    zoomInIcon->setIcon(QPixmap(":/zoomin.png"));
    zoomInIcon->setIconSize(iconSize);
    QToolButton *zoomOutIcon = new QToolButton;
    zoomOutIcon->setAutoRepeat(true);
    zoomOutIcon->setAutoRepeatInterval(33);
    zoomOutIcon->setAutoRepeatDelay(0);
    zoomOutIcon->setIcon(QPixmap(":/zoomout.png"));
    zoomOutIcon->setIconSize(iconSize);
    m_pZoomSlider = new QSlider;
    m_pZoomSlider->setMinimum(0);
    m_pZoomSlider->setMaximum(500);
    m_pZoomSlider->setValue(250);
    m_pZoomSlider->setTickPosition(QSlider::TicksRight);

    // Zoom slider layout
    QVBoxLayout *zoomSliderLayout = new QVBoxLayout;
    zoomSliderLayout->addWidget(zoomInIcon);
    zoomSliderLayout->addWidget(m_pZoomSlider);
    zoomSliderLayout->addWidget(zoomOutIcon);

    QToolButton *rotateLeftIcon = new QToolButton;
    rotateLeftIcon->setIcon(QPixmap(":/rotateleft.png"));
    rotateLeftIcon->setIconSize(iconSize);
    QToolButton *rotateRightIcon = new QToolButton;
    rotateRightIcon->setIcon(QPixmap(":/rotateright.png"));
    rotateRightIcon->setIconSize(iconSize);
    m_pRotateSlider = new QSlider;
    m_pRotateSlider->setOrientation(Qt::Horizontal);
    m_pRotateSlider->setMinimum(-180);
    m_pRotateSlider->setMaximum(180);
    m_pRotateSlider->setValue(0);
    m_pRotateSlider->setTickInterval(90);
    m_pRotateSlider->setTickPosition(QSlider::TicksBelow);

    // Rotate slider layout
    QHBoxLayout *rotateSliderLayout = new QHBoxLayout;
    rotateSliderLayout->addWidget(rotateLeftIcon);
    rotateSliderLayout->addWidget(m_pRotateSlider);
    rotateSliderLayout->addWidget(rotateRightIcon);

    m_pResetButton = new QToolButton;
    m_pResetButton->setText(tr("Reset"));
    m_pResetButton->setEnabled(false);

    // Label layout
    QHBoxLayout *labelLayout = new QHBoxLayout;
    m_pSelectModeButton = new QToolButton;//Pointer Mode
    m_pSelectModeButton->setText(tr("Select"));
    m_pSelectModeButton->setCheckable(true);
    m_pSelectModeButton->setChecked(true);
    m_pDragModeButton = new QToolButton;
    m_pDragModeButton->setText(tr("Drag"));
    m_pDragModeButton->setCheckable(true);
    m_pDragModeButton->setChecked(false);
    m_pPrintButton = new QToolButton;
    m_pPrintButton->setIcon(QIcon(QPixmap(":/fileprint.png")));

    QButtonGroup *pointerModeGroup = new QButtonGroup(this);
    pointerModeGroup->setExclusive(true);
    pointerModeGroup->addButton(m_pSelectModeButton);
    pointerModeGroup->addButton(m_pDragModeButton);

    labelLayout->addStretch();
    labelLayout->addWidget(m_pSelectModeButton);
    labelLayout->addWidget(m_pDragModeButton);
    labelLayout->addStretch();
    labelLayout->addWidget(m_pPrintButton);

    QGridLayout *topLayout = new QGridLayout;
    topLayout->addLayout(labelLayout, 0, 0);
    topLayout->addWidget(m_pGgraphicsView, 1, 0);
    topLayout->addLayout(zoomSliderLayout, 1, 1);
    topLayout->addLayout(rotateSliderLayout, 2, 0);
    topLayout->addWidget(m_pResetButton, 2, 1);
    setLayout(topLayout);

    connect(m_pResetButton, &QToolButton::clicked, this, &View::resetView);
    connect(m_pZoomSlider, &QSlider::valueChanged, this, &View::setupMatrix);
    connect(m_pRotateSlider, &QSlider::valueChanged, this, &View::setupMatrix);
    connect(m_pGgraphicsView->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &View::setResetButtonEnabled);
    connect(m_pGgraphicsView->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &View::setResetButtonEnabled);
    connect(m_pSelectModeButton, &QToolButton::toggled, this, &View::togglePointerMode);
    connect(m_pDragModeButton, &QToolButton::toggled, this, &View::togglePointerMode);
    connect(rotateLeftIcon, &QToolButton::clicked, this, &View::rotateLeft);
    connect(rotateRightIcon, &QToolButton::clicked, this, &View::rotateRight);
    connect(zoomInIcon, &QToolButton::clicked, this, &View::zoomIn);
    connect(zoomOutIcon, &QToolButton::clicked, this, &View::zoomOut);
    connect(m_pPrintButton, &QToolButton::clicked, this, &View::print);

    setupMatrix();

    enableOpenGL(m_bUseOpengl);
}


//*************************************************************************************************************

QGraphicsView *View::getView() const
{
    return static_cast<QGraphicsView *>(m_pGgraphicsView);
}


//*************************************************************************************************************

void View::resetView()
{
    m_pZoomSlider->setValue(250);
    m_pRotateSlider->setValue(0);
    setupMatrix();
    m_pGgraphicsView->ensureVisible(QRectF(0, 0, 0, 0));

    m_pResetButton->setEnabled(false);
}


//*************************************************************************************************************

void View::setResetButtonEnabled()
{
    m_pResetButton->setEnabled(true);
}


//*************************************************************************************************************

void View::setupMatrix()
{
    qreal scale = qPow(qreal(2), (m_pZoomSlider->value() - 250) / qreal(50));

    QMatrix matrix;
    matrix.scale(scale, scale);
    matrix.rotate(m_pRotateSlider->value());

    m_pGgraphicsView->setMatrix(matrix);
    setResetButtonEnabled();
}


//*************************************************************************************************************

void View::togglePointerMode()
{
    m_pGgraphicsView->setDragMode(m_pSelectModeButton->isChecked()
                              ? QGraphicsView::RubberBandDrag
                              : QGraphicsView::ScrollHandDrag);
    m_pGgraphicsView->setInteractive(m_pSelectModeButton->isChecked());
}


//*************************************************************************************************************

#ifndef QT_NO_OPENGL
void View::enableOpenGL(bool useOpengl)
{
    m_pGgraphicsView->setViewport(useOpengl ? new QGLWidget(QGLFormat(QGL::SampleBuffers)) : new QWidget);
    m_bUseOpengl = useOpengl;
}
#endif


//*************************************************************************************************************

void View::enableAntialiasing(bool useAntialiasing)
{
    qDebug() << "enableAntialiasing(bool use_antialiasing)" << useAntialiasing;
    m_pGgraphicsView->setRenderHint(QPainter::Antialiasing, useAntialiasing);
    m_bUseAntialiasing = useAntialiasing;
}


//*************************************************************************************************************

void View::print()
{
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() == QDialog::Accepted) {
        QPainter painter(&printer);
        m_pGgraphicsView->render(&painter);
    }
#endif
}


//*************************************************************************************************************

void View::zoomIn(int level)
{
    if(!level) level = 1;
    m_pZoomSlider->setValue(m_pZoomSlider->value() + level);
}


//*************************************************************************************************************

void View::zoomOut(int level)
{
    if(!level) level = 1;
    m_pZoomSlider->setValue(m_pZoomSlider->value() - level);
}


//*************************************************************************************************************

void View::rotateLeft()
{
    m_pRotateSlider->setValue(m_pRotateSlider->value() - 90);
}


//*************************************************************************************************************

void View::rotateRight()
{
    m_pRotateSlider->setValue(m_pRotateSlider->value() + 90);
}
