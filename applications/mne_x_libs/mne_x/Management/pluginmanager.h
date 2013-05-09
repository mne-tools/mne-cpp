//=============================================================================================================
/**
* @file     pluginmanager.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the declaration of the PluginManager class.
*
*/

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include "../mne_x_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QPluginLoader>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEX
//=============================================================================================================

namespace MNEX
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class IPlugin;
class ISensor;
class IRTAlgorithm;
class IRTVisualization;
class IRTRecord;
class IAlert;


//=============================================================================================================
/**
* DECLARE CLASS PluginManager
*
* @brief The PluginManager class provides a dynamic plugin loader. As well as the handling of the loaded plugins.
*/
class MNE_X_SHARED_EXPORT PluginManager : public QPluginLoader
{
    Q_OBJECT

    friend class MainWindow;
    friend class PluginDockWidget;

public:
    typedef QSharedPointer<PluginManager> SPtr;               /**< Shared pointer type for PluginManager. */
    typedef QSharedPointer<const PluginManager> ConstSPtr;    /**< Const shared pointer type for PluginManager. */

    //=========================================================================================================
    /**
    * Constructs a PluginManager with the given parent.
    *
    * @param [in] parent pointer to parent Object. (It's normally the default value.)
    */
    PluginManager(QObject* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the PluginManager.
    */
    virtual ~PluginManager();

    //=========================================================================================================
    /**
    * Loads plugins from given directory.
    *
    * @param dir the plugin directory.
    */
    void loadPlugins(const QString& dir);

    //=========================================================================================================
    /**
    * Starts all plugins.
    *
    * @return true if at least one ISensor plugin was started successfully, false otherwise.
    */
    static bool startPlugins();

    //=========================================================================================================
    /**
    * Starts ISensor Plugins
    *
    * @return true if at least one ISensor plugin was started successfully, false otherwise.
    */
    static bool startSensorPlugins();

    //=========================================================================================================
    /**
    * Starts IRTAlgorithm plugins.
    */
    static void startRTAlgorithmPlugins();

    //=========================================================================================================
    /**
    * Starts IRTVisualization plugins.
    */
    static void startRTVisualizationPlugins();

    //=========================================================================================================
    /**
    * Starts IRTRecord plugins.
    */
    static void startRTRecordPlugins();

    //=========================================================================================================
    /**
    * Starts IAlert plugins.
    */
    static void startAlertPlugins();


    //=========================================================================================================
    /**
    * Stops all plugins.
    */
    static void stopPlugins();

    //=========================================================================================================
    /**
    * Finds index of plugin by name.
    *
    * @return index of plugin.
    * @param name the plugin name.
    */
    static int findByName(const QString& name);

    //=========================================================================================================
    /**
    * Returns vector containing all plugins.
    *
    * @return reference to vector containing all plugins.
    */
    static inline const QVector<IPlugin*>& getPlugins();

    //=========================================================================================================
    /**
    * Returns vector containing ISensor plugins.
    *
    * @return reference to vector containing ISensor plugins.
    */
    static inline const QVector<ISensor*>& getSensorPlugins();

    //=========================================================================================================
    /**
    * Returns vector containing IRTAlgorithm plugins
    *
    * @return reference to vector containing IRTAlgorithm plugins
    */
    static inline const QVector<IRTAlgorithm*>& getRTAlgorithmPlugins();

    //=========================================================================================================
    /**
    * Returns vector containing IRTVisualization plugins
    *
    * @return reference to vector containing IRTVisulaiztaion plugins
    */
    static inline const QVector<IRTVisualization*>& getRTVisualizationPlugins();

    //=========================================================================================================
    /**
    * Returns vector containing IRTRecord plugins.
    *
    * @return reference to vector containing IRTRecord plugins
    */
    static inline const QVector<IRTRecord*>& getRTRecordPlugins();

    //=========================================================================================================
    /**
    * Returns vector containing IAlert plugins.
    *
    * @return reference to vector containing IAlert plugins.
    */
    static inline const QVector<IAlert*>& getAlertPlugins();

    //=========================================================================================================
    /**
    * Returns vector containing active ISensor plugins.
    *
    * @return reference to vector containing active ISensor plugins.
    */
    static inline const QVector<ISensor*>& getActiveSensorPlugins();

    //=========================================================================================================
    /**
    * Returns vector containing active IRTAlgorithm plugins.
    *
    * @return reference to vector containing active IAlgorithm plugins.
    */
    static inline const QVector<IRTAlgorithm*>& getActiveRTAlgorithmPlugins();

    //=========================================================================================================
    /**
    * Returns vector containing active IRTAlgorithm plugins.
    *
    * @return reference to vector containing active IAlgorithm plugins.
    */
    static inline const QVector<IRTVisualization*>& getActiveRTVisualizationPlugins();

    //=========================================================================================================
    /**
    * Returns vector containing active IRTRecord plugins.
    *
    * @return reference to vector containing active IRTRecord plugins.
    */
    static inline const QVector<IRTRecord*>& getActiveRTRecordPlugins();

    //=========================================================================================================
    /**
    * Returns vector containing active IAlert plugins.
    *
    * @return reference to vector containing active IAlert plugins.
    */
    static inline const QVector<IAlert*>& getActiveAlertPlugins();

private:

    static QVector<IPlugin*>            s_vecPlugins;               /**< Holds vector of all plugins. */

    static QVector<ISensor*>            s_vecSensorPlugins;         /**< Holds vector of all ISensor plugins. */
    static QVector<IRTAlgorithm*>       s_vecRTAlgorithmPlugins;    /**< Holds vector of all IRTAlgorithm plugins. */
    static QVector<IRTVisualization*>   s_vecRTVisualizationPlugins;/**< Holds vector of all IRTVisualization plugins. */
    static QVector<IRTRecord*>          s_vecRTRecordPlugins;       /**< Holds vector of all IRTRecord plugins. */
    static QVector<IAlert*>             s_vecAlertPlugins;          /**< Holds vector of all IAlert plugins. */

    static QVector<ISensor*>            s_vecActiveSensorPlugins;           /**< Holds vector of all active ISensor plugins. */
    static QVector<IRTAlgorithm*>       s_vecActiveRTAlgorithmPlugins;      /**< Holds vector of all active IRTAlgorithm plugins. */
    static QVector<IRTVisualization*>   s_vecActiveRTVisualizationPlugins;  /**< Holds vector of all active IRTVisualization plugins. */
    static QVector<IRTRecord*>          s_vecActiveRTRecordPlugins;         /**< Holds vector of all active IRTRecord plugins. */
    static QVector<IAlert*>             s_vecActiveAlertPlugins;            /**< Holds vector of all active IAlert plugins. */

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QVector<IPlugin*>& PluginManager::getPlugins()
{
    return s_vecPlugins;
}


//*************************************************************************************************************

inline const QVector<ISensor*>& PluginManager::getSensorPlugins()
{
    return s_vecSensorPlugins;
}


//*************************************************************************************************************

inline const QVector<IRTAlgorithm*>& PluginManager::getRTAlgorithmPlugins()
{
    return s_vecRTAlgorithmPlugins;
}


//*************************************************************************************************************

inline const QVector<IRTVisualization*>& PluginManager::getRTVisualizationPlugins()
{
    return s_vecRTVisualizationPlugins;
}


//*************************************************************************************************************

inline const QVector<IRTRecord*>& PluginManager::getRTRecordPlugins()
{
    return s_vecRTRecordPlugins;
}


//*************************************************************************************************************

inline const QVector<IAlert*>& PluginManager::getAlertPlugins()
{
    return s_vecAlertPlugins;
}















//*************************************************************************************************************

inline const QVector<ISensor*>& PluginManager::getActiveSensorPlugins()
{
    return s_vecActiveSensorPlugins;
}


//*************************************************************************************************************

inline const QVector<IRTAlgorithm*>& PluginManager::getActiveRTAlgorithmPlugins()
{
    return s_vecActiveRTAlgorithmPlugins;
}



//*************************************************************************************************************

inline const QVector<IRTVisualization*>& PluginManager::getActiveRTVisualizationPlugins()
{
    return s_vecActiveRTVisualizationPlugins;
}


//*************************************************************************************************************

inline const QVector<IRTRecord*>& PluginManager::getActiveRTRecordPlugins()
{
    return s_vecActiveRTRecordPlugins;
}


//*************************************************************************************************************

inline const QVector<IAlert*>& PluginManager::getActiveAlertPlugins()
{
    return s_vecActiveAlertPlugins;
}

} // NAMESPACE

#endif // PLUGINMANAGER_H
