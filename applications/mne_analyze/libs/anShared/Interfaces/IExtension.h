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
* Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
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


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QAction>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB
{


//*************************************************************************************************************
//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================



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
class IExtension
{
//    Q_OBJECT
public:
    typedef QSharedPointer<IExtension> SPtr;               /**< Shared pointer type for IExtension. */
    typedef QSharedPointer<const IExtension> ConstSPtr;    /**< Const shared pointer type for IExtension. */

    //=========================================================================================================
    /**
    * Destroys the IExtension.
    */
    virtual ~IExtension() {}

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<IExtension> clone() const = 0;

    //=========================================================================================================
    /**
    * Initializes the plugin.
    */
    virtual void init() = 0;

    //=========================================================================================================
    /**
    * Is called when plugin is detached of the stage. Can be used to safe settings.
    */
    virtual void unload() = 0;// = 0 call is not longer possible - it has to be reimplemented in child;

    //=========================================================================================================
    /**
    * Returns the plugin name.
    * Pure virtual method.
    *
    * @return the name of plugin.
    */
    virtual QString getName() const = 0;


    virtual bool hasControl() const;

    virtual QWidget* getControl() const = 0;

    virtual bool hasView() const;

    virtual QWidget* getView() const = 0;


    virtual inline void setData(QSharedPointer<AnalyzeData>& data);

    virtual inline QSharedPointer<AnalyzeData>& data();

    virtual inline void setSettings(QSharedPointer<AnalyzeSettings>& settings);

    virtual inline QSharedPointer<AnalyzeSettings>& settings();

protected:

private:
    QSharedPointer<AnalyzeData> m_analyzeData;
    QSharedPointer<AnalyzeSettings> m_analyzeSettings;

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

bool IExtension::hasControl() const
{
    return getControl() != Q_NULLPTR;
}


//*************************************************************************************************************

bool IExtension::hasView() const
{
    return getView() != Q_NULLPTR;
}


//*************************************************************************************************************

void IExtension::setData(QSharedPointer<AnalyzeData> &data)
{
    m_analyzeData = data;
}


//*************************************************************************************************************

QSharedPointer<AnalyzeData> &IExtension::data()
{
    return m_analyzeData;
}


//*************************************************************************************************************

inline void IExtension::setSettings(QSharedPointer<AnalyzeSettings> &settings)
{
    m_analyzeSettings = settings;
}


//*************************************************************************************************************

inline QSharedPointer<AnalyzeSettings> &IExtension::settings()
{
    return m_analyzeSettings;
}

} //Namespace

Q_DECLARE_INTERFACE(ANSHAREDLIB::IExtension, "ansharedlib/1.0")

#endif //IEXTENSION_H
