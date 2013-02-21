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
//#include "numericwidget.h"
#include "realtimesamplearraywidget.h"
#include "realtimemultisamplearraywidget.h"
//#include "progressbarwidget.h"
//#include "textwidget.h"


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

using namespace DISPLIB;
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

//NumericWidget* DisplayManager::addNumericWidget(Numeric* pNum, QWidget* parent, MSR_ID::Measurement_ID id)
//{
//    NumericWidget* numWidget = new NumericWidget(pNum, parent);
//    numWidget->hide();
//    s_hashMeasurementWidgets.insert(id, numWidget);
//    s_hashNumericWidgets.insert(id, numWidget);
//    return numWidget;
//}


//*************************************************************************************************************

//RealTimeSampleArrayWidget* DisplayManager::addRealTimeSampleArrayWidget(RealTimeSampleArray* pRTSA, QWidget* parent, MSR_ID::Measurement_ID id, QTime* t)
//{
//    RealTimeSampleArrayWidget* rtsaWidget = new RealTimeSampleArrayWidget(pRTSA, t, parent);
//    rtsaWidget->hide();
//    s_hashMeasurementWidgets.insert(id, rtsaWidget);
//    s_hashRealTimeSampleArrayWidgets.insert(id, rtsaWidget);
//    return rtsaWidget;
//}


//*************************************************************************************************************

//RealTimeMultiSampleArrayWidget* DisplayManager::addRealTimeMultiSampleArrayWidget(RealTimeMultiSampleArray* pRTSM, QWidget* parent, MSR_ID::Measurement_ID id, QTime* t)
//{
//    RealTimeMultiSampleArrayWidget* rtsmWidget = new RealTimeMultiSampleArrayWidget(pRTSM, t, parent);
//    rtsmWidget->hide();
//    s_hashMeasurementWidgets.insert(id, rtsmWidget);
//    s_hashRealTimeMultiSampleArrayWidgets.insert(id, rtsmWidget);
//    return rtsmWidget;
//}


//*************************************************************************************************************

//ProgressBarWidget* DisplayManager::addProgressBarWidget(ProgressBar* pProgress, QWidget* parent, MSR_ID::Measurement_ID id)
//{
//    ProgressBarWidget* progressWidget = new ProgressBarWidget(pProgress, parent);
//    progressWidget->hide();
//    s_hashMeasurementWidgets.insert(id, progressWidget);
//    s_hashProgressBarWidgets.insert(id, progressWidget);
//    return progressWidget;
//}


//*************************************************************************************************************

//TextWidget* DisplayManager::addTextWidget(Text* pText, QWidget* parent, MSR_ID::Measurement_ID id)
//{
//    TextWidget* textWidget = new TextWidget(pText, parent);
//    textWidget->hide();
//    s_hashMeasurementWidgets.insert(id, textWidget);
//    s_hashTextWidgets.insert(id, textWidget);
//    return textWidget;
//}


//*************************************************************************************************************

void DisplayManager::init()
{
    qDebug() << "DisplayManager::init(): s_hashMeasurementWidgets.size() = " << s_hashMeasurementWidgets.size();

    foreach(MeasurementWidget* pMSRW, s_hashMeasurementWidgets.values())
        pMSRW->init();
}


//*************************************************************************************************************

QWidget* DisplayManager::show()
{
    qDebug() << "DisplayManager::show()";

    QWidget* newDisp = new QWidget;

    QVBoxLayout* vboxLayout = new QVBoxLayout;

    QHBoxLayout* hboxLayout = new QHBoxLayout;

    // Add all widgets but NumericWidgets to layout and display them
	foreach(MeasurementWidget* pMSRW, s_hashMeasurementWidgets.values())
	{
        if(dynamic_cast<NumericWidget*>(pMSRW))
            continue;
        vboxLayout->addWidget(pMSRW);
        pMSRW->show();
    }

    foreach(NumericWidget* pNUMW, s_hashNumericWidgets.values())
    {
    	hboxLayout->addWidget(pNUMW);
    	pNUMW->show();
    }

    vboxLayout->addLayout(hboxLayout);

    newDisp->setLayout(vboxLayout);

    // Initialize all MeasurementWidgets
    init();

    return newDisp;
}


//*************************************************************************************************************

void DisplayManager::clean()
{
    qDebug() << "DisplayManager::clean()";

    foreach(MeasurementWidget* pMSRW, s_hashMeasurementWidgets.values())
        delete pMSRW;

    s_hashMeasurementWidgets.clear();
//    s_hashNumericWidgets.clear();
    s_hashRealTimeSampleArrayWidgets.clear();
    s_hashRealTimeMultiSampleArrayWidgets.clear();
//    s_hashProgressBarWidgets.clear();
//    s_hashTextWidgets.clear();

    qDebug() << "DisplayManager::clean() end size:" << s_hashMeasurementWidgets.size();
}


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

//QHash<MSR_ID::Measurement_ID, MeasurementWidget*>         DisplayManager::s_hashMeasurementWidgets;
//QHash<MSR_ID::Measurement_ID, NumericWidget*>             DisplayManager::s_hashNumericWidgets;
//QHash<MSR_ID::Measurement_ID, RealTimeSampleArrayWidget*> DisplayManager::s_hashRealTimeSampleArrayWidgets;
//QHash<MSR_ID::Measurement_ID, RealTimeMultiSampleArrayWidget*> DisplayManager::s_hashRealTimeMultiSampleArrayWidgets;
//QHash<MSR_ID::Measurement_ID, ProgressBarWidget*>         DisplayManager::s_hashProgressBarWidgets;
//QHash<MSR_ID::Measurement_ID, TextWidget*>                DisplayManager::s_hashTextWidgets;
