//=============================================================================================================
/**
* @file     IExtension.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017 Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains declaration of IExtension interface class.
*
*/

#ifndef IEXTENSION_H
#define IEXTENSION_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QMenu>
#include <QDockWidget>
#include <QSharedPointer>


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

class AnalyzeData;
class AnalyzeSettings;


//=========================================================================================================
/**
* DECLARE CLASS IExtension
*
* @brief The IExtension class is the base interface class for all extensions.
*/
class ANSHAREDSHARED_EXPORT IExtension : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<IExtension> SPtr;               /**< Shared pointer type for IExtension. */
    typedef QSharedPointer<const IExtension> ConstSPtr;    /**< Const shared pointer type for IExtension. */

    //=========================================================================================================
    /**
    * Destroys the extension.
    */
    virtual ~IExtension() {}

    //=========================================================================================================
    /**
    * Clone the extension
    */
    virtual QSharedPointer<IExtension> clone() const = 0;

    //=========================================================================================================
    /**
    * Initializes the extension.
    */
    virtual void init() = 0;

    //=========================================================================================================
    /**
    * Is called when extension unloaded.
    */
    virtual void unload() = 0;

    //=========================================================================================================
    /**
    * Returns the plugin name.
    * Pure virtual method.
    *
    * @return the name of plugin.
    */
    virtual QString getName() const = 0;

    //=========================================================================================================
    /**
    * Returns if extension provides its own menu
    *
    * @return true if provides menu, false otherwise
    */
    virtual bool hasMenu() const = 0;

    //=========================================================================================================
    /**
    * Provides the menu, in case no menu is provided it returns a Q_NULLPTR
    *
    * @return the menu
    */
    virtual QMenu* getMenu() = 0;

    //=========================================================================================================
    /**
    * Returns if extension provides its own control
    *
    * @return true if provides control, false otherwise
    */
    virtual bool hasControl() const = 0;

    //=========================================================================================================
    /**
    * Provides the control, in case no control is provided it returns a Q_NULLPTR
    *
    * @return the control
    */
    virtual QDockWidget* getControl() = 0;

    //=========================================================================================================
    /**
    * Returns if extension provides its own view
    *
    * @return true if provides control, false otherwise
    */
    virtual bool hasView() const = 0;

    //=========================================================================================================
    /**
    * Provides the view, in case no view is provided it returns a Q_NULLPTR
    *
    * @return the view
    */
    virtual QWidget* getView() = 0;

    //=========================================================================================================
    /**
    * Sets the global data, which provides the central database
    *
    * @param [in] globalData  the global data
    */
    virtual inline void setGlobalData(QSharedPointer<AnalyzeData>& globalData);

    //=========================================================================================================
    /**
    * Returns the global data base
    *
    * @return the global data
    */
    virtual inline QSharedPointer<AnalyzeData>& globalData();

    //=========================================================================================================
    /**
    * Sets the global settings, which provides the mne analyze settings
    *
    * @param [in] globalSettings  the global settings
    */
    virtual inline void setGlobalSettings(QSharedPointer<AnalyzeSettings>& globalSettings);

    //=========================================================================================================
    /**
    * Returns the global settings base
    *
    * @return the global settings
    */
    virtual inline QSharedPointer<AnalyzeSettings>& globalSettings();

private:
    QSharedPointer<AnalyzeData> m_analyzeData;              /**< Pointer to the global data base */
    QSharedPointer<AnalyzeSettings> m_analyzeSettings;      /**< Pointer to the global analyze settings */

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

void IExtension::setGlobalData(QSharedPointer<AnalyzeData> &data)
{
    m_analyzeData = data;
}


//*************************************************************************************************************

QSharedPointer<AnalyzeData> &IExtension::globalData()
{
    return m_analyzeData;
}


//*************************************************************************************************************

void IExtension::setGlobalSettings(QSharedPointer<AnalyzeSettings> &settings)
{
    m_analyzeSettings = settings;
}


//*************************************************************************************************************

QSharedPointer<AnalyzeSettings> &IExtension::globalSettings()
{
    return m_analyzeSettings;
}

} //Namespace

Q_DECLARE_INTERFACE(ANSHAREDLIB::IExtension, "ansharedlib/1.0")

#endif //IEXTENSION_H
