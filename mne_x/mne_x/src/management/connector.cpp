//=============================================================================================================
/**
* @file	   	connector.cpp
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the implementation of the Connector class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connector.h"

#include "../management/modulemanager.h"

#include "../../../comp/rtdtmng/rtmeasurementmanager.h"
#include "../../../comp/rtdisp/displaymanager.h"

#include "../../../comp/rtmeas/DesignPatterns/observerpattern.h"

#include "../interfaces/IModule.h"
#include "../interfaces/ISensor.h"
#include "../interfaces/IRTAlgorithm.h"
#include "../interfaces/IRTVisualization.h"
#include "../interfaces/IRTRecord.h"
#include "../interfaces/IAlert.h"


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

using namespace CSART;


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
