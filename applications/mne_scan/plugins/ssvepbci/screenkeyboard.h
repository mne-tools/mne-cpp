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
* Copyright (C) 2016, Viktor Klüber Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
#include "FormFiles/ssvepbcisetupStimuluswidget.h"
#include "ssvepbci.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMediaPlayer>


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
    ScreenKeyboard(QSharedPointer<SsvepBci> pSsvepBci, QSharedPointer<SsvepBciSetupStimulusWidget> pSsvepBciSetupStimulusWidget, QSharedPointer<SsvepBciScreen> pSsvepBciScreen);

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
    void getLetter(QString letter);
    void isCorrectCommand(bool correctCommand);
    void spellingFinished();

public slots:
    void updateClassList(MyQList classList);
    void updateCommand(double value);
    void setPhrase(QString phrase);
    void initScreenKeyboard();
    void initSpellAccuracyFeature();
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
    QSharedPointer<SsvepBci>                        m_pSsvepBci;                        /**< pointer to the SsvepBci class */
    QSharedPointer<SsvepBciSetupStimulusWidget>     m_pSsvepBciSetupStimulusWidget;     /**< pointer to SsvepBciSetupStimulusWidget class */
    QSharedPointer<SsvepBciScreen>                  m_pSsvepBciScreen;                  /**< holds the pointer to the SsvepBciScreen class */

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
    QMediaPlayer                           *m_qSound;               /**< sound-object for emiting audio feedback */
};

} // namespace

#endif // SCREENKEYBOARD_H
