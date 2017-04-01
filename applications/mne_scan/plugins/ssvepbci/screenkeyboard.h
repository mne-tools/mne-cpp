//=============================================================================================================
/**
* @file     screenkeyboard.h
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Viktor Klüber Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the ssvepscreenkeyboard class.
*
*/

#ifndef SCREENKEYBOARD_H
#define SCREENKEYBOARD_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <ssvepbci_global.h>
#include "ssvepbciscreen.h"
#include "FormFiles/ssvepbcisetupstimuluswidget.h"
#include "ssvepbci.h"


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


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class SsvepBci;
class SsvepBciSetupStimulusWidget;
class SsvepBciScreen;


//=============================================================================================================
/**
* DECLARE CLASS ScreenKeyboard
*
* @brief The ScreenKeyboard class provides the screen keyboard device for the ssvep BCI plug-in.
*/
class SSVEPBCISHARED_EXPORT ScreenKeyboard : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructrs the ScreenKeyboard class.
    */
    ScreenKeyboard(SsvepBci *pSsvepBci,
                   SsvepBciSetupStimulusWidget *pSsvepBciSetupStimulusWidget,
                   SsvepBciScreen *pSsvepBciScreen);

    //=========================================================================================================
    /**
    * Destroys the ScreenKeyboard class.
    */
    ~ScreenKeyboard();

    //=========================================================================================================
    /**
    * Painting keyboard update to the screen
    *
    * @param[in]    paint device which links to to the corresponding widget.
    */
    void paint(QPaintDevice *device);

signals:
    //=========================================================================================================
    /**
    * emits the spelled letter
    */
    void getLetter(QString letter);

    //=========================================================================================================
    /**
    * emits a flag if the letter was spelled correctly or not.
    *
    * @param [out]  correctCommand      Flag for correct or wrong spelled letter
    */
    void isCorrectCommand(bool correctCommand);

    //=========================================================================================================
    /**
    * emits trigger signal for a finished spelling sequence
    */
    void spellingFinished();

public slots:
    //=========================================================================================================
    /**
    * slot when frequency list is changed.
    *
    * @param [in]  classList      List of new frequencies
    */
    void updateClassList(MyQList classList);

    //=========================================================================================================
    /**
    * managing and interpreting a new detected frequency from the SsvepBci class
    *
    * @param [in]  value      detected frequency. Is supposed to be one of the values of the classification list
    */
    void updateCommand(double value);

    //=========================================================================================================
    /**
    * sets a new phrase which has to be spelt
    *
    * @param [out]  phrase      newly adjusted phrase
    */
    void setPhrase(QString phrase);

    //=========================================================================================================
    /**
    * initializes the screen keyboard
    */
    void initScreenKeyboard();

    //=========================================================================================================
    /**
    * initializes the accuracy feature of the screen keyboard device
    */
    void initSpellAccuracyFeature();

    //=========================================================================================================
    /**
    * stops the accuracy feature
    */
    void stopSpellAccuracyFeature();

private:
    //=========================================================================================================
    /**
    * process the spelled letter and emit the classifiaction signal
    *
    * @param [in] letter    sign which will be classified
    *
    */
    void spellLetter(QString letter);

    // Pointer to other classes
    SsvepBci*                               m_pSsvepBci;                        /**< pointer to the SsvepBci class */
    SsvepBciSetupStimulusWidget*            m_pSsvepBciSetupStimulusWidget;     /**< pointer to SsvepBciSetupStimulusWidget class */
    SsvepBciScreen*                         m_pSsvepBciScreen;                  /**< holds the pointer to the SsvepBciScreen class */

    // displaying
    QPainter                                m_qPainter;             /**< Painter, holding paint device of SsvepBciScreen class */
    QMap<QPair<int, int>, QString>          m_mapKeys;              /**< QMap, holding the key-values and according coordinates */
    QPair<int, int>                         m_qCurCursorCoord;      /**< current cursor coordinates */
    QPair<int, int>                         m_qOldCursorCoord;      /**< old cursor coordinates */
    QList<double>                           m_lClassList;           /**< list containing all displayed frequencies [Hz] */
    bool                                    m_bInitializeKeyboard;  /**< initalizes the painting of the screen keyboard */
    bool                                    m_bUpdatePhraseDisplay; /**< updates the phrases's display */
    // speller
    QPair<int, int>                         m_qNextCoord;           /**< coordinate of the next sign which has to be picked */
    bool                                    m_bDisplaySpeller;      /**< flag for displaying speller panel */
    bool                                    m_bUseSpellAccuracy;    /**< flag for determine the spell accuracy */
    QString                                 m_sSettledPhrase;       /**< phrase established by the UI */
    QString                                 m_sSpelledPhrase;       /**< phrase spelled by the user with the SsvepBci */
    QString::Iterator                       m_qSpellIterator;       /**< iterator for spelling */
};

} // namespace

#endif // SCREENKEYBOARD_H
