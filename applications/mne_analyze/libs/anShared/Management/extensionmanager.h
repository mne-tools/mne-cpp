//=============================================================================================================
/**
 * @file     extensionmanager.h
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
 * @brief    Contains the declaration of the ExtensionManager class.
 *
 */

#ifndef EXTENSIONMANAGER_H
#define EXTENSIONMANAGER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QPluginLoader>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class IExtension;
class AnalyzeSettings;
class AnalyzeData;


//=============================================================================================================
/**
 * DECLARE CLASS ExtensionManager
 *
 * @brief The ExtensionManager class provides a dynamic plugin loader. As well as the handling of the loaded extensions.
 */
class ANSHAREDSHARED_EXPORT ExtensionManager : public QPluginLoader
{
    Q_OBJECT
public:
    typedef QSharedPointer<ExtensionManager> SPtr;               /**< Shared pointer type for ExtensionManager. */
    typedef QSharedPointer<const ExtensionManager> ConstSPtr;    /**< Const shared pointer type for ExtensionManager. */

    //=========================================================================================================
    /**
    * Constructs a ExtensionManager with the given parent.
    *
    * @param[in] parent pointer to parent Object. (It's normally the default value.)
    */
    ExtensionManager(QObject* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the ExtensionManager.
    */
    virtual ~ExtensionManager();

    //=========================================================================================================
    /**
    * Loads extensions from given directory.
    *
    * @param [in] dir    the plugin directory.
    */
    void loadExtension(const QString& dir);

    //=========================================================================================================
    /**
    * Initializes the extensions.
    *
    * @param [in] settings      the global mne analyze settings
    * @param [in] data          the global mne analyze data
    */
    void initExtensions(QSharedPointer<AnalyzeSettings>& settings, QSharedPointer<AnalyzeData>& data);

    //=========================================================================================================
    /**
    * Finds index of extension by name.
    *
    * @param [in] name  the extension name.
    *
    * @return index of extension.
    */
    int findByName(const QString& name);

    //=========================================================================================================
    /**
    * Returns vector containing all extensions.
    *
    * @return reference to vector containing all extensions.
    */
    inline const QVector<IExtension*>& getExtensions();

private:
    QVector<IExtension*>    m_qVecExtensions;       /**< Vector containing all extensions. */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const QVector<IExtension*>& ExtensionManager::getExtensions()
{
    return m_qVecExtensions;
}

} // NAMESPACE

#endif // EXTENSIONMANAGER_H
