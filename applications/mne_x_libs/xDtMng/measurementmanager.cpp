//=============================================================================================================
/**
* @file     measurementmanager.cpp
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
* @brief    Contains the implementation of the MeasurementManager class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "measurementmanager.h"
#include <xMeas/Measurement/IMeasurementSource.h>
#include <xMeas/Measurement/IMeasurementSink.h>

#include <xMeas/Measurement/realtimesamplearray.h>
#include <xMeas/Measurement/realtimemultisamplearray.h>
#include <xMeas/Measurement/realtimemultisamplearray_new.h>
#include <xMeas/Measurement/numeric.h>
//#include "Measurement/progressbar.h"
////#include "Measurement/alert.h"
//#include "Measurement/text.h"

#include <xDisp/realtimesamplearraywidget.h>
#include <xDisp/realtimemultisamplearraywidget.h>
#include <xDisp/realtimemultisamplearray_new_widget.h>
#include <xDisp/numericwidget.h>
#include <xDisp/displaymanager.h>


#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XMEASLIB;
using namespace XDTMNGLIB;
using namespace XDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MeasurementManager::MeasurementManager()
{

}


//*************************************************************************************************************

MeasurementManager::~MeasurementManager()
{

}


//*************************************************************************************************************

void MeasurementManager::addMeasurementProvider(IMeasurementSource* pMSRPrv)
{
    qDebug() << "Number of Measurement Providers before: "<< s_hashMeasurementProvider.size() << "keys" << s_hashMeasurementProvider.keys();
    qDebug() << "Adding Measurement Provider: "<< pMSRPrv->getPlugin_ID();
    //check first if MeasurementProvider with id exists jet - and return existing first otherwise do the following stuff
    if(!s_hashMeasurementProvider.contains(pMSRPrv->getPlugin_ID()))
        s_hashMeasurementProvider.insert(pMSRPrv->getPlugin_ID(), pMSRPrv);

    qDebug() << "Number of Measurement Providers: "<< s_hashMeasurementProvider.size() << "keys" << s_hashMeasurementProvider.keys();
}


//*************************************************************************************************************

void MeasurementManager::addMeasurementAcceptor(IPlugin* pMSRAcc)//IMeasurementSink* pMSRAcc)
{
    qDebug() << "Number of Measurement Acceptors before: "<< s_hashMeasurementAcceptor.size() << "keys" << s_hashMeasurementAcceptor.keys();
    qDebug() << "Adding Measurement Acceptor: "<< pMSRAcc->getPlugin_ID();
    //check first if MeasurementProvider with id exists jet - and return existing first otherwise do the following stuff
    if(!s_hashMeasurementAcceptor.contains(pMSRAcc->getPlugin_ID()))
        s_hashMeasurementAcceptor.insert(pMSRAcc->getPlugin_ID(), pMSRAcc);

    qDebug() << "Number of Measurement Acceptor: "<< s_hashMeasurementAcceptor.size() << "keys" << s_hashMeasurementAcceptor.keys();
}



//RTSA
//*************************************************************************************************************

void MeasurementManager::attachToRTSA(IObserver* pObserver) //attaching to all Measurements of all Measurement Providers
{
    QList<PLG_ID::Plugin_ID> plg_idList = s_hashMeasurementProvider.keys();
    attachToRTSA(pObserver, plg_idList);
}


//*************************************************************************************************************

void MeasurementManager::attachToRTSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList) //attaching to all Measurements of given Measurement Providers List
{
    QList<MSR_ID::Measurement_ID> msr_idList;

    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
            msr_idList += pMsrPvr->getProviderRTSA().keys();
        }
        else
            qDebug() << "Error while attachToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
    }
    attachToRTSA(pObserver, plg_idList, msr_idList);
}


//*************************************************************************************************************

void MeasurementManager::attachToRTSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList) //attaching to given Measurements List of given Measurement Providers List
{
    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
            foreach(MSR_ID::Measurement_ID msr_id, msr_idList)
            {
                if(pMsrPvr->getProviderRTSA().contains(msr_id))
                {
                    RealTimeSampleArray::SPtr pRTSA = pMsrPvr->getProviderRTSA().value(msr_id);
                    pRTSA->attach(pObserver);
                }
                else
                {
                    qDebug() << "Warning while attachToRTSA: MSR_ID::Measurement_ID " << msr_id << "is not part of measurement provider " << plg_id << ".";
                }
            }
        }
        else
        {
            qDebug() << "Error while attachToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
        }
    }
}


//*************************************************************************************************************

void MeasurementManager::attachWidgetsToRTSA(PLG_ID::Plugin_ID plg_id, QSharedPointer<QTime> t) //attaching to given Measurements List of given Measurement Providers List
{
    qDebug() << "Number of Measurement Providers to attach Widget RTSA to: " << s_hashMeasurementProvider.size();
    if(s_hashMeasurementProvider.contains(plg_id))
    {
        IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
        foreach(MSR_ID::Measurement_ID msr_id, pMsrPvr->getProviderRTSA().keys())
            attachWidgetToRTSA(plg_id, msr_id, t);
    }
    else
        qDebug() << "Error while attachWidgetToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
}


//*************************************************************************************************************

void MeasurementManager::attachWidgetToRTSA(PLG_ID::Plugin_ID plg_id, MSR_ID::Measurement_ID msr_id, QSharedPointer<QTime> t) //attaching to given Measurements List of given Measurement Providers List
{
    if(s_hashMeasurementProvider.contains(plg_id))
    {
        IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
        if(pMsrPvr->getProviderRTSA().contains(msr_id))
        {
            RealTimeSampleArray::SPtr pRTSA = pMsrPvr->getProviderRTSA().value(msr_id);
            if(pRTSA->isVisible())
            {
                IObserver* pRTSAWidget = dynamic_cast<IObserver*>(DisplayManager::addRealTimeSampleArrayWidget(pRTSA, 0, msr_id, t));
                pRTSA->attach(pRTSAWidget);
            }
        }
        else
        {
            qDebug() << "Warning while attachWidgetToRTSA: MSR_ID::Measurement_ID " << msr_id << "is not part of measurement provider " << plg_id << ".";
        }
    }
    else
        qDebug() << "Error while attachWidgetToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
}


//*************************************************************************************************************

void MeasurementManager::detachFromRTSA(IObserver* pObserver) //attaching to all Measurements of all Measurement Providers
{
    QList<PLG_ID::Plugin_ID> plg_idList = s_hashMeasurementProvider.keys();

    detachFromRTSA(pObserver, plg_idList);
}


//*************************************************************************************************************

void MeasurementManager::detachFromRTSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList) //attaching to all Measurements of given Measurement Providers List
{
    QList<MSR_ID::Measurement_ID> msr_idList;

    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
            msr_idList += pMsrPvr->getProviderRTSA().keys();
        }
        else
            qDebug() << "Error while attachToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
    }

    detachFromRTSA(pObserver, plg_idList, msr_idList);
}


//*************************************************************************************************************

void MeasurementManager::detachFromRTSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList) //attaching to given Measurements List of given Measurement Providers List
{
    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
            foreach(MSR_ID::Measurement_ID msr_id, msr_idList)
            {
                if(pMsrPvr->getProviderRTSA().contains(msr_id))
                {
                    QSharedPointer<RealTimeSampleArray> pRTSA = pMsrPvr->getProviderRTSA().value(msr_id);
                    pRTSA->detach(pObserver);
                }
                else
                {
                    qDebug() << "Warning while detachFromRTSA: MSR_ID::Measurement_ID " << msr_id << "is not part of measurement provider " << plg_id << ".";
                }
            }
        }
        else
        {
            qDebug() << "Error while detachFromRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
        }
    }
}





































//RTMSA
//*************************************************************************************************************

void MeasurementManager::attachToRTMSA(IObserver* pObserver) //attaching to all Measurements of all Measurement Providers
{
    QList<PLG_ID::Plugin_ID> plg_idList = s_hashMeasurementProvider.keys();
    attachToRTMSA(pObserver, plg_idList);
}


//*************************************************************************************************************

void MeasurementManager::attachToRTMSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList) //attaching to all Measurements of given Measurement Providers List
{
    QList<MSR_ID::Measurement_ID> msr_idList;

    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
            msr_idList += pMsrPvr->getProviderRTMSA().keys();
        }
        else
            qDebug() << "Error while attachToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
    }
    attachToRTMSA(pObserver, plg_idList, msr_idList);
}


//*************************************************************************************************************

void MeasurementManager::attachToRTMSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList) //attaching to given Measurements List of given Measurement Providers List
{
    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
            foreach(MSR_ID::Measurement_ID msr_id, msr_idList)
            {
                if(pMsrPvr->getProviderRTMSA().contains(msr_id))
                {
                    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMsrPvr->getProviderRTMSA().value(msr_id);
                    pRTMSA->attach(pObserver);
                }
                else
                {
                    qDebug() << "Warning while attachToRTSA: MSR_ID::Measurement_ID " << msr_id << "is not part of measurement provider " << plg_id << ".";
                }
            }
        }
        else
        {
            qDebug() << "Error while attachToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
        }
    }
}


//*************************************************************************************************************

void MeasurementManager::attachWidgetsToRTMSA(PLG_ID::Plugin_ID plg_id, QSharedPointer<QTime> t) //attaching to given Measurements List of given Measurement Providers List
{
    qDebug() << "Number of Measurement Providers to attach Widget RTSA to: " << s_hashMeasurementProvider.size();
    if(s_hashMeasurementProvider.contains(plg_id))
    {
        IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
        foreach(MSR_ID::Measurement_ID msr_id, pMsrPvr->getProviderRTMSA().keys())
            attachWidgetToRTMSA(plg_id, msr_id, t);
    }
    else
        qDebug() << "Error while attachWidgetToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
}


//*************************************************************************************************************

void MeasurementManager::attachWidgetToRTMSA(PLG_ID::Plugin_ID plg_id, MSR_ID::Measurement_ID msr_id, QSharedPointer<QTime> t) //attaching to given Measurements List of given Measurement Providers List
{
    if(s_hashMeasurementProvider.contains(plg_id))
    {
        IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
        if(pMsrPvr->getProviderRTMSA().contains(msr_id))
        {
            QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMsrPvr->getProviderRTMSA().value(msr_id);
            if(pRTMSA->isVisible())
            {
                IObserver* pRTMSAWidget = dynamic_cast<IObserver*>(DisplayManager::addRealTimeMultiSampleArrayWidget(pRTMSA, 0, msr_id, t));
                pRTMSA->attach(pRTMSAWidget);
            }
        }
        else
        {
            qDebug() << "Warning while attachWidgetToRTSA: MSR_ID::Measurement_ID " << msr_id << "is not part of measurement provider " << plg_id << ".";
        }
    }
    else
        qDebug() << "Error while attachWidgetToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
}


//*************************************************************************************************************

void MeasurementManager::detachFromRTMSA(IObserver* pObserver) //attaching to all Measurements of all Measurement Providers
{
    QList<PLG_ID::Plugin_ID> plg_idList = s_hashMeasurementProvider.keys();

    detachFromRTMSA(pObserver, plg_idList);
}


//*************************************************************************************************************

void MeasurementManager::detachFromRTMSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList) //attaching to all Measurements of given Measurement Providers List
{
    QList<MSR_ID::Measurement_ID> msr_idList;

    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
            msr_idList += pMsrPvr->getProviderRTMSA().keys();
        }
        else
            qDebug() << "Error while attachToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
    }

    detachFromRTMSA(pObserver, plg_idList, msr_idList);
}


//*************************************************************************************************************

void MeasurementManager::detachFromRTMSA(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList) //attaching to given Measurements List of given Measurement Providers List
{
    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
            foreach(MSR_ID::Measurement_ID msr_id, msr_idList)
            {
                if(pMsrPvr->getProviderRTMSA().contains(msr_id))
                {
                    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMsrPvr->getProviderRTMSA().value(msr_id);
                    pRTMSA->detach(pObserver);
                }
                else
                {
                    qDebug() << "Warning while detachFromRTSA: MSR_ID::Measurement_ID " << msr_id << "is not part of measurement provider " << plg_id << ".";
                }
            }
        }
        else
        {
            qDebug() << "Error while detachFromRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
        }
    }
}




//RTMSANew
//*************************************************************************************************************

void MeasurementManager::attachToRTMSANew(IObserver* pObserver) //attaching to all Measurements of all Measurement Providers
{
    QList<PLG_ID::Plugin_ID> plg_idList = s_hashMeasurementProvider.keys();
    attachToRTMSANew(pObserver, plg_idList);
}


//*************************************************************************************************************

void MeasurementManager::attachToRTMSANew(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList) //attaching to all Measurements of given Measurement Providers List
{
    QList<MSR_ID::Measurement_ID> msr_idList;

    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
            msr_idList += pMsrPvr->getProviderRTMSANew().keys();
        }
        else
            qDebug() << "Error while attachToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
    }
    attachToRTMSANew(pObserver, plg_idList, msr_idList);
}


//*************************************************************************************************************

void MeasurementManager::attachToRTMSANew(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList) //attaching to given Measurements List of given Measurement Providers List
{
    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
            foreach(MSR_ID::Measurement_ID msr_id, msr_idList)
            {
                if(pMsrPvr->getProviderRTMSANew().contains(msr_id))
                {
                    QSharedPointer<RealTimeMultiSampleArrayNew> pRTMSANew = pMsrPvr->getProviderRTMSANew().value(msr_id);
                    pRTMSANew->attach(pObserver);
                }
                else
                {
                    qDebug() << "Warning while attachToRTSA: MSR_ID::Measurement_ID " << msr_id << "is not part of measurement provider " << plg_id << ".";
                }
            }
        }
        else
        {
            qDebug() << "Error while attachToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
        }
    }
}


//*************************************************************************************************************

void MeasurementManager::attachWidgetsToRTMSANew(PLG_ID::Plugin_ID plg_id, QSharedPointer<QTime> t) //attaching to given Measurements List of given Measurement Providers List
{
    qDebug() << "Number of Measurement Providers to attach Widget RTSA to: " << s_hashMeasurementProvider.size();
    if(s_hashMeasurementProvider.contains(plg_id))
    {
        IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
        foreach(MSR_ID::Measurement_ID msr_id, pMsrPvr->getProviderRTMSANew().keys())
            attachWidgetToRTMSANew(plg_id, msr_id, t);
    }
    else
        qDebug() << "Error while attachWidgetToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
}


//*************************************************************************************************************

void MeasurementManager::attachWidgetToRTMSANew(PLG_ID::Plugin_ID plg_id, MSR_ID::Measurement_ID msr_id, QSharedPointer<QTime> t) //attaching to given Measurements List of given Measurement Providers List
{
    if(s_hashMeasurementProvider.contains(plg_id))
    {
        IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
        if(pMsrPvr->getProviderRTMSANew().contains(msr_id))
        {
            QSharedPointer<RealTimeMultiSampleArrayNew> pRTMSANew = pMsrPvr->getProviderRTMSANew().value(msr_id);
            if(pRTMSANew->isVisible())
            {
                IObserver* pRTMSANewWidget = dynamic_cast<IObserver*>(DisplayManager::addRealTimeMultiSampleArrayNewWidget(pRTMSANew, 0, msr_id, t));
                pRTMSANew->attach(pRTMSANewWidget);
            }
        }
        else
        {
            qDebug() << "Warning while attachWidgetToRTSA: MSR_ID::Measurement_ID " << msr_id << "is not part of measurement provider " << plg_id << ".";
        }
    }
    else
        qDebug() << "Error while attachWidgetToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
}


//*************************************************************************************************************

void MeasurementManager::detachFromRTMSANew(IObserver* pObserver) //attaching to all Measurements of all Measurement Providers
{
    QList<PLG_ID::Plugin_ID> plg_idList = s_hashMeasurementProvider.keys();

    detachFromRTMSANew(pObserver, plg_idList);
}


//*************************************************************************************************************

void MeasurementManager::detachFromRTMSANew(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList) //attaching to all Measurements of given Measurement Providers List
{
    QList<MSR_ID::Measurement_ID> msr_idList;

    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
            msr_idList += pMsrPvr->getProviderRTMSANew().keys();
        }
        else
            qDebug() << "Error while attachToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
    }

    detachFromRTMSANew(pObserver, plg_idList, msr_idList);
}


//*************************************************************************************************************

void MeasurementManager::detachFromRTMSANew(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList) //attaching to given Measurements List of given Measurement Providers List
{
    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
            foreach(MSR_ID::Measurement_ID msr_id, msr_idList)
            {
                if(pMsrPvr->getProviderRTMSANew().contains(msr_id))
                {
                    QSharedPointer<RealTimeMultiSampleArrayNew> pRTMSANew = pMsrPvr->getProviderRTMSANew().value(msr_id);
                    pRTMSANew->detach(pObserver);
                }
                else
                {
                    qDebug() << "Warning while detachFromRTSA: MSR_ID::Measurement_ID " << msr_id << "is not part of measurement provider " << plg_id << ".";
                }
            }
        }
        else
        {
            qDebug() << "Error while detachFromRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
        }
    }
}







//Numeric
//*************************************************************************************************************

void MeasurementManager::attachToNumeric(IObserver* pObserver) //attaching to all Measurements of all Measurement Providers
{
    foreach(IMeasurementSource* pMsrPvr, s_hashMeasurementProvider.values())
    {
        foreach(QSharedPointer<Numeric> pNum, pMsrPvr->getProviderNumeric().values())
        {
            pNum->attach(pObserver);
        }
    }
}


//*************************************************************************************************************

void MeasurementManager::attachToNumeric(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList) //attaching to all Measurements of given Measurement Providers List
{
    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
            foreach(QSharedPointer<Numeric> pNum, pMsrPvr->getProviderNumeric().values())
            {
                pNum->attach(pObserver);
            }
        }
        else
        {
            qDebug() << "Error while attachToNumeric: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
        }
    }
}


//*************************************************************************************************************

void MeasurementManager::attachToNumeric(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList) //attaching to given Measurements List of given Measurement Providers List
{
    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
    	{
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
		    foreach(MSR_ID::Measurement_ID msr_id, msr_idList)
		    {
		    	if(pMsrPvr->getProviderNumeric().contains(msr_id))
		    	{
                    QSharedPointer<Numeric> pNum = pMsrPvr->getProviderNumeric().value(msr_id);
                    pNum->attach(pObserver);
		    	}
		    	else
		    	{
                    qDebug() << "Warning while attachToNumeric: MSR_ID::Measurement_ID " << msr_id << "is not part of measurement provider " << plg_id << ".";
		    	}
		    }
    	}
    	else
    	{
            qDebug() << "Error while attachToNumeric: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
    	}
    }
}


//*************************************************************************************************************

void MeasurementManager::attachWidgetsToNumeric(PLG_ID::Plugin_ID plg_id) //attaching to given Measurements List of given Measurement Providers List
{
    if(s_hashMeasurementProvider.contains(plg_id))
	{
        IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
		foreach(MSR_ID::Measurement_ID msr_id, pMsrPvr->getProviderNumeric().keys())
            attachWidgetToNumeric(plg_id, msr_id);
	}
	else
        qDebug() << "Error while attachWidgetToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
}


//*************************************************************************************************************

void MeasurementManager::attachWidgetToNumeric(PLG_ID::Plugin_ID plg_id, MSR_ID::Measurement_ID msr_id) //attaching to given Measurements List of given Measurement Providers List
{
    if(s_hashMeasurementProvider.contains(plg_id))
    {
        IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
        if(pMsrPvr->getProviderNumeric().contains(msr_id))
        {
            QSharedPointer<Numeric> pNum = pMsrPvr->getProviderNumeric().value(msr_id);
            if(pNum->isVisible())
            {
                IObserver* pNumWidget = dynamic_cast<IObserver*>(DisplayManager::addNumericWidget(pNum, 0, msr_id));
                pNum->attach(pNumWidget);
            }
        }
        else
        {
            qDebug() << "Warning while attachWidgetToRTSA: MSR_ID::Measurement_ID " << msr_id << "is not part of measurement provider " << plg_id << ".";
        }
    }
    else
        qDebug() << "Error while attachWidgetToRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
}

//*************************************************************************************************************

void MeasurementManager::detachFromNumeric(IObserver* pObserver) //attaching to all Measurements of all Measurement Providers
{
//	foreach(MeasurementProvider* pMsrPvr, s_hashMeasurementProvider.values())
//	{
//		foreach(Numeric* pNum, pMsrPvr->getProviderNumeric().values())
//		{
//			pNum->detach(pObserver);
//		}
//	}

    QList<PLG_ID::Plugin_ID> plg_idList = s_hashMeasurementProvider.keys();

    detachFromNumeric(pObserver, plg_idList);
}


//*************************************************************************************************************

void MeasurementManager::detachFromNumeric(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList) //attaching to all Measurements of given Measurement Providers List
{
    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);

            qDebug() << "MeasurementManager::detachFromNumeric: MSR_ID::Measurement_IDS: " << pMsrPvr->getProviderRTSA().keys();
            foreach(QSharedPointer<Numeric> pNum, pMsrPvr->getProviderNumeric().values())
            {
                pNum->detach(pObserver);
            }
        }
        else
        {
            qDebug() << "Error while detachFromNumeric: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
        }
    }


    //ToDo ToDo
//	QList<MSR_ID::Measurement_ID> msr_idList;
//
//    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
//    {
//    	if(s_hashMeasurementProvider.contains(plg_id))
//    	{
//			MeasurementProvider* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
//			msr_idList += pMsrPvr->getProviderRTSA().keys();
//    	}
//    	else
//    		qDebug() << "Error while detachFromNumeric: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
//    }
//
//    detachFromNumeric(pObserver, plg_idList, msr_idList);
}


//*************************************************************************************************************

void MeasurementManager::detachFromNumeric(IObserver* pObserver, QList<PLG_ID::Plugin_ID> plg_idList, QList<MSR_ID::Measurement_ID> msr_idList) //attaching to given Measurements List of given Measurement Providers List
{
    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
    {
        if(s_hashMeasurementProvider.contains(plg_id))
        {
            IMeasurementSource* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
            foreach(MSR_ID::Measurement_ID msr_id, msr_idList)
            {
                if(pMsrPvr->getProviderNumeric().contains(msr_id))
                {
                    QSharedPointer<Numeric> pNum = pMsrPvr->getProviderNumeric().value(msr_id);
                    pNum->detach(pObserver);
                }
                else
                {
                    qDebug() << "Warning while detachFromNumeric: MSR_ID::Measurement_ID " << msr_id << "is not part of measurement provider " << plg_id << ".";
                }
            }
        }
        else
        {
            qDebug() << "Error while detachFromNumeric: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
        }
    }



//    foreach(PLG_ID::Plugin_ID plg_id, plg_idList)
//    {
//    	if(s_hashMeasurementProvider.contains(plg_id))
//    	{
//			MeasurementProvider* pMsrPvr = s_hashMeasurementProvider.value(plg_id);
//		    foreach(MSR_ID::Measurement_ID msr_id, msr_idList)
//		    {
//		    	if(pMsrPvr->getProviderRTSA().contains(msr_id))
//		    	{
//					Numeric* pNum = pMsrPvr->getProviderNumeric().value(msr_id);
//					pNum->detach(pObserver);
//		    	}
//		    	else
//		    	{
//		    		qDebug() << "Warning while detachFromRTSA: MSR_ID::Measurement_ID " << msr_id << "is not part of measurement provider " << plg_id << ".";
//		    	}
//		    }
//    	}
//    	else
//    	{
//    		qDebug() << "Error while detachFromRTSA: PLG_ID::Plugin_ID " << plg_id << "is no measurement provider.";
//    	}
//    }
}




//*************************************************************************************************************

void MeasurementManager::detachWidgets(QList<PLG_ID::Plugin_ID> plg_idList)
{
    qDebug() << "MeasurementManager::detachWidgets(QList<PLG_ID::Plugin_ID> plg_idList)";

    qDebug() << "DisplayManager::getRTSAWidgets() Size: " << DisplayManager::getRTSAWidgets().size();

    foreach(RealTimeSampleArrayWidget* pRTSAW, DisplayManager::getRTSAWidgets().values())
    {
        IObserver* pRTSAWidget = dynamic_cast<IObserver*>(pRTSAW);

        detachFromRTSA(pRTSAWidget, plg_idList);
    }

    qDebug() << "DisplayManager::getRTMSAWidgets() Size: " << DisplayManager::getRTMSAWidgets().size();

    foreach(RealTimeMultiSampleArrayWidget* pRTMSAW, DisplayManager::getRTMSAWidgets().values())
    {
        IObserver* pRTMSAWidget = dynamic_cast<IObserver*>(pRTMSAW);

        detachFromRTMSA(pRTMSAWidget, plg_idList);
    }

    qDebug() << "DisplayManager::getRTMSANewWidgets() Size: " << DisplayManager::getRTMSANewWidgets().size();

    foreach(RealTimeMultiSampleArrayNewWidget* pRTMSANewW, DisplayManager::getRTMSANewWidgets().values())
    {
        IObserver* pRTMSANewWidget = dynamic_cast<IObserver*>(pRTMSANewW);

        detachFromRTMSANew(pRTMSANewWidget, plg_idList);
    }


    qDebug() << "DisplayManager::getNumericWidgets() Size: " << DisplayManager::getNumericWidgets().size();

    foreach(NumericWidget* pNUMW, DisplayManager::getNumericWidgets().values())
    {
        IObserver* pNumWidget = dynamic_cast<IObserver*>(pNUMW);

        detachFromNumeric(pNumWidget, plg_idList);
    }

}




//*************************************************************************************************************

void MeasurementManager::detachWidgets()
{
    qDebug() << "MeasurementManager::detachWidgets()";

    foreach(RealTimeSampleArrayWidget* pRTSAW, DisplayManager::getRTSAWidgets().values())
    {
        IObserver* pRTSAWidget = dynamic_cast<IObserver*>(pRTSAW);

        detachFromRTSA(pRTSAWidget);
    }

    foreach(NumericWidget* pNUMW, DisplayManager::getNumericWidgets().values())
    {
        IObserver* pNumWidget = dynamic_cast<IObserver*>(pNUMW);

        detachFromNumeric(pNumWidget);
    }

}


//*************************************************************************************************************

void MeasurementManager::clean()
{
    qDebug()<<"MeasurementManager::clean(): Cleaning up rt measurement manager";

//    foreach (IMeasurementSource* value, s_hashMeasurementProvider)
//    		delete value;
//
//    foreach (IMeasurementSink* value, s_hashMeasurementAcceptor)
//    		delete value;

    s_hashMeasurementProvider.clear();
    s_hashMeasurementAcceptor.clear();
}


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

QHash<PLG_ID::Plugin_ID, IMeasurementSource*>    MeasurementManager::s_hashMeasurementProvider;
QHash<PLG_ID::Plugin_ID, IPlugin*>    MeasurementManager::s_hashMeasurementAcceptor;
