//=============================================================================================================
/**
* @file     deepviewer.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    DeepViewer class declaration.
*
*/

#ifndef DEEPVIEWER_H
#define DEEPVIEWER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "disp_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>

#include <QChart>
#include <QChartView>
#include <QLineSeries>

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
* Visualization helpers - TODO: move parts into disp library
*
* @brief Deep Viewer helpers
*/
class DISPSHARED_EXPORT DeepViewer
{
public:
    typedef QSharedPointer<DeepViewer> SPtr;            /**< Shared pointer type for DeepViewer. */
    typedef QSharedPointer<const DeepViewer> ConstSPtr; /**< Const shared pointer type for DeepViewer. */

    //=========================================================================================================
    /**
    * Default constructor
    */
    DeepViewer();

    //=========================================================================================================
    /**
    * Destructs DeepModelCreator
    */
    virtual ~DeepViewer();

    //=========================================================================================================
    /**
    * Plot a line series
    *
    * @param [in] y         Data to plot
    * @param [in] title     Plot title
    *
    * @return the view handle
    */
    static QtCharts::QChartView * linePlot(const QVector<double>& y, const QString& title = "");

    //=========================================================================================================
    /**
    * Plot a line series
    *
    * @param [in] x         X-Axis data to plot
    * @param [in] y         Y-Axis data to plot
    * @param [in] title     Plot title
    *
    * @return the view handle
    */
    static QtCharts::QChartView * linePlot(const QVector<double>& x, const QVector<double>& y, const QString& title = "");

private:

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE

#endif // DEEPVIEWER_H
