//=============================================================================================================
/**
 * @file     AbstractSensor.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh. All rights reserved.
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
 * @brief    Contains declaration of AbstractSensor class.
 *
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
     * Returns whether a plugin has a GUI component.
     */
    virtual inline bool hasGUI() const;

    //=========================================================================================================
    /**
     * Whether the plugin can be added to a scene.
     */
    virtual bool inline isScenePlugin() const;

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

//=============================================================================================================

inline bool AbstractSensor::hasGUI() const
{
    return true;
}

//=============================================================================================================

inline bool AbstractSensor::isScenePlugin() const
{
    return true;
}

} //NAMESPACE

Q_DECLARE_INTERFACE(SCSHAREDLIB::AbstractSensor, "scsharedlib/1.0")

#endif // MNESCAN_ABSTRACTSENSOR_H
