//=============================================================================================================
/**
* @file     quickcontrolwidget.h
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

#include <disp/viewers/helpers/draggableframelesswidget.h>
#include <disp/viewers/butterflyview.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

namespace Ui {
    class QuickControlWidget;
}

class QCheckBox;
class QDoubleSpinBox;
class QSlider;
class QPushButton;
class QSignalMapper;
class QTabWidget;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace SCDISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// SCDISPLIB FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS QuickControlWidget
*
* @brief The QuickControlWidget class provides a quick control widget for scaling, filtering, projector and other control options.
*/
class QuickControlWidget : public DISPLIB::DraggableFramelessWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<QuickControlWidget> SPtr;              /**< Shared pointer type for QuickControlWidget. */
    typedef QSharedPointer<const QuickControlWidget> ConstSPtr;   /**< Const shared pointer type for QuickControlWidget. */

    //=========================================================================================================
    /**
    * Constructs a QuickControlWidget which is a child of parent.
    *
    * @param [in] qMapChScaling     The pointer to scaling information.
    * @param [in] name              The name to be displayed on the minimize button.
    * @param [in] pFiffInfo         The fiff info.
    * @param [in] slFlags           The flags indicating which tools to display. Scaling is displayed as default. Possible flags are: projections, compensators, view, filter, triggerdetection, modalities, scaling, sphara.
    * @param [in] parent            The parent of widget.
    */
    QuickControlWidget(const QMap<qint32, float>& qMapChScaling,
                       const QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo,
                       const QString& name = "",
                       const QStringList& slFlags = QStringList("Scaling"),
                       QWidget *parent = Q_NULLPTR);

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
    * @param [in] opactiy       the new opacity value
    */
    void setViewParameters(double zoomFactor, int windowSize, int opactiy);

    //=========================================================================================================
    /**
    * Get current opacity value.
    *
    * @return thecurrent set opacity value of this window.
    */
    int getOpacityValue();

    //=========================================================================================================
    /**
    * Get current distance time spacer combo box index.
    *
    * @return the current index of the distance time spacer combo box.
    */
    int getDistanceTimeSpacerIndex();

    //=========================================================================================================
    /**
    * Set current distance time spacer combo box index.
    *
    * @param [in] index     the new index value of the combo box
    */
    void setDistanceTimeSpacerIndex(int index);

    //=========================================================================================================
    /**
    * Set current signal and background colors.
    *
    * @param [in] signalColor       The new signal color.
    * @param [in] backgroundColor   The new background color.
    */
    void setSignalBackgroundColors(const QColor& signalColor, const QColor& backgroundColor);

    //=========================================================================================================
    /**
    * Set total number of detected triggers and trigger types.
    *
    * @param [in] totalNumberDetections     The numger of detected triggers
    * @param [in] mapDetectedTriggers       The currently detected triggers
    */
    void setNumberDetectedTriggersAndTypes(int totalNumberDetections, const QMap<int,QList<QPair<int,double> > >& mapDetectedTriggers);

    //=========================================================================================================
    /**
    * Set number of detected triggers.
    *
    * @param [in] lTriggerTypes     the trigger types.
    */
    void setTriggerTypes(const QList<double>& lTriggerTypes);

    //=========================================================================================================
    /**
    * Returns the current signal color.
    *
    * @return The current signal color.
    */
    const QColor& getSignalColor();

    //=========================================================================================================
    /**
    * Returns the current background color.
    *
    * @return The current background color.
    */
    const QColor& getBackgroundColor();

    //=========================================================================================================
    /**
    * Set the old average map which holds the inforamtion about the calcuated averages.
    *
    * @param [in] qMapAverageInfoOld     the old average info map.
    */
    void setAverageInformationMapOld(const QMap<double, QPair<QColor, QPair<QString,bool> > >& qMapAverageInfoOld);

    //=========================================================================================================
    /**
    * Set the average map which holds the inforamtion about the currently calcuated averages.
    *
    * @param [in] qMapAverageColor     the average map.
    */
    void setAverageInformationMap(const QMap<double, QPair<QColor, QPair<QString,bool> > >& qMapAverageColor);

    //=========================================================================================================
    /**
    * Create list of channels which are to be filtered based on channel names
    *
    * @return the average information map
    */
    QMap<double, QPair<QColor, QPair<QString,bool> > > getAverageInformationMap();

