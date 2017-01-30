//=============================================================================================================
/**
* @file     ssvepBCIScreen.h
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Viktor Klüber, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the ssvepBCIScreen class.
*
*/

#ifndef SSVEPBCISCREEN_H
#define SSVEPBCISCREEN_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <ssvepbci_global.h>
#include "FormFiles/ssvepbcisetupstimuluswidget.h"
#include "ssvepbciflickeringitem.h"
#include "screenkeyboard.h"
#include "ssvepbci.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QOpenGLWidget>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SSVEPBCIPLUGIN
//=============================================================================================================

namespace SSVEPBCIPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// TypeDefs
//=============================================================================================================

typedef  QList<double>  MyQList;


//=============================================================================================================
/**
* DECLARE CLASS SsvepBciScreen
*
* @brief The SsvepBciScreen class provides the subject screen. Contains the list of items which will be painted
* to screen automatically
*/
class SSVEPBCISHARED_EXPORT SsvepBciScreen : public QOpenGLWidget
{
    Q_OBJECT

    friend class SsvepBciSetupStimulusWidget;           /**< permit SsvepBciSetupStimulusWidget getting full access */

public:
    //=========================================================================================================
    /**
     * constructs the SsvepBciScreen object
     *
     */
    SsvepBciScreen(QSharedPointer<SsvepBci> pSsvepBci, QSharedPointer<SsvepBciSetupStimulusWidget> pSsvepBciSetupStimulusWidget, QOpenGLWidget *parent = 0 );

    //=========================================================================================================
    /**
     * deconstructs the SsvepBciScreen object
     *
     */
    ~SsvepBciScreen();

public slots:
    //=========================================================================================================
    /**
     * slot for setting the classification result
     *
     * @param [in]  classResult     detected frequency
     */
    void setClassResults(double classResult);

    //=========================================================================================================
    /**
     * adjusting the list of detectable frequencies
     *
     * @param [in]  freqList     list of detectable frequencies
     */
    void updateFrequencyList(MyQList freqList);

    //=========================================================================================================
    /**
     * slot for using the screenkeyboard
     *
     * @param [in]  useKeyboard     useKeyboard
     */
    void useScreenKeyboard(bool useKeyboard);

    //=========================================================================================================
    /**
     * clears the screenkeyboard from the screen
     */
    void clearScreen();

private:
    QSharedPointer<SsvepBci>                        m_pSsvepBci;                        /**< pointer to referring SsvepBci class */
    QSharedPointer<SsvepBciSetupStimulusWidget>     m_pSsvepBciSetupStimulusWidget;     /**< pointer to referring SsvepBciSetupStimulusWidget class */
    // draw items
    QList<SsvepBciFlickeringItem>   m_Items;            /**< QList containing all flickering Items to be painted */
    QSharedPointer<ScreenKeyboard>  m_pScreenKeyboard;  /**< pointer that holds the Screen-keyboard */
    // classifiaction updates
    double                          m_dXPosCross;       /**< X position of reference cross */
    double                          m_dYPosCross;       /**< Y position of reference cross */
    double                          m_dStep;            /**< moving step increment for reference cross */
    QList<double>                   m_lFreqList;        /**< list of current flickering frequencies */
    QColor                          m_qCrossColor;      /**< color of the reference cross */
    QPainter                        m_qPainter;         /**< painter for drawing items to the widget scene */

    bool                            m_bUseScreenKeyboard;    /**< flag for updating screen keayboard */
    bool                            m_bClearScreen;          /**< flag for clearing swap buffer */

protected:
    void resizeGL(int w, int h);
    void paintGL();
    void initializeGL();
};

}//Namescpace

#endif // SSVEPBCISCREEN_H
