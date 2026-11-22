//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_diagnostics.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Internal instrumentation of mne_acq for tests; not installed.
 */

#ifndef ACQ_DIAGNOSTICS_H
#define ACQ_DIAGNOSTICS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../acq_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

//=============================================================================================================
/**
 * @return Number of mutex acquisitions inside mne_acq since process start; the data path must not change it.
 */
ACQSHARED_EXPORT std::uint64_t controlLockCount() noexcept;

} // namespace mne::detail

#endif // ACQ_DIAGNOSTICS_H
