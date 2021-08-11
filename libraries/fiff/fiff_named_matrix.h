//=============================================================================================================
/**
 * @file     fiff_named_matrix.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    FiffNamedMatrix class declaration.
 *
 */

#ifndef FIFF_NAMED_MATRIX_H
#define FIFF_NAMED_MATRIX_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_constants.h"
#include "fiff_types.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStringList>
#include <QSharedData>
#include <QSharedDataPointer>
#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * Matrix specification with named rows and cols.
 *
 * @brief A named matrix
 */
class FIFFSHARED_EXPORT FiffNamedMatrix : public QSharedData
{
public:
    typedef QSharedPointer<FiffNamedMatrix> SPtr;               /**< Shared pointer type for FiffNamedMatrix. */
    typedef QSharedPointer<const FiffNamedMatrix> ConstSPtr;    /**< Const shared pointer type for FiffNamedMatrix. */
    typedef QSharedDataPointer<FiffNamedMatrix> SDPtr;          /**< Shared data pointer type for FiffNamedMatrix. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    FiffNamedMatrix();

    //=========================================================================================================
    /**
     * Constructs named matrix with given parameters. (No plausibility check is performed)
     *
     * @param[in] p_nrow         Number of rows.
     * @param[in] p_ncol         Number of cols.
     * @param[in] p_row_names    Row names.
     * @param[in] p_col_names    Column names.
     * @param[in] p_data         Data of the named matrix.
     */
    explicit FiffNamedMatrix(fiff_int_t p_nrow,
                             fiff_int_t p_ncol,
                             const QStringList& p_row_names,
                             const QStringList& p_col_names,
                             const Eigen::MatrixXd& p_data);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffNamedMatrix  Named matrix which should be copied.
     */
    FiffNamedMatrix(const FiffNamedMatrix& p_FiffNamedMatrix);

    //=========================================================================================================
    /**
     * Destroys the named matrix.
     */
    ~FiffNamedMatrix() = default;

    //=========================================================================================================
    /**
     * Initializes the named matrix.
     */
    void clear();

    //=========================================================================================================
    /**
     * Returns true if named matrix contains no data.
     *
     * @return true if named matrix is empty.
     */
    inline bool isEmpty() const;

    //ToDo return the transposed matrix instead of applying it to its members
    //=========================================================================================================
    /**
     * ### MNE toolbox root function ###: Definition of the mne_transpose_named_matrix function
     *
     * Transpose a named matrix (FiffNamedMatrix)
     */
    void transpose_named_matrix();

//    //=========================================================================================================
//    /**
//    * Assignment Operator
//    *
//    * @return rhs   named matrix which hould be assigned.
//    */
//    inline FiffNamedMatrix& operator=(const FiffNamedMatrix& rhs);

    //=========================================================================================================
    /**
     * overloading the stream out operator<<
     *
     * @param[in] out                The stream to which the fiff projector should be assigned to.
     * @param[in] p_FiffNamedMatrix  Fiff named matrix which should be assigned to the stream.
     *
     * @return the stream with the attached fiff named matrix.
     */
    friend std::ostream& operator<<(std::ostream& out, const FIFFLIB::FiffNamedMatrix &p_FiffNamedMatrix);

    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] object    The object which should be compared to.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator== (const FiffNamedMatrix &a, const FiffNamedMatrix &b);

public:
    fiff_int_t nrow;        /**< Number of rows. */
    fiff_int_t  ncol;       /**< Number of columns. */
    QStringList row_names;  /**< Row names. */
    QStringList col_names;  /**< Column names. */
    Eigen::MatrixXd data;   /**< Matrix data. */

// ### OLD STRUCT ###
//typedef struct {            /* Matrix specification with a channel list */
//    int   nrow;             /* Number of rows */
//    int   ncol;             /* Number of columns */
//    char  **rowlist;        /* Name list for the rows (may be NULL) */
//    char  **collist;        /* Name list for the columns (may be NULL) */
//    float **data;           /* The data itself (dense) */
//} *mneNamedMatrix,mneNamedMatrixRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

//inline FiffNamedMatrix& FiffNamedMatrix::operator=(const FiffNamedMatrix& rhs)
//{
//    // Check for self-assignment!
//    if (this == &rhs)
//        return *this;
//    //Else
//    nrow = rhs.nrow;
//    ncol = rhs.ncol;
//    row_names = rhs.row_names;
//    col_names = rhs.col_names;
//    data = rhs.data;

//    return *this;
//}

inline bool FiffNamedMatrix::isEmpty() const
{
    return !(this->data.size() > 0);
}

