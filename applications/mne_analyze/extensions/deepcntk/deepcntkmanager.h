//=============================================================================================================
/**
 * @file     deepcntkmanager.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     February, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the DeepCNTKManager class.
 *
 */

#ifndef DEEPCNTKMANAGER_H
#define DEEPCNTKMANAGER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "deepcntk_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QPluginLoader>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DEEPCNTKEXTENSION
//=============================================================================================================

namespace DEEPCNTKEXTENSION
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class IDeepCNTKNet;


//=============================================================================================================
/**
 * DECLARE CLASS DeepCNTKManager
 *
 * @brief The DeepCNTKManager class provides a dynamic plugin loader. As well as the handling of the loaded extensions.
 */
class DEEPCNTKSHARED_EXPORT DeepCNTKManager : public QPluginLoader
{
    Q_OBJECT
public:
    typedef QSharedPointer<DeepCNTKManager> SPtr;               /**< Shared pointer type for DeepCNTKManager. */
    typedef QSharedPointer<const DeepCNTKManager> ConstSPtr;    /**< Const shared pointer type for DeepCNTKManager. */

    //=========================================================================================================
    /**
    * Constructs a DeepCNTKManager with the given parent.
    *
    * @param[in] parent     pointer to parent Object. (It's normally the default value.)
    */
    DeepCNTKManager(QObject* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the DeepCNTKManager.
    */
    virtual ~DeepCNTKManager();

    //=========================================================================================================
    /**
    * Loads deep configurations from given directory.
    *
    * @param [in] dir   the configuration directory.
    */
    void loadDeepConfigurations(const QString& dir);

    //=========================================================================================================
    /**
    * Initializes the deep configurations.
    */
    void initDeepConfigurations();

    //=========================================================================================================
    /**
    * Finds index of configuration by name.
    *
    * @param [in] name  the configuration name.
    *
    * @return index of extension.
    */
    int findByName(const QString& name);

    //=========================================================================================================
    /**
    * Returns vector containing all deep configurations.
    *
    * @return reference to vector containing all plugins.
    */
    inline const QVector<IDeepCNTKNet*>& getDeepConfigurations();

    //=========================================================================================================
    /**
    * Returns a string list of configuration names.
    *
    * @return Deep configuration names.
    */
    QStringList getDeepConfigurationNames() const;

    //=========================================================================================================
    /**
    * Returns the current deep configuration
    *
    * @return Current deep configuration.
    */
    IDeepCNTKNet* currentDeepConfiguration() const;

// Member functions for current configuration
public:
    //=========================================================================================================
    /**
    * Selects a deep configuration
    *
    * @param [in] idx   Idx of the configuration to select
    */
    void selectDeepConfiguration(int idx);

    //=========================================================================================================
    /**
    * Train current Configuration
    */
    void trainCurrentConfiguration();

    //=========================================================================================================
    /**
    * Train test Configuration
    */
    void evalCurrentConfiguration();

signals:
    void currentConfigurationChanged_signal();

    void finishedTraining_signal();

private:
    int m_iCurrentConfiguration;                        /**< Current configuration. */

    QVector<IDeepCNTKNet*>    m_qVecDeepConfiguration;  /**< Vector containing all deep configurations. */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QVector<IDeepCNTKNet*>& DeepCNTKManager::getDeepConfigurations()
{
    return m_qVecDeepConfiguration;
}

} // NAMESPACE

#endif // DEEPCNTKMANAGER_H
