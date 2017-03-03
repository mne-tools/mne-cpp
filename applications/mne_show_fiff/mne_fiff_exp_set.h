//=============================================================================================================
/**
* @file     mne_fiff_exp_set.h
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
* @brief     MneFiffExpSet class declaration.
*
*/

#ifndef MNEFIFFEXPSET_H
#define MNEFIFFEXPSET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_fiff_exp.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>
#include <QString>


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
// DEFINE NAMESPACE SHOWFIFF
//=============================================================================================================

namespace SHOWFIFF
{

//*************************************************************************************************************
//=============================================================================================================
// FIFFLIB FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Implements Electric Current Dipole Set (Replaces *ecdSet,ecdSetRec struct of MNE-C fit_types.h).
*
* @brief Holds a set of Electric Current Dipoles.
*/

class MneFiffExpSet
{

public:
    typedef QSharedPointer<MneFiffExpSet> SPtr;            /**< Shared pointer type for MneFiffExpSet. */
    typedef QSharedPointer<const MneFiffExpSet> ConstSPtr; /**< Const shared pointer type for MneFiffExpSet. */

    //=========================================================================================================
    /**
    * Constructs an MNE Fiff Explanation Set object.
    */
    MneFiffExpSet();

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_MneFiffExpSet       MNE Fiff Explanation Set which should be copied
    */
    MneFiffExpSet(const MneFiffExpSet &p_MneFiffExpSet);

    //=========================================================================================================
    /**
    * Destroys the Electric Current Dipole description
    */
    ~MneFiffExpSet();

    //=========================================================================================================
    /**
    * Returns the number of stored MneFiffExps
    *
    * @return number of stored MneFiffExps
    */
    inline qint32 size() const;

    //=========================================================================================================
    /**
    * Subscript operator [] to access MneFiffExp by index
    *
    * @param[in] idx    the MneFiffExp index.
    *
    * @return MneFiffExp related to the parameter index.
    */
    const MneFiffExp& operator[] (int idx) const;

    //=========================================================================================================
    /**
    * Subscript operator [] to access MneFiffExp by index
    *
    * @param[in] idx    the MneFiffExp index.
    *
    * @return MneFiffExp related to the parameter index.
    */
    MneFiffExp& operator[] (int idx);

    //=========================================================================================================
    /**
    * Subscript operator << to add a new MneFiffExp
    *
    * @param[in] p_MneFiffExp      MneFiffExp to be added
    *
    * @return MneFiffExpSet
    */
    MneFiffExpSet& operator<< (const MneFiffExp& p_MneFiffExp);

    //=========================================================================================================
    /**
    * Read an explanation file and sort the entries
    * Refactored: mne_read_fiff_explanations (explanation.c)
    *
    * @param[in] name   File to read
    */
    static MneFiffExpSet read_fiff_explanations(const QString& name);

    //=========================================================================================================
    /**
    * Write the content to a std stream
    * Refactored: mne_list_fiff_explanations (explanation.c)
    *
    * @param[in] out   Stream to write the content to
    */
    void list_fiff_explanations(FILE *out);


//    mneFiffExp mne_find_fiff_explanation(mneFiffExpSet set,
//                                         int exclass,
//                         int kind)

//    {
//      MneFiffExp one;

//      one.exclass = exclass;
//      one.kind  = kind;
//      one.text  = NULL;

//      return bsearch(&one,set->exp,set->nexp,sizeof(mneFiffExpRec),comp_exp);

//      qBinaryFind(vect.begin(), vect.end(), 6);

//    }

private:

    //=========================================================================================================
    /**
    * Sort the fiff explanation set
    * Refactored: mne_sort_fiff_explanations (explanation.c)
    */
    void sort_fiff_explanations();

private:
    QList<MneFiffExp> m_qListExp;     /**< List of Explanations. */

// ### OLD STRUCT ###
//    typedef struct {
//        mneFiffExp exp;
//        int        nexp;
//    } *mneFiffExpSet,mneFiffExpSetRec;
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

qint32 MneFiffExpSet::size() const
{
    return m_qListExp.size();
}

} // NAMESPACE SHOWFIFF

#endif // MNEFIFFEXPSET_H
