//=============================================================================================================
/**
* @file		IMeasurementprovider.cpp
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
* @brief	Contains the implementation of the IMeasurementProvider interface.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "IMeasurementprovider.h"
#include "numeric.h"
#include "realtimesamplearray.h"
#include "realtimemultisamplearray.h"
#include "progressbar.h"
#include "text.h"
//#include "alert.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CSART;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

IMeasurementProvider::~IMeasurementProvider()
{

}


//*************************************************************************************************************

Numeric* IMeasurementProvider::addProviderNumeric(MSR_ID::Measurement_ID id)
{
	//ToDo Check if belongs to group (division with group id)
	// if id already exists push warning and return the existing channel Todo Todo
    Numeric* num = new Numeric;
    num->setID(id);
    //num->setModuleID(m_MDL_ID);
    m_hashNumeric.insert(id, num);
    return num;
}


//*************************************************************************************************************

RealTimeSampleArray* IMeasurementProvider::addProviderRealTimeSampleArray(MSR_ID::Measurement_ID id)
{
    RealTimeSampleArray* rtsa = new RealTimeSampleArray;
    rtsa->setID(id);
    //rtsa->setModuleID(m_MDL_ID);
    m_hashRealTimeSampleArray.insert(id, rtsa);
    return rtsa;
}











//*************************************************************************************************************

RealTimeMultiSampleArray* IMeasurementProvider::addProviderRealTimeMultiSampleArray(MSR_ID::Measurement_ID id, unsigned int uiNumChannels)
{
    RealTimeMultiSampleArray* rtmsa = new RealTimeMultiSampleArray(uiNumChannels);
    rtmsa->setID(id);
    //rtsa->setModuleID(m_MDL_ID);
    m_hashRealTimeMultiSampleArray.insert(id, rtmsa);
    return rtmsa;
}




//*************************************************************************************************************

ProgressBar* IMeasurementProvider::addProviderProgressBar(MSR_ID::Measurement_ID id)
{
    ProgressBar* progress = new ProgressBar;
    progress->setID(id);
    //progress->setModuleID(m_MDL_ID);
    m_hashProgressBar.insert(id, progress);
    return progress;
}


//*************************************************************************************************************

Text* IMeasurementProvider::addProviderText(MSR_ID::Measurement_ID id)
{
    Text* text = new Text;
    text->setID(id);
    //text->setModuleID(m_MDL_ID);
    m_hashText.insert(id, text);
    return text;
}


////*************************************************************************************************************
//
//Alert* IMeasurementProvider::addProviderAlert(MSR_ID::Measurement_ID id)
//{
//    Alert* alert = new Alert;
//    alert->setID(id);
//    //alert->setModuleID(m_MDL_ID);
//    m_hashAlert.insert(id, alert);
//    return alert;
//}


//*************************************************************************************************************

QList<MSR_ID::Measurement_ID> IMeasurementProvider::getProviderMeasurement_IDs() const
{
	QList<MSR_ID::Measurement_ID> idList;
	idList << getProviderNumeric_IDs();
	idList << getProviderRTSA_IDs();
	idList << getProviderProgressbar_IDs();
	idList << getProviderText_IDs();
//	idList << getProviderAlert_IDs();

    return idList;
}


//*************************************************************************************************************

QList<MSR_ID::Measurement_ID> IMeasurementProvider::getProviderNumeric_IDs() const
{
	QList<MSR_ID::Measurement_ID> idList;
	idList << m_hashNumeric.uniqueKeys();

    return idList;
}


//*************************************************************************************************************

QList<MSR_ID::Measurement_ID> IMeasurementProvider::getProviderRTSA_IDs() const
{
	QList<MSR_ID::Measurement_ID> idList;
	idList << m_hashRealTimeSampleArray.uniqueKeys();

    return idList;
}


//*************************************************************************************************************

QList<MSR_ID::Measurement_ID> IMeasurementProvider::getProviderRTMSA_IDs() const
{
    QList<MSR_ID::Measurement_ID> idList;
    idList << m_hashRealTimeMultiSampleArray.uniqueKeys();

    return idList;
}


//*************************************************************************************************************

QList<MSR_ID::Measurement_ID> IMeasurementProvider::getProviderProgressbar_IDs() const
{
	QList<MSR_ID::Measurement_ID> idList;
	idList << m_hashProgressBar.uniqueKeys();

    return idList;
}


//*************************************************************************************************************

QList<MSR_ID::Measurement_ID> IMeasurementProvider::getProviderText_IDs() const
{
	QList<MSR_ID::Measurement_ID> idList;
	idList << m_hashText.uniqueKeys();

    return idList;
}


////*************************************************************************************************************
//
//QList<MSR_ID::Measurement_ID> IMeasurementProvider::getProviderAlert_IDs() const
//{
//	QList<MSR_ID::Measurement_ID> idList;
//	idList << m_hashAlert.uniqueKeys();
//
//    return idList;
//}


//*************************************************************************************************************

void IMeasurementProvider::cleanProvider()
{
    foreach (Numeric* value, m_hashNumeric)
    		delete value;

    foreach (RealTimeSampleArray* value, m_hashRealTimeSampleArray)
    		delete value;

    foreach (RealTimeMultiSampleArray* value, m_hashRealTimeMultiSampleArray)
            delete value;

    foreach (ProgressBar* value, m_hashProgressBar)
    		delete value;

    foreach (Text* value, m_hashText)
    		delete value;

//    foreach (Alert* value, m_hashAlert)
//    		delete value;

    m_hashNumeric.clear();
    m_hashRealTimeSampleArray.clear();
    m_hashRealTimeMultiSampleArray.clear();
    m_hashProgressBar.clear();
    m_hashText.clear();
//    m_hashAlert.clear();
}
