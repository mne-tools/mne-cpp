//=============================================================================================================
/**
 * @file     control3dview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch. All rights reserved.
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
 * @brief    Control3DView class declaration
 *
 */

#ifndef DISPLIB_CONTROL3DVIEW_H
#define DISPLIB_CONTROL3DVIEW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class Control3DViewWidget;
}

class QStyledItemDelegate;
class QStandardItemModel;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
 * User GUI control for View3D.
 *
 * @brief User GUI control for the View3D.
 */
class DISPSHARED_EXPORT Control3DView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<Control3DView> SPtr;              /**< Shared pointer type for Control3DView. */
    typedef QSharedPointer<const Control3DView> ConstSPtr;   /**< Const shared pointer type for Control3DView. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] parent      The parent of the QObject.
    * @param [in] slFlags    The flags indicating which tools to display. Scaling is displayed as default. Possible flags are: "Data", "View", "Light".
    * @param [in] type
    */
    explicit Control3DView(QWidget* parent = 0,
                           const QStringList& slFlags = QStringList() << "Data" << "View" << "Light",
                           Qt::WindowType type = Qt::Widget);

    //=========================================================================================================
    /**
    * Default destructor.
    */
    ~Control3DView();

    //=========================================================================================================
    /**
    * Set the flags specifying which part of the GUI should be shown.
    *
    * @param[in] slFlags   flags.
    */
    void setFlags(const QStringList& slFlags);

    //=========================================================================================================
    /**
    * Set the delegate for the tree view.
    *
    * @param[in] pItemDelegate   The delegate.
    */
    void setDelegate(QStyledItemDelegate* pItemDelegate);

    //=========================================================================================================
    /**
    * Init the control widget based on the 3D view and data model.
    *
    * @param[in] pData3DTreeModel   The 3D data tree model.
    * @param[in] pView3D            The view3D to bec connected to this widget.
    */
    void setModel(QStandardItemModel* pDataTreeModel);

    //=========================================================================================================
    /**
    * Slot called when tree view header visibilty changed.
    */
    void onTreeViewHeaderHide();

    //=========================================================================================================
    /**
    * Slot called when tree view description visibilty changed.
    */
    void onTreeViewDescriptionHide();

protected:
    //=========================================================================================================
    /**
    * Slot called when opacity slider was changed.
    *
    * @param [in] value         opacity value.
    */
    void onOpacityChange(qint32 value);

    //=========================================================================================================
    /**
    * Slot called when the scene color changed.
    */
    void onSceneColorPicker();

    //=========================================================================================================
    /**
    * @brief customContextMenuRequested
    *
    * @param[in] pos    The position, where the right-click occurred
    */
    void onCustomContextMenuRequested(QPoint pos);

    //=========================================================================================================
    /**
    * Slot called when the user wants to change the always on top window flag.
    *
    * @param[in] state      The newly picked on top state.
    */
    void onAlwaysOnTop(bool state);

    //=========================================================================================================
    /**
    * Slot called when the user wants change the color of the scene.
    *
    * @param[in] color      The newly picked scene color.
    *
    */
    void onSceneColorChanged(const QColor& color);

    //=========================================================================================================
    /**
    * Slot called when the user wants to show the view in full screen.
    *
    * @param[in] checked      The newly picked full screen state.
    */
    void onShowFullScreen(bool checked);

    //=========================================================================================================
    /**
    * Slot called when the user wants to rotate the models.
    *
    * @param[in] checked      The newly picked rotation state.
    */
    void onRotationClicked(bool checked);

    //=========================================================================================================
    /**
    * Slot called when the user wants to toggle the coord axis.
    */
    void onCoordAxisClicked(bool checked);

    //=========================================================================================================
    /**
    * Slot called when the user wants change the color of the lights.
    */
    void onLightColorPicker();

    //=========================================================================================================
    /**
    * Slot called when the user wants change the color of the lights.
    *
    * @param[in] color      The newly picked light color.
    */
    void onLightColorChanged(const QColor& color);

    //=========================================================================================================
    /**
    * Slot called when the user wants to change the light intensity.
    *
    * @param[in] value      The newly picked light intensity value.
    */
    void onLightIntensityChanged(double value);

    Ui::Control3DViewWidget*    ui;                         /**< The pointer to the QtDesigner ui class. */

    QColor                      m_colCurrentSceneColor;     /**< Current color of the scene in all View3D's. */
    QColor                      m_colCurrentLightColor;     /**< Current color of the lights in all View3D's. */

signals:
    //=========================================================================================================
    /**
    * Use this signal whenever the scene color was changed by the user.
    *
    * @param[in] color      The newly picked color.
    */
    void sceneColorChanged(const QColor& color);

    //=========================================================================================================
    /**
    * Use this signal whenever the user wants to use full screen mode.
    *
    * @param[in] bShowFullScreen      The full screen flag.
    */
    void showFullScreen(bool bShowFullScreen);

    //=========================================================================================================
    /**
    * Use this signal whenever the user wants to rotate the 3D model.
    *
    * @param[in] bRotationChanged      The rotation flag.
    */
    void rotationChanged(bool bRotationChanged);

    //=========================================================================================================
    /**
    * Use this signal whenever the user wants to show the coordinate axis.
    *
    * @param[in] bShowCoordAxis      The coordinate axis flag.
    */
    void showCoordAxis(bool bShowCoordAxis);

    //=========================================================================================================
    /**
    * Use this signal whenever the light color was changed by the user.
    *
    * @param[in] color      The newly picked color.
    */
    void lightColorChanged(const QColor& color);

    //=========================================================================================================
    /**
    * Use this signal whenever the light intensity was changed by the user.
    *
    * @param[in] value      The newly picked intensity.
    */
    void lightIntensityChanged(double value);

    //=========================================================================================================
    /**
    * Use this signal whenever the take screenshot button was clicked by the user.
    */
    void takeScreenshotChanged();
};

} // NAMESPACE DISPLIB

#endif // DISPLIB_CONTROL3DVIEW_H
