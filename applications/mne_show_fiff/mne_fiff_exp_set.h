//=============================================================================================================
/**
 * @file     mne_fiff_exp_set.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  1.0
 * @date     December, 2016
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
// FORWARD DECLARATIONS
//=============================================================================================================

class MneShowFiffSettings;


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

    //=========================================================================================================
    /**
    * Find fiff explanation
    * Refactored: mne_find_fiff_explanation (explanation.c)
    *
    * @param[in] exclass    explanation class to find
    * @param[in] kind       kind to find
    */
    QList<MneFiffExp>::const_iterator find_fiff_explanation(int exclass, int kind) const;

    //=========================================================================================================
    /**
    * Returns a const STL-style iterator pointing to the imaginary item after the last item in the list.
    *
    * @return const STL-style iterator pointing to the item after the last item
    */
    QList<MneFiffExp>::const_iterator constEnd() const;

    //=========================================================================================================
    /**
    * Show contents of a fif file
    *
    * @param[in] out            Output file
    * @param[in] settings       Show Fiff Settings object
    *
    * @return true if succeeded
    */
    bool show_fiff_contents (FILE *out, const MneShowFiffSettings& settings);

    //=========================================================================================================
    /**
    * Show contents of a fif file
    * Refactored: show_fiff_contents (mne_show_fiff.c)
    *
    * @param[in] out            Output file
    * @param[in] name           Input file
    * @param[in] verbose        Verbose output?
    * @param[in] tags           Output these specific tags?
    * @param[in] indent_step    Indentation step
    * @param[in] long_strings   Print long strings in full?
    * @param[in] blocks_only    Print blocks only?
    *
    * @return true if succeeded
    */
    bool show_fiff_contents (FILE *out, const QString& name, bool verbose, const QList<int>& tags, int indent_step, bool long_strings, bool blocks_only);

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
