//=============================================================================================================
/**
 * @file     abstractview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch. All rights reserved.
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
 * @brief    AbstractView class declaration.
 *
 */

#ifndef DISP3DLIB_ABSTRACTVIEW_H
#define DISP3DLIB_ABSTRACTVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QWidget>
#include <QPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISPLIB {
    class QuickControlView;
    class Control3DView;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class View3D;
class Data3DTreeModel;

//=============================================================================================================
/**
 * Adapter which provides the abstract class for all adapter views.
 *
 * @brief Adapter which provides the abstract class for all adapter views.
 */
class DISP3DSHARED_EXPORT AbstractView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<AbstractView> SPtr;             /**< Shared pointer type for AbstractView class. */
    typedef QSharedPointer<const AbstractView> ConstSPtr;  /**< Const shared pointer type for AbstractView class. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    explicit AbstractView(QWidget *parent = 0,
                          Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Default destructor.
     */
    ~AbstractView();

    //=========================================================================================================
    /**
     * Returns the View3D.
     *
     * @return The currently set View3D.
     */
    QSharedPointer<DISP3DLIB::View3D> getView();

    //=========================================================================================================
    /**
     * Returns the Control3D.
     *
     * @return The currently set Control3D.
     */
    QSharedPointer<DISPLIB::Control3DView> getControlView();

    //=========================================================================================================
    /**
     * Returns the Data3DTreeModel.
     *
     * @return The currently set Data3DTreeModel.
     */
    QSharedPointer<DISP3DLIB::Data3DTreeModel> getTreeModel();

    //=========================================================================================================
    /**
     * Returns the quick control view.
     *
     * @return The currently set quick control view.
     */
    QPointer<DISPLIB::QuickControlView> getQuickControl();

    //=========================================================================================================
    /**
     * Sets the extra control widgets in the quick control view. Takes ownership of the QWidgets.
     *
     * @param[in] lControlWidgets    The new extra control widgets.
     */
    void setQuickControlWidgets(const QList<QWidget*> &lControlWidgets);

protected:
    //=========================================================================================================
    /**
     * Creates the GUI.
     */
    void createGUI();

    QSharedPointer<DISP3DLIB::View3D>                   m_p3DView;              /**< The Disp3D view. */
    QSharedPointer<DISP3DLIB::Data3DTreeModel>          m_pData3DModel;         /**< The Disp3D model. */

    QSharedPointer<DISPLIB::Control3DView>              m_pControl3DView;       /**< The Disp3D control. */
    QPointer<DISPLIB::QuickControlView>                 m_pQuickControlView;    /**< The quick control view. */
};
} // NAMESPACE

#endif // DISP3DLIB_ABSTRACTVIEW_H
