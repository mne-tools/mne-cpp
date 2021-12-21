//=============================================================================================================
/**
 * @file     pluginmanager.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 *           Juan Garcia-Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
 * @brief    Contains the declaration of the PluginManager class.
 *
 */

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QPluginLoader>

//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class AbstractPlugin;
class AnalyzeData;

//=============================================================================================================
/**
 * DECLARE CLASS PluginManager
 *
 * @brief The PluginManager class provides a dynamic plugin loader. As well as the handling of the loaded plugins.
 */
class ANSHAREDSHARED_EXPORT PluginManager : public QPluginLoader
{
    Q_OBJECT

public:
    typedef QSharedPointer<PluginManager> SPtr;               /**< Shared pointer type for PluginManager. */
    typedef QSharedPointer<const PluginManager> ConstSPtr;    /**< Const shared pointer type for PluginManager. */

    //=========================================================================================================
    /**
     * Constructs a PluginManager with the given parent.
     *
     * @param[in] parent pointer to parent Object. (It's normally the default value.).
     */
    PluginManager(QObject* parent = nullptr);

    //=========================================================================================================
    /**
     * Destroys the PluginManager.
     */
    virtual ~PluginManager();

    //=========================================================================================================
    /**
     * Loads plugins from given directory.
     *
     * @param[in] dir    the plugin directory.
     */
    void loadPluginsFromDirectory(const QString& dir);

    //=========================================================================================================
    /**
     * Loads a single from given absolute file path string.
     *
     * @param [in] dir    The plugin file path.
     */
    void loadPlugin(const QString& file);

    //=========================================================================================================
    /**
     * Initializes the plugins.
     *
     * @param [in] data    The global mne analyze data.
     */
    void initPlugins(QSharedPointer<AnalyzeData> data);

    //=========================================================================================================
    /**
     * Finds index of plugin by name.
     *
     * @param[in] name  the plugin name.
     *
     * @return index of plugin.
     */
    int findByName(const QString& name);

    //=========================================================================================================
    /**
     * Returns vector containing all plugins.
     *
     * @return reference to vector containing all plugins.
     */
    inline const QVector<AbstractPlugin*> getPlugins();

    //=========================================================================================================
    /**
     * This is called during main-program shutdown.
     * It simply calls the unload-method for every plugin.
     */
    void shutdown();

private:
    //=========================================================================================================
    /**
     * Insert a plugin into the vector of plugins inserted.
     */
    void insertPlugin(AbstractPlugin*  plugin);

    QVector<AbstractPlugin*>    m_qVecPlugins;       /**< Vector containing all plugins. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QVector<AbstractPlugin*> PluginManager::getPlugins()
{
    return m_qVecPlugins;
}

} // NAMESPACE

#endif // PLUGINMANAGER_H
