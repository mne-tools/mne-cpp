//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_synthetic_backend.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Registration of the synthetic:// source, a seeded oddball EEG/MEG generator.
 */

#ifndef ACQ_SYNTHETIC_BACKEND_H
#define ACQ_SYNTHETIC_BACKEND_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../acq_device_registry.h"

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/** Registers synthetic:// with @p registry. */
AcqError registerSyntheticScheme(AcqDeviceRegistry& registry);

} // namespace mne::detail

#endif // ACQ_SYNTHETIC_BACKEND_H
