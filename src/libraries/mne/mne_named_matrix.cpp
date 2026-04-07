//=============================================================================================================
/**
 * @file     mne_named_matrix.cpp
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
 * @brief    MNENamedMatrix class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_named_matrix.h"
#include <fiff/fiff_constants.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNENamedMatrix::MNENamedMatrix()
: nrow(0)
, ncol(0)
{
}

//=============================================================================================================

MNENamedMatrix::MNENamedMatrix(const MNENamedMatrix &p_MneNamedMatrix)
: nrow(p_MneNamedMatrix.nrow)
, ncol(p_MneNamedMatrix.ncol)
, rowlist(p_MneNamedMatrix.rowlist)
, collist(p_MneNamedMatrix.collist)
, data(p_MneNamedMatrix.data)
{
}

//=============================================================================================================

MNENamedMatrix::~MNENamedMatrix()
{
}

//=============================================================================================================

std::unique_ptr<MNENamedMatrix> MNENamedMatrix::build(int nrow,
                                                      int ncol,
                                                      const QStringList& rowlist,
                                                      const QStringList& collist,
                                                      const Eigen::MatrixXf& data)
{
    auto mat = std::make_unique<MNENamedMatrix>();
    mat->nrow    = nrow;
    mat->ncol    = ncol;
    mat->rowlist = rowlist;
    mat->collist = collist;
    mat->data    = data;
    return mat;
}

//=============================================================================================================

std::unique_ptr<MNENamedMatrix> MNENamedMatrix::pick(const QStringList& pickrowlist,
                                                     int picknrow,
                                                     const QStringList& pickcollist,
                                                     int pickncol) const
{
    /*
     * Validate: picking by name requires names in the original matrix.
     */
    if (!pickrowlist.isEmpty() && this->rowlist.isEmpty()) {
        qCritical("MNENamedMatrix::pick - Cannot pick rows: no row names in original matrix.");
        return nullptr;
    }
    if (!pickcollist.isEmpty() && this->collist.isEmpty()) {
        qCritical("MNENamedMatrix::pick - Cannot pick columns: no column names in original matrix.");
        return nullptr;
    }

    /*
     * When no pick-list is given, keep all rows / columns.
     */
    if (pickrowlist.isEmpty())
        picknrow = this->nrow;
    if (pickcollist.isEmpty())
        pickncol = this->ncol;

    /*
     * Build row index mapping: for each picked row find its index in the original.
     */
    Eigen::VectorXi pick_row = Eigen::VectorXi::Zero(picknrow);
    QStringList my_pickrowlist;

    if (!pickrowlist.isEmpty()) {
        for (int j = 0; j < picknrow; ++j) {
            const QString& name = pickrowlist[j];
            pick_row[j] = -1;
            for (int k = 0; k < this->nrow; ++k) {
                if (QString::compare(name, this->rowlist[k]) == 0) {
                    pick_row[j] = k;
                    break;
                }
            }
            if (pick_row[j] == -1) {
                qCritical("MNENamedMatrix::pick - Row '%s' not found in original matrix.",
                          name.toUtf8().constData());
                return nullptr;
            }
        }
        my_pickrowlist = pickrowlist;
    } else {
        for (int k = 0; k < picknrow; ++k)
            pick_row[k] = k;
        my_pickrowlist = this->rowlist;
    }

    /*
     * Build column index mapping analogously.
     */
    Eigen::VectorXi pick_col = Eigen::VectorXi::Zero(pickncol);
    QStringList my_pickcollist;

    if (!pickcollist.isEmpty()) {
        for (int j = 0; j < pickncol; ++j) {
            const QString& name = pickcollist[j];
            pick_col[j] = -1;
            for (int k = 0; k < this->ncol; ++k) {
                if (QString::compare(name, this->collist[k]) == 0) {
                    pick_col[j] = k;
                    break;
                }
            }
            if (pick_col[j] == -1) {
                qCritical("MNENamedMatrix::pick - Column '%s' not found in original matrix.",
                          name.toUtf8().constData());
                return nullptr;
            }
        }
        my_pickcollist = pickcollist;
    } else {
        for (int k = 0; k < pickncol; ++k)
            pick_col[k] = k;
        my_pickcollist = this->collist;
    }

    /*
     * Assemble the picked data matrix.
     */
    Eigen::MatrixXf pickdata(picknrow, pickncol);
    for (int j = 0; j < picknrow; ++j) {
        const int row = pick_row[j];
        for (int k = 0; k < pickncol; ++k)
            pickdata(j, k) = this->data(row, pick_col[k]);
    }

    return build(picknrow, pickncol, my_pickrowlist, my_pickcollist, pickdata);
}

