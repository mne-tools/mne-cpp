//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     abstractalgorithm.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains declaration of AbstractAlgorithm class.
 */

#ifndef MNESCAN_ABSTRACTALGORITHM_H
#define MNESCAN_ABSTRACTALGORITHM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "abstractplugin.h"

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
 * DECLARE CLASS AbstractAlgorithm
 *
 * @brief The AbstractAlgorithm class provides an interface for plugin.
 */
class AbstractAlgorithm : public AbstractPlugin
{
public:
    typedef QSharedPointer<AbstractAlgorithm> SPtr;               /**< Shared pointer type for AbstractAlgorithm. */
    typedef QSharedPointer<const AbstractAlgorithm> ConstSPtr;    /**< Const shared pointer type for AbstractAlgorithm. */

    //=========================================================================================================
    /**
     * Destroys the AbstractAlgorithm.
     */
    virtual ~AbstractAlgorithm() {}

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
     * Starts the AbstractAlgorithm.
     * Pure virtual method inherited by AbstractPlugin.
     *
     * @return true if success, false otherwise.
     */
    virtual bool start() = 0;

    //=========================================================================================================
    /**
     * Stops the AbstractAlgorithm.
     * Pure virtual method inherited by AbstractPlugin.
     *
     * @return true if success, false otherwise.
     */
    virtual bool stop() = 0;

    //=========================================================================================================
    /**
     * Returns the plugin type.
     * Pure virtual method inherited by AbstractPlugin.
     *
     * @return type of the AbstractAlgorithm.
     */
    virtual PluginType getType() const = 0;

    //=========================================================================================================
    /**
     * Returns the plugin name.
     * Pure virtual method inherited by AbstractPlugin.
     *
     * @return the name of the AbstractAlgorithm.
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
     * Returns the set up widget for configuration of AbstractAlgorithm.
     * Pure virtual method inherited by AbstractPlugin.
     *
     * @return the setup widget.
     */
    virtual QWidget* setupWidget() = 0; //setup();

protected:
    //=========================================================================================================
    /**
     * The starting point for the thread. After calling start(), the newly created thread calls this function.
     * Returning from this method will end the execution of the thread.
     * Pure virtual method inherited by QThread
     */
    virtual void run() = 0;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool AbstractAlgorithm::multiInstanceAllowed() const
{
    return true;
}
} // NAMESPACE

Q_DECLARE_INTERFACE(SCSHAREDLIB::AbstractAlgorithm, "scsharedlib/1.0")

#endif // MNESCAN_ABSTRACTALGORITHM_H
