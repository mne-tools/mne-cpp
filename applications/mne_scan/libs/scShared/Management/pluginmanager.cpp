//=============================================================================================================
/**
 * @file     pluginmanager.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of the PluginManager class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginmanager.h"

#include "../Interfaces/IPlugin.h"
#include "../Interfaces/ISensor.h"
#include "../Interfaces/IAlgorithm.h"
#include "../Interfaces/IIO.h"

#ifdef STATICLIB
#include <../plugins/fiffsimulator/fiffsimulator.h>
#include <../plugins/neuromag/neuromag.h>
#include <../plugins/babymeg/babymeg.h>
#include <../plugins/natus/natus.h>
#include <../plugins/ftbuffer/ftbuffer.h>
//#include <../plugins/gusbamp/gusbamp.h>
//#include <../plugins/eegosports/eegosports.h>
//#include <../plugins/brainamp/brainamp.h>
//#include <../plugins/tmsi/tmsi.h>
//#include <../plugins/lsladapter/lsladapter.h>
#include <../plugins/dummytoolbox/dummytoolbox.h>
#include <../plugins/rtcmne/rtcmne.h>
#include <../plugins/rtcmusic/rtcmusic.h>
#include <../plugins/averaging/averaging.h>
#include <../plugins/covariance/covariance.h>
#include <../plugins/noisereduction/noisereduction.h>
#include <../plugins/ssvepbci/ssvepbci.h>
#include <../plugins/neuronalconnectivity/neuronalconnectivity.h>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginManager::PluginManager(QObject *parent)
: QPluginLoader(parent)
{
}

//=============================================================================================================

PluginManager::~PluginManager()
{
}

//=============================================================================================================

void PluginManager::loadPlugins(const QString& dir)
{
#ifdef STATICLIB
    Q_UNUSED(dir);

    // In case of a static build we have to load plugins manually.
    QList<QObject*> lObjects;
    lObjects << new FIFFSIMULATORPLUGIN::FiffSimulator;
    lObjects << new NEUROMAGPLUGIN::Neuromag;
    lObjects << new BABYMEGPLUGIN::BabyMEG;
    lObjects << new NATUSPLUGIN::Natus;
    lObjects << new FTBUFFERPLUGIN::FtBuffer;
    //lObjects << new GUSBAMPPLUGIN::GUSBAmp;
    //lObjects << new EEGOSPORTSPLUGIN::EEGoSports;
    //lObjects << new BRAINAMPPLUGIN::BrainAMP;
    //lObjects << new TMSIPLUGIN::TMSI;
    //lObjects << new LSLADAPTERPLUGIN::LSLAdapter;
    lObjects << new DUMMYTOOLBOXPLUGIN::DummyToolbox;
    lObjects << new RTCMNEPLUGIN::RtcMne;
    lObjects << new RTCMUSICPLUGIN::RtcMusic;
    lObjects << new AVERAGINGPLUGIN::Averaging;
    lObjects << new COVARIANCEPLUGIN::Covariance;
    lObjects << new NOISEREDUCTIONPLUGIN::NoiseReduction;
    lObjects << new SSVEPBCIPLUGIN::SsvepBci;
    lObjects << new NEURONALCONNECTIVITYPLUGIN::NeuronalConnectivity;

    for(int i = 0; i < lObjects.size(); ++i) {
        // IPlugin
        if(lObjects[i]) {
            // plugins are always disabled when they are first loaded
            m_qVecPlugins.push_back(qobject_cast<IPlugin*>(lObjects[i]));

            IPlugin::PluginType pluginType = qobject_cast<IPlugin*>(lObjects[i])->getType();

            // ISensor
            if(pluginType == IPlugin::_ISensor) {
                ISensor* pSensor = qobject_cast<ISensor*>(lObjects[i]);
                if(pSensor) {
                    m_qVecSensorPlugins.push_back(pSensor);
                    qDebug() << "Sensor " << pSensor->getName() << " loaded.";
                }
            }
            // IAlgorithm
            else if(pluginType == IPlugin::_IAlgorithm) {
                IAlgorithm* pAlgorithm = qobject_cast<IAlgorithm*>(lObjects[i]);
                if(pAlgorithm) {
                    m_qVecAlgorithmPlugins.push_back(pAlgorithm);
                    qDebug() << "RTAlgorithm " << pAlgorithm->getName() << " loaded.";
                }
            }
            // IIO
            else if(pluginType == IPlugin::_IIO) {
                IIO* pIO = qobject_cast<IIO*>(lObjects[i]);
                if(pIO) {
                    m_qVecIOPlugins.push_back(pIO);
                    qDebug() << "RTVisualization " << pIO->getName() << " loaded.";
                }
            }
        } else {
            qDebug() << "Plugin could not be instantiated!";
        }
    }
#else
    QDir PluginsDir(dir);

    foreach(QString file, PluginsDir.entryList(QDir::Files))
    {
        this->setFileName(PluginsDir.absoluteFilePath(file));
        QObject *pPlugin = this->instance();

        // IPlugin
        if(pPlugin)
        {
            qDebug() << "Try to load Plugin " << file;

            // plugins are always disabled when they are first loaded
            m_qVecPlugins.push_back(qobject_cast<IPlugin*>(pPlugin));

            IPlugin::PluginType pluginType = qobject_cast<IPlugin*>(pPlugin)->getType();

            // ISensor
            if(pluginType == IPlugin::_ISensor)
            {
                ISensor* pSensor = qobject_cast<ISensor*>(pPlugin);
                if(pSensor)
                {
                    m_qVecSensorPlugins.push_back(pSensor);
                    qDebug() << "Sensor " << pSensor->getName() << " loaded.";
                }
            }
            // IAlgorithm
            else if(pluginType == IPlugin::_IAlgorithm)
            {
                IAlgorithm* pAlgorithm = qobject_cast<IAlgorithm*>(pPlugin);
                if(pAlgorithm)
                {
                    m_qVecAlgorithmPlugins.push_back(pAlgorithm);
                    qDebug() << "RTAlgorithm " << pAlgorithm->getName() << " loaded.";
                }
            }
            // IIO
            else if(pluginType == IPlugin::_IIO)
            {
                IIO* pIO = qobject_cast<IIO*>(pPlugin);
                if(pIO)
                {
                    m_qVecIOPlugins.push_back(pIO);
                    qDebug() << "RTVisualization " << pIO->getName() << " loaded.";
                }
            }

            //ToDo other Plugins - like Visualization

        }
//        else
//            qDebug() << "Plugin " << file << " could not be instantiated!";
    }
#endif
}

//=============================================================================================================

int PluginManager::findByName(const QString& name)
{
    QVector<IPlugin*>::const_iterator it = m_qVecPlugins.begin();
    for(int i = 0; it != m_qVecPlugins.end(); ++i, ++it)
        if((*it)->getName() == name)
            return i;

    return -1;
}
