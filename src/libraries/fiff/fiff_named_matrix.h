//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_named_matrix.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2012
 * @brief    Matrix paired with row and column name lists, the on-disk form of FIFFB_PROJ_ITEM / FIFFB_MNE_NAMED_MATRIX payloads.
 *
 * Several FIFF blocks store a matrix whose rows and columns refer to
 * channel names (SSP projection vectors, CTF compensation matrices,
 * forward gain matrices, MNE noise-covariance whiteners, ...). The
 * on-disk representation is a @c FIFF_MNE_NROW + @c FIFF_MNE_NCOL pair
 * plus a dense or sparse matrix tag plus two string-list tags with the
 * row and column names. @ref mne::FiffNamedMatrix bundles all of that into a
 * single object: the Eigen matrix and the two @c StringList name
 * vectors, with the same default-construction semantics as
 * @c numpy.zeros + name lists in @c mne.SourceEstimate / @c mne.Forward.
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

#include <algorithm>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief FIFF named matrix: dense / sparse Eigen matrix plus row-name and column-name string lists.
 *
 * Used wherever the on-disk FIFF block pairs a matrix with channel-name
 * metadata: SSP projectors (@c FIFFB_PROJ_ITEM), CTF compensators
 * (@c FIFFB_MNE_CTF_COMP_DATA), forward gain matrices, MNE inverse
 * operators, noise covariances. The name lists let downstream code index
 * the matrix by name instead of by position, which is what the
 * @c pick_channels / @c apply_proj paths rely on.
 *
 * @snippet ex_fiff_api/main.cpp fiff_proj_usage
 */
class FIFFSHARED_EXPORT FiffNamedMatrix
{
public:
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
                             const StringList& p_row_names,
                             const StringList& p_col_names,
                             const Eigen::MatrixXd& p_data);

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
    inline bool empty() const;

    //ToDo return the transposed matrix instead of applying it to its members
    //=========================================================================================================
    /**
     * Transpose a named matrix (FiffNamedMatrix)
     */
    void transpose_named_matrix();

    //=========================================================================================================
    /**
     * overloading the stream out operator<<
     *
     * @param[in] out                The stream to which the fiff projector should be assigned to.
     * @param[in] p_FiffNamedMatrix  Fiff named matrix which should be assigned to the stream.
     *
     * @return the stream with the attached fiff named matrix.
     */
    friend std::ostream& operator<<(std::ostream& out, const mne::FiffNamedMatrix& p_FiffNamedMatrix);

    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] a    The object which should be compared to.
     * @param[in] b Right-hand named matrix.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator==(const FiffNamedMatrix& a, const FiffNamedMatrix& b);

public:
    fiff_int_t nrow;      /**< Number of rows. */
    fiff_int_t ncol;      /**< Number of columns. */
    StringList row_names; /**< Row names. */
    StringList col_names; /**< Column names. */
    Eigen::MatrixXd data; /**< Matrix data. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool FiffNamedMatrix::empty() const
{
    return !(this->data.size() > 0);
}

//=============================================================================================================

inline std::ostream& operator<<(std::ostream& out, const mne::FiffNamedMatrix& p_FiffNamedMatrix)
{
    bool t_bIsShort = true;
    out << "#### Fiff Named Matrix ####\n";
    out << "\tnrow: " << p_FiffNamedMatrix.nrow << std::endl;
    out << "\tncol: " << p_FiffNamedMatrix.ncol << std::endl;

    Eigen::MatrixXd data; /**< Matrix data. */

    out << "\trow_names " << p_FiffNamedMatrix.row_names.size() << ":\n\t";
    if (t_bIsShort) {
        const std::size_t nchan = std::min<std::size_t>(p_FiffNamedMatrix.row_names.size(), 6);
        for (std::size_t i = 0; i < nchan / 2; ++i)
            out << p_FiffNamedMatrix.row_names[i].c_str() << " ";
        out << "... ";
        for (std::size_t i = p_FiffNamedMatrix.row_names.size() - nchan / 2; i < p_FiffNamedMatrix.row_names.size(); ++i)
            out << p_FiffNamedMatrix.row_names[i].c_str() << " ";
        out << std::endl;
    }

    out << "\tcol_names " << p_FiffNamedMatrix.col_names.size() << ":\n\t";
    if (t_bIsShort) {
        const std::size_t nchan = std::min<std::size_t>(p_FiffNamedMatrix.col_names.size(), 6);
        for (std::size_t i = 0; i < nchan / 2; ++i)
            out << p_FiffNamedMatrix.col_names[i].c_str() << " ";
        out << "... ";
        for (std::size_t i = p_FiffNamedMatrix.col_names.size() - nchan / 2; i < p_FiffNamedMatrix.col_names.size(); ++i)
            out << p_FiffNamedMatrix.col_names[i].c_str() << " ";
        out << std::endl;
    }

    out << "\tdata " << p_FiffNamedMatrix.data.rows() << " x " << p_FiffNamedMatrix.data.cols() << ":\n\t";
    if (t_bIsShort) {
        const Eigen::Index nrows = std::min<Eigen::Index>(p_FiffNamedMatrix.data.rows(), 6);
        const Eigen::Index ncols = std::min<Eigen::Index>(p_FiffNamedMatrix.data.cols(), 6);
        if (nrows == 1) {
            for (Eigen::Index i = 0; i < nrows; ++i) {
                for (Eigen::Index j = 0; j < ncols / 2; ++j)
                    out << p_FiffNamedMatrix.data(i, j) << " ";
                out << "... ";
                for (Eigen::Index j = p_FiffNamedMatrix.data.cols() - ncols / 2; j < p_FiffNamedMatrix.data.cols(); ++j)
                    out << p_FiffNamedMatrix.data(i, j) << " ";
                out << "\n\t";
            }
        } else {
            for (Eigen::Index i = 0; i < nrows / 2; ++i) {
                for (Eigen::Index j = 0; j < ncols / 2; ++j)
                    out << p_FiffNamedMatrix.data(i, j) << " ";
                out << "... ";
                for (Eigen::Index j = p_FiffNamedMatrix.data.cols() - ncols / 2; j < p_FiffNamedMatrix.data.cols(); ++j)
                    out << p_FiffNamedMatrix.data(i, j) << " ";
                out << "\n\t";
            }
            out << "...\n\t";
            for (Eigen::Index i = p_FiffNamedMatrix.data.rows() - nrows / 2; i < p_FiffNamedMatrix.data.rows(); ++i) {
                for (Eigen::Index j = 0; j < ncols / 2; ++j)
                    out << p_FiffNamedMatrix.data(i, j) << " ";
                out << "... ";
                for (Eigen::Index j = p_FiffNamedMatrix.data.cols() - ncols / 2; j < p_FiffNamedMatrix.data.cols(); ++j)
                    out << p_FiffNamedMatrix.data(i, j) << " ";
                out << "\n\t";
            }
        }
        out << "\n";
    }

    return out;
}

//=============================================================================================================

inline bool operator==(const FiffNamedMatrix& a, const FiffNamedMatrix& b)
{
    return (a.nrow == b.nrow &&
            a.ncol == b.ncol &&
            a.row_names == b.row_names &&
            a.col_names == b.col_names &&
            a.data.isApprox(b.data, 0.0001));
}
} // NAMESPACE

#endif // FIFF_NAMED_MATRIX_H
