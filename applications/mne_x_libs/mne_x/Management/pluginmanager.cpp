//=============================================================================================================
/**
* @file     pluginmanager.cpp
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
* @brief    Contains the implementation of the PluginManager class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginmanager.h"

#include <xDtMng/measurementmanager.h>

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
using namespace XDTMNGLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginManager::PluginManager(QObject *parent)
: QPluginLoader(parent)
{

}


//*************************************************************************************************************

PluginManager::~PluginManager()
{

}


//*************************************************************************************************************

void PluginManager::loadPlugins(const QString& dir)
{
    QDir PluginsDir(dir);

    foreach(QString fileName, PluginsDir.entryList(QDir::Files))
    {
        this->setFileName(PluginsDir.absoluteFilePath(fileName));
        QObject *pPlugin = this->instance();

        // IPlugin
        if(pPlugin)
        {
            qDebug() << "try to load Plugin " << fileName;

            s_vecPlugins.push_back(qobject_cast<IPlugin*>(pPlugin));

            Type module_type = qobject_cast<IPlugin*>(pPlugin)->getType();

            // ISensor
            if(module_type == _ISensor)
            {
                ISensor* pSensor = qobject_cast<ISensor*>(pPlugin);
                if(pSensor)
                {
                    s_vecSensorPlugins.push_back(pSensor);
                    qDebug() << "Sensor " << pSensor->getName() << " loaded.";

                    MeasurementManager::addMeasurementProvider(pSensor);
                }
            }
            // IRTRecord
            else if(module_type == _IRTRecord)
            {
                IRTRecord* pRTRecord = qobject_cast<IRTRecord*>(pPlugin);
                if(pRTRecord)
                {
                    s_vecRTRecordPlugins.push_back(pRTRecord);
                    qDebug() << "RTRecord " << pRTRecord->getName() << " loaded.";
                }
            }
            // IAlgorithm
            else if(module_type == _IRTAlgorithm)
            {
                IRTAlgorithm* pRTAlgorithm = qobject_cast<IRTAlgorithm*>(pPlugin);
                if(pRTAlgorithm)
                {
                    s_vecRTAlgorithmPlugins.push_back(pRTAlgorithm);
                    qDebug() << "RTAlgorithm " << pRTAlgorithm->getName() << " loaded.";

                    MeasurementManager::addMeasurementProvider(pRTAlgorithm);
                    MeasurementManager::addMeasurementAcceptor(pRTAlgorithm);
                }
            }
            // IAlgorithm
            else if(module_type == _IRTVisualization)
            {
                IRTVisualization* pRTVisualization = qobject_cast<IRTVisualization*>(pPlugin);
                if(pRTVisualization)
                {
                    s_vecRTVisualizationPlugins.push_back(pRTVisualization);
                    qDebug() << "RTVisualization " << pRTVisualization->getName() << " loaded.";

                    MeasurementManager::addMeasurementProvider(pRTVisualization);
                    MeasurementManager::addMeasurementAcceptor(pRTVisualization);
                }
            }
            // IAlert
            else if(module_type == _IAlert)
            {
                IAlert* pAlert = qobject_cast<IAlert*>(pPlugin);
                if(pAlert)
                {
                    s_vecAlertPlugins.push_back(pAlert);
                    qDebug() << "Alert " << pAlert->getName() << " loaded.";
                }
            }

            //ToDo other Plugins - like Visualization

        }
        else
        {
         //   qDebug() << "Module " << fileName << " could not be instantiated!";
        }
    }

}


//*************************************************************************************************************

bool PluginManager::startPlugins()
{
    qDebug() << "PluginManager::startPlugins()";
    // Start ISensor and IRTAlgorithm plugins first!
    bool bFlag = startSensorPlugins();
    startRTAlgorithmPlugins();
    startRTVisualizationPlugins();
    startRTRecordPlugins();
    startAlertPlugins();
    //ToDo other Plugins

    return bFlag;
}


//*************************************************************************************************************

bool PluginManager::startSensorPlugins()
{
    bool bFlag = false;

    QVector<ISensor*>::const_iterator it = s_vecSensorPlugins.begin();
    for( ; it != s_vecSensorPlugins.end(); ++it)
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
                    s_vecActiveSensorPlugins.push_back(qobject_cast<ISensor*>(*it));
                }
            }
        }
    }

    return bFlag;
}


//*************************************************************************************************************

void PluginManager::startRTAlgorithmPlugins()
{
    QVector<IRTAlgorithm*>::const_iterator it = s_vecRTAlgorithmPlugins.begin();
    for( ; it != s_vecRTAlgorithmPlugins.end(); ++it)
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
                    s_vecActiveRTAlgorithmPlugins.push_back(qobject_cast<IRTAlgorithm*>(*it));
                }
            }
        }
    }
}


//*************************************************************************************************************

void PluginManager::startRTVisualizationPlugins()
{
    QVector<IRTVisualization*>::const_iterator it = s_vecRTVisualizationPlugins.begin();
    for( ; it != s_vecRTVisualizationPlugins.end(); ++it)
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
                    s_vecActiveRTVisualizationPlugins.push_back(qobject_cast<IRTVisualization*>(*it));
                }
            }
        }
    }
}


//*************************************************************************************************************

void PluginManager::startRTRecordPlugins()
{
    QVector<IRTRecord*>::const_iterator it = s_vecRTRecordPlugins.begin();
    for( ; it != s_vecRTRecordPlugins.end(); ++it)
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
                    s_vecActiveRTRecordPlugins.push_back(qobject_cast<IRTRecord*>(*it));
                }
            }
        }
    }
}


//*************************************************************************************************************

void PluginManager::startAlertPlugins()
{
    QVector<IAlert*>::const_iterator it = s_vecAlertPlugins.begin();
    for( ; it != s_vecAlertPlugins.end(); ++it)
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
                    s_vecActiveAlertPlugins.push_back(qobject_cast<IAlert*>(*it));
                }
            }
        }
    }
}


//*************************************************************************************************************

void PluginManager::stopPlugins()
{
    // Stop ISensor plugins first!
    qDebug() << "Try stopping sensor Plugins.";
    QVector<IPlugin*>::const_iterator it = s_vecPlugins.begin();
    for( ; it != s_vecPlugins.end(); ++it)
    {
        if((*it)->isActive())
        {
            if((*it)->getType() == _ISensor)
            {
                if(!(*it)->stop())
                {
                    qDebug() << "Could not stop IPlugin: " << (*it)->getName();
                }
            }
        }
    }

    // Stop all other plugins!
    qDebug() << "Try stopping all other plugins";
    it = s_vecPlugins.begin();
    for( ; it != s_vecPlugins.end(); ++it)
    {
        if((*it)->isActive())
        {
            if((*it)->getType() != _ISensor)
            {
                if(!(*it)->stop())
                {
                    qDebug() << "Could not stop IPlugin: " << (*it)->getName();
                }
            }
        }
    }

    s_vecActiveSensorPlugins.clear();
    s_vecActiveRTRecordPlugins.clear();
    s_vecActiveRTAlgorithmPlugins.clear();
    s_vecActiveRTVisualizationPlugins.clear();
    s_vecActiveAlertPlugins.clear();
}


//*************************************************************************************************************

int PluginManager::findByName(const QString& name)
{
    QVector<IPlugin*>::const_iterator it = s_vecPlugins.begin();
    for(int i = 0; it != s_vecPlugins.end(); ++i, ++it)
        if((*it)->getName() == name)
            return i;

    return -1;
}


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

QVector<IPlugin*> PluginManager::       s_vecPlugins;

QVector<ISensor*> PluginManager::       s_vecSensorPlugins;
QVector<IRTRecord*> PluginManager::		s_vecRTRecordPlugins;
QVector<IRTAlgorithm*> PluginManager::  s_vecRTAlgorithmPlugins;
QVector<IRTVisualization*> PluginManager::s_vecRTVisualizationPlugins;
QVector<IAlert*> PluginManager::        s_vecAlertPlugins;

QVector<ISensor*> PluginManager::       s_vecActiveSensorPlugins;
QVector<IRTRecord*> PluginManager::  	s_vecActiveRTRecordPlugins;
QVector<IRTAlgorithm*> PluginManager::  s_vecActiveRTAlgorithmPlugins;
QVector<IRTVisualization*> PluginManager::s_vecActiveRTVisualizationPlugins;
QVector<IAlert*> PluginManager::        s_vecActiveAlertPlugins;
