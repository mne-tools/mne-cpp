//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     pluginoutputdata.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Contains the declaration of the PluginOutputData class.
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