protected slots:
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
    void onCheckProjStatusChanged(bool state);

    //=========================================================================================================
    /**
    * Slot called when user enables/disables all projectors
    */
    void onEnableDisableAllProj(bool status);

    //=========================================================================================================
    /**
    * Slot called when the compensator check state changes
    */
    void onCheckCompStatusChanged(const QString & compName);

    //=========================================================================================================
    /**
    * Slot called when scaling spin boxes change
    */
    void onUpdateSpinBoxScaling(double value);

    //=========================================================================================================
    /**
    * Slot called when slider scaling change
    */
    void onUpdateSliderScaling(int value);

    //=========================================================================================================
    /**
    * Slot called when trigger detection check box was toggled
    */
    void onRealTimeTriggerActiveChanged(int state);

    //=========================================================================================================
    /**
    * Slot called when trigger detection color button was clicked
    */
    void onRealTimeTriggerColorChanged(bool state);

    //=========================================================================================================
    /**
    * Slot called when trigger detection threshold was changed
    */
    void onRealTimeTriggerThresholdChanged(double value);

    //=========================================================================================================
    /**
    * Slot called when trigger type changed
    */
    void onRealTimeTriggerColorTypeChanged(const QString& value);

    //=========================================================================================================
    /**
    * Slot called when trigger channel selection changed
    */
    void onRealTimeTriggerCurrentChChanged(const QString &value);

    //=========================================================================================================
    /**
    * Is called when the minimize or maximize button was pressed.
    *
    * @param [in] state toggle state.
    */
    void onToggleHideAll(bool state);

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
    void onUpdateModalityCheckbox(qint32 state);

    //=========================================================================================================
    /**
    * Slot called when opacity slider was changed
    *
    * @param [in] value opacity value.
    */
    void onOpacityChange(qint32 value);

    //=========================================================================================================
    /**
    * Slot called when time spacer distance changes
    *
    * @param [in] value for time spacer distance.
    */
    void onDistanceTimeSpacerChanged(qint32 value);

    //=========================================================================================================
    /**
    * Slot called when reset number of detected triggers was pressed
    */
    void onResetTriggerNumbers();

    //=========================================================================================================
    /**
    * Slot called when the user designed filter was toggled
    */
    void onUserFilterToggled(bool state);

    //=========================================================================================================
    /**
    * Slot called when the sphara tool was toggled
    */
    void onSpharaButtonClicked(bool state);

    //=========================================================================================================
    /**
    * Slot called when the user changes the sphara options
    */
    void onSpharaOptionsChanged();

    //=========================================================================================================
    /**
    * Slot called when the user changes the signal or background color.
    */
    void onViewColorButtonClicked();

    //=========================================================================================================
    /**
    * Call this slot whenever you want to make a screenshot of the butterfly or layout view.
    */
    void onMakeScreenshot();

    //=========================================================================================================
    /**
    * Call this slot whenever the averages changed.
    */
    void onAveragesChanged();

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
    * Create the widgets used in the sphara group
    */
    void createSpharaGroup();

    //=========================================================================================================
    /**
    * Create the widgets used in the view group
    */
    void createViewGroup();

    //=========================================================================================================
    /**
    * Create the widgets used in the color group
    */
    void createColorsGroup();

    //=========================================================================================================
    /**
    * Create the widgets used in the trigger detection group
    */
    void createTriggerDetectionGroup();

    //=========================================================================================================
    /**
    * Create the widgets used in the modality group
    */
    void createModalityGroup();

    //=========================================================================================================
    /**
    * Create the widgets used in the compensator group
    */
    void createCompensatorGroup();

    //=========================================================================================================
    /**
    * Create the widgets used in the averages group
    */
    void createAveragesGroup();

    //=========================================================================================================
    /**
    * Slot called when the user designed filter was toggled
    *
    * @param [in] pTabWidget    pointer to the tab widget of interest
    * @param [in] sTabText      text of the tab to find
    *
    * @return The found tab widget.
    */
    QWidget* findTabWidgetByText(const QTabWidget *pTabWidget, const QString& sTabText);

