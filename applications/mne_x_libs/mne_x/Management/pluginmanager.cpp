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
#include "../Interfaces/IAlgorithm.h"
#include "../Interfaces/IIO.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QDebug>


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

            // plugins are always disabled when they are first loaded
            qobject_cast<IPlugin*>(pPlugin)->setStatus(false);

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
            // IAlgorithm
            else if(module_type == _IAlgorithm)
            {
                IAlgorithm* pAlgorithm = qobject_cast<IAlgorithm*>(pPlugin);
                if(pAlgorithm)
                {
                    s_vecAlgorithmPlugins.push_back(pAlgorithm);
                    qDebug() << "RTAlgorithm " << pAlgorithm->getName() << " loaded.";

                    MeasurementManager::addMeasurementProvider(pAlgorithm);
                    MeasurementManager::addMeasurementAcceptor(pAlgorithm);
                }
            }
            // IIO
            else if(module_type == _IIO)
            {
                IIO* pIO = qobject_cast<IIO*>(pPlugin);
                if(pIO)
                {
                    s_vecIOPlugins.push_back(pIO);
                    qDebug() << "RTVisualization " << pIO->getName() << " loaded.";

                    MeasurementManager::addMeasurementAcceptor(pIO);
                }
            }

            //ToDo other Plugins - like Visualization

        }
        else
        {
            qDebug() << "Module " << fileName << " could not be instantiated!";
        }
    }

}


//*************************************************************************************************************

bool PluginManager::startPlugins()
{
    qDebug() << "PluginManager::startPlugins()";
    // Start ISensor and IRTAlgorithm plugins first!
    bool bFlag = startSensorPlugins();
    startAlgorithmPlugins();
    startIOPlugins();
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

void PluginManager::startAlgorithmPlugins()
{
    QVector<IAlgorithm*>::const_iterator it = s_vecAlgorithmPlugins.begin();
    for( ; it != s_vecAlgorithmPlugins.end(); ++it)
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
                if((*it)->getType() == _IAlgorithm)
                {
                    s_vecActiveAlgorithmPlugins.push_back(qobject_cast<IAlgorithm*>(*it));
                }
            }
        }
    }
}


//*************************************************************************************************************

void PluginManager::startIOPlugins()
{
    QVector<IIO*>::const_iterator it = s_vecIOPlugins.begin();
    for( ; it != s_vecIOPlugins.end(); ++it)
    {
        if((*it)->isActive())
        {
            if(!(*it)->start())
            {
                qDebug() << "Could not start IIO: " << (*it)->getName();
            }

            else
            {
                // IRTVisualization
                if((*it)->getType() == _IIO)
                {
                    s_vecActiveIOPlugins.push_back(qobject_cast<IIO*>(*it));
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
    s_vecActiveAlgorithmPlugins.clear();
    s_vecActiveIOPlugins.clear();
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
QVector<IAlgorithm*> PluginManager::    s_vecAlgorithmPlugins;
QVector<IIO*> PluginManager::           s_vecIOPlugins;

QVector<ISensor*> PluginManager::       s_vecActiveSensorPlugins;
QVector<IAlgorithm*> PluginManager::    s_vecActiveAlgorithmPlugins;
QVector<IIO*> PluginManager::           s_vecActiveIOPlugins;
