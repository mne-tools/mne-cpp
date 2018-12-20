//=============================================================================================================
/**
* @file     scalingview.h
* @author   Lorenz Esch <lesch@mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the ScalingView Class.
*
*/

#ifndef SCALINGVIEW_H
#define SCALINGVIEW_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

#include <fiff/fiff_ch_info.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QMap>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QDoubleSpinBox;
class QSlider;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


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
* DECLARE CLASS ScalingView
*
* @brief The ScalingView class provides a view to select the scaling for different channels modalities
*/
class DISPSHARED_EXPORT ScalingView : public QWidget
{
    Q_OBJECT

public:    
    typedef QSharedPointer<ScalingView> SPtr;              /**< Shared pointer type for ScalingView. */
    typedef QSharedPointer<const ScalingView> ConstSPtr;   /**< Const shared pointer type for ScalingView. */

    //=========================================================================================================
    /**
    * Constructs a ScalingView which is a child of parent.
    *
    * @param [in] parent        parent of widget
    */
    ScalingView(const QString& sSettingsPath = "",
                const QList<FIFFLIB::FiffChInfo> &lChannelList = QList<FIFFLIB::FiffChInfo>(),
                QWidget *parent = 0,
                Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
    * Destroys the ScalingView.
    */
    ~ScalingView();

    //=========================================================================================================
    /**
    * Get the current scaling map
    *
    * @return The current scaling map.
    */
    QMap<qint32,float> getScaleMap() const;

    //=========================================================================================================
    /**
    * Set the current scaling map. This also recreates the GUI.
    */
    void setScaleMap(const QMap<qint32, float> &qMapChScaling);

protected:
    //=========================================================================================================
    /**
    * Redraw the GUI.
    */
    void redrawGUI();

    //=========================================================================================================
    /**
    * Saves all important settings of this view via QSettings.
    *
    * @param[in] settingsPath        the path to store the settings to.
    */
    void saveSettings(const QString& settingsPath);

    //=========================================================================================================
    /**
    * Loads and inits all important settings of this view via QSettings.
    *
    * @param[in] settingsPath        the path to load the settings from.
    */
    void loadSettings(const QString& settingsPath);

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

    QMap<qint32, float>                 m_qMapChScaling;                /**< Channel scaling values. */
    QMap<qint32, QDoubleSpinBox*>       m_qMapScalingDoubleSpinBox;     /**< Map of types and channel scaling line edits. */
    QMap<qint32, QSlider*>              m_qMapScalingSlider;            /**< Map of types and channel scaling line edits. */

    QStringList                         m_lChannelTypeList;             /**< Channel type list. */

    QString                             m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the scaling sliders or spin boxes changed.
    */
    void scalingChanged(const QMap<qint32, float>& scalingMap);

};

} // NAMESPACE

#endif // SCALINGVIEW_H
