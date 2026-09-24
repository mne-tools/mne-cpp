//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     plugininputdata.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Contains the declaration of the PluginInputData class.
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
