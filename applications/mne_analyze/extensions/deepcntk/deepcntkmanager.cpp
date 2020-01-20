//=============================================================================================================
/**
 * @file     deepcntkmanager.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the DeepCNTKManager class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "deepcntkmanager.h"
#include "IDeepCNTKNet.h"


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

using namespace DEEPCNTKEXTENSION;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DeepCNTKManager::DeepCNTKManager(QObject *parent)
: QPluginLoader(parent)
, m_iCurrentConfiguration(0)
{

}


//*************************************************************************************************************

DeepCNTKManager::~DeepCNTKManager()
{
    for(int i = 0; i < m_qVecDeepConfiguration.size(); ++i) {
        delete m_qVecDeepConfiguration[i];
    }
}


//*************************************************************************************************************

void DeepCNTKManager::loadDeepConfigurations(const QString& dir)
{
    QDir deepConfigurationsDir(dir);

    foreach(QString file, deepConfigurationsDir.entryList(QDir::Files))
    {
        fprintf(stderr,"Loading deep configuration %s... ",file.toUtf8().constData());

        this->setFileName(deepConfigurationsDir.absoluteFilePath(file));
        QObject *pDeepConfiguration = this->instance();

        // IExtension
        if(pDeepConfiguration) {
            fprintf(stderr,"Deep configuration %s loaded.\n",file.toUtf8().constData());
            m_qVecDeepConfiguration.push_back(qobject_cast<IDeepCNTKNet*>(pDeepConfiguration));
        }
        else {
            fprintf(stderr,"Deep configuration %s could not be instantiated!\n",file.toUtf8().constData());
        }
    }
}


//*************************************************************************************************************

void DeepCNTKManager::initDeepConfigurations()
{
    foreach(IDeepCNTKNet* deepConfiguration, m_qVecDeepConfiguration)
    {
        deepConfiguration->init();
    }
}


//*************************************************************************************************************

int DeepCNTKManager::findByName(const QString& name)
{
    QVector<IDeepCNTKNet*>::const_iterator it = m_qVecDeepConfiguration.begin();
    for(int i = 0; it != m_qVecDeepConfiguration.end(); ++i, ++it)
        if((*it)->getName() == name)
            return i;

    return -1;
}


//*************************************************************************************************************

QStringList DeepCNTKManager::getDeepConfigurationNames() const
{
    QStringList names;

    foreach(IDeepCNTKNet* deepConfiguration, m_qVecDeepConfiguration)
    {
        names << deepConfiguration->getName();
    }

    return names;
}


//*************************************************************************************************************

IDeepCNTKNet *DeepCNTKManager::currentDeepConfiguration() const
{
    if(m_qVecDeepConfiguration.size() > 0) {
        return m_qVecDeepConfiguration[m_iCurrentConfiguration];
    }

    return Q_NULLPTR;
}


//*************************************************************************************************************

void DeepCNTKManager::selectDeepConfiguration(int idx)
{
    if(idx != m_iCurrentConfiguration && idx >= 0 && idx < m_qVecDeepConfiguration.size()) {
        m_iCurrentConfiguration = idx;
        emit currentConfigurationChanged_signal();
        qDebug() << "void DeepCNTKManager::selectDeepConfiguration(int idx)" << idx;
    }

}


//*************************************************************************************************************

void DeepCNTKManager::trainCurrentConfiguration()
{
    if (m_iCurrentConfiguration < m_qVecDeepConfiguration.size()) {
        m_qVecDeepConfiguration[m_iCurrentConfiguration]->train();
        emit finishedTraining_signal();
    }
}


//*************************************************************************************************************

void DeepCNTKManager::evalCurrentConfiguration()
{
    if (m_iCurrentConfiguration < m_qVecDeepConfiguration.size()) {
        m_qVecDeepConfiguration[m_iCurrentConfiguration]->eval();
    }
}
