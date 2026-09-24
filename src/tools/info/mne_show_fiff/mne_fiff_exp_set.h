//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     mne_fiff_exp_set.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     December, 2016
 * @version  dev
 * @brief     MNEFiffExpSet class declaration.
 */

#ifndef MNEFIFFEXPSET_H
#define MNEFIFFEXPSET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_fiff_exp.h"

#include <fiff/fiff_tag.h>
#include <fiff/fiff_stream.h>


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

class MNEShowFiffSettings;


//=============================================================================================================
/**
 * Implements Electric Current Dipole Set (Replaces *ecdSet,ecdSetRec struct of MNE-C fit_types.h).
 *
 * @brief Holds a set of Electric Current Dipoles.
 */

class MNEFiffExpSet
{

public:
    typedef QSharedPointer<MNEFiffExpSet> SPtr;            /**< Shared pointer type for MNEFiffExpSet. */
    typedef QSharedPointer<const MNEFiffExpSet> ConstSPtr; /**< Const shared pointer type for MNEFiffExpSet. */

    //=========================================================================================================
    /**
     * Constructs an MNE Fiff Explanation Set object.
     */
    MNEFiffExpSet();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_MneFiffExpSet       MNE Fiff Explanation Set which should be copied
     */
    MNEFiffExpSet(const MNEFiffExpSet &p_MneFiffExpSet);

    //=========================================================================================================
    /**
     * Destroys the Electric Current Dipole description
     */
    ~MNEFiffExpSet();

    //=========================================================================================================
    /**
     * Returns the number of stored MNEFiffExps
     *
     * @return number of stored MNEFiffExps
     */
    inline qint32 size() const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access MNEFiffExp by index
     *
     * @param[in] idx    the MNEFiffExp index.
     *
     * @return MNEFiffExp related to the parameter index.
     */
    const MNEFiffExp& operator[] (int idx) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access MNEFiffExp by index
     *
     * @param[in] idx    the MNEFiffExp index.
     *
     * @return MNEFiffExp related to the parameter index.
     */
    MNEFiffExp& operator[] (int idx);

    //=========================================================================================================
    /**
     * Subscript operator << to add a new MNEFiffExp
     *
     * @param[in] p_MneFiffExp      MNEFiffExp to be added
     *
     * @return MNEFiffExpSet
     */
    MNEFiffExpSet& operator<< (const MNEFiffExp& p_MneFiffExp);

    //=========================================================================================================
    /**
     * Read an explanation file and sort the entries
     * Refactored: mne_read_fiff_explanations (explanation.c)
     *
     * @param[in] name   File to read
     */
    static MNEFiffExpSet read_fiff_explanations(const QString& name);

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
    QList<MNEFiffExp>::const_iterator find_fiff_explanation(int exclass, int kind) const;

    //=========================================================================================================
    /**
     * Returns a const STL-style iterator pointing to the imaginary item after the last item in the list.
     *
     * @return const STL-style iterator pointing to the item after the last item
     */
    QList<MNEFiffExp>::const_iterator constEnd() const;

    //=========================================================================================================
    /**
     * Show contents of a fif file
     *
     * @param[in] out            Output file
     * @param[in] settings       Show Fiff Settings object
     *
     * @return true if succeeded
     */
    bool show_fiff_contents (FILE *out, const MNEShowFiffSettings& settings);

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

    void print_file_id (FILE *out, const FIFFLIB::FiffTag::UPtr& tag);

    void print_ch_info (FILE *out, const FIFFLIB::FiffTag::UPtr& tag);

    void print_transform(FILE *out, const FIFFLIB::FiffTag::UPtr& tag);

    void print_dig_point(FILE *out, const FIFFLIB::FiffTag::UPtr& tag);

    void print_matrix(FILE *out, FIFFLIB::FiffStream::SPtr stream, FIFFLIB::FiffDirEntry::SPtr this_ent);

private:
    QList<MNEFiffExp> m_qListExp;     /**< List of Explanations. */

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

qint32 MNEFiffExpSet::size() const
{
    return m_qListExp.size();
}

} // NAMESPACE SHOWFIFF

#endif // MNEFIFFEXPSET_H
