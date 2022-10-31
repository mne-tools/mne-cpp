//=============================================================================================================
/**
 * @file     plugininputdata.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2013
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
 * @brief    Contains the declaration of the PluginInputData class.
 *
 */

#ifndef PLUGININPUTDATA_CPP //Because this cpp is part of the header -> template
#define PLUGININPUTDATA_CPP

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "plugininputdata.h"

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

template <class T>
PluginInputData<T>::PluginInputData(AbstractPlugin *parent, const QString &name, const QString &descr)
: PluginInputConnector(parent, name, descr)
, m_pFunc(NULL)
{
}

//=============================================================================================================

template <class T>
void PluginInputData<T>::setCallbackMethod(callback_function pFunc)
{
    m_pFunc = pFunc;
    connect(this, &PluginInputConnector::notify,
            this, &PluginInputData<T>::notifyCallbackFunction);
}

//=============================================================================================================

template <class T>
void PluginInputData<T>::notifyCallbackFunction(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    //qDebug() << "Here in input data.";
    if(m_pFunc)
    {
        QSharedPointer<T> measurement = pMeasurement.dynamicCast<T>();

        (*m_pFunc)(measurement);
    }
}
}//Namespace

#endif //PLUGININPUTDATA_CPP
