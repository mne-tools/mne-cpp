//=============================================================================================================
/**
* @file     dummytoolbox.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the NoiseReductionOptionsWidget class.
*
*/

#ifndef NOISEREDUCTIONOPTIONSWIDGET_H
#define NOISEREDUCTIONOPTIONSWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_noisereductionoptionswidget.h"
#include "fiff/fiff_info.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QSignalMapper>
#include <QPushButton>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NOISEREDUCTIONPLUGIN
//=============================================================================================================

namespace NOISEREDUCTIONPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class NoiseReduction;


//=============================================================================================================
/**
* DECLARE CLASS NoiseReductionOptionsWidget
*
* @brief The NoiseReductionOptionsWidget class provides a NoiseReduction option toolbar widget structure.
*/
}
class NoiseReductionOptionsWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<NoiseReductionOptionsWidget> SPtr;         /**< Shared pointer type for NoiseReductionOptionsWidget. */
    typedef QSharedPointer<NoiseReductionOptionsWidget> ConstSPtr;    /**< Const shared pointer type for NoiseReductionOptionsWidget. */

    //=========================================================================================================
    /**
    * Constructs a DummyToolbox.
    */
    explicit NoiseReductionOptionsWidget(NOISEREDUCTIONPLUGIN::NoiseReduction* toolbox, QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the DummyToolbox.
    */
    ~NoiseReductionOptionsWidget();

    //=========================================================================================================
    /**
    * Set the fiff info.
    *
    * @param[in] pFiffInfo    The new FiffInfo.
    */
    void setFiffInfo(const FIFFLIB::FiffInfo::SPtr pFiffInfo);

    //=========================================================================================================
    /**
    * Set the acquisition system type (BabyMEG, VecotrView, EEG).
    *
    * @param[in] sSystem    The type of the acquisition system.
    */
    void setAcquisitionSystem(const QString &sSystem);

    //=========================================================================================================
    /**
    * Call this whenever the current filters have changed.
    *
    * @param [in] list    list of QCheckBoxes which are to be added to the filter group
    */
    void filterGroupChanged(QList<QCheckBox*> list);

protected slots:
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
    * Call this slot whenever the number basis functions changed.
    */
    void onNBaseFctsChanged();

    //=========================================================================================================
    /**
    * Show the filter option screen to the user.
    *
    * @param [in] state toggle state.
    */
    void onShowFilterOptions(bool state);

    //=========================================================================================================
    /**
    * Slot called when the user designed filter was toggled
    */
    void onUserFilterToggled(bool state);

protected:
    //=========================================================================================================
    /**
    * Create the widgets used in the projector group
    */
    void createProjectorGroup();    

    //=========================================================================================================
    /**
    * Create the widgets used in the compensator group
    */
    void createCompensatorGroup();

private:
    Ui::NoiseReductionOptionsWidgetClass*   ui;                             /**< The UI class specified in the designer. */

    NOISEREDUCTIONPLUGIN::NoiseReduction*   m_pNoiseReductionToolbox;

    FIFFLIB::FiffInfo::SPtr                 m_pFiffInfo;                    /**< Connected fiff info. */

    QList<QCheckBox*>                       m_qListProjCheckBox;            /**< List of projection CheckBox. */
    QList<QCheckBox*>                       m_qListCompCheckBox;            /**< List of compensator CheckBox. */
    QList<QCheckBox*>                       m_qFilterListCheckBox;          /**< List of filter CheckBox. */

    QCheckBox *                             m_enableDisableProjectors;      /**< Holds the enable disable all check box. */
    QPushButton*                            m_pShowFilterOptions;           /**< Holds the show filter options button. */

    QSignalMapper*                          m_pCompSignalMapper;            /**< The signal mapper. */

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever the user changes the projections.
    */
    void projSelectionChanged();

    //=========================================================================================================
    /**
    * Signal mapper signal for compensator changes.
    */
    void compClicked(const QString& text);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user changes the compensator.
    */
    void compSelectionChanged(int to);

    //=========================================================================================================
    /**
    * Emit this signal whenever the user is supposed to see the filter option window.
    */
    void showFilterOptions(bool state);

    //=========================================================================================================
    /**
    * Emit this signal whenever you want to cople this control widget to updating a view for which it is providing control.
    */
    void updateConnectedView();
};

#endif // NOISEREDUCTIONOPTIONSWIDGET_H
