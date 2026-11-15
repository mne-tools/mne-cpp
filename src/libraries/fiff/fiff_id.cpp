//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_id.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
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
#include <base/log.h>

#include <format>
#include <string>

#include <vector>
#include "fiff_file.h"

#ifndef __EMSCRIPTEN__
#include <QNetworkInterface>
#endif
#include <QDateTime>

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
        id.time.secs = QDateTime::currentMSecsSinceEpoch() / 1000;
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
    QStringList possibleHardwareAdresses;
    fixed_id[0] = 0;
    fixed_id[1] = 0;

#ifndef __EMSCRIPTEN__
    const QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();

    if (!ifaces.empty()) {
        for (int i = 0; i < std::ssize(ifaces); ++i) {
            unsigned int flags = ifaces[i].flags();
            bool isLoopback = static_cast<bool>(flags & QNetworkInterface::IsLoopBack);
            bool isP2P = static_cast<bool>(flags & QNetworkInterface::IsPointToPoint);
            bool isRunning = static_cast<bool>(flags & QNetworkInterface::IsRunning);
            // If this interface isn't running, we don't care about it
            if (!isRunning)
                continue;
            // We only want valid interfaces that aren't loopback/virtual and not point to point
            if (!ifaces[i].isValid() || isLoopback || isP2P)
                continue;
            possibleHardwareAdresses << ifaces[i].hardwareAddress();
        }
        if (possibleHardwareAdresses.size() > 0) {
            // We take the first address as machine identifier
            QStringList hexPresentation = possibleHardwareAdresses[0].split(":");
            if (hexPresentation.size() == 6) {
                fixed_id[0] = QString(hexPresentation[0] + hexPresentation[1] + hexPresentation[2]).toInt(nullptr, 16);
                fixed_id[1] = QString(hexPresentation[3] + hexPresentation[4] + hexPresentation[5]).toInt(nullptr, 16);
                return true;
            }
        }
    }
#endif

    return false;
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
