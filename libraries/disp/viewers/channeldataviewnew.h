//=============================================================================================================
/**
* @file     ChannelDataViewNew.h
* @author   Lorenz Esch <lesc@mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the ChannelDataViewNew Class.
*
*/

#ifndef ChannelDataViewNew_H
#define ChannelDataViewNew_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QOpenGLWidget>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QGraphicsScene;
class QGraphicsView;

namespace FIFFLIB {
    class FiffInfo;
}



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

class ChannelDataItem;


//=============================================================================================================
/**
* DECLARE CLASS ChannelDataViewNew
*
* @brief The ChannelDataViewNew class provides a channel view display
*/
class DISPSHARED_EXPORT ChannelDataViewNew : public QWidget
{    
    Q_OBJECT

public:
    typedef QSharedPointer<ChannelDataViewNew> SPtr;              /**< Shared pointer type for ChannelDataViewNew. */
    typedef QSharedPointer<const ChannelDataViewNew> ConstSPtr;   /**< Const shared pointer type for ChannelDataViewNew. */

    //=========================================================================================================
    /**
    * Constructs a ChannelDataViewNew which is a child of parent.
    *
    * @param [in] parent    The parent of widget.
    */
    ChannelDataViewNew(QWidget* parent = 0,
                       Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
    * Initilaizes the ChannelDataViewNew based on a FiffInfo.
    *
    * @param [in] info    The FiffInfo.
    */
    void init(QSharedPointer<FIFFLIB::FiffInfo> &info);

    //=========================================================================================================
    /**
    * Add data to the view.
    *
    * @param [in] data    The new data.
    */
    void addData(const QList<Eigen::MatrixXd>& data);

protected: 
    QSharedPointer<FIFFLIB::FiffInfo> m_pFiffInfo;
    QPointer<QGraphicsScene>            m_pGraphicsScene;
    QPointer<QGraphicsView>             m_pQGraphicsView;

    QList<QPointer<ChannelDataItem> > m_lItemList;

signals:
};

} // NAMESPACE

#endif // ChannelDataViewNew_H
