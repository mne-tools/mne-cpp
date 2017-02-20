//=============================================================================================================
/**
* @file     eegref.h
* @author   Viktor Klüber <v.klueber@gmx.net>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Viktor Klüber, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief     EEGRef class declaration.
*
*/

#ifndef EEGREF_H
#define EEGREF_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"
#include <fiff/fiff_info.h>
#include <utils/ioutils.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Dense>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB {


//=============================================================================================================
/**
* This class provides transformations for EEG reference operations.
*
* @brief This class provides transformations for EEG reference operations.
*/

class UTILSSHARED_EXPORT EEGRef
{

public:
    typedef QSharedPointer<EEGRef> SPtr;            /**< Shared pointer type for EEGRef. */
    typedef QSharedPointer<const EEGRef> ConstSPtr; /**< Const shared pointer type for EEGRef. */

    //=========================================================================================================
    /**
    * Constructs a EEGRef object.
    */
    EEGRef();

    //=========================================================================================================
    /**
    * transforms the EEG data matrix with indifferent electrode reference to an EEG data matrix with common average reference. Bad channels are set to zero.
    *
    * @param[in] matIER         EEG data matrix with indifferent electrode reference
    * @param[in] pFiffInfo      pointer to the corresponding Fiff-Info of the EEG data stream
    *
    * @return EEG data matrix with common average reference
    */
    static Eigen::MatrixXd applyCAR(Eigen::MatrixXd& matIER, FIFFLIB::FiffInfo::SPtr &pFiffInfo);


};


} // namespace UTILSLIB

#endif // EEGREF_H
