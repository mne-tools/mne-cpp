//=============================================================================================================
/**
 * @file     application.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     November, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh. All rights reserved.
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
 * @brief    Application class declaration
 *
 */

#ifndef APPLICATION_H
#define APPLICATION_H

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QObject>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class MNELaunchControl;

//=============================================================================================================
/**
 * DECLARE CLASS Application
 *
 * @brief The Application class is the actual MNE Launch application.
 */
class Application : public QObject
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
     * Constructs the MNE Launch Application.
     *
     * @param[in] parent     If parent is not NULL the QObject becomes a child of QObject inside parent.
     */
    Application(QObject *parent = nullptr);

    //=========================================================================================================
    /**
     * Initializes the Application
     *
     * @param[in] argc   Number of arguments
     * @param[in] argv   Argument vector
     * @param[in] app    Gui Application
     *
     * @return The error code.
     */
    int init(int &argc, char **argv, QGuiApplication& app);

    //=========================================================================================================
    /**
     * Registers the qml meta types
     */
    void registerTypes();

    //=========================================================================================================
    /**
     * Starts the application
     */
    void start();

private:
    QPointer<QQmlApplicationEngine> m_pQMLEngine;       /**< QML Engine. */
    QPointer<MNELaunchControl>      m_pLaunchControl;   /**< The MNE Launch ViewModel. */

};

#endif // APPLICATION_H
