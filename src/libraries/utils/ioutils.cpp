//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     ioutils.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Out-of-line implementations for the non-templated overloads of @ref UTILSLIB::IOUtils.
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