//=============================================================================================================
/**
* @file     screenkeyboard.h
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July 2016
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

#include "ssvepbciscreen.h"
#include "FormFiles/ssvepbcisetupStimuluswidget.h"
#include "ssvepbci.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ssvepBCIScreen
//=============================================================================================================

namespace ssvepBCIPlugin
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

class ssvepBCI;
class ssvepBCISetupStimulusWidget;
class ssvepBCIScreen;

//=============================================================================================================





class ScreenKeyboard : public QObject
{
    Q_OBJECT

public:
    ScreenKeyboard(QSharedPointer<ssvepBCI> pSSVEPBCI, QSharedPointer<ssvepBCISetupStimulusWidget> pSSVEPBCISetupStimulusWidget, QSharedPointer<ssvepBCIScreen> pSSVEPBCIScreen);

    ~ScreenKeyboard();

    //=========================================================================================================
    /**
    * painting keyboard update to the screen
    *
    */
    void paint(QPaintDevice *device);

signals:
    void getLetter(QString letter);
    void isCorrectCommand(bool correctCommand);

public slots:
    void updateClassList(MyQList classList);
    void updateCommand(double value);
    void setPhrase(QString phrase);
    void initScreenKeyboard();
    void initSpellAccuracyFeature();
    void stopSpellAccuracyFeature();

private:
    QSharedPointer<ssvepBCI>                        m_pSSVEPBCI;                        /**< pointer to the ssvepBCI class */
    QSharedPointer<ssvepBCISetupStimulusWidget>     m_pSSVEPBCISetupStimulusWidget;     /**< pointer to ssvepBCISetupStimulusWidget class */
    QSharedPointer<ssvepBCIScreen>                  m_pSSVEPBCIScreen;                  /**< holds the pointer to the ssvepBCIScreen class */

    // displaying
    QPainter                                m_qPainter;             /**< Painter, holding paint device of ssvepBCIScreen class */
    QMap<QPair<int, int>, QString>          m_mapKeys;              /**< QMap, holding the key-values and according coordinates */
    QPair<int, int>                         m_qCurCursorCoord;      /**< current cursor coordinates */
    QPair<int, int>                         m_qOldCursorCoord;      /**< old cursor coordinates */
    QList<double>                           m_lClassList;           /**< list containing all displayed frequencies [Hz] */
    bool                                    m_bInitializeKeyboard;  /**< initalizes the painting of the screen keyboard */
    bool                                    m_bUpdatePhraseDisplay; /**< updates the phrases's display */

    // speller
    bool                                    m_bDisplaySpeller;      /**< flag for displaying speller panel */
    bool                                    m_bUseSpellAccuracy;    /**< flag for determine the spell accuracy */
    QString                                 m_sSettledPhrase;       /**< phrase established by the UI */
    QString                                 m_sSpelledPhrase;       /**< phrase spelled by the user with the SSVEPBCI */
    QString::Iterator                       m_qSpellIterator;       /**< iterator for spelling */

    //=========================================================================================================
    /**
    * process the spelled letter and emit the classifiaction signal
    *
    * @param [in] letter    sign which will be classified
    *
    */
    void spellLetter(QString letter);

};

} // namespace

#endif // SCREENKEYBOARD_H
