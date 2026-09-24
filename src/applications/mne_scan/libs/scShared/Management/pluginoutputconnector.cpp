//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     pluginoutputconnector.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Contains the declaration of the PluginOutputConnector class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginoutputconnector.h"
#include "../Plugins/abstractplugin.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginOutputConnector::PluginOutputConnector(AbstractPlugin *parent,
                                             const QString &name,
                                             const QString &descr)
: PluginConnector(parent, name, descr)
{
}

//=============================================================================================================

bool PluginOutputConnector::isInputConnector() const
{
    return false;
}

//=============================================================================================================

bool PluginOutputConnector::isOutputConnector() const
{
    return true;
}

