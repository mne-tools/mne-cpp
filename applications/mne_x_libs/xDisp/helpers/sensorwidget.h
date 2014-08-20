//=============================================================================================================
/**
* @file     sensorwidget.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the SensorWidget Class.
*
*/

#ifndef SENSORWIDGET_H
#define SENSORWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensormodel.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{


//=============================================================================================================
/**
* DECLARE CLASS SensorWidget
*
* @brief The SensorWidget class provides the sensor selection widget
*/
class SensorWidget : public QWidget
{
    Q_OBJECT
public:

    //=========================================================================================================
    /**
    * Constructs a SensorWidget which is a child of parent.
    *
    * @param [in] parent    parent of widget
    * @param [in] f         widget flags
    */
    SensorWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);

    //=========================================================================================================
    /**
    * Create the user interface
    */
    void createUI();

    //=========================================================================================================
    /**
    * draw the channels
    */
    void drawChannels();

    //=========================================================================================================
    /**
    * Sets the SensorModel to display
    *
    * @param [in] model     Model to set
    */
    void setModel(SensorModel *model);

    //=========================================================================================================
    /**
    * Repaint the sensor widget with given parameters
    *
    * @param [in] topLeft       Index of upper left corner which has to be updated
    * @param [in] bottomRight   Index of bottom right corner which has to be updated
    * @param [in] roles         Role which has been updated
    */
    void contextUpdate(const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles = QVector<int> ());

    //=========================================================================================================
    /**
    * Repaint the sensor widget
    */
    void contextUpdate();

private:
    SensorModel*    m_pSensorModel;     /**< Connected sensor model */
    QGraphicsView*  m_pGraphicsView;    /**< View where channel items are displayed */
    QGraphicsScene* m_pGraphicsScene;   /**< Scene holding the channel items */

};

} // NAMESPACE

#endif // SENSORWIDGET_H
