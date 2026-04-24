//=============================================================================================================
/**
 * @file     mna_port.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    MnaPort struct declaration — typed input/output slot on a graph node.
 *
 */

#ifndef MNA_PORT_H
#define MNA_PORT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QJsonObject>
#include <QCborMap>
#include <QSet>

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB
{

//=============================================================================================================
/**
 * Typed input/output slot on a graph node.
 *
 * @brief Graph port descriptor.
 */
struct MNASHARED_EXPORT MnaPort
{
    QString     name;               ///< Port name (unique within a node)
    MnaDataKind dataKind = MnaDataKind::Custom;  ///< Data kind flowing through this port
    MnaPortDir  direction = MnaPortDir::Input;   ///< Input or Output

    // Connection (for input ports — identifies the upstream source)
    QString     sourceNodeId;       ///< Which node produces this input? (empty → graph-level input)
    QString     sourcePortName;     ///< Which output port on that node?

    // Real-time stream binding (used when dataKind == RealTimeStream)
    //
    // Supported stream protocols:
    //   "fiff-rt"   — FIFF real-time protocol (MNE Scan native, TCP-based)
    //   "lsl"       — Lab Streaming Layer (multicast, cross-platform)
    //   "ftbuffer"  — FieldTrip buffer protocol (TCP, language-agnostic)
    //   "shm"       — Shared memory transport (single-machine, zero-copy)
    //   ""          — Internal Qt signal/slot wiring (in-process, default)
    //
    QString     streamProtocol;     ///< "fiff-rt", "lsl", "ftbuffer", "shm", "" = internal signal/slot
    QString     streamEndpoint;     ///< Protocol-specific address (e.g. "localhost:4218" for fiff-rt)
    int         streamBufferMs = 0; ///< Ring-buffer length in ms (0 = unbounded)

    // Cached result reference (for output ports)
    QString     cachedResultPath;   ///< Relative path to cached result
    QString     cachedResultHash;   ///< SHA-256 for invalidation
    QJsonObject extras;             ///< Unknown keys preserved for lossless round-trip

    QJsonObject toJson() const;
    static MnaPort fromJson(const QJsonObject& json);
    QCborMap toCbor() const;
    static MnaPort fromCbor(const QCborMap& cbor);
};

} // namespace MNALIB

#endif // MNA_PORT_H
