//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fastrak_parser.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Polhemus Fastrak / FastSCAN ASCII record parser.
 *
 *           Supported ASCII record formats:
 *
 *             Euler:      <st> <x> <y> <z> <az> <el> <ro>    CR LF
 *             Quaternion: <st> <x> <y> <z> <q0> <q1> <q2> <q3> CR LF
 *
 *           Auto-detected by token count (7 = Euler, 8 = quaternion).
 *           Quaternion mode (O-item 7) is preferred because it avoids
 *           gimbal lock at elevation ±90°. Positions are in the units
 *           configured on the device (inches for Fastrak, centimetres
 *           for FastSCAN).
 *
 *           This parser is intentionally I/O-free so it can be unit
 *           tested without a serial port: feed bytes via @ref append,
 *           pop completed samples via @ref nextSample.
 */

#ifndef UTILS_FASTRAK_PARSER_H
#define UTILS_FASTRAK_PARSER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../utils_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QByteArray>
#include <QQuaternion>
#include <QVector3D>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief One decoded sample from a Fastrak ASCII stream.
 */
struct UTILSSHARED_EXPORT FastrakSample
{
    int          station = 0;     ///< 1-based station id reported by the device.
    QVector3D    position;        ///< Position in metres (always normalised to SI).
    QQuaternion  orientation;     ///< Sensor orientation; identity if not provided.
    bool         hasOrientation = false;
};

//=============================================================================================================
/**
 * @brief Streaming parser for Fastrak / FastSCAN ASCII records.
 *
 * Thread-affine: the caller is responsible for serialising calls. No
 * heap allocations on the hot path other than the internal buffer.
 */
class UTILSSHARED_EXPORT FastrakParser
{
public:
    enum class Units {
        Inches,           ///< Fastrak factory default.
        Centimetres       ///< FastSCAN / G4 default.
    };

    FastrakParser() = default;

    /** Configure the linear unit reported by the device. */
    void setUnits(Units units) { m_units = units; }

    Units units() const { return m_units; }

    /** Append raw bytes received from the serial port. */
    void append(const QByteArray& chunk) { m_buffer.append(chunk); }

    /**
     * Pop the next fully-decoded sample, if any.
     *
     * @return true when @p out was populated; false when the buffer does
     *         not yet contain a complete record.
     */
    bool nextSample(FastrakSample& out);

    /** Reset accumulated buffer state. */
    void reset() { m_buffer.clear(); }

    /**
     * Parse one record line (without trailing CR/LF) — exposed for unit
     * tests and for callers that pre-frame their data.
     *
     * @return true on success; false on malformed input.
     */
    static bool parseRecord(const QByteArray& record, Units units, FastrakSample& out);

private:
    QByteArray m_buffer;
    Units      m_units = Units::Centimetres;
};

} // namespace UTILSLIB

#endif // UTILS_FASTRAK_PARSER_H
