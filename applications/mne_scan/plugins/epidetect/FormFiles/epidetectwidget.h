//=============================================================================================================
/**
 * @file     epidetectwidget.h
 * @author   Louis Eichhorst <Louis.Eichhorst@tu-ilmenau.de>
 * @version  dev
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Louis Eichhorst. All rights reserved.
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
 * @brief    Contains the declaration of the EpidetectWidget class.
 *
 */

#ifndef EPIDETECTWIDGET_H
#define EPIDETECTWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_epidetecttoolbarwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE EPIDETECTPLUGIN
//=============================================================================================================

namespace EPIDETECTPLUGIN
{


//=============================================================================================================
/**
 * DECLARE CLASS EpidetectWidget
 *
 * @brief The EpidetectToolbox class provides a Epidetect toolbar widget structure.
 */
}
class EpidetectWidget : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<EpidetectWidget> SPtr;         /**< Shared pointer type for EpidetectWidget. */
    typedef QSharedPointer<EpidetectWidget> ConstSPtr;    /**< Const shared pointer type for EpidetectWidget. */

    //=========================================================================================================
    /**
     * Constructs a EpidetectToolbox.
     */
    explicit EpidetectWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the EpidetectToolbox.
     */
    ~EpidetectWidget();
    EpidetectWidget(int dimOld, double rOld, int nOld, double marginOld, double threshold1Old, double threshold2Old, int listLengthOld, int fuzzyStepOld, int chWheightOld, QWidget *parent = 0);

    double      m_dRVal;                  /**< Holds the r value inside the GUI. */
    double      m_dMarginVal;             /**< Holds the margin value inside the GUI. */
    double      m_dThreshold1Val;         /**< Holds the threshold1 value inside the GUI. */
    double      m_dThreshold2Val;         /**< Holds the threshold2 value inside the GUI. */

    int         m_iDimVal;                /**< Holds the dim value inside the GUI. */
    int         m_iNVal;                  /**< Holds the n value inside the GUI. */
    int         m_iListLengthVal;         /**< Holds the history length value inside the GUI. */
    int         m_iFuzzyEnStepVal;        /**< Holds the FuzzyEnStep value inside the GUI. */
    int         m_iChWeight;              /**< Holds the channel weight value inside the GUI. */


private:
    Ui::EpidetectToolbarWidget* ui;        /**< The UI class specified in the designer. */

    //=========================================================================================================
    /**
     * Updates the parameters if changed in the GUI
     */
    void updateValues();

signals:
    void newValues();                       /**< Emmited if values are changed in the GUI. */
};

#endif // EPIDETECTWIDGET_H
