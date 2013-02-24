//=============================================================================================================
/**
* @file	   	modulemanager.cpp
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
* @brief	Contains the implementation of the ModuleManager class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "modulemanager.h"

#include <rtDtMng/rtmeasurementmanager.h>

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

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace RTDTMNGLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ModuleManager::ModuleManager(QObject *parent)
: QPluginLoader(parent)
{

}


//*************************************************************************************************************

ModuleManager::~ModuleManager()
{

}


//*************************************************************************************************************

void ModuleManager::loadModules(const QString& dir)
{
    QDir ModulesDir(dir);

    foreach(QString fileName, ModulesDir.entryList(QDir::Files))
    {
        this->setFileName(ModulesDir.absoluteFilePath(fileName));
        QObject *pModule = this->instance();

        // IModule
        if(pModule)
        {
        	qDebug() << "try to load Module " << fileName;

            s_vecModules.push_back(qobject_cast<IModule*>(pModule));

            Type module_type = qobject_cast<IModule*>(pModule)->getType();

            // ISensor
            if(module_type == _ISensor)
            {
                ISensor* pSensor = qobject_cast<ISensor*>(pModule);
                if(pSensor)
                {
                    s_vecSensorModules.push_back(pSensor);
                    qDebug() << "Sensor " << pSensor->getName() << " loaded.";

                    RTMeasurementManager::addMeasurementProvider(pSensor);
                }
            }
            // IRTRecord
            else if(module_type == _IRTRecord)
            {
            	IRTRecord* pRTRecord = qobject_cast<IRTRecord*>(pModule);
                if(pRTRecord)
                {
                    s_vecRTRecordModules.push_back(pRTRecord);
                    qDebug() << "RTRecord " << pRTRecord->getName() << " loaded.";
                }
            }
            // IAlgorithm
            else if(module_type == _IRTAlgorithm)
            {
            	IRTAlgorithm* pRTAlgorithm = qobject_cast<IRTAlgorithm*>(pModule);
                if(pRTAlgorithm)
                {
                    s_vecRTAlgorithmModules.push_back(pRTAlgorithm);
                    qDebug() << "RTAlgorithm " << pRTAlgorithm->getName() << " loaded.";

                    RTMeasurementManager::addMeasurementProvider(pRTAlgorithm);
                    RTMeasurementManager::addMeasurementAcceptor(pRTAlgorithm);
                }
            }
            // IAlgorithm
            else if(module_type == _IRTVisualization)
            {
                IRTVisualization* pRTVisualization = qobject_cast<IRTVisualization*>(pModule);
                if(pRTVisualization)
                {
                    s_vecRTVisualizationModules.push_back(pRTVisualization);
                    qDebug() << "RTVisualization " << pRTVisualization->getName() << " loaded.";

                    RTMeasurementManager::addMeasurementProvider(pRTVisualization);
                    RTMeasurementManager::addMeasurementAcceptor(pRTVisualization);
                }
            }
            // IAlert
            else if(module_type == _IAlert)
            {
                IAlert* pAlert = qobject_cast<IAlert*>(pModule);
                if(pAlert)
                {
                    s_vecAlertModules.push_back(pAlert);
                    qDebug() << "Alert " << pAlert->getName() << " loaded.";
                }
            }

            //ToDo other Modules - like Visualization

        }
        else
        {
            qDebug() << "Module " << fileName << " could not be instantiated!";
        }
    }

}


//*************************************************************************************************************

bool ModuleManager::startModules()
{
	qDebug() << "ModuleManager::startModules()";
    // Start ISensor and IRTAlgorithm modules first!
    bool bFlag = startSensorModules();
    startRTAlgorithmModules();
    startRTVisualizationModules();
    startRTRecordModules();
    startAlertModules();
    //ToDo other Modules

    return bFlag;
}


//*************************************************************************************************************

bool ModuleManager::startSensorModules()
{
    bool bFlag = false;

    QVector<ISensor*>::const_iterator it = s_vecSensorModules.begin();
    for( ; it != s_vecSensorModules.end(); ++it)
    {

    	qDebug() << "ISensor: " << (*it)->getName() << "; Active " << (*it)->isActive();
        if((*it)->isActive())
        {
            if(!(*it)->start())
            {
                qDebug() << "Could not start ISensor: " << (*it)->getName();
            }

            else
            {
                // ISensor
                if((*it)->getType() == _ISensor)
                {
                    bFlag = true;
                    s_vecActiveSensorModules.push_back(qobject_cast<ISensor*>(*it));
                }
            }
        }
    }

    return bFlag;
}


//*************************************************************************************************************

void ModuleManager::startRTAlgorithmModules()
{
    QVector<IRTAlgorithm*>::const_iterator it = s_vecRTAlgorithmModules.begin();
    for( ; it != s_vecRTAlgorithmModules.end(); ++it)
    {
        if((*it)->isActive())
        {
            if(!(*it)->start())
            {
                qDebug() << "Could not start IAlgorithm: " << (*it)->getName();
            }

            else
            {
                // IRTAlgorithm
                if((*it)->getType() == _IRTAlgorithm)
                {
                    s_vecActiveRTAlgorithmModules.push_back(qobject_cast<IRTAlgorithm*>(*it));
                }
            }
        }
    }
}


//*************************************************************************************************************

void ModuleManager::startRTVisualizationModules()
{
    QVector<IRTVisualization*>::const_iterator it = s_vecRTVisualizationModules.begin();
    for( ; it != s_vecRTVisualizationModules.end(); ++it)
    {
        if((*it)->isActive())
        {
            if(!(*it)->start())
            {
                qDebug() << "Could not start IVisualization: " << (*it)->getName();
            }

            else
            {
                // IRTVisualization
                if((*it)->getType() == _IRTVisualization)
                {
                    s_vecActiveRTVisualizationModules.push_back(qobject_cast<IRTVisualization*>(*it));
                }
            }
        }
    }
}


//*************************************************************************************************************

void ModuleManager::startRTRecordModules()
{
	QVector<IRTRecord*>::const_iterator it = s_vecRTRecordModules.begin();
    for( ; it != s_vecRTRecordModules.end(); ++it)
    {
        if((*it)->isActive())
        {
            if(!(*it)->start())
            {
                qDebug() << "Could not start IPersistence: " << (*it)->getName();
            }

            else
            {
                // IRTRecord
                if((*it)->getType() == _IRTRecord)
                {
                    s_vecActiveRTRecordModules.push_back(qobject_cast<IRTRecord*>(*it));
                }
            }
        }
    }
}


//*************************************************************************************************************

void ModuleManager::startAlertModules()
{
	QVector<IAlert*>::const_iterator it = s_vecAlertModules.begin();
    for( ; it != s_vecAlertModules.end(); ++it)
    {
        if((*it)->isActive())
        {
            if(!(*it)->start())
            {
                qDebug() << "Could not start IAlert: " << (*it)->getName();
            }

            else
            {
                // IAlert
                if((*it)->getType() == _IAlert)
                {
                    s_vecActiveAlertModules.push_back(qobject_cast<IAlert*>(*it));
                }
            }
        }
    }
}


//*************************************************************************************************************

void ModuleManager::stopModules()
{
    // Stop ISensor modules first!
	qDebug() << "Try stopping sensor modules.";
	QVector<IModule*>::const_iterator it = s_vecModules.begin();
    for( ; it != s_vecModules.end(); ++it)
    {
        if((*it)->isActive())
        {
            if((*it)->getType() == _ISensor)
            {
                if(!(*it)->stop())
                {
                    qDebug() << "Could not stop IModule: " << (*it)->getName();
                }
            }
        }
    }

    // Stop all other modules!
	qDebug() << "Try stopping all other modules";
    it = s_vecModules.begin();
    for( ; it != s_vecModules.end(); ++it)
    {
        if((*it)->isActive())
        {
            if((*it)->getType() != _ISensor)
            {
                if(!(*it)->stop())
                {
                    qDebug() << "Could not stop IModule: " << (*it)->getName();
                }
            }
        }
    }

    s_vecActiveSensorModules.clear();
    s_vecActiveRTRecordModules.clear();
    s_vecActiveRTAlgorithmModules.clear();
    s_vecActiveRTVisualizationModules.clear();
    s_vecActiveAlertModules.clear();
}


//*************************************************************************************************************

int ModuleManager::findByName(const QString& name)
{
	QVector<IModule*>::const_iterator it = s_vecModules.begin();
    for(int i = 0; it != s_vecModules.end(); ++i, ++it)
        if((*it)->getName() == name)
            return i;

    return -1;
}


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

QVector<IModule*> ModuleManager::       s_vecModules;

QVector<ISensor*> ModuleManager::       s_vecSensorModules;
QVector<IRTRecord*> ModuleManager::		s_vecRTRecordModules;
QVector<IRTAlgorithm*> ModuleManager::  s_vecRTAlgorithmModules;
QVector<IRTVisualization*> ModuleManager::s_vecRTVisualizationModules;
QVector<IAlert*> ModuleManager::        s_vecAlertModules;

QVector<ISensor*> ModuleManager::       s_vecActiveSensorModules;
QVector<IRTRecord*> ModuleManager::  	s_vecActiveRTRecordModules;
QVector<IRTAlgorithm*> ModuleManager::  s_vecActiveRTAlgorithmModules;
QVector<IRTVisualization*> ModuleManager::s_vecActiveRTVisualizationModules;
QVector<IAlert*> ModuleManager::        s_vecActiveAlertModules;
