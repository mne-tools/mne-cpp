//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_uri_params.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Typed, locale-independent access to URI query parameters for backends.
 */

#ifndef ACQ_URI_PARAMS_H
#define ACQ_URI_PARAMS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../acq_device_registry.h"
#include "../acq_thread.h"

#include <cstdint>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/** @return The integer value of key in [min, max], fallback if absent, or InvalidParameter naming the key. */
ACQSHARED_EXPORT AcqExpected<std::int64_t> paramInt(const AcqUri& uri, std::string_view key, std::int64_t fallback,
                                                    std::int64_t min, std::int64_t max);

/** @return The decimal value of key in [min, max], fallback if absent, or InvalidParameter naming the key. */
ACQSHARED_EXPORT AcqExpected<double> paramDouble(const AcqUri& uri, std::string_view key, double fallback, double min,
                                                 double max);

/** @return The value of key (1/0, true/false, yes/no, on/off), fallback if absent, or InvalidParameter. */
ACQSHARED_EXPORT AcqExpected<bool> paramBool(const AcqUri& uri, std::string_view key, bool fallback);

/**
 * @return The policy of a backend's acquisition thread from cpus (comma-separated CPU numbers) and priority
 *         (normal, high, realtime); both optional. InvalidParameter for malformed values; whether the CPUs
 *         exist is checked when the thread applies the policy.
 */
ACQSHARED_EXPORT AcqExpected<AcqThreadPolicy> paramThreadPolicy(const AcqUri& uri);

} // namespace mne::detail

#endif // ACQ_URI_PARAMS_H
