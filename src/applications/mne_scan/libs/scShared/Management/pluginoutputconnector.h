//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     pluginoutputconnector.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Contains the declaration of the PluginOutputConnector class.
 */
#ifndef PLUGINOUTPUTCONNECTOR_H
#define PLUGINOUTPUTCONNECTOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../scshared_global.h"

#include "pluginconnector.h"
#include <scMeas/measurement.h>

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

//=============================================================================================================
/**
 * Class to connect plug-in data streams.
 *
 * @brief The PluginConnector class provides the base to connect plug-in data
 */
class SCSHAREDSHARED_EXPORT PluginOutputConnector : public PluginConnector
{
    Q_OBJECT

public:
    typedef QSharedPointer<PluginOutputConnector> SPtr;               /**< Shared pointer type for PluginOutputConnector. */
    typedef QSharedPointer<const PluginOutputConnector> ConstSPtr;    /**< Const shared pointer type for PluginOutputConnector. */

    //=========================================================================================================
    /**
     * Constructs a PluginOutputConnector with the given parent.
     *
     * @param[in] parent     pointer to parent plugin.
     * @param[in] name       connection name.
     * @param[in] descr      connection description.
     */
    PluginOutputConnector(AbstractPlugin *parent,
                          const QString &name,
                          const QString &descr);

    //=========================================================================================================
    /**
     * Destructor
     */
    virtual ~PluginOutputConnector(){}

    //=========================================================================================================
    /**
     * Returns false
     *
     * @return false.
     */
    virtual bool isInputConnector() const;

    //=========================================================================================================
    /**
     * Returns true
     *
     * @return true.
     */
    virtual bool isOutputConnector() const;

signals:
    void notify(SCMEASLIB::Measurement::SPtr);
};
} // NAMESPACE

#endif // PLUGINOUTPUTCONNECTOR_H