//=============================================================================================================

std::unique_ptr<MNENamedMatrix> MNENamedMatrix::read(FiffStream::SPtr& stream,
                                                     const FiffDirNode::SPtr& node,
                                                     int kind)
{
    QStringList colnames;
    QStringList rownames;
    int  ncol = 0;
    int  nrow = 0;
    qint32 ndim;
    QVector<qint32> dims;
    MatrixXf data;
    FiffTag::UPtr t_pTag;
    bool dataFound = false;

    FiffDirNode::SPtr tmp_node = node;

    /*
     * If the node itself is a FIFFB_MNE_NAMED_MATRIX block, read from it.
     * Otherwise look in its first-generation children for such a block.
     */
    if (tmp_node->type == FIFFB_MNE_NAMED_MATRIX) {
        if (!tmp_node->find_tag(stream, kind, t_pTag))
            return nullptr;

        t_pTag->getMatrixDimensions(ndim, dims);
        if (ndim != 2) {
            qCritical("MNENamedMatrix::read - Only two-dimensional matrices are supported.");
            return nullptr;
        }

        data = t_pTag->toFloatMatrix().transpose();
        dataFound = true;
    } else {
        for (int k = 0; k < tmp_node->nchild(); ++k) {
            if (tmp_node->children[k]->type == FIFFB_MNE_NAMED_MATRIX) {
                if (tmp_node->children[k]->find_tag(stream, kind, t_pTag)) {
                    t_pTag->getMatrixDimensions(ndim, dims);
                    if (ndim != 2) {
                        qCritical("MNENamedMatrix::read - Only two-dimensional matrices are supported.");
                        return nullptr;
                    }

                    data = t_pTag->toFloatMatrix().transpose();
                    dataFound = true;
                    tmp_node = tmp_node->children[k];
                    break;
                }
            }
        }
        if (!dataFound)
            return nullptr;
    }

    /*
     * Read optional FIFF_MNE_NROW / FIFF_MNE_NCOL dimension tags and
     * cross-check them against the matrix data.
     */
    if (!tmp_node->find_tag(stream, FIFF_MNE_NROW, t_pTag)) {
        nrow = dims[0];
    } else {
        nrow = *t_pTag->toInt();
        if (nrow != dims[0]) {
            qCritical("MNENamedMatrix::read - FIFF_MNE_NROW tag (%d) conflicts with matrix data (%d).",
                      nrow, dims[0]);
            return nullptr;
        }
    }

    if (!tmp_node->find_tag(stream, FIFF_MNE_NCOL, t_pTag)) {
        ncol = dims[1];
    } else {
        ncol = *t_pTag->toInt();
        if (ncol != dims[1]) {
            qCritical("MNENamedMatrix::read - FIFF_MNE_NCOL tag (%d) conflicts with matrix data (%d).",
                      ncol, dims[1]);
            return nullptr;
        }
    }

    /*
     * Read optional row and column name lists.
     */
    if (!tmp_node->find_tag(stream, FIFF_MNE_ROW_NAMES, t_pTag)) {
        const QString s = t_pTag->toString();
        rownames = FiffStream::split_name_list(s);
        if (rownames.size() != nrow) {
            qCritical("MNENamedMatrix::read - Row name count (%d) does not match nrow (%d).",
                      static_cast<int>(rownames.size()), nrow);
            return nullptr;
        }
    }

    if (!tmp_node->find_tag(stream, FIFF_MNE_COL_NAMES, t_pTag)) {
        const QString s = t_pTag->toString();
        colnames = FiffStream::split_name_list(s);
        if (colnames.size() != ncol) {
            qCritical("MNENamedMatrix::read - Column name count (%d) does not match ncol (%d).",
                      static_cast<int>(colnames.size()), ncol);
            return nullptr;
        }
    }

    return build(nrow, ncol, rownames, colnames, data);
}
