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
* @brief    Declaration of the QuickControlView Class.
*
*/

#ifndef QUICKCONTROLVIEW_H
#define QUICKCONTROLVIEW_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

#include "helpers/draggableframelesswidget.h"
#include "butterflyview.h"


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

namespace Ui {
    class QuickControlViewWidget;
}

class QCheckBox;
class QPushButton;


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
* DECLARE CLASS QuickControlView
*
* @brief The QuickControlView class provides a quick control view for scaling, filtering, projector and other control options.
*/
class DISPSHARED_EXPORT QuickControlView : public DISPLIB::DraggableFramelessWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<QuickControlView> SPtr;              /**< Shared pointer type for QuickControlView. */
    typedef QSharedPointer<const QuickControlView> ConstSPtr;   /**< Const shared pointer type for QuickControlView. */

    //=========================================================================================================
    /**
    * Constructs a QuickControlView which is a child of parent.
    *
    * @param [in] name              The name to be displayed on the minimize button.
    * @param [in] parent            The parent of widget.
    */
    QuickControlView(const QString& name = "",
                       QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
    * Destructs a QuickControlView
    */
    ~QuickControlView();

    void addGroupBox(QWidget* pWidget,
                        QString sGroupBoxName);

    void addGroupBoxWithTabs(QWidget* pWidget,
                                QString sGroupBoxName,
                                QString sTabName);

    //=========================================================================================================
    /**
    * Sets the values of the opacity slider
    *
    * @param [in] opactiy       the new opacity value
    */
    void setOpacityValue(int opactiy);

    //=========================================================================================================
    /**
    * Get current opacity value.
    *
    * @return thecurrent set opacity value of this window.
    */
    int getOpacityValue();

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

protected:
    //=========================================================================================================
    /**
    * Slot called when opacity slider was changed
    *
    * @param [in] value opacity value.
    */
    void onOpacityChange(qint32 value);

    //=========================================================================================================
    /**
    * Is called when the minimize or maximize button was pressed.
    *
    * @param [in] state toggle state.
    */
    void onToggleHideAll(bool state);

    //=========================================================================================================
    /**
    * Slot called when modality check boxes were changed
    */
    void onUpdateModalityCheckbox(qint32 state);

    //=========================================================================================================
    /**
    * Call this slot whenever the averages changed.
    */
    void onAveragesChanged();

protected:
    //=========================================================================================================
    /**
    * Create the widgets used in the modality group
    */
    void createModalityGroup();

    //=========================================================================================================
    /**
    * Create the widgets used in the averages group
    */
    void createAveragesGroup();

private:    
    bool                                                m_bModalitiy;                   /**< Flag for displaying the modality group box. */
    bool                                                m_bCompensator;                 /**< Flag for displaying the compensator group box. */
    bool                                                m_bAverages;                    /**< Flag for displaying the averages group box. */

    QMap<double, QPair<QColor, QPair<QString,bool> > >  m_qMapAverageInfo;              /**< Average colors and names. */
    QMap<double, QPair<QColor, QPair<QString,bool> > >  m_qMapAverageInfoOld;           /**< Old average colors and names. */
    QMap<QCheckBox*, double>                            m_qMapChkBoxAverageType;        /**< Check box to average type map. */
    QMap<QPushButton*, double>                          m_qMapButtonAverageType;        /**< Push button to average type map. */

    QList<DISPLIB::Modality>                            m_qListModalities;              /**< List of different modalities. */
    QList<QCheckBox*>                                   m_qListModalityCheckBox;        /**< List of modality checkboxes. */

    QString                                             m_sName;                        /**< Name of the widget which uses this quick control. */
    Ui::QuickControlViewWidget*                         ui;                             /**< The generated UI file. */

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the user changed the modality.
    */
    void modalitiesChanged(const QList<DISPLIB::Modality>& modalityList);

    //=========================================================================================================
    /**
    * Emit this signal whenever you want to cople this control widget to updating a view for which it is providing control.
    */
    void updateConnectedView();

    //=========================================================================================================
    /**
    * Emit this signal whenever the user wants to make a screenshot.
    *
    * @param[out] map     The current average map.
    */
    void averageInformationChanged(const QMap<double, QPair<QColor, QPair<QString,bool> > >& map);
};

} // NAMESPACE DISPLIB

#endif // QUICKCONTROLVIEW_H
