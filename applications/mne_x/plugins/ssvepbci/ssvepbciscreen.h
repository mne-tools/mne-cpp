//=============================================================================================================
/**
* @file     ssvepBCIScreen.h
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May 2016
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
* @brief    Contains the declaration of the ssvepBCIScreen class.
*
*/

#ifndef SSVEPBCISCREEN_H
#define SSVEPBCISCREEN_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ssvepbciflickeringitem.h"
#include "ssvepbci.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QOpenGLWidget>
#include <QSound>

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

//=============================================================================================================
/**
* DECLARE CLASS ssvepBCIScreen
*
* @brief The ssvepBCIScreen class provides the subject screen. Contains the list of items which will be painted
* to screen automatically
*/
class ssvepBCIScreen : public QOpenGLWidget
{
    Q_OBJECT

    friend class ssvepBCISetupStimulusWidget;           /**< permit ssvepBCISetupStimulusWidget getting full access */

public:
    //=========================================================================================================
    /**
     * constructs the ssvepBCIScreen object
     *
     */
    ssvepBCIScreen(QSharedPointer<ssvepBCI> pSSVEPBCI, QOpenGLWidget *parent = 0 );

public slots:
    void setClassResults(double classResult);
    void updateFrequencyList(MyQList freqList);

private:
    QSharedPointer<ssvepBCI>        m_pSSVEPBCI;        /**< pointer to referring SSVEPBCI class */

    // flickering items
    QList<ssvepBCIFlickeringItem>   m_Items;            /**< QList containing all flickering Items to be painted */

    // classifiaction updates
    double                          m_dXPosCross;       /**< X position of reference cross */
    double                          m_dYPosCross;       /**< Y position of reference cross */
    double                          m_dStep;            /**< moving step increment for reference cross */
    QList<double>                   m_lFreqList;        /**< list of current flickering frequencies */
    QColor                          m_qCrossColor;      /**< color of the reference cross */
    QSound                          m_sBeep;            /**< beep sound for successful classifiaction */


protected:
    //=========================================================================================================
    /**
     * overwritten functions from the QOpenGLWidget class which will be called automatically
     *
     */
    void resizeGL(int w, int h);
    void paintGL();
    void initializeGL();
};


}//Namescpace
#endif // SSVEPBCISCREEN_H
