//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     abstractsensor.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains declaration of AbstractSensor class.
 */

#ifndef MNESCAN_ABSTRACTSENSOR_H
#define MNESCAN_ABSTRACTSENSOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "abstractplugin.h"

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

//=============================================================================================================
/**
 * DECLARE CLASS IRTAlgorithm
 *
 * @brief The AbstractSensor class provides an interface for a sensor plugin.
 */
class AbstractSensor : public AbstractPlugin
{
public:

    //=========================================================================================================
    /**
     * Destroys the AbstractSensor.
     */
    virtual ~AbstractSensor() {}

    //=========================================================================================================
    /**
     * Clone the plugin
     */
    virtual QSharedPointer<AbstractPlugin> clone() const = 0;

    //=========================================================================================================
    /**
     * Initializes the plugin.
     */
    virtual void init() = 0;

    //=========================================================================================================
    /**
     * Is called when plugin is detached of the stage. Can be used to safe settings.
     */
    virtual void unload() = 0;

    //=========================================================================================================
    /**
     * Starts the AbstractSensor.
     * Pure virtual method inherited by IModule.
     *
     * @return true if success, false otherwise.
     */
    virtual bool start() = 0;

    //=========================================================================================================
    /**
     * Stops the AbstractSensor.
     * Pure virtual method inherited by IModule.
     *
     * @return true if success, false otherwise.
     */
    virtual bool stop() = 0;

    //=========================================================================================================
    /**
     * Returns the plugin type.
     * Pure virtual method inherited by IModule.
     *
     * @return type of the AbstractSensor.
     */
    virtual PluginType getType() const = 0;

    //=========================================================================================================
    /**
     * Returns the plugin name.
     * Pure virtual method inherited by IModule.
     *
     * @return the name of the AbstractSensor.
     */
    virtual QString getName() const = 0;

    //=========================================================================================================
    /**
     * True if multi instantiation of plugin is allowed.
     *
     * @return true if multi instantiation of plugin is allowed.
     */
    virtual inline bool multiInstanceAllowed() const;

    //=========================================================================================================
    /**
     * Returns the set up widget for configuration of AbstractSensor.
     * Pure virtual method inherited by IModule.
     *
     * @return the setup widget.
     */
    virtual QWidget* setupWidget() = 0;

protected:

    //=========================================================================================================
    /**
     * The starting point for the thread. After calling start(), the newly created thread calls this function.
     * Returning from this method will end the execution of the thread.
     * Pure virtual method inherited by QThread.
     */
    virtual void run() = 0;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool AbstractSensor::multiInstanceAllowed() const
{
    return false;
}
} //NAMESPACE

Q_DECLARE_INTERFACE(SCSHAREDLIB::AbstractSensor, "scsharedlib/1.0")

#endif // MNESCAN_ABSTRACTSENSOR_H
