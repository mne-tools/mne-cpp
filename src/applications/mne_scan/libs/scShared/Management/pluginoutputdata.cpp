//=============================================================================================================
/**
 * @file     pluginoutputdata.cpp
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
 * @brief    Contains the declaration of the PluginOutputData class.
 *
 */

#ifndef PLUGINOUTPUTDATA_CPP //Because this cpp is part of the header -> template
#define PLUGINOUTPUTDATA_CPP

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginoutputdata.h"

#include <scMeas/measurement.h>

#include <QDebug>
#include <QSharedPointer>

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
PluginOutputData<T>::PluginOutputData(AbstractPlugin *parent,
                                      const QString &name,
                                      const QString &descr)
: PluginOutputConnector(parent, name, descr)
{
    m_pMeasurement = QSharedPointer<T>(new T);

    QSharedPointer<SCMEASLIB::Measurement> t_measurement = qSharedPointerDynamicCast<SCMEASLIB::Measurement>(m_pMeasurement);

    if(t_measurement.isNull())
        qFatal("Template type is not a measurement and therefor not supported!");
    else
        connect(t_measurement.data(), &SCMEASLIB::Measurement::notify,
                this, &PluginOutputData<T>::update, Qt::DirectConnection);
}

//=============================================================================================================

template <class T>
void PluginOutputData<T>::update()
{
    emit notify(qSharedPointerDynamicCast<SCMEASLIB::Measurement>(m_pMeasurement));
}
}//Namespace

#endif //PLUGINOUTPUTDATA_CPP
