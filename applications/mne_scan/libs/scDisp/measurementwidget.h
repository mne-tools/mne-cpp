//=============================================================================================================
/**
 * @file     measurementwidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the MeasurementWidget Class.
 *
 */

#ifndef NEWMEASUREMENTWIDGET_H
#define NEWMEASUREMENTWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"

#include <scMeas/measurement.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISPLIB {
    class QuickControlView;
}

//=============================================================================================================
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace SCDISPLIB
{

//=============================================================================================================
// SCDISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Tool enumeration.
 */
enum Tool
{
    Freeze     = 0,       /**< Freezing tool. */
    Annotation = 1        /**< Annotation tool. */
};

//=============================================================================================================
/**
 * DECLARE CLASS MeasurementWidget
 *
 * @brief The MeasurementWidget class is the base class of all measurement widgets.
 */
class SCDISPSHARED_EXPORT MeasurementWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a MeasurementWidget which is a child of parent.
     *
     * @param [in] parent pointer to parent widget; If parent is 0, the new MeasurementWidget becomes a window. If parent is another widget, MeasurementWidget becomes a child window inside parent. MeasurementWidget is deleted when its parent is deleted.
     */
    MeasurementWidget(QWidget* parent = 0);

    //=========================================================================================================
    /**
     * Destroys the MeasurementWidget.
     */
    virtual ~MeasurementWidget();

    //=========================================================================================================
    /**
     * Is called when new data are available.
     * Pure virtual method inherited by IObserver.
     *
     * @param [in] pSubject  pointer to Subject -> not used because its direct attached to the measurement.
     */
    virtual void update(SCMEASLIB::Measurement::SPtr pMeasurement) = 0;

    //=========================================================================================================
    /**
     * Initialise the MeasurementWidget.
     * Pure virtual method.
     */
    virtual void init() = 0;

    //=========================================================================================================
    /**
     * A list of display actions for the current measurement widget.
     *
     * @return a list of display actions
     */
    inline QList< QAction* > getDisplayActions();

    //=========================================================================================================
    /**
     * A list of display widgets for the current measurement widget.
     *
     * @return a list of display widgets
     */
    inline QList< QWidget* > getDisplayWidgets();

protected:
    //=========================================================================================================
    /**
     * Adds a display action to the current measurement widget.
     *
     * @param [in] pAction  pointer to the action to be added to the measurement widget
     */
    inline void addDisplayAction(QAction* pAction);

    //=========================================================================================================
    /**
     * Adds a display widgetto the current measurement widget, which is attached to the toolbar
     *
     * @param [in] pWidget  pointer to the widget to be added to the measurement widget
     */
    inline void addDisplayWidget(QWidget* pWidget);

    //=========================================================================================================
    /**
     * Adds control widgets to a QuickControlView based on their set objects names
     *
     * @param [in] pQuickControlView  The quick control view to add the control widgets to
     * @param [in] lControlWidgets  The control widgets
     */
    void addControlWidgets(QSharedPointer<DISPLIB::QuickControlView> pQuickControlView,
                           const QList<QSharedPointer<QWidget> >& lControlWidgets);

private:
    QList< QAction* >   m_qListDisplayActions;      /**< List of display actions */
    QList< QWidget* >   m_qListDisplayWidgets;       /**< List of display widgets to attach to the toolbar */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

QList< QAction* > MeasurementWidget::getDisplayActions()
{
    return m_qListDisplayActions;
}

//=============================================================================================================

QList< QWidget* > MeasurementWidget::getDisplayWidgets()
{
    return m_qListDisplayWidgets;
}

//=============================================================================================================

inline void MeasurementWidget::addDisplayAction(QAction* pAction)
{
    m_qListDisplayActions.append(pAction);
}

//=============================================================================================================

inline void MeasurementWidget::addDisplayWidget(QWidget* pWidget)
{
    m_qListDisplayWidgets.append(pWidget);
}
} //NAMESPACE

#endif // NEWMEASUREMENTWIDGET_H
