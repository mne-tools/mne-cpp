//=============================================================================================================
/**
* @file     rerefoption.h
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Viktor Klüber and Matti Hamalainen. All rights reserved.
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
* @brief     ReRefOption class declaration.
*
*/

#ifndef REREFOPTION_REREFOPTION_H
#define REREFOPTION_REREFOPTION_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_rerefoption.h"
#include "reref.h"
#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QWidget>


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
// DEFINE NAMESPACE REREFOPTION
//=============================================================================================================

namespace REREFPLUGIN{


//*************************************************************************************************************
//=============================================================================================================
// REREFOPTION FORWARD DECLARATIONS
//=============================================================================================================

class ReRef;


//=============================================================================================================
/**
* The ReRefOption class provides several feature configurations possibilities for the ReRef project.
*
* @brief Brief description of this class.
*/

class ReRefOption : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<ReRefOption> SPtr;            /**< Shared pointer type for ReRefOption. */
    typedef QSharedPointer<const ReRefOption> ConstSPtr; /**< Const shared pointer type for ReRefOption. */

    //=========================================================================================================
    /**
    * Constructs a ReRefOption object.
    */
    explicit ReRefOption(ReRef *pReRef, QWidget *parent = 0);

public slots:
    //=========================================================================================================
    /**
    * updates the channels and sets them to the QListWidget
    */
    void updateChannels(FIFFLIB::FiffInfo::SPtr &pFiffInfo);

private:

    Ui::ReRefOptionWidget          *ui;     /**< Holds the user interface for the ReRefSetupWidget.*/

    QSharedPointer<ReRef>           m_pReRef;       /**< pointer to the upper ReRef class. */


};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace REREFOPTION

#endif // REREFOPTION_REREFOPTION_H
