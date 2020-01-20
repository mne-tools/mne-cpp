//=============================================================================================================
/**
 * @file     mnelaunchcontrol.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     November, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    MNELaunchControl class declaration
 *
 */

#ifndef MNELAUNCHCONTROL_H
#define MNELAUNCHCONTROL_H

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QObject>
#include <QProcess>
#include <QPointer>
#include <QList>

//=============================================================================================================
/**
 * DECLARE CLASS MNELaunchControl
 *
 * @brief The MNELaunchControl class is the MNE Launch ViewModel (Control).
 */
class MNELaunchControl : public QObject
{
    Q_OBJECT

    Q_PROPERTY ( bool sampleDataAvailable READ getSampleDataAvailable NOTIFY sampleDataAvailableChanged_Signal )

public:
    //=========================================================================================================
    /**
    * Constructs the MNE Launch Application.
    *
    * @param[in] parent     If parent is not NULL the QObject becomes a child of QObject inside parent.
    */
    MNELaunchControl(QObject *parent = nullptr);

    //=========================================================================================================
    /**
    * Launches MNE Scan
    */
    Q_INVOKABLE void invokeScan();

    //=========================================================================================================
    /**
    * Launches MNE Browse
    */
    Q_INVOKABLE void invokeBrowse();

    //=========================================================================================================
    /**
    * Launches MNE Analyze
    */
    Q_INVOKABLE void invokeAnalyze();

    //=========================================================================================================
    /**
    * Invokes an application with the given arguments
    *
    * @param[in] application    The MNE application name to start.
    * @param[in] arguments      The arguments to start the MNE application with.
    */
    void invokeApplication(const QString& application, const QStringList& arguments);

    //=========================================================================================================
    /**
    * Checks whether the SampleData are available.
    *
    * @return true if the SampleData are available.
    */
    bool getSampleDataAvailable() const;

signals:
    //=========================================================================================================
    /**
    * Shall be emitted when the SampleData availability has changed.
    */
    void sampleDataAvailableChanged_Signal();

private:
    QList<QPointer<QProcess> > m_ListProcesses;  /**< List of started processes. */
    QStringList m_requiredSampleFiles;          /**< List of required sample data files. */
};

#endif // MNELAUNCHCONTROL_H
