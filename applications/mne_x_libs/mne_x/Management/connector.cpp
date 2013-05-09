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

#include "../Management/pluginmanager.h"

#include <xDtMng/measurementmanager.h>
#include <xDisp/displaymanager.h>

#include <generics/observerpattern.h>

#include "../Interfaces/IPlugin.h"
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
using namespace XDTMNGLIB;
using namespace XDISPLIB;


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

    QList<PLG_ID::Plugin_ID> plg_idList;
    QList<MSR_ID::Measurement_ID> msr_idList;

    for(int j = 0; j < PluginManager::getActiveRTRecordPlugins().size(); ++j)
    {
        plg_idList.clear();
        msr_idList.clear();
        plg_idList << (PluginManager::getActiveRTRecordPlugins()[j])->getAcceptorPlugin_IDs();
        msr_idList << (PluginManager::getActiveRTRecordPlugins()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTRecord = dynamic_cast<IObserver*>(PluginManager::getActiveRTRecordPlugins()[j]);

        qDebug() << "########2#########" << plg_idList << " MSR " << msr_idList;
        MeasurementManager::attachToRTSA(pRTRecord, plg_idList, msr_idList);
    }

    //ToDo not within here
    // Connect RealTimeSampleArray (Subject) to IAlgorithm plug-ins (IObserver)
    for(int j = 0; j < PluginManager::getActiveRTAlgorithmPlugins().size(); ++j)
    {
        plg_idList.clear();
        msr_idList.clear();
        plg_idList << (PluginManager::getActiveRTAlgorithmPlugins()[j])->getAcceptorPlugin_IDs();
        msr_idList << (PluginManager::getActiveRTAlgorithmPlugins()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTAlgorithm = dynamic_cast<IObserver*>(PluginManager::getActiveRTAlgorithmPlugins()[j]);

        qDebug() << "########2#########" << plg_idList << " MSR " << msr_idList;
        MeasurementManager::attachToRTSA(pRTAlgorithm, plg_idList, msr_idList);
    }

    //ToDo not within here
    // Connect RealTimeSampleArray (Subject) to IVisualization plug-ins (IObserver)
    for(int j = 0; j < PluginManager::getActiveRTVisualizationPlugins().size(); ++j)
    {
        plg_idList.clear();
        msr_idList.clear();
        plg_idList << (PluginManager::getActiveRTVisualizationPlugins()[j])->getAcceptorPlugin_IDs();
        msr_idList << (PluginManager::getActiveRTVisualizationPlugins()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTVisualization = dynamic_cast<IObserver*>(PluginManager::getActiveRTVisualizationPlugins()[j]);

        qDebug() << "########2#########" << plg_idList << " MSR " << msr_idList;
        MeasurementManager::attachToRTSA(pRTVisualization, plg_idList, msr_idList);
    }





    //*********************************************************************************************************
    //=========================================================================================================
    // Connect RealTimeMultiSampleArray (Subject) to all IObservers
    //=========================================================================================================

//    qDebug() << "Size " << MeasurementManager::getRTSA().size();

    for(int j = 0; j < PluginManager::getActiveRTRecordPlugins().size(); ++j)
    {
        plg_idList.clear();
        msr_idList.clear();
        plg_idList << (PluginManager::getActiveRTRecordPlugins()[j])->getAcceptorPlugin_IDs();
        msr_idList << (PluginManager::getActiveRTRecordPlugins()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTRecord = dynamic_cast<IObserver*>(PluginManager::getActiveRTRecordPlugins()[j]);

        qDebug() << "########3#########" << plg_idList << " MSR " << msr_idList;
        MeasurementManager::attachToRTMSA(pRTRecord, plg_idList, msr_idList);
    }

    //ToDo not within here
    // Connect RealTimeSampleArray (Subject) to IAlgorithm plug-ins (IObserver)
    for(int j = 0; j < PluginManager::getActiveRTAlgorithmPlugins().size(); ++j)
    {
        plg_idList.clear();
        msr_idList.clear();
        plg_idList << (PluginManager::getActiveRTAlgorithmPlugins()[j])->getAcceptorPlugin_IDs();
        msr_idList << (PluginManager::getActiveRTAlgorithmPlugins()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTAlgorithm = dynamic_cast<IObserver*>(PluginManager::getActiveRTAlgorithmPlugins()[j]);

        qDebug() << "########3#########" << plg_idList << " MSR " << msr_idList;
        MeasurementManager::attachToRTMSA(pRTAlgorithm, plg_idList, msr_idList);
    }

    //ToDo not within here
    // Connect RealTimeSampleArray (Subject) to IVisualization plug-ins (IObserver)
    for(int j = 0; j < PluginManager::getActiveRTVisualizationPlugins().size(); ++j)
    {
        plg_idList.clear();
        msr_idList.clear();
        plg_idList << (PluginManager::getActiveRTVisualizationPlugins()[j])->getAcceptorPlugin_IDs();
        msr_idList << (PluginManager::getActiveRTVisualizationPlugins()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTVisualization = dynamic_cast<IObserver*>(PluginManager::getActiveRTVisualizationPlugins()[j]);

        qDebug() << "########3#########" << plg_idList << " MSR " << msr_idList;
        MeasurementManager::attachToRTMSA(pRTVisualization, plg_idList, msr_idList);
    }













    //*********************************************************************************************************
    //=========================================================================================================
    // Connect RealTimeMultiSampleArrayNew (Subject) to all IObservers
    //=========================================================================================================

//    qDebug() << "Size " << MeasurementManager::getRTSA().size();

    for(int j = 0; j < PluginManager::getActiveRTRecordPlugins().size(); ++j)
    {
        plg_idList.clear();
        msr_idList.clear();
        plg_idList << (PluginManager::getActiveRTRecordPlugins()[j])->getAcceptorPlugin_IDs();
        msr_idList << (PluginManager::getActiveRTRecordPlugins()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTRecord = dynamic_cast<IObserver*>(PluginManager::getActiveRTRecordPlugins()[j]);

        qDebug() << "########4#########" << plg_idList << " MSR " << msr_idList;
        MeasurementManager::attachToRTMSANew(pRTRecord, plg_idList, msr_idList);
    }

    //ToDo not within here
    // Connect RealTimeSampleArray (Subject) to IAlgorithm plug-ins (IObserver)
    for(int j = 0; j < PluginManager::getActiveRTAlgorithmPlugins().size(); ++j)
    {
        plg_idList.clear();
        msr_idList.clear();
        plg_idList << (PluginManager::getActiveRTAlgorithmPlugins()[j])->getAcceptorPlugin_IDs();
        msr_idList << (PluginManager::getActiveRTAlgorithmPlugins()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTAlgorithm = dynamic_cast<IObserver*>(PluginManager::getActiveRTAlgorithmPlugins()[j]);

        qDebug() << "########4#########" << plg_idList << " MSR " << msr_idList;
        MeasurementManager::attachToRTMSANew(pRTAlgorithm, plg_idList, msr_idList);
    }

    //ToDo not within here
    // Connect RealTimeSampleArray (Subject) to IVisualization plug-ins (IObserver)
    for(int j = 0; j < PluginManager::getActiveRTVisualizationPlugins().size(); ++j)
    {
        plg_idList.clear();
        msr_idList.clear();
        plg_idList << (PluginManager::getActiveRTVisualizationPlugins()[j])->getAcceptorPlugin_IDs();
        msr_idList << (PluginManager::getActiveRTVisualizationPlugins()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTVisualization = dynamic_cast<IObserver*>(PluginManager::getActiveRTVisualizationPlugins()[j]);

        qDebug() << "########4#########" << plg_idList << " MSR " << msr_idList;
        MeasurementManager::attachToRTMSANew(pRTVisualization, plg_idList, msr_idList);
    }











    //*********************************************************************************************************
    //=========================================================================================================
    // Connect Numeric (Subject) to all IObservers
    //=========================================================================================================

//    qDebug() << "Size " << MeasurementManager::getNumeric().size();

    for(int j = 0; j < PluginManager::getActiveRTRecordPlugins().size(); ++j)
    {
        plg_idList.clear();
        msr_idList.clear();
        plg_idList << (PluginManager::getActiveRTRecordPlugins()[j])->getAcceptorPlugin_IDs();
        msr_idList << (PluginManager::getActiveRTRecordPlugins()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTRecord = dynamic_cast<IObserver*>(PluginManager::getActiveRTRecordPlugins()[j]);

        qDebug() << "########5#########" << plg_idList << " MSR " << msr_idList;
        MeasurementManager::attachToNumeric(pRTRecord, plg_idList, msr_idList);
    }

    //ToDo not within here
    // Connect RealTimeSampleArray (Subject) to IAlgorithm plug-ins (IObserver)
    for(int j = 0; j < PluginManager::getActiveRTAlgorithmPlugins().size(); ++j)
    {
        plg_idList.clear();
        msr_idList.clear();
        plg_idList << (PluginManager::getActiveRTAlgorithmPlugins()[j])->getAcceptorPlugin_IDs();
        msr_idList << (PluginManager::getActiveRTAlgorithmPlugins()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTAlgorithm = dynamic_cast<IObserver*>(PluginManager::getActiveRTAlgorithmPlugins()[j]);

        qDebug() << "########5#########" << plg_idList << " MSR " << msr_idList;
        MeasurementManager::attachToNumeric(pRTAlgorithm, plg_idList, msr_idList);
    }

    //ToDo not within here
    // Connect RealTimeSampleArray (Subject) to IVisualization plug-ins (IObserver)
    for(int j = 0; j < PluginManager::getActiveRTVisualizationPlugins().size(); ++j)
    {
        plg_idList.clear();
        msr_idList.clear();
        plg_idList << (PluginManager::getActiveRTVisualizationPlugins()[j])->getAcceptorPlugin_IDs();
        msr_idList << (PluginManager::getActiveRTVisualizationPlugins()[j])->getAcceptorMeasurement_IDs();
        IObserver* pRTVisualization = dynamic_cast<IObserver*>(PluginManager::getActiveRTVisualizationPlugins()[j]);

        qDebug() << "########5#########" << plg_idList << " MSR " << msr_idList;
        MeasurementManager::attachToNumeric(pRTVisualization, plg_idList, msr_idList);
    }



}


//*************************************************************************************************************

void Connector::disconnectMeasurements()//disconnect observer elements from subjects
{

}


//*************************************************************************************************************

void Connector::connectMeasurementWidgets(QList<PLG_ID::Plugin_ID>& idList, QSharedPointer<QTime> t)//(PLG_ID::Plugin_ID id) ToDO GRP List
{
    DisplayManager::init();

    connectMeasurements();//ToDo remove this


    //*********************************************************************************************************
    //=========================================================================================================
    // Connect Widgets to current visible RealTimeSampleArray (Subject) to specific IObserver
    //=========================================================================================================

    //ToDo visibility has to solved

    foreach(PLG_ID::Plugin_ID id, idList)
        MeasurementManager::attachWidgetsToRTSA(id, t);




    //*********************************************************************************************************
    //=========================================================================================================
    // Connect Widgets to current visible RealTimeMultiSampleArray (Subject) to specific IObserver
    //=========================================================================================================

    //ToDo visibility has to solved

    foreach(PLG_ID::Plugin_ID id, idList)
        MeasurementManager::attachWidgetsToRTMSA(id, t);




    //*********************************************************************************************************
    //=========================================================================================================
    // Connect Widgets to current visible RealTimeMultiSampleArrayNew (Subject) to specific IObserver
    //=========================================================================================================

    //ToDo visibility has to solved

    foreach(PLG_ID::Plugin_ID id, idList)
        MeasurementManager::attachWidgetsToRTMSANew(id, t);











//    for(unsigned int j = 0; j < PluginManager::getActiveRTRecordPlugins().size(); ++j)
//    {
//        IObserver* pRTRecord = dynamic_cast<IObserver*>(PluginManager::getActiveRTRecordPlugins()[j]);
//        RTMeasurementManager::attachToRTSA(pRTRecord);
//    }
//
//    //ToDo not within here
//    // Connect RealTimeSampleArray (Subject) to IAlgorithm plug-ins (IObserver)
//    for(unsigned int j = 0; j < PluginManager::getActiveRTAlgorithmPlugins().size(); ++j)
//    {
//        IObserver* pRTAlgorithm = dynamic_cast<IObserver*>(PluginManager::getActiveRTAlgorithmPlugins()[j]);
//        RTMeasurementManager::attachToRTSA(pRTAlgorithm);
//    }




    //*********************************************************************************************************
    //=========================================================================================================
    // Connect Widgets to current visible Numeric (Subject) to specific IObserver
    //=========================================================================================================

    foreach(PLG_ID::Plugin_ID id, idList)
        MeasurementManager::attachWidgetsToNumeric(id);




//    for(unsigned int j = 0; j < PluginManager::getActiveRTRecordPlugins().size(); ++j)
//    {
//        IObserver* pRTRecord = dynamic_cast<IObserver*>(PluginManager::getActiveRTRecordPlugins()[j]);
//        RTMeasurementManager::attachToNumeric(pRTRecord);
//    }
//
//    //ToDo not within here
//    // Connect RealTimeSampleArray (Subject) to IAlgorithm plug-ins (IObserver)
//    for(unsigned int j = 0; j < PluginManager::getActiveRTAlgorithmPlugins().size(); ++j)
//    {
//        IObserver* pRTAlgorithm = dynamic_cast<IObserver*>(PluginManager::getActiveRTAlgorithmPlugins()[j]);
//        RTMeasurementManager::attachToNumeric(pRTAlgorithm);
//    }















    //*********************************************************************************************************
    //=========================================================================================================
    // Connect ProgressBar (Subject) to to all IObservers
    //=========================================================================================================

//	foreach(PLG_ID::Plugin_ID id, idList)
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

//	foreach(PLG_ID::Plugin_ID id, idList)
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

//	foreach(PLG_ID::Plugin_ID id, idList)
//	{
//		foreach(Alert* pAlert, MeasurementManager::getAlert().values(id))
//		{
//			// Connect Alert (Subject) to IAlert plug-ins (IObserver)
//			for(unsigned int j = 0; j < PluginManager::getActiveAlertPlugins().size(); ++j)
//			{
//				IObserver* pAlertObserver = dynamic_cast<IObserver*>(PluginManager::getActiveAlertPlugins()[j]);
//				pAlert->attach(pAlertObserver);
//			}
//		}
//    }


    //ToDo
//    m_pDisplayManager->show();
}


//*************************************************************************************************************

void Connector::disconnectMeasurementWidgets(QList<PLG_ID::Plugin_ID>& idList)//disconnect group observer elements from subjects
{
    qDebug() << "Connector::disconnectMeasurementWidgets";

    MeasurementManager::detachWidgets(idList);

    qDebug() << "Connector::disconnectMeasurementWidgets after detach";

    DisplayManager::clean();
}
