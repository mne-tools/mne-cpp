//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_port.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Typed input or output slot on a graph node — carries an @ref MnaDataKind, an upstream link, an optional real-time stream binding and a cached-result reference.
 *
 * @ref MnaPort is the unit of connectivity in an MNA graph: every
 * data hand-off from one @ref MnaNode to another goes through a
 * named, typed port pair, never an anonymous string. Input ports
 * carry an explicit @c sourceNodeId / @c sourcePortName pointing at
 * the producing output, which is what @ref MnaGraph::topologicalSort
 * and @ref MnaGraph::validate rely on to detect cycles and reject
 * mismatched @ref MnaDataKind connections at edit time.
 *
 * When a port participates in a streaming pipeline (e.g. MNE Scan),
 * @c streamProtocol selects the transport — @c fiff-rt, @c lsl,
 * @c ftbuffer, @c shm or the in-process default — and
 * @c streamEndpoint / @c streamBufferMs configure the live
 * connection. For batch outputs, @c cachedResultPath plus
 * @c cachedResultHash let the executor skip re-computation when
 * upstream inputs are unchanged. @c extras keeps any forward-
 * compatible attributes a newer registry may attach.
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
 * @brief Named, typed port on an MNA graph node with upstream link and optional real-time stream binding.
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
