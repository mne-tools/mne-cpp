//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     transport_error.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November, 2026
 * @brief    The errors of the mne_com transports as acquisition errors.
 */

#ifndef ACQ_DEVICES_TRANSPORT_ERROR_H
#define ACQ_DEVICES_TRANSPORT_ERROR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../acq_error.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
#include <string_view>
#include <system_error>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/**
 * @param[in] error      What the transport reports.
 * @param[in] fallback   The code unless the error says more: a timeout is Timeout, a missing device DeviceNotFound,
 *                       a refusal by the system PermissionDenied, a bad setting InvalidParameter.
 * @param[in] what       What failed, e.g. "cannot connect to host:4217".
 *
 * @return Success for no error, else the acquisition error with @p what and the system's message.
 */
inline AcqError transport_error(std::error_code error, AcqErrorCode fallback, std::string_view what)
{
    if (!error) {
        return {};
    }
    if (error == std::errc::timed_out) {
        return AcqError(AcqErrorCode::Timeout);
    }
    AcqErrorCode code = fallback;
    if (error == std::errc::no_such_file_or_directory || error == std::errc::no_such_device || error == std::errc::no_such_device_or_address) {
        code = AcqErrorCode::DeviceNotFound;
    } else if (error == std::errc::permission_denied || error == std::errc::operation_not_permitted) {
        code = AcqErrorCode::PermissionDenied;
    } else if (error == std::errc::invalid_argument) {
        code = AcqErrorCode::InvalidParameter;
    } else if (error == std::errc::not_supported) {
        code = AcqErrorCode::PlatformUnsupported;
    }
    return {code, std::format("{} ({})", what, error.message())};
}

} // namespace mne::detail

#endif // ACQ_DEVICES_TRANSPORT_ERROR_H