//=============================================================================================================

inline std::ostream& operator<<(std::ostream& out, const FIFFLIB::FiffNamedMatrix &p_FiffNamedMatrix)
{
    bool t_bIsShort = true;
    out << "#### Fiff Named Matrix ####\n";
    out << "\tnrow: " << p_FiffNamedMatrix.nrow << std::endl;
    out << "\tncol: " << p_FiffNamedMatrix.ncol << std::endl;

    Eigen::MatrixXd data;          /**< Matrix data. */

    out << "\trow_names " << p_FiffNamedMatrix.row_names.size() << ":\n\t";
    if(t_bIsShort)
    {
        qint32 nchan = p_FiffNamedMatrix.row_names.size() > 6 ? 6 : p_FiffNamedMatrix.row_names.size();
        for(qint32 i = 0; i < nchan/2; ++i)
            out << p_FiffNamedMatrix.row_names[i].toUtf8().constData() << " ";
        out << "... ";
        for(qint32 i = p_FiffNamedMatrix.row_names.size() - nchan/2; i < p_FiffNamedMatrix.row_names.size(); ++i)
            out << p_FiffNamedMatrix.row_names[i].toUtf8().constData() << " ";
        out << std::endl;
    }

    out << "\tcol_names " << p_FiffNamedMatrix.col_names.size() << ":\n\t";
    if(t_bIsShort)
    {
        qint32 nchan = p_FiffNamedMatrix.col_names.size() > 6 ? 6 : p_FiffNamedMatrix.col_names.size();
        for(qint32 i = 0; i < nchan/2; ++i)
            out << p_FiffNamedMatrix.col_names[i].toUtf8().constData() << " ";
        out << "... ";
        for(qint32 i = p_FiffNamedMatrix.col_names.size() - nchan/2; i < p_FiffNamedMatrix.col_names.size(); ++i)
            out << p_FiffNamedMatrix.col_names[i].toUtf8().constData() << " ";
        out << std::endl;
    }

    out << "\tdata " << p_FiffNamedMatrix.data.rows() << " x " << p_FiffNamedMatrix.data.cols() << ":\n\t";
    if(t_bIsShort)
    {
        qint32 nrows = p_FiffNamedMatrix.data.rows() > 6 ? 6 : p_FiffNamedMatrix.data.rows();
        qint32 ncols = p_FiffNamedMatrix.data.cols() > 6 ? 6 : p_FiffNamedMatrix.data.cols();
        if(nrows == 1)
        {
            for(qint32 i = 0; i < nrows; ++i)
            {
                for(qint32 j = 0; j < ncols/2; ++j)
                    out << p_FiffNamedMatrix.data(i,j) << " ";
                out << "... ";
                for(qint32 j = p_FiffNamedMatrix.data.cols() - ncols/2; j < p_FiffNamedMatrix.data.cols(); ++j)
                    out << p_FiffNamedMatrix.data(i,j) << " ";
                out << "\n\t";
            }
        }
        else
        {
        for(qint32 i = 0; i < nrows/2; ++i)
        {
            for(qint32 j = 0; j < ncols/2; ++j)
                out << p_FiffNamedMatrix.data(i,j) << " ";
            out << "... ";
            for(qint32 j = p_FiffNamedMatrix.data.cols() - ncols/2; j < p_FiffNamedMatrix.data.cols(); ++j)
                out << p_FiffNamedMatrix.data(i,j) << " ";
            out << "\n\t";
        }
        out << "...\n\t";
        for(qint32 i = p_FiffNamedMatrix.data.rows()-nrows/2; i < p_FiffNamedMatrix.data.rows(); ++i)
        {
            for(qint32 j = 0; j < ncols/2; ++j)
                out << p_FiffNamedMatrix.data(i,j) << " ";
            out << "... ";
            for(qint32 j = p_FiffNamedMatrix.data.cols() - ncols/2; j < p_FiffNamedMatrix.data.cols(); ++j)
                out << p_FiffNamedMatrix.data(i,j) << " ";
            out << "\n\t";
        }
        }
        out << "\n";
    }

    return out;
}

//=============================================================================================================

inline bool operator== (const FiffNamedMatrix &a, const FiffNamedMatrix &b)
{
    return (a.nrow == b.nrow &&
            a.ncol == b.ncol &&
            a.row_names == b.row_names &&
            a.col_names == b.col_names &&
            a.data.isApprox(b.data, 0.0001));
}
} // NAMESPACE

#endif // FIFF_SOLUTION_H
