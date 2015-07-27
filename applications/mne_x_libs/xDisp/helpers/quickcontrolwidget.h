//=============================================================================================================
/**
* @file     projectorwidget.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the QuickControlWidget Class.
*
*/

#ifndef QUICKCONTROLWIDGET_H
#define QUICKCONTROLWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include "../ui_quickcontrolwidget.h"
#include "fiff/fiff_info.h"
#include "fiff/fiff_constants.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QMouseEvent>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// STRUCTS
//=============================================================================================================

struct Modality {
    QString m_sName;
    bool m_bActive;
    float m_fNorm;

    Modality(QString name, bool active, double norm)
    : m_sName(name), m_bActive(active), m_fNorm(norm)
    {}
};


//=============================================================================================================
/**
* DECLARE CLASS QuickControlWidget
*
* @brief The ProjectorWidget class provides a quick control widget for scaling, filtering, projector and view options
*/
class QuickControlWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a QuickControlWidget which is a child of parent.
    *
    * @param [in] parent    parent of widget
    * @param [in] qMapChScaling    pointer to scaling information
    */
    QuickControlWidget(QMap<qint32, float> qMapChScaling, const FiffInfo::SPtr pFiffInfo, QString name = "", QWidget *parent = 0, bool bScaling = true, bool bProjections = true, bool bView = true, bool bFilter = true, bool bModalities = false, bool bTriggerDetection = true);

    //=========================================================================================================
    /**
    * Destructs a QuickControlWidget
    */
    ~QuickControlWidget();

    //=========================================================================================================
    /**
    * Call this whenever the current filters have changed.
    *
    * @param [in] list    list of QCheckBoxes which are to be added to the filter group
    */
    void filterGroupChanged(QList<QCheckBox*> list);

    //=========================================================================================================
    /**
    * Sets the values of the zoomFactor and windowSize spin boxes
    *
    * @param [in] zoomFactor    new zoomFactor value
    * @param [in] windowSize    new window size value
    */
    void setViewParameters(double zoomFactor, int windowSize);

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the scaling sliders or spin boxes changed.
    */
    void scalingChanged(QMap<qint32, float> scalingMap);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user changes the projections.
    */
    void projSelectionChanged();

    //=========================================================================================================
    /**
    * Emit this signal whenever the user changes the window size.
    */
    void timeWindowChanged(int value);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user changes the row height (zoom) of the channels.
    */
    void zoomChanged(double value);

    //=========================================================================================================
    /**
    * Emit this signal whenever the trigger infomration changed.
    */
    void triggerInfoChanged(const QMap<QString, QColor>& value, bool active, QString triggerCh, double threshold);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user is supposed to see the filter option window.
    */
    void showFilterOptions(bool state);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user changed the modality.
    */
    void settingsChanged(QList<Modality> modalityList);

