//=============================================================================================================
/**
* @file     deepconfigurationmanager.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the ExtensionManager class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "deepconfigurationmanager.h"
#include "IDeepConfiguration.h"


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

using namespace DEEPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DeepConfigurationManager::DeepConfigurationManager(QObject *parent)
: QPluginLoader(parent)
{

}


//*************************************************************************************************************

DeepConfigurationManager::~DeepConfigurationManager()
{
    for(int i = 0; i < m_qVecDeepConfiguration.size(); ++i) {
        delete m_qVecDeepConfiguration[i];
    }
}


//*************************************************************************************************************

void DeepConfigurationManager::loadDeepConfigurations(const QString& dir)
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
            m_qVecDeepConfiguration.push_back(qobject_cast<IDeepConfiguration*>(pDeepConfiguration));
        }
        else {
            fprintf(stderr,"Deep configuration %s could not be instantiated!\n",file.toUtf8().constData());
        }
    }
}


//*************************************************************************************************************

void DeepConfigurationManager::initDeepConfigurations()
{
    foreach(IDeepConfiguration* deepConfiguration, m_qVecDeepConfiguration)
    {
        deepConfiguration->init();
    }
}


//*************************************************************************************************************

int DeepConfigurationManager::findByName(const QString& name)
{
    QVector<IDeepConfiguration*>::const_iterator it = m_qVecDeepConfiguration.begin();
    for(int i = 0; it != m_qVecDeepConfiguration.end(); ++i, ++it)
        if((*it)->getName() == name)
            return i;

    return -1;
}
