//=============================================================================================================
/**
* @file     view.h
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
* @brief    View class declaration.
*
*/

#ifndef VIEW_H
#define VIEW_H

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QGraphicsView>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

QT_BEGIN_NAMESPACE
class QLabel;
class QSlider;
class QToolButton;
QT_END_NAMESPACE


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class View;


//=============================================================================================================
/**
* GraphicsView visualizes the contents of a QGraphicsScene in a scrollable viewport. To create a scene with
* geometrical items, see QGraphicsScene's documentation.
*
* @brief The GraphicsView class provides a widget for displaying the contents of a QGraphicsScene.
*/
class GraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
    * Constructs the GraphicsView
    *
    * @param [in] v     The View which this is the viewport of
    */
    GraphicsView(View *v) : QGraphicsView(), m_pView(v) { }

protected:
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *) override;
#endif

private:
    View *m_pView;      /**< The Widgte of this GraphicsView */
};


//=============================================================================================================
/**
* The Deep model View Widget containing the graphics view, as well as view related functions
*
* @brief The View Widget containing the graphics view
*/
class View : public QWidget
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
    * Constructs the View Widget which is a child of parent
    *
    * @param [in] parent    The parent widget
    */
    explicit View(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Returns the GraphicsView viewport of this view
    *
    * @return GraphicsView viewport
    */
    QGraphicsView *getGraphicsView() const;

public:
    //=========================================================================================================
    /**
    * Zooms into the scene
    *
    * @param [in] level    The zooming level increment
    */
    void zoomIn(int level = 1);

    //=========================================================================================================
    /**
    * Zooms out of the scene
    *
    * @param [in] level    The zooming level decrement
    */
    void zoomOut(int level = 1);


#ifndef QT_NO_OPENGL
    //=========================================================================================================
    /**
    * Sets if opengl shpuld be used
    *
    * @param [in] useOpengl     if OpenGL should be used
    */
    void enableOpenGL(bool useOpengl);
    //=========================================================================================================
    /**
    * Returns if OpenGl is used
    *
    * @return if OpenGl is used
    */
    bool usesOpenGL() const { return m_bUseOpengl; }
#endif
    //=========================================================================================================
    /**
    * Sets if antialiasing shpuld be used
    *
    * @param [in] useAntialiasing   if antialiasing should be used
    */
    void enableAntialiasing(bool useAntialiasing);
    //=========================================================================================================
    /**
    * Returns if Antialiasing is used
    *
    * @return if Antialiasing is used
    */
    bool usesAntialiasing() const { return m_bUseAntialiasing; }

    //=========================================================================================================
    /**
    * Toggles the pointer mode: Select or Drag
    */
    void togglePointerMode();

    //=========================================================================================================
    /**
    * Invokes the printing Dialog for the view
    */
    void print();

private:
    //=========================================================================================================
    /**
    * Resets the View
    */
    void resetView();
    //=========================================================================================================
    /**
    * Enables the Reset button
    */
    void setResetButtonEnabled();
    //=========================================================================================================
    /**
    * Sets the rotation Matrix up
    */
    void setupMatrix();
    //=========================================================================================================
    /**
    * Rotates left
    */
    void rotateLeft();
    //=========================================================================================================
    /**
    * Rotates right
    */
    void rotateRight();

private:
    GraphicsView *m_pGgraphicsView;     /**< The GraphicsView view port of the view */

    QToolButton *m_pSelectModeButton;   /**< Select Mode Button */
    QToolButton *m_pDragModeButton;     /**< Drag Mode Button */
    QToolButton *m_pAntialiasButton;    /**< Antialias Button */
    QToolButton *m_pPrintButton;        /**< Print Button */
    QToolButton *m_pResetButton;        /**< Reset Button */

    QSlider *m_pZoomSlider;             /**< Zoom Slider */
    QSlider *m_pRotateSlider;           /**< Rotate Slider */

    bool m_bUseAntialiasing;    /**< Antialiasing enabled? */
#ifndef QT_NO_OPENGL
    bool m_bUseOpengl;          /**< OpenGL enabled? */
#endif
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // VIEW_H
