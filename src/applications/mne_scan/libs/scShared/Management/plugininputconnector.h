//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     plugininputconnector.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Contains the declaration of the PluginConnector class.
 */
#ifndef PLUGININPUTCONNECTOR_H
#define PLUGININPUTCONNECTOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../scshared_global.h"

#include "pluginconnector.h"

#include <scMeas/measurement.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

//=============================================================================================================
/**
 * Base class to connect plug-in data streams.
 *
 * @brief The PluginConnector class provides the base to connect plug-in data
 */
class SCSHAREDSHARED_EXPORT PluginInputConnector : public PluginConnector
{
    Q_OBJECT

public:
    typedef QSharedPointer<PluginInputConnector> SPtr;               /**< Shared pointer type for PluginInputConnector. */
    typedef QSharedPointer<const PluginInputConnector> ConstSPtr;    /**< Const shared pointer type for PluginInputConnector. */

    //=========================================================================================================
    /**
     * Constructs a PluginInputConnector with the given parent.
     *
     * @param[in] parent     pointer to parent plugin.
     * @param[in] name       connection name.
     * @param[in] descr      connection description.
     */
    PluginInputConnector(AbstractPlugin *parent,
                         const QString &name,
                         const QString &descr);

    //=========================================================================================================
    /**
     * Destructor
     */
    virtual ~PluginInputConnector(){}

    //=========================================================================================================
    /**
     * Returns true.
     *
     * @return true.
     */
    virtual bool isInputConnector() const;

    //=========================================================================================================
    /**
     * Returns false.
     *
     * @return false.
     */
    virtual bool isOutputConnector() const;

signals:
    void notify(SCMEASLIB::Measurement::SPtr pMeasurement);

public slots:
    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

};
} // NAMESPACE

#endif // PLUGININPUTCONNECTOR_H
