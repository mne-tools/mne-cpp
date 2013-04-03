//=============================================================================================================
/**
* @file     connector.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the Connector class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connector.h"

#include "../management/modulemanager.h"

#include <rtDtMng/rtmeasurementmanager.h>
#include <disp/displaymanager.h>

#include <generics/observerpattern.h>

#include "../Interfaces/IModule.h"
#include "../Interfaces/ISensor.h"
#include "../Interfaces/IRTAlgorithm.h"
#include "../Interfaces/IRTVisualization.h"
#include "../Interfaces/IRTRecord.h"
#include "../Interfaces/IAlert.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

#include <QTime>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace RTDTMNGLIB;
using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Connector::Connector()
//: m_pDisplayManager(new DisplayManager)
{

}


//*************************************************************************************************************

Connector::~Connector()
{

}


//*************************************************************************************************************

void Connector::init()
{
//	connectMeasurements();
}


//*************************************************************************************************************

void Connector::connectMeasurements()
{
    //*********************************************************************************************************
    //=========================================================================================================
    // Connect RealTimeSampleArray (Subject) to all IObservers
    //=========================================================================================================

//    qDebug() << "Size " << MeasurementManager::getRTSA().size();

    QList<MDL_ID::Module_ID> mdl_idList;
    QList<MSR_ID::Measurement_ID> msr_idList;

    for(int j = 0; j < ModuleManager::getActiveRTRecordModules().size(); ++j)
    {
        mdl_idList.clear();
        msr_idList.clear();
        mdl_idList << (ModuleManager::getActiveRTRecordModules()[j])->getAcceptorModule_IDs();
        msr_idList << (ModuleManager::getActiveRTRecordModules()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTRecord = dynamic_cast<IObserver*>(ModuleManager::getActiveRTRecordModules()[j]);

        qDebug() << "########1#########" << mdl_idList << " MSR " << msr_idList;
        RTMeasurementManager::attachToRTSA(pRTRecord, mdl_idList, msr_idList);
    }

    //ToDo not within here
    // Connect RealTimeSampleArray (Subject) to IAlgorithm plug-ins (IObserver)
    for(int j = 0; j < ModuleManager::getActiveRTAlgorithmModules().size(); ++j)
    {
        mdl_idList.clear();
        msr_idList.clear();
        mdl_idList << (ModuleManager::getActiveRTAlgorithmModules()[j])->getAcceptorModule_IDs();
        msr_idList << (ModuleManager::getActiveRTAlgorithmModules()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTAlgorithm = dynamic_cast<IObserver*>(ModuleManager::getActiveRTAlgorithmModules()[j]);

        qDebug() << "########2#########" << mdl_idList << " MSR " << msr_idList;
        RTMeasurementManager::attachToRTSA(pRTAlgorithm, mdl_idList, msr_idList);
    }

    //ToDo not within here
    // Connect RealTimeSampleArray (Subject) to IVisualization plug-ins (IObserver)
    for(int j = 0; j < ModuleManager::getActiveRTVisualizationModules().size(); ++j)
    {
        mdl_idList.clear();
        msr_idList.clear();
        mdl_idList << (ModuleManager::getActiveRTVisualizationModules()[j])->getAcceptorModule_IDs();
        msr_idList << (ModuleManager::getActiveRTVisualizationModules()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTVisualization = dynamic_cast<IObserver*>(ModuleManager::getActiveRTVisualizationModules()[j]);

        qDebug() << "########2#########" << mdl_idList << " MSR " << msr_idList;
        RTMeasurementManager::attachToRTSA(pRTVisualization, mdl_idList, msr_idList);
    }



    //*********************************************************************************************************
    //=========================================================================================================
    // Connect Numeric (Subject) to all IObservers
    //=========================================================================================================

//    qDebug() << "Size " << MeasurementManager::getNumeric().size();

    for(int j = 0; j < ModuleManager::getActiveRTRecordModules().size(); ++j)
    {
        mdl_idList.clear();
        msr_idList.clear();
        mdl_idList << (ModuleManager::getActiveRTRecordModules()[j])->getAcceptorModule_IDs();
        msr_idList << (ModuleManager::getActiveRTRecordModules()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTRecord = dynamic_cast<IObserver*>(ModuleManager::getActiveRTRecordModules()[j]);

        qDebug() << "########3#########" << mdl_idList << " MSR " << msr_idList;
        RTMeasurementManager::attachToNumeric(pRTRecord, mdl_idList, msr_idList);
    }

    //ToDo not within here
    // Connect RealTimeSampleArray (Subject) to IAlgorithm plug-ins (IObserver)
    for(int j = 0; j < ModuleManager::getActiveRTAlgorithmModules().size(); ++j)
    {
        mdl_idList.clear();
        msr_idList.clear();
        mdl_idList << (ModuleManager::getActiveRTAlgorithmModules()[j])->getAcceptorModule_IDs();
        msr_idList << (ModuleManager::getActiveRTAlgorithmModules()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTAlgorithm = dynamic_cast<IObserver*>(ModuleManager::getActiveRTAlgorithmModules()[j]);

        qDebug() << "########4#########" << mdl_idList << " MSR " << msr_idList;
        RTMeasurementManager::attachToNumeric(pRTAlgorithm, mdl_idList, msr_idList);
    }

    //ToDo not within here
    // Connect RealTimeSampleArray (Subject) to IVisualization plug-ins (IObserver)
    for(int j = 0; j < ModuleManager::getActiveRTVisualizationModules().size(); ++j)
    {
        mdl_idList.clear();
        msr_idList.clear();
        mdl_idList << (ModuleManager::getActiveRTVisualizationModules()[j])->getAcceptorModule_IDs();
        msr_idList << (ModuleManager::getActiveRTVisualizationModules()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTVisualization = dynamic_cast<IObserver*>(ModuleManager::getActiveRTVisualizationModules()[j]);

        qDebug() << "########4#########" << mdl_idList << " MSR " << msr_idList;
        RTMeasurementManager::attachToNumeric(pRTVisualization, mdl_idList, msr_idList);
    }



}


//*************************************************************************************************************

void Connector::disconnectMeasurements()//disconnect observer elements from subjects
{

}


//*************************************************************************************************************

void Connector::connectMeasurementWidgets(QList<MDL_ID::Module_ID>& idList, QTime* t)//(MDL_ID::Module_ID id) ToDO GRP List
{
    DisplayManager::init();

    connectMeasurements();//ToDo remove this


    //*********************************************************************************************************
    //=========================================================================================================
    // Connect Widgets to current visible RealTimeSampleArray (Subject) to specific IObserver
    //=========================================================================================================

    //ToDo visibility has to solved

    foreach(MDL_ID::Module_ID id, idList)
        RTMeasurementManager::attachWidgetsToRTSA(id, t);




    //*********************************************************************************************************
    //=========================================================================================================
    // Connect Widgets to current visible RealTimeMultiSampleArray (Subject) to specific IObserver
    //=========================================================================================================

    //ToDo visibility has to solved

    foreach(MDL_ID::Module_ID id, idList)
        RTMeasurementManager::attachWidgetsToRTMSA(id, t);











//    for(unsigned int j = 0; j < ModuleManager::getActiveRTRecordModules().size(); ++j)
//    {
//        IObserver* pRTRecord = dynamic_cast<IObserver*>(ModuleManager::getActiveRTRecordModules()[j]);
//        RTMeasurementManager::attachToRTSA(pRTRecord);
//    }
//
//    //ToDo not within here
//    // Connect RealTimeSampleArray (Subject) to IAlgorithm plug-ins (IObserver)
//    for(unsigned int j = 0; j < ModuleManager::getActiveRTAlgorithmModules().size(); ++j)
//    {
//        IObserver* pRTAlgorithm = dynamic_cast<IObserver*>(ModuleManager::getActiveRTAlgorithmModules()[j]);
//        RTMeasurementManager::attachToRTSA(pRTAlgorithm);
//    }




    //*********************************************************************************************************
    //=========================================================================================================
    // Connect Widgets to current visible Numeric (Subject) to specific IObserver
    //=========================================================================================================

    foreach(MDL_ID::Module_ID id, idList)
        RTMeasurementManager::attachWidgetsToNumeric(id);




//    for(unsigned int j = 0; j < ModuleManager::getActiveRTRecordModules().size(); ++j)
//    {
//        IObserver* pRTRecord = dynamic_cast<IObserver*>(ModuleManager::getActiveRTRecordModules()[j]);
//        RTMeasurementManager::attachToNumeric(pRTRecord);
//    }
//
//    //ToDo not within here
//    // Connect RealTimeSampleArray (Subject) to IAlgorithm plug-ins (IObserver)
//    for(unsigned int j = 0; j < ModuleManager::getActiveRTAlgorithmModules().size(); ++j)
//    {
//        IObserver* pRTAlgorithm = dynamic_cast<IObserver*>(ModuleManager::getActiveRTAlgorithmModules()[j]);
//        RTMeasurementManager::attachToNumeric(pRTAlgorithm);
//    }















    //*********************************************************************************************************
    //=========================================================================================================
    // Connect ProgressBar (Subject) to to all IObservers
    //=========================================================================================================

//	foreach(MDL_ID::Module_ID id, idList)
//	{
//		foreach(ProgressBar* pProgress, MeasurementManager::getProgressBar().values(id))
//		{
//			// Connect ProgressBar (Subject) to ProgressBarWidget (IObserver)
//			if(pProgress->isVisible())
//			{
//				IObserver* pProgressBarWidget = dynamic_cast<IObserver*>(DisplayManager::addProgressBarWidget(pProgress, 0));
//				pProgress->attach(pProgressBarWidget);
//			}
//		}
//    }


    //*********************************************************************************************************
    //=========================================================================================================
    // Connect Text (Subject) to to all IObservers
    //=========================================================================================================

//	foreach(MDL_ID::Module_ID id, idList)
//	{
//		foreach(Text* pText, MeasurementManager::getText().values(id))
//		{
//			// Connect Text (Subject) to TextWidget (IObserver)
//			if(pText->isVisible())
//			{
//				IObserver* pTextWidget = dynamic_cast<IObserver*>(DisplayManager::addTextWidget(pText, 0));
//				pText->attach(pTextWidget);
//			}
//		}
//    }


    //*********************************************************************************************************
    //=========================================================================================================
    // Connect Alert (Subject) to all IObservers
    //=========================================================================================================

//	foreach(MDL_ID::Module_ID id, idList)
//	{
//		foreach(Alert* pAlert, MeasurementManager::getAlert().values(id))
//		{
//			// Connect Alert (Subject) to IAlert plug-ins (IObserver)
//			for(unsigned int j = 0; j < ModuleManager::getActiveAlertModules().size(); ++j)
//			{
//				IObserver* pAlertObserver = dynamic_cast<IObserver*>(ModuleManager::getActiveAlertModules()[j]);
//				pAlert->attach(pAlertObserver);
//			}
//		}
//    }


    //ToDo
//    m_pDisplayManager->show();
}


//*************************************************************************************************************

void Connector::disconnectMeasurementWidgets(QList<MDL_ID::Module_ID>& idList)//disconnect group observer elements from subjects
{
    qDebug() << "Connector::disconnectMeasurementWidgets";

    RTMeasurementManager::detachWidgets(idList);

    qDebug() << "Connector::disconnectMeasurementWidgets after detach";

    DisplayManager::clean();
}
