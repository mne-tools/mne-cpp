//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file com_global.h
 * @since 2026
 * @date  March 2026
 * @brief Export macros, build-info entry points and namespace anchor for the @c COMLIB communication library.
 *
 * @c COMLIB is the transport layer that connects mne-cpp acquisition and
 * analysis code to a running @c mne_rt_server instance — the long-running
 * daemon shipped by mne-cpp that bridges MEG/EEG hardware drivers (or a
 * pre-recorded FIFF file) to networked clients. Two TCP endpoints are
 * exposed by the server and consumed here: the @e command port (default
 * 4217), used to negotiate session state through line- or JSON-encoded
 * messages, and the @e data port (default 4218), used to stream raw
 * sample buffers tagged with FIFF block kinds.
 *
 * The library is intentionally narrow: it contains the threaded TCP
 * clients (@ref COMLIB::RtClient, @ref COMLIB::RtCmdClient,
 * @ref COMLIB::RtDataClient) and a small command vocabulary
 * (@ref COMLIB::Command, @ref COMLIB::RawCommand,
 * @ref COMLIB::CommandParser, @ref COMLIB::CommandManager) that both
 * sides agree on. Higher-level orchestration (channel selection, ring
 * buffering, source localisation) lives in the @c rtprocessing /
 * @c disp / acquisition plugins that link against this library.
 *
 * This header defines @ref COMSHARED_EXPORT, expanding to
 * @c Q_DECL_EXPORT when building the shared library, @c Q_DECL_IMPORT
 * when consuming it, and to nothing for the static build configuration
 * (@c STATICBUILD). It also declares the three @c buildDateTime /
 * @c buildHash / @c buildHashLong entry points used by tooling and the
 * About dialogs to report the exact commit the binary was built from.
 */

#ifndef COMMUNICATION_GLOBAL_H
#define COMMUNICATION_GLOBAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtCore/qglobal.h>
#include <utils/buildinfo.h>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#if defined(STATICBUILD)
#  define COMSHARED_EXPORT
#elif defined(MNE_COM_LIBRARY)
#  define COMSHARED_EXPORT Q_DECL_EXPORT    /**< Q_DECL_EXPORT must be added to the declarations of symbols used when compiling a shared library. */
#else
#  define COMSHARED_EXPORT Q_DECL_IMPORT    /**< Q_DECL_IMPORT must be added to the declarations of symbols used when compiling a client that uses the shared library. */
#endif

//=============================================================================================================
/**
 * @namespace COMLIB
 * @brief Real-time client/server communication primitives for talking to @c mne_rt_server.
 *
 * Aggregates two cooperating subsystems. @c rt_client/ holds the
 * @c QThread- and @c QTcpSocket-derived clients that own the actual
 * socket connections and surface received data through Qt signals; they
 * are designed to be moved to (or constructed in) a worker thread so the
 * UI thread is never blocked by network I/O. @c rt_command/ holds the
 * command vocabulary used on the wire: @ref RawCommand carries the
 * tokenised request before types are known, @ref Command carries a
 * named, typed, parameter-bearing message that round-trips through JSON,
 * @ref CommandParser dispatches incoming text to all interested
 * managers, and @ref CommandManager is a per-component registry of the
 * commands that component understands.
 */
namespace COMLIB{

//=============================================================================================================
/**
 * @brief Compile-time date/time stamp baked into the @c COMLIB binary.
 *
 * Resolved from the @c __DATE__ / @c __TIME__ macros at build time and
 * surfaced through @c utils/buildinfo.h. Used by About dialogs and bug
 * reports to disambiguate identically-versioned builds.
 */
COMSHARED_EXPORT const char* buildDateTime();

//=============================================================================================================
/**
 * @brief Abbreviated git commit hash (typically 7 hex chars) of the source tree this library was built from.
 *
 * Populated by the @c MNE_GIT_HASH_SHORT compile definition set by the
 * top-level CMake project. Returns an empty string for tree-only builds.
 */
COMSHARED_EXPORT const char* buildHash();

//=============================================================================================================
/**
 * @brief Full-length git commit hash (40 hex chars) of the source tree this library was built from.
 *
 * Populated by the @c MNE_GIT_HASH_LONG compile definition; intended for
 * scripted provenance checks where the abbreviated hash is ambiguous.
 */
COMSHARED_EXPORT const char* buildHashLong();
}

#endif // COM_GLOBAL_H
