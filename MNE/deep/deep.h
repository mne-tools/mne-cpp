//=============================================================================================================
/**
* @file     deep.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
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
* @brief    Mne Deep class declaration.
*
*/

#ifndef DEEP_H
#define DEEP_H

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "deep_global.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DEEPLIB
//=============================================================================================================

namespace DEEPLIB
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
* MNE Deep cntk wrapper descritpion
*
* @brief MNE Deep cntk wrapper to evaluate pretrained models
*/
class DEEPSHARED_EXPORT Deep
{
public:
    typedef QSharedPointer<Deep> SPtr;            /**< Shared pointer type for Deep. */
    typedef QSharedPointer<const Deep> ConstSPtr; /**< Const shared pointer type for Deep. */

    //=========================================================================================================
    /**
    * Default constructor
    */
    Deep();

    //=========================================================================================================
    /**
    * Constructor
    *
    * @param [in] sModelFilename    The model filename to set
    */
    Deep(const QString &sModelFilename);

    //=========================================================================================================
    /**
    * Destructs Deep
    */
    virtual ~Deep();


    //=========================================================================================================
    /**
    * Returns the current set model file name
    *
    * @return the current model file name
    */
    const QString& getModelFilename() const;

    //=========================================================================================================
    /**
    * Set the model filename
    *
    * @param [in] sModelFilename    The model filename to set
    */
    void setModelFilename(const QString &sModelFilename);

    //=========================================================================================================
    /**
    * Evaluate the MNE Deep Model set by the model file name
    *
    * @return true when MNE Deep model was sucessfully evaluated.
    */
    bool evalModel();

private:
    QString m_sModelFilename;

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE


#endif // DEEP_H
