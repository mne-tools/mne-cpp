#ifndef MNE_SPARSE_NAMED_MATRIX_H
#define MNE_SPARSE_NAMED_MATRIX_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <fiff/fiff_sparse_matrix.h>

#include <QStringList>

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

/**
 * Sparse named matrix - matrix specification with row/column name lists.
 */
class MNESHARED_EXPORT MneSparseNamedMatrix
{
public:
    MneSparseNamedMatrix() = default;
    ~MneSparseNamedMatrix() = default;

    int   nrow = 0;                        /**< Number of rows (same as in data). */
    int   ncol = 0;                        /**< Number of columns (same as in data). */
    QStringList rowlist;                   /**< Name list for the rows. */
    QStringList collist;                   /**< Name list for the columns. */
    std::unique_ptr<FIFFLIB::FiffSparseMatrix> data; /**< The data itself (sparse). */
};

} // namespace MNELIB

#endif // MNE_SPARSE_NAMED_MATRIX_H
