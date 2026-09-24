//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     plugininputconnector.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Definition of the PluginInputConnector class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "plugininputconnector.h"
#include "../Plugins/abstractplugin.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginInputConnector::PluginInputConnector(AbstractPlugin *parent,
                                           const QString &name,
                                           const QString &descr)
: PluginConnector(parent, name, descr)
{
}

//=============================================================================================================

bool PluginInputConnector::isInputConnector() const
{
    return true;
}

//=============================================================================================================

bool PluginInputConnector::isOutputConnector() const
{
    return false;
}

//=============================================================================================================

void PluginInputConnector::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    emit notify(pMeasurement);
}
