//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mat_file.h
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Dense double matrices to a MATLAB Level 5 MAT-file.
 *
 * @ref mne::write_mat_file stores named matrices uncompressed, in the layout
 * MATLAB's @c load and SciPy's @c scipy.io.loadmat read. It is what
 * mne_raw2mat and mne_epochs2mat write.
 *
 * @snippet ex_math/main.cpp mat_file_usage
 */

#ifndef MATHLIB_MAT_FILE_H
#define MATHLIB_MAT_FILE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "math_global.h"

#include <mne/base/error.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <initializer_list>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * One variable of a MAT-file: its name and the matrix it holds. The matrix is referenced, not copied.
 */
struct MatVariable
{
    std::string_view name;        /**< Variable name in MATLAB, e.g. "data". */
    const Eigen::MatrixXd& value; /**< Matrix stored under the name, written column-major. */
};

//=============================================================================================================
/**
 * Writes matrices as double arrays to an uncompressed MATLAB Level 5 MAT-file.
 *
 * @param[in] path      Output file; an existing file is overwritten.
 * @param[in] creator   Named in the file's text header ("MATLAB 5.0 MAT-file, created by <creator>").
 * @param[in] variables Variables in the order they are written.
 *
 * @return Nothing on success; an Io error if the file cannot be written, a Format error if a variable has no
 *         name or is too large for the format (4 GiB per variable).
 */
[[nodiscard]] MATHSHARED_EXPORT Expected<void> write_mat_file(const std::filesystem::path& path,
                                                              std::string_view creator,
                                                              std::initializer_list<MatVariable> variables);

} // namespace mne

#endif // MATHLIB_MAT_FILE_H
