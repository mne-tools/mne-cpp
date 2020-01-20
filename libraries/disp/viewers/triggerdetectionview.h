//=============================================================================================================
/**
 * @file     triggerdetectionview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the TriggerDetectionView Class.
 *
 */

#ifndef TRIGGERDETECTIONVIEW_H
#define TRIGGERDETECTIONVIEW_H


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
#include <QMap>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class TriggerDetectionViewWidget;
}

namespace FIFFLIB {
    class FiffInfo;
}


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
 * DECLARE CLASS TriggerDetectionView
 *
 * @brief The TriggerDetectionView class provides a view to control the trigger detection
 */
class DISPSHARED_EXPORT TriggerDetectionView : public QWidget
{
    Q_OBJECT

public:    
    typedef QSharedPointer<TriggerDetectionView> SPtr;              /**< Shared pointer type for TriggerDetectionView. */
    typedef QSharedPointer<const TriggerDetectionView> ConstSPtr;   /**< Const shared pointer type for TriggerDetectionView. */

    //=========================================================================================================
    /**
    * Constructs a TriggerDetectionView which is a child of parent.
    *
    * @param [in] parent        parent of widget
    */
    TriggerDetectionView(const QString& sSettingsPath = "",
                         QWidget *parent = 0,
                         Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
    * Destroys the TriggerDetectionView.
    */
    ~TriggerDetectionView();

    //=========================================================================================================
    /**
    * Init the view.
    */
    void init(const QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo);

    //=========================================================================================================
    /**
    * Set total number of detected triggers and trigger types.
    *
    * @param [in] totalNumberDetections     The numger of detected triggers
    * @param [in] mapDetectedTriggers       The currently detected triggers
    */
    void setNumberDetectedTriggersAndTypes(int totalNumberDetections, const QMap<int,QList<QPair<int,double> > >& mapDetectedTriggers);

protected:
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
    * Slot called when trigger info changed
    */
    void onTriggerInfoChanged();

    //=========================================================================================================
    /**
    * Slot called when trigger detection color button was clicked
    */
    void onRealTimeTriggerColorChanged(bool state);

    //=========================================================================================================
    /**
    * Slot called when trigger type changed
    */
    void onRealTimeTriggerColorTypeChanged(const QString& value);

    //=========================================================================================================
    /**
    * Slot called when reset number of detected triggers was pressed
    */
    void onResetTriggerNumbers();

    Ui::TriggerDetectionViewWidget* ui;

    QSharedPointer<FIFFLIB::FiffInfo>                   m_pFiffInfo;                    /**< Connected fiff info. */

    QMap<double, QColor>                                m_qMapTriggerColor;             /**< Trigger colors per detected type. */

    QString                                             m_sSettingsPath;                /**< The settings path to store the GUI settings to. */

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the user pressed the trigger counter.
    */
    void resetTriggerCounter();

    //=========================================================================================================
    /**
    * Emit this signal whenever the trigger infomration changed.
    */
    void triggerInfoChanged(const QMap<double, QColor>& value, bool active, const QString& triggerCh, double threshold);

};

} // NAMESPACE

#endif // TRIGGERDETECTIONVIEW_H
