//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     pluginconnector.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the PluginConnector class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginconnector.h"
#include "../Plugins/abstractplugin.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginConnector::PluginConnector(AbstractPlugin *parent, const QString &name, const QString &descr)
: QObject(parent)
, m_pPlugin(parent)
, m_sName(name)
, m_sDescription(descr)
{
}
