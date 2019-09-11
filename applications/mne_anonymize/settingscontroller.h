//=============================================================================================================
/**
* @file     settingscontroller.h
* @author   Juan Garcia-Prieto <Juan.GarciaPrieto@uth.tmc.edu> <juangpc@gmail.com>;
*           Wayne Mead <wayne.mead@uth.tmc.edu> <wayne.isk@gmail.com>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           John C. Mosher <John.C.Mosher@uth.tmc.edu> <jcmosher@gmail.com>;
* @version  1.0
* @date     September, 2019
*
* @section  LICENSE
*
* Copyright (C) 2019, Juan Garcia-Prieto and Matti Hamalainen. All rights reserved.
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
* @brief     SettingsController class declaration.
*
*/

#ifndef FIFFANONYMIZER_SETTINGSCONTROLLER_H
#define FIFFANONYMIZER_SETTINGSCONTROLLER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffanonymizer.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QDir>
#include <QtConcurrent>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEANONYMIZE
//=============================================================================================================

namespace MNEANONYMIZE {


//*************************************************************************************************************
//=============================================================================================================
// MNEANONYMIZE FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Description of what this class is intended to do (in detail).
*
* @brief Brief description of this class.
*/
class SettingsController
{

public:
    typedef QSharedPointer<SettingsController> SPtr;            /**< Shared pointer type for SettingsController. */
    typedef QSharedPointer<const SettingsController> ConstSPtr; /**< Const shared pointer type for SettingsController. */

    //=========================================================================================================
    /**
    * Constructs a SettingsController object.
    */
    SettingsController(QCoreApplication* qtApp);
    ~SettingsController();
    void generateAnonymizerInstances();
    void execute();

private:
    void initParser();
    void parseInputs();
    void parseInputAndOutputFiles();
    //we at least create one app
    //if we later see we have more than one file we can create
    //more apps and deploy concurrent execution (one app per thread).
    FiffAnonymizer m_anonymizer;
    QStringList m_slInFiles;
    QStringList m_slOutFiles;
    bool m_bMultipleInFiles;
    QList<QSharedPointer<FiffAnonymizer> > m_pAppList;

    QCoreApplication * m_pQCoreApp;
    QCommandLineParser m_parser;

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace MNEANONYMIZE

#endif // FIFFANONYMIZER_SETTINGSCONTROLLER_H
