//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mri_cor_io.h
 * @since 2026
 * @date  May 2026
 * @brief Reader for the legacy FreeSurfer COR-NNN per-slice volume layout (256 unsigned-char coronal slices on a 1 mm isotropic grid).
 *
 * COR is the directory-based volume format used by FreeSurfer
 * before MGH became the default. A subject's @c mri/T1 (and
 * historically @c mri/orig, @c mri/brain, ...) directory contains
 * 256 files named @c COR-001 through @c COR-256, each a flat
 * 65 536-byte (256\u00d7256 unsigned char) coronal slice. While
 * modern FreeSurfer prefers MGH, COR-format trees still appear
 * in legacy subject directories and in mne-c tutorial data, so
 * mne-cpp must read them to keep round-trip parity with the
 * original MNE C tooling.
 *
 * The reader fans the 256 files into an @ref MriSlice vector
 * with the canonical coronal-to-surface-RAS coordinate transform:
 *
 * - Origin offset (mm): (128, -128, 128)
 * - Axis permutation:   x \u2192 -x,  y \u2192 z,  z \u2192 y
 *
 * which matches the convention emitted by @c make_cor_set() in
 * the original @c mne_make_cor_set C tool and is what every
 * downstream BEM / source-space builder in MNE-CPP expects.
 *
 * Ported from @c make_cor_set() in MNE C @c mne_make_cor_set
 * by Matti Hamalainen.
 */
//=============================================================================================================

#ifndef MRI_COR_IO_H
#define MRI_COR_IO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_global.h"
#include "mri_vol_data.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE MRILIB
//=============================================================================================================

namespace MRILIB {

//=============================================================================================================
/**
 * @brief Stateless reader for the FreeSurfer COR-NNN directory layout.
 *
 * Walks a @c mri/T1 (or @c mri/orig, @c mri/brain, …) directory, loads
 * the 256 @c COR-001 … @c COR-256 flat 256×256 unsigned-char coronal
 * slice files, and assembles them into an @ref MriVolData with the per-slice
 * coordinate transform (slice→surface RAS) the rest of MRILIB expects.
 * The class exposes only @c static methods --- there is no per-instance
 * state, the caller supplies the directory path and an output volume.
 *
 * Ported from @c make_cor_set() in MNE C @c mne_make_cor_set by Matti
 * Hamalainen.
 */
class MRISHARED_EXPORT MriCorIO
{
public:
    //=========================================================================================================
    /**
     * Reads COR slice files from a directory.
     *
     * Expects files named COR-001 through COR-256, each containing
     * 256×256 unsigned char pixel data.
     *
     * @param[in]  dir      Path to the directory containing COR-NNN files.
     * @param[out] slices   Vector of MriSlice structures to populate.
     * @param[in]  verbose  If true, print progress information.
     *
     * @return True on success, false on error.
     */
    static bool read(const QString& dir,
                     QVector<MriSlice>& slices,
                     bool verbose = false);
};

} // namespace MRILIB

#endif // MRI_COR_IO_H
