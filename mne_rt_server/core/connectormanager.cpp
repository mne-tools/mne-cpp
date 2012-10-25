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

#include "connectormanager.h"

#include "IConnector.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QVector>
#include <QString>
#include <QStringList>
#include <QTextStream>



#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SourceConnector;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ConnectorManager::ConnectorManager(QObject *parent)
: QPluginLoader(parent)
{

}


//*************************************************************************************************************

ConnectorManager::~ConnectorManager()
{

}


//*************************************************************************************************************

void ConnectorManager::loadConnectors(const QString& dir)
{
    clearConnectorActivation();

    QDir ConnectorsDir(dir);

    printf("Loading connectors in directory... %s\n", ConnectorsDir.path().toUtf8().constData() );

    foreach(QString fileName, ConnectorsDir.entryList(QDir::Files))
    {
        this->setFileName(ConnectorsDir.absoluteFilePath(fileName));
        QObject *pConnector = this->instance();

        printf("\tLoading %s... ", fileName.toUtf8().constData() );

        // IModule
        if(pConnector)
        {
            s_vecConnectors.push_back(qobject_cast<IConnector*>(pConnector));
            printf("[done]\n");
        }
        else
        {
            printf("failed!\n");
        }
    }

    //
    // search config for default connector
    //
    qint32 configConnector = -1;
    QString configFileName("config.txt");
    QFile configFile(dir+"/../"+configFileName);
    if(!configFile.open(QIODevice::ReadOnly)) {
        printf("Not able to read config file... %s\n", configFile.fileName().toUtf8().constData());
    }
    else
    {
        printf("Reading %s... ", configFileName.toUtf8().constData());

        QTextStream in(&configFile);
        QString line = in.readLine();
        QStringList list;
        while (!line.isNull()) {
            list = line.split(":");

            if(list[0].simplified().compare("defaultConnector") == 0)
            {
                configConnector = list[1].simplified().toInt();
                break;
            }
            line = in.readLine();
        }
    }
    if(s_vecConnectors.size() > 0)
    {

        bool activated = false;

        if( configConnector != -1)
        {
            for(qint32 i = 0; i < s_vecConnectors.size(); ++i)
            {
                if(s_vecConnectors[i]->getConnectorID() == configConnector)
                {
                    s_vecConnectors[i]->setStatus(true);
                    printf("activate %s... ", s_vecConnectors[i]->getName());
                    activated = true;
                    break;
                }
            }
        }

        printf("[done]\n");

        //default
        if(!activated)
            s_vecConnectors[0]->setStatus(true);
    }

    printConnectors();
}


//*************************************************************************************************************

bool ConnectorManager::startConnector()
{
//    qDebug() << "ModuleManager::startModules()";
//    // Start ISensor and IRTAlgorithm modules first!
//    bool bFlag = startSensorModules();
//    startRTAlgorithmModules();
//    startRTVisualizationModules();
//    startRTRecordModules();
//    startAlertModules();
//    //ToDo other Modules

    return true;//bFlag;
}


//*************************************************************************************************************

void ConnectorManager::stopConnector()
{
//    // Stop ISensor modules first!
//    qDebug() << "Try stopping sensor modules.";
//    QVector<IModule*>::const_iterator it = s_vecModules.begin();
//    for( ; it != s_vecModules.end(); ++it)
//    {
//        if((*it)->isActive())
//        {
//            if((*it)->getType() == _ISensor)
//            {
//                if(!(*it)->stop())
//                {
//                    qDebug() << "Could not stop IModule: " << (*it)->getName();
//                }
//            }
//        }
//    }

//    // Stop all other modules!
//    qDebug() << "Try stopping all other modules";
//    it = s_vecModules.begin();
//    for( ; it != s_vecModules.end(); ++it)
//    {
//        if((*it)->isActive())
//        {
//            if((*it)->getType() != _ISensor)
//            {
//                if(!(*it)->stop())
//                {
//                    qDebug() << "Could not stop IModule: " << (*it)->getName();
//                }
//            }
//        }
//    }
}


//*************************************************************************************************************

int ConnectorManager::findByName(const QString& name)
{
//    QVector<IModule*>::const_iterator it = s_vecModules.begin();
//    for(int i = 0; it != s_vecModules.end(); ++i, ++it)
//        if((*it)->getName() == name)
//            return i;

    return -1;
}


//*************************************************************************************************************

const void ConnectorManager::printConnectors()
{
    // deactivate already loaded connectors
    if(s_vecConnectors.size() > 0)
    {
        printf("Connector list\n");
        QVector<IConnector*>::const_iterator it = s_vecConnectors.begin();
        for( ; it != s_vecConnectors.end(); ++it)
        {
            if((*it)->isActive())
                printf("  *  (%d) %s\n", (*it)->getConnectorID(), (*it)->getName());
            else
                printf("     (%d) %s\n", (*it)->getConnectorID(), (*it)->getName());
        }
    }
    else
        printf(" - no connector loaded - \n");
}


//*************************************************************************************************************

IConnector* ConnectorManager::getActiveConnector()
{
    QVector<IConnector*>::const_iterator it = s_vecConnectors.begin();
    for( ; it != s_vecConnectors.end(); ++it)
    {
        if((*it)->isActive())
            return *it;
    }

    return NULL;
}



void ConnectorManager::clearConnectorActivation()
{
    // deactivate activated connectors
    if(s_vecConnectors.size() > 0)
    {
        QVector<IConnector*>::const_iterator it = s_vecConnectors.begin();
        for( ; it != s_vecConnectors.end(); ++it)
            if((*it)->isActive())
                (*it)->setStatus(false);
    }
}



//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

QVector<IConnector*>    ConnectorManager::  s_vecConnectors;
