//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     mne_fiff_exp.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     December, 2016
 * @version  dev
 * @brief    MNEFiffExp class declaration.
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
class MNEFiffExp
{
public:
    typedef QSharedPointer<MNEFiffExp> SPtr;              /**< Shared pointer type for MNEFiffExp. */
    typedef QSharedPointer<const MNEFiffExp> ConstSPtr;   /**< Const shared pointer type for MNEFiffExp. */

    //=========================================================================================================
    /**
     * Constructs the Mne Fiff Explanation
     */
    MNEFiffExp();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_MneFiffExp      Mne Fiff Explanation which should be copied
     */
    MNEFiffExp(const MNEFiffExp& p_MneFiffExp);

    //=========================================================================================================
    /**
     * Destroys the Mne Fiff Explanation
     */
    ~MNEFiffExp();

    //=========================================================================================================
    /**
     * Compares if Explanation ex1 is lesser than ex2
     *
     * @param[in] ex1    MNEFiffExp which should be checked wheter it is lesser than ex2.
     * @param[in] ex2    MNEFiffExp which should be checked wheter it is larger than ex1.
     */
    static bool comp_exp(const MNEFiffExp& ex1, const MNEFiffExp& ex2);

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
