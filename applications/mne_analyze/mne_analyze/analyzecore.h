//=============================================================================================================
/**
* @file     analyzecore.h
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Simon Heinke <simon.heinke@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lars Debor, Simon Heinke and Matti Hamalainen. All rights reserved.
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
* @brief     AnalyzeCore class declaration.
*
*/

#ifndef MNEANALYZE_ANALYZECORE_H
#define MNEANALYZE_ANALYZECORE_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainwindow.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB
{
    class IExtension;
    class ExtensionManager;
    class AnalyzeData;
    class AnalyzeSettings;
}

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEANALYZE
//=============================================================================================================

namespace MNEANALYZE {


//*************************************************************************************************************
//=============================================================================================================
// MNEANALYZE FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* The AnalyzeCore class holds all of MNE-Analyze components.
* It is the entry point for starting MNE-Analyze.
*
* @brief The AnalyzeCore holds all of MNE-Analyze components.
*/
class AnalyzeCore : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<AnalyzeCore> SPtr;            /**< Shared pointer type for AnalyzeCore. */
    typedef QSharedPointer<const AnalyzeCore> ConstSPtr; /**< Const shared pointer type for AnalyzeCore. */

    //=========================================================================================================
    /**
    * Constructs an AnalyzeCore object.
    */
    AnalyzeCore(QObject* parent = nullptr);

    //=========================================================================================================
    /**
    * Destructs an AnalyzeCore object.
    */
    ~AnalyzeCore();

    //=========================================================================================================
    /**
    * This makes the main window visible.
    */
    void showMainWindow();

    //=========================================================================================================
    /**
    * Returns a pointer to the main window.
    *
    * @return A pointer to the main window.
    */
    inline QPointer<MainWindow> getMainWindow() const;

    //=========================================================================================================
    /**
    * Initializes the global settings
    */
    void initGlobalSettings();

    //=========================================================================================================
    /**
    * Initializes the global data base
    */
    void initGlobalData();

protected:

private slots:

    //=========================================================================================================
    /**
    * This is executed when the user presses "close" button (via QConnection from MainWindow)
    */
    void onMainWindowClosed();

private:

    //=========================================================================================================
    /**
    * This initializes the EventSystem.
    */
    void initEventSystem();

    //=========================================================================================================
    /**
    * This initializes the ExtensionManager.
    */
    void initExtensionManager();

    //=========================================================================================================
    /**
    * This initializes the MainWindow.
    */
    void initMainWindow();

    //=========================================================================================================
    /**
    * This function call qRegisterMetatype() on all custom types that are used in QObject::connect() calls.
    */
    void registerMetaTypes();



    QSharedPointer<ANSHAREDLIB::ExtensionManager>   m_pExtensionManager;    /**< Holds extension manager. */
    QPointer<MainWindow>                            m_pMainWindow;          /**< The main window. */
    QSharedPointer<ANSHAREDLIB::AnalyzeSettings>    m_analyzeSettings;      /**< The global settings. */
    QSharedPointer<ANSHAREDLIB::AnalyzeData>        m_analyzeData;          /**< The global data base. */

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

QPointer<MainWindow> AnalyzeCore::getMainWindow() const {
    return m_pMainWindow;
}

} // namespace MNEANALYZE

#endif // MNEANALYZE_ANALYZECORE_H
