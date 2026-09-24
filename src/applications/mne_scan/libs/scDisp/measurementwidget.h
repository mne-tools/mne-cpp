//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     measurementwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Declaration of the MeasurementWidget Class.
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
    FsAnnotation = 1        /**< FsAnnotation tool. */
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
     * @param[in] parent pointer to parent widget; If parent is 0, the new MeasurementWidget becomes a window. If parent is another widget, MeasurementWidget becomes a child window inside parent. MeasurementWidget is deleted when its parent is deleted.
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
     * @param[in] pSubject  pointer to Subject -> not used because its direct attached to the measurement.
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
     * @return a list of display actions.
     */
    inline QList<QAction*> getDisplayActions();

signals:
    //=========================================================================================================
    /**
     * Signal to notify that new display control widgets are available.
     *
     * @param[in] lControlWidgets      A QList with pointers to the control widgets. Note that the signal sender.
     *                                  does not have ownership of these pointers.
     * @param[in] sDisplayName         The display emmiting the signal.
     */
    void displayControlWidgetsChanged(QList<QWidget*>& lControlWidgets,
                                      const QString& sDisplayName);

protected:
    //=========================================================================================================
    /**
     * Adds a display action to the current measurement widget.
     *
     * @param[in] pAction  pointer to the action to be added to the measurement widget.
     */
    inline void addDisplayAction(QAction* pAction);

    bool                m_bDisplayWidgetsInitialized;   /**< Whether the control widgets were sucesfully initialized. */

private:
    QList< QAction* >   m_qListDisplayActions;          /**< List of display actions. */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

QList<QAction*> MeasurementWidget::getDisplayActions()
{
    return m_qListDisplayActions;
}

//=============================================================================================================

inline void MeasurementWidget::addDisplayAction(QAction* pAction)
{
    m_qListDisplayActions.append(pAction);
}

} //NAMESPACE

#endif // NEWMEASUREMENTWIDGET_H