private:
    QStringList                                         m_slFlags;                      /**< The list holding the current flags. */

    bool                                                m_bScaling;                     /**< Flag for displaying the scaling group box. */
    bool                                                m_bProjections;                 /**< Flag for displaying the projection group box. */
    bool                                                m_bSphara;                      /**< Flag for displaying teh SPHARA group box. */
    bool                                                m_bView;                        /**< Flag for displaying the view group box. */
    bool                                                m_bFilter;                      /**< Flag for displaying the filter group box. */
    bool                                                m_bModalitiy;                   /**< Flag for displaying the modality group box. */
    bool                                                m_bCompensator;                 /**< Flag for displaying the compensator group box. */
    bool                                                m_bTriggerDetection;            /**< Flag for displaying the trigger detection tab in the view group box. */
    bool                                                m_bAverages;                    /**< Flag for displaying the averages group box. */

    QMap<qint32,float>                                  m_qMapChScaling;                /**< Channel scaling values. */
    QMap<qint32, QDoubleSpinBox*>                       m_qMapScalingDoubleSpinBox;     /**< Map of types and channel scaling line edits. */
    QMap<qint32, QSlider*>                              m_qMapScalingSlider;            /**< Map of types and channel scaling line edits. */
    QMap<double, QColor>                                m_qMapTriggerColor;             /**< Trigger colors per detected type. */
    QMap<double, QPair<QColor, QPair<QString,bool> > >  m_qMapAverageInfo;              /**< Average colors and names. */
    QMap<double, QPair<QColor, QPair<QString,bool> > >  m_qMapAverageInfoOld;           /**< Old average colors and names. */
    QMap<QCheckBox*, double>                            m_qMapChkBoxAverageType;        /**< Check box to average type map. */
    QMap<QPushButton*, double>                          m_qMapButtonAverageType;        /**< Push button to average type map. */

    QColor                                              m_colCurrentSignalColor;        /**< Current color of the scene in all View3D's. */
    QColor                                              m_colCurrentBackgroundColor;    /**< Current color of the scene in all View3D's. */

    QList<DISPLIB::Modality>                            m_qListModalities;              /**< List of different modalities. */
    QList<QCheckBox*>                                   m_qListProjCheckBox;            /**< List of projection CheckBox. */
    QList<QCheckBox*>                                   m_qListCompCheckBox;            /**< List of compensator CheckBox. */
    QList<QCheckBox*>                                   m_qFilterListCheckBox;          /**< List of filter CheckBox. */
    QList<QCheckBox*>                                   m_qListModalityCheckBox;        /**< List of modality checkboxes. */
    QSharedPointer<FIFFLIB::FiffInfo>                   m_pFiffInfo;                    /**< Connected fiff info. */

    QString                                             m_sName;                        /**< Name of the widget which uses this quick control. */
    QCheckBox*                                          m_pEnableDisableProjectors;     /**< Holds the enable disable all check box. */
    QPushButton*                                        m_pShowFilterOptions;           /**< Holds the show filter options button. */

    QSignalMapper*                                      m_pCompSignalMapper;            /**< The signal mapper. */

    Ui::QuickControlWidget*                             ui;                             /**< The generated UI file. */

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the scaling sliders or spin boxes changed.
    */
    void scalingChanged(const QMap<qint32, float>& scalingMap);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user changes the projections.
    */
    void projSelectionChanged();

    //=========================================================================================================
    /**
    * Emit this signal whenever the user changes the compensator.
    */
    void compSelectionChanged(int to);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user toggled the SPHARA operator.
    */
    void spharaActivationChanged(bool state);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user changes the SPHARA operator.
    */
    void spharaOptionsChanged(const QString& sSytemType, int nBaseFctsFirst, int nBaseFctsSecond);

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
    void triggerInfoChanged(const QMap<double, QColor>& value, bool active, const QString& triggerCh, double threshold);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user is supposed to see the filter option window.
    */
    void showFilterOptions(bool state);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user changed the modality.
    */
    void settingsChanged(const QList<DISPLIB::Modality>& modalityList);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user changed the time spacer distance.
    */
    void distanceTimeSpacerChanged(int value);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user pressed the trigger counter.
    */
    void resetTriggerCounter();

    //=========================================================================================================
    /**
    * Emit this signal whenever you want to cople this control widget to updating a view for which it is providing control.
    */
    void updateConnectedView();

    //=========================================================================================================
    /**
    * Signal mapper signal for compensator changes.
    */
    void compClicked(const QString& text);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user changed the signal color.
    */
    void signalColorChanged(const QColor& signalColor);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user changed the background color.
    */
    void backgroundColorChanged(const QColor& backgroundColor);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user wants to make a screenshot.
    *
    * @param[out] imageType     The current image type: png, svg.
    */
    void makeScreenshot(const QString& imageType);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user wants to make a screenshot.
    *
    * @param[out] map     The current average map.
    */
    void averageInformationChanged(const QMap<double, QPair<QColor, QPair<QString,bool> > >& map);
};

} // NAMESPACE SCDISPLIB

#endif // QUICKCONTROLWIDGET_H
