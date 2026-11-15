//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_global.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Definitions of the mne_acq build-info accessors declared in @ref acq_global.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"
#include "acq_sample_format.h"

// Standalone (embedded) builds do not pass the git hashes of the MNE-CPP super-build.
#ifndef MNE_GIT_HASH_SHORT
#define MNE_GIT_HASH_SHORT "unknown"
#endif
#ifndef MNE_GIT_HASH_LONG
#define MNE_GIT_HASH_LONG "unknown"
#endif

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

const char* mne::acq_build_date_time()
{
    return __DATE__ " " __TIME__;
}

//=============================================================================================================

const char* mne::acq_build_hash()
{
    return MNE_GIT_HASH_SHORT;
}

//=============================================================================================================

const char* mne::acq_build_hash_long()
{
    return MNE_GIT_HASH_LONG;
}

//=============================================================================================================

std::string_view mne::sampleFormatName(AcqSampleFormat format) noexcept
{
    switch (format) {
        case AcqSampleFormat::Int16:
            return "int16";
        case AcqSampleFormat::Int24Packed:
            return "int24";
        case AcqSampleFormat::Int32:
            return "int32";
        case AcqSampleFormat::Float32:
            return "float32";
        case AcqSampleFormat::Float64:
            return "float64";
        case AcqSampleFormat::ComplexInt16:
            return "cint16";
        case AcqSampleFormat::ComplexFloat32:
            return "cfloat32";
    }
    return "unknown";
}
