//=============================================================================================================
/**
 * @file     pluginmanager.h
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
 * @brief    Contains the declaration of the PluginManager class.
 *
 */

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../scshared_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QPluginLoader>
#include <QColor>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNESCAN
{
    class MainWindow;
}

//=============================================================================================================
// DEFINE NAMESPACE SCSHAREDLIB
//=============================================================================================================

namespace SCSHAREDLIB
{

//=============================================================================================================
// SCSHAREDLIB FORWARD DECLARATIONS
//=============================================================================================================

class AbstractPlugin;
class AbstractSensor;
class AbstractAlgorithm;
class IIO;

//=============================================================================================================
/**
 * DECLARE CLASS PluginManager
 *
 * @brief The PluginManager class provides a dynamic plugin loader. As well as the handling of the loaded plugins.
 */
class SCSHAREDSHARED_EXPORT PluginManager : public QPluginLoader
{
    Q_OBJECT

    friend class MNESCAN::MainWindow;
    friend class PluginDockWidget;

public:
    typedef QSharedPointer<PluginManager> SPtr;               /**< Shared pointer type for PluginManager. */
    typedef QSharedPointer<const PluginManager> ConstSPtr;    /**< Const shared pointer type for PluginManager. */

    //=========================================================================================================
    /**
     * Constructs a PluginManager with the given parent.
     *
     * @param[in] parent    pointer to parent Object. (It's normally the default value.).
     */
    PluginManager(QObject* parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destroys the PluginManager.
     */
    virtual ~PluginManager();

    //=========================================================================================================
    /**
     * Loads plugins from given directory.
     *
     * @param[in] dir   the plugin directory.
     */
    void loadPlugins(const QString& dir);

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
    inline const QVector<AbstractPlugin*>& getPlugins();

    //=========================================================================================================
    /**
     * Returns vector containing AbstractSensor plugins.
     *
     * @return reference to vector containing AbstractSensor plugins.
     */
    inline const QVector<AbstractSensor*>& getSensorPlugins();

    //=========================================================================================================
    /**
     * Returns vector containing AbstractAlgorithm plugins
     *
     * @return reference to vector containing IRTAlgorithm plugins.
     */
    inline const QVector<AbstractAlgorithm*>& getAlgorithmPlugins();

    //=========================================================================================================
    /**
     * Returns vector containing IIO plugins
     *
     * @return reference to vector containing IRTVisulaiztaion plugins.
     */
    inline const QVector<IIO*>& getIOPlugins();

signals:
    void pluginLoaded(const QString &msg, int alignment = Qt::AlignLeft, const QColor &color = Qt::black );

private:
    QVector<AbstractPlugin*>    m_qVecPlugins;             /**< Vector of all plugins. */

    QVector<AbstractSensor*>    m_qVecSensorPlugins;       /**< Vector of all AbstractSensor plugins. */
    QVector<AbstractAlgorithm*> m_qVecAlgorithmPlugins;    /**< Vector of all AbstractAlgorithm plugins. */
    QVector<IIO*>        m_qVecIOPlugins;           /**< Vector of all IIO plugins. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QVector<AbstractPlugin*>& PluginManager::getPlugins()
{
    return m_qVecPlugins;
}

//=============================================================================================================

inline const QVector<AbstractSensor*>& PluginManager::getSensorPlugins()
{
    return m_qVecSensorPlugins;
}

//=============================================================================================================

inline const QVector<AbstractAlgorithm*>& PluginManager::getAlgorithmPlugins()
{
    return m_qVecAlgorithmPlugins;
}

//=============================================================================================================

inline const QVector<IIO*>& PluginManager::getIOPlugins()
{
    return m_qVecIOPlugins;
}
} // NAMESPACE

#endif // PLUGINMANAGER_H
