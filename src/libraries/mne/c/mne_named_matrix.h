//=============================================================================================================
/**
 * @file     mne_named_matrix.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNE Named Matrix (MneNamedMatrix) class declaration.
 *
 */

#ifndef MNENAMEDMATRIX_H
#define MNENAMEDMATRIX_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QDebug>
#include <QStringList>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB
{
    class FiffStream;
    class FiffDirNode;
}

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * Implements MNE Named Matrix (Replaces *mneNamedMatrix,mneNamedMatrixRec; struct of MNE-C mne_types.h).
 * !!!!TODO Merge with existing FiffNamedMatrix!!!!
 *
 * @brief Matrix specification with a channel list
 */
class MNESHARED_EXPORT MneNamedMatrix
{
public:
    typedef QSharedPointer<MneNamedMatrix> SPtr;              /**< Shared pointer type for MneNamedMatrix. */
    typedef QSharedPointer<const MneNamedMatrix> ConstSPtr;   /**< Const shared pointer type for MneNamedMatrix. */

    //=========================================================================================================
    /**
     * Constructs the MNE Named Matrix
     */
    MneNamedMatrix();

    //=========================================================================================================
    /**
     * Copy constructor.
     * Refactored: mne_dup_named_matrix (mne_named_matrix.c)
     *
     * @param[in] p_MneNamedMatrix   MNE Named Matrix which should be copied.
     */
    MneNamedMatrix(const MneNamedMatrix& p_MneNamedMatrix);

    //=========================================================================================================
    /**
     * Destroys the MNE Named Matrix description
     * Refactored: mne_free_named_matrix (mne_named_matrix.c)
     */
    ~MneNamedMatrix();

    //=========================================================================================================
    /**
     * Build a named matrix from the ingredients
     * Refactored: mne_build_named_matrix (mne_named_matrix.c)
     *
     * @param[in] nrow       Number of rows.
     * @param[in] ncol       Number of columns.
     * @param[in] rowlist    List of row (channel) names.
     * @param[in] collist    List of column (channel) names.
     * @param[in] data       Data to store.
     *
     * @return   The new named matrix.
     */
    static MneNamedMatrix* build_named_matrix(int  nrow, int  ncol, const QStringList& rowlist, const QStringList& collist, float **data);

    //=========================================================================================================
    /**
     * Read a named matrix from the specified node
     * Refactored: mne_read_named_matrix (mne_named_matrix.c)
     *
     * @param[in] stream     Stream to read from.
     * @param[in] node       Node to read from.
     * @param[in] kind       Block kind which should be read.
     *
     * @return   The read named matrix.
     */
    static MneNamedMatrix* read_named_matrix(QSharedPointer<FIFFLIB::FiffStream>& stream,const QSharedPointer<FIFFLIB::FiffDirNode>& node,int kind);

    //=========================================================================================================
    /**
     * Pick appropriate rows and columns and build a new matrix
     * Refactored: mne_pick_from_named_matrix (mne_named_matrix.c)
     *
     * @param[in] pickrowlist    List of row names to pick.
     * @param[in] picknrow       Number of rows.
     * @param[in] pickcollist    List of column names to pick.
     * @param[in] pickncol       Number of columns.
     *
     * @return   The read named matrix.
     */
    MneNamedMatrix* pick_from_named_matrix(const QStringList& pickrowlist, int picknrow, const QStringList& pickcollist, int pickncol) const;

public:
    int   nrow;             /* Number of rows */
    int   ncol;             /* Number of columns */
    QStringList rowlist;    /* Name list for the rows (may be NULL) */
    QStringList collist;    /* Name list for the columns (may be NULL) */
    float **data;           /* The data itself (dense) */

// ### OLD STRUCT ###
//typedef struct {        /* Matrix specification with a channel list */
//    int   nrow;         /* Number of rows */
//    int   ncol;         /* Number of columns */
//    char  **rowlist;    /* Name list for the rows (may be NULL) */
//    char  **collist;    /* Name list for the columns (may be NULL) */
//    float **data;       /* The data itself (dense) */
//} *mneNamedMatrix,mneNamedMatrixRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNENAMEDMATRIX_H
