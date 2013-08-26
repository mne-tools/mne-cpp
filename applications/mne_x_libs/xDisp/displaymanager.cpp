//=============================================================================================================
/**
* @file     displaymanager.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of the DisplayManager Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "displaymanager.h"
#include "measurementwidget.h"
#include "numericwidget.h"
#include "realtimesamplearraywidget.h"
#include "realtimemultisamplearraywidget.h"
#include "realtimemultisamplearray_new_widget.h"
#include "realtimesourceestimatewidget.h"
#include "progressbarwidget.h"
#include "textwidget.h"

#include <xMeas/Measurement/text.h>
#include <xMeas/Measurement/realtimesourceestimate.h>
#include <xMeas/Measurement/realtimemultisamplearray_new.h>
#include <xMeas/Measurement/realtimemultisamplearray.h>
#include <xMeas/Measurement/realtimesamplearray.h>
#include <xMeas/Measurement/progressbar.h>
#include <xMeas/Measurement/numeric.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QDebug>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;
using namespace XMEASLIB;
using namespace IOBuffer;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DisplayManager::DisplayManager()
{

}


//*************************************************************************************************************

DisplayManager::~DisplayManager()
{
    clean();
}


//*************************************************************************************************************

void DisplayManager::init()
{
//    qDebug() << "DisplayManager::init(): s_hashMeasurementWidgets.size() = " << s_hashMeasurementWidgets.size();

//    foreach(MeasurementWidget* pMSRW, s_hashMeasurementWidgets.values())
//        pMSRW->init();
}


//*************************************************************************************************************

QWidget* DisplayManager::show()
{
    qDebug() << "DisplayManager::show()";

    QWidget* newDisp = new QWidget;

    QVBoxLayout* vboxLayout = new QVBoxLayout;

    QHBoxLayout* hboxLayout = new QHBoxLayout;

//    // Add all widgets but NumericWidgets to layout and display them
//    foreach(MeasurementWidget* pMSRW, s_hashMeasurementWidgets.values())
//    {
//        if(dynamic_cast<NumericWidget*>(pMSRW))
//            continue;
//        vboxLayout->addWidget(pMSRW);
//        pMSRW->show();
//    }

//    foreach(NumericWidget* pNUMW, s_hashNumericWidgets.values())
//    {
//        hboxLayout->addWidget(pNUMW);
//        pNUMW->show();
//    }

    vboxLayout->addLayout(hboxLayout);

    newDisp->setLayout(vboxLayout);

    // Initialize all MeasurementWidgets
    init();

    return newDisp;
}


//*************************************************************************************************************

void DisplayManager::clean()
{

}


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
