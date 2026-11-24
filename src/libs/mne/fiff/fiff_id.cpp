//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_id.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     October 2012
 * @brief    Implementation of @ref FiffId: streaming of the 20-byte FIFF identifier record (version + machid + creation time).
 *
 * The @c machid is filled from the host MAC address when generating new
 * IDs so written files keep parity with the Neuromag acquisition stack
 * and stay lineage-traceable.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_id.h"

#include "fiff_file.h"
#include <mne/base/log.h>
#include <mne/base/process.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
#include <string>
#include <vector>

#ifndef __EMSCRIPTEN__
#endif

#include <chrono>
#include <ctime>
#include <cstdint>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffId::FiffId()
: version(-1)
{
    machid[0] = -1;
    machid[1] = -1;
    time.secs = -1;
    time.usecs = -1;
}

//=============================================================================================================

FiffId::FiffId(const FiffId& p_FiffId)
: version(p_FiffId.version)
{
    machid[0] = p_FiffId.machid[0];
    machid[1] = p_FiffId.machid[1];
    time.secs = p_FiffId.time.secs;
    time.usecs = p_FiffId.time.usecs;
}

//=============================================================================================================

FiffId::~FiffId()
{
}

//=============================================================================================================

FiffId FiffId::new_file_id()
{
    FiffId id;
    id.version = FIFFC_VERSION;

    int fixed_id[2];
    get_machid(fixed_id);
    /*
     * Internet address in the first two words
     */
    id.machid[0] = fixed_id[0];
    id.machid[1] = fixed_id[1];
    /*
     * Time in the third and fourth words
     */
    /*
     * Time in the third and fourth words
     * Since practically no system gives times in
     * true micro seconds, the last three digits
     * are randomized to insure uniqueness.
     */
    {
        id.time.secs = static_cast<fiff_int_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        id.time.usecs = rand() % 1000;
    }
    return id;
}

//=============================================================================================================

void FiffId::clear()
{
    version = -1;
    machid[0] = -1;
    machid[1] = -1;
    time.secs = -1;
    time.usecs = -1;
}

//=============================================================================================================

bool FiffId::get_machid(int* fixed_id)
{
    fixed_id[0] = 0;
    fixed_id[1] = 0;
    // The first running interface that is neither loopback nor point-to-point, three bytes of its address per word
    const std::vector<HardwareAddress> addresses = hardware_addresses();
    if (addresses.empty())
        return false;
    const HardwareAddress& mac = addresses.front();
    fixed_id[0] = (mac[0] << 16) | (mac[1] << 8) | mac[2];
    fixed_id[1] = (mac[3] << 16) | (mac[4] << 8) | mac[5];
    return true;
}

//=============================================================================================================

void FiffId::print() const
{
    if (!empty()) {
        mne::Log::info("\t{}.{} 0x{:x}{:x} {} {}\n", this->version >> 16, this->version & 0xFFFF, this->machid[0], this->machid[1], this->time.secs, this->time.usecs);
    }
}

//=============================================================================================================

std::string FiffId::toMachidString() const
{
    // Two 32-bit words as 16 upper-case hex digits, colon after every pair: XX:XX:...:XX
    const std::string hex = std::format("{:08X}{:08X}", static_cast<std::uint32_t>(machid[0]), static_cast<std::uint32_t>(machid[1]));
    std::string out;
    out.reserve(hex.size() * 3 / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        if (i > 0) {
            out += ':';
        }
        out.append(hex, i, 2);
    }
    return out;
}

//=============================================================================================================

FiffId& FiffId::getDefault()
{
    static FiffId defaultFiffId;
    return defaultFiffId;
}

//=============================================================================================================

std::string FiffId::to_string() const
{
    time_t secs = time.secs;

    // localtime() returns a pointer to a shared static buffer, so two threads
    // formatting an id at once can corrupt each other's result. Use the
    // reentrant variant, which writes into a caller-supplied struct. MSVC
    // spells it localtime_s and reverses the argument order.
    struct tm ltime{};
#ifdef _WIN32
    const bool bTimeValid = (localtime_s(&ltime, &secs) == 0);
#else
    const bool bTimeValid = (localtime_r(&secs, &ltime) != nullptr);
#endif

    char timebuf[100];
    if (!bTimeValid || strftime(timebuf, sizeof(timebuf), "%c", &ltime) == 0) {
        timebuf[0] = '\0';
    }

    return std::format("{}.{} 0x{:x}{:x} {}", version >> 16, version & 0xFFFF, static_cast<std::uint32_t>(machid[0]),
                       static_cast<std::uint32_t>(machid[1]), timebuf);
}
