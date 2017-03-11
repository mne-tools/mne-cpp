//=============================================================================================================
/**
* @file     mne_fiff_exp.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    MneFiffExp class declaration.
*
*/

#ifndef MNEFIFFEXP_H
#define MNEFIFFEXP_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QString>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SHOWFIFF
//=============================================================================================================

namespace SHOWFIFF
{


//=============================================================================================================
/**
* Implements one Mne Fiff Explanation (Replaces *mneFiffExp,mneFiffExpRec struct of MNE-C mne_show_fiff.h).
*
* @brief Mne Fiff Explanation description
*/
class MneFiffExp
{
public:
    typedef QSharedPointer<MneFiffExp> SPtr;              /**< Shared pointer type for MneFiffExp. */
    typedef QSharedPointer<const MneFiffExp> ConstSPtr;   /**< Const shared pointer type for MneFiffExp. */

    //=========================================================================================================
    /**
    * Constructs the Mne Fiff Explanation
    */
    MneFiffExp();

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_MneFiffExp      Mne Fiff Explanation which should be copied
    */
    MneFiffExp(const MneFiffExp& p_MneFiffExp);

    //=========================================================================================================
    /**
    * Destroys the Mne Fiff Explanation
    */
    ~MneFiffExp();

    //=========================================================================================================
    /**
    * Compares if Explanation ex1 is lesser than ex2
    *
    * @param[in] ex1    MneFiffExp which should be checked wheter it is lesser than ex2.
    * @param[in] ex2    MneFiffExp which should be checked wheter it is larger than ex1.
    */
    static bool comp_exp(const MneFiffExp& ex1, const MneFiffExp& ex2);

public:
    int  exclass;   /**< Class of this explanation */
    int  kind;      /**< Kind of object */
    QString text;   /**< Explanation text */

// ### OLD STRUCT ###
//    typedef struct {
//        int  class;     /* Class of this explanation */
//        int  kind;      /* Kind of object */
//        char *text;     /* Explanation text */
//    } *mneFiffExp,mneFiffExpRec;
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // NAMESPACE SHOWFIFF

#endif // MNEFIFFEXP_H
