//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 *   Andreas Griesshammer <ag@fieldlineinc.com>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file ioutils.cpp
 * @since 2022
 * @date  March 2026
 * @brief Out-of-line implementations for the non-templated overloads of @ref UTILSLIB::IOUtils.
 *
 * The bulk of the read/write logic lives inline in the header
 * because the public API is template-heavy; this translation
 * unit exists to anchor a small number of @c QString helpers
 * and to keep the compile-time cost of including @ref ioutils.h
 * in many headers bounded.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ioutils.h"