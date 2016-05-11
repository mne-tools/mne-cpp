/**
* @file     flashobject.h
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     April, 2016
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
* @brief    Contains the declaration of the FlashObject class.
*
*/

#ifndef FLASHOBJECT_H
#define FLASHOBJECT_H


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore>
#include <QObject>
#include <QGraphicsObject>
#include <QTime>
#include <QPalette>
#include <QPaintEvent>

//=============================================================================================================
/**
* FlashObject class creates a QGaphicsItem with an adjustable blinking frequency.
*
*/
class FlashObject : public QGraphicsObject
{
    Q_OBJECT
public:
    //=============================================================================================================
    /**
    * creates an object with the properties of FlashObject class.
    *
    */
    explicit FlashObject(QGraphicsItem *parent = 0);

    //=============================================================================================================
    /**
    * @return bounding Rect of the item
    *
    */
    QRectF boundingRect() const;

    //=============================================================================================================
    /**
    * paints a rectangle with the given dimensons of setDim(int x, int y) automatically, when updata() is called
    *
    * @param[in]    *painter    pointer to QPainter class
    * @param[in]    *option     unused pointer
    * @param[in]    *widget     unused pointer
    *
    */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    //=============================================================================================================
    /**
    * setting the frequeny of the blinking object
    *
    * @param[in]    *freq       frequency in [Hz]
    *
    */
    void    setFreq(double freq);

    //=============================================================================================================
    /**
    * setting the position of the blinking object in a QGraphicView
    *
    * @param[in]    x       horizontal position in pixel
    * @param[in]    y       vertical position in pixel
    *
    */
    void    setPos(int x, int y);

    //=============================================================================================================
    /**
    * setting the dimensions of the blinking object
    *
    * @param[in]    x       horizontal dimension in pixel
    * @param[in]    y       vertical dimension in pixel
    *
    */
    void    setDim(int x, int y);

    //=============================================================================================================
    /**
    * getting the Frequency of the blanking object
    *
    * @return   double value of the adjusted frequency in [Hz]
    *
    */
    double getFreq();

private:
    int         m_iT;               /**< cylcle duration >**/
    double      m_dFreq;            /**< adjusted Frequency >**/
    bool        m_bFlashState;      /**< state of Flashing >**/
    bool        m_bDraw;            /**< state for drawing the object >**/
    QTimer     *m_pTimer;           /**< timer, controlling the flash period >**/
    int         m_iPosX;            /**< x-position of the item >**/
    int         m_iPosY;            /**< y-position of the item >**/
    int         m_iDimX;            /**< x dimension of figure >**/
    int         m_iDimY;            /**< y-dimension of figure >**/
    //FPS
    QTime       m_t0;               /**< Time of last frame >**/
    QTime       m_t1;               /**< Time of current frame >**/
    int         m_iDelta;           /**< time difference between two frames >**/
    int         m_iFPS;             /**< FPS number >**/




private slots:
    //=============================================================================================================
    /**
    * slot, which will be called by a object internal QTimer
    *
    */
    void flash();

signals:
    void adjDelta(int newAdjDelta);

    void currDelta(int newCurDelta);

    void trigger(bool changedState);

};

#endif // FLASHOBJECT_H