protected:
    //=========================================================================================================
    /**
    * Create the widgets used in the scaling group
    */
    void createScalingGroup();

    //=========================================================================================================
    /**
    * Create the widgets used in the projector group
    */
    void createProjectorGroup();

    //=========================================================================================================
    /**
    * Create the widgets used in the view group
    */
    void createViewGroup();

    //=========================================================================================================
    /**
    * Create the widgets used in the modality group
    */
    void createModalityGroup();

    //=========================================================================================================
    /**
    * Slot called when time window size changes
    */
    void onTimeWindowChanged(int value);

    //=========================================================================================================
    /**
    * Slot called when zoome changes
    */
    void onZoomChanged(double value);

    //=========================================================================================================
    /**
    * Slot called when the projector check state changes
    */
    void checkStatusChanged(int state);

    //=========================================================================================================
    /**
    * Slot called when user enables/disables all projectors
    */
    void enableDisableAll(bool status);

    //=========================================================================================================
    /**
    * Slot called when scaling spin boxes change
    */
    void updateSpinBoxScaling(double value);

    //=========================================================================================================
    /**
    * Slot called when slider scaling change
    */
    void updateSliderScaling(int value);

    //=========================================================================================================
    /**
    * Slot called when trigger detection check box was toggled
    */
    void realTimeTriggerActiveChanged(int state);

    //=========================================================================================================
    /**
    * Slot called when trigger detection color button was clicked
    */
    void realTimeTriggerColorChanged(bool state);

    //=========================================================================================================
    /**
    * Slot called when trigger detection color button was clicked
    */
    void realTimeTriggerThresholdChanged(double value);

    //=========================================================================================================
    /**
    * Slot called when trigger detection color button was clicked
    */
    void realTimeTriggerCurrentChChanged(const QString& value);

    //=========================================================================================================
    /**
    * Reimplmented mouseMoveEvent.
    */
    void mouseMoveEvent(QMouseEvent *event);

    //=========================================================================================================
    /**
    * Reimplmented mouseMoveEvent.
    */
    void mousePressEvent(QMouseEvent *event);

    //=========================================================================================================
    /**
    * Reimplmented mouseMoveEvent.
    */
    void resizeEvent(QResizeEvent *event);

    //=========================================================================================================
    /**
    * Calculates a rect with rounded edged.
    *
    * @param [in] rect the rect which is supposed to be rounded.
    * @param [in] r the radius of round edges.
    * @return the rounded rect in form of a QRegion
    */
    QRegion roundedRect(const QRect& rect, int r);

    //=========================================================================================================
    /**
    * Is called when the minimize or maximize button was pressed.
    *
    * @param [in] state toggle state.
    */
    void toggleHideAll(bool state);

    //=========================================================================================================
    /**
    * Show the filter option screen to the user.
    *
    * @param [in] state toggle state.
    */
    void onShowFilterOptions(bool state);

    //=========================================================================================================
    /**
    * Slot called when modality check boxes were changed
    */
    void updateModalityCheckbox(qint32 state);

    //=========================================================================================================
    /**
    * Slot called when opacity slider was changed
    *
    * @param [in] value opacity value.
    */
    void onOpacityChange(qint32 value);


private:
    QPoint      m_dragPosition;     /**< the drag position of the window */

    bool        m_bScaling;         /**< Flag for drawing the scaling group box */
    bool        m_bProjections;     /**< Flag for drawing the projection group box */
    bool        m_bView;            /**< Flag for drawing the view group box */
    bool        m_bFilter;          /**< Flag for drawing the filter group box */
    bool        m_bModalitiy;       /**< Flag for drawing the modality group box */
    bool        m_bTriggerDetection;/**< Flag for drawing the trigger detection tab in the view group box */

    QMap<qint32,float>              m_qMapChScaling;                /**< Channel scaling values. */
    QMap<qint32, QDoubleSpinBox*>   m_qMapScalingDoubleSpinBox;     /**< Map of types and channel scaling line edits */
    QMap<qint32, QSlider*>          m_qMapScalingSlider;            /**< Map of types and channel scaling line edits */
    QMap<QString, QColor>           m_qMapTriggerColor;             /**< Trigger channel colors. */

    QList<Modality>     m_qListModalities;              /**< List of different modalities. */
    QList<QCheckBox*>   m_qListCheckBox;                /**< List of projection CheckBox. */
    QList<QCheckBox*>   m_qFilterListCheckBox;          /**< List of filter CheckBox. */
    QList<QCheckBox*>   m_qListModalityCheckBox;        /**< List of modality checkboxes */
    FiffInfo::SPtr      m_pFiffInfo;                    /**< Connected fiff info. */

    QString             m_sName;                        /**< Name of the widget which uses this quick control. */
    QCheckBox *         m_enableDisableProjectors;      /**< Holds the enable disable all check box. */
    QPushButton*        m_pShowFilterOptions;           /**< Holds the show filter options button. */
    QGroupBox*          m_pModalityGroupBox;            /**< Holds the modality group box. */

    Ui::QuickControlWidget *ui;                         /**< The generated UI file */
};

} // NAMESPACE XDISPLIB

Q_DECLARE_METATYPE(QList<XDISPLIB::Modality>);

#endif // QUICKCONTROLWIDGET_H
