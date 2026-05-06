//=============================================================================================================
/**
 * @file     fasttrak_parser.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    Polhemus FastTrak / FastSCAN ASCII record parser.
 *
 *           Format-0 ASCII record (default factory output):
 *
 *               <st><sp> <x> <y> <z> <az> <el> <ro> CR LF
 *
 *           where each numeric field is fixed-width and signed
 *           (e.g. "+123.456"). Positions are in the units configured on
 *           the device (inches by default for FastTrak, centimetres for
 *           the FastSCAN family). Orientation is azimuth/elevation/roll
 *           in degrees, applied in that order around Z, Y, X.
 *
 *           This parser is intentionally I/O-free so it can be unit
 *           tested without a serial port: feed bytes via @ref append,
 *           pop completed samples via @ref nextSample.
 */

#ifndef UTILS_FASTTRAK_PARSER_H
#define UTILS_FASTTRAK_PARSER_H

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
 * @brief One decoded sample from a FastTrak ASCII stream.
 */
struct UTILSSHARED_EXPORT FastTrakSample
{
    int          station = 0;     ///< 1-based station id reported by the device.
    QVector3D    position;        ///< Position in metres (always normalised to SI).
    QQuaternion  orientation;     ///< Sensor orientation; identity if not provided.
    bool         hasOrientation = false;
};

//=============================================================================================================
/**
 * @brief Streaming parser for FastTrak / FastSCAN ASCII records.
 *
 * Thread-affine: the caller is responsible for serialising calls. No
 * heap allocations on the hot path other than the internal buffer.
 */
class UTILSSHARED_EXPORT FastTrakParser
{
public:
    enum class Units {
        Inches,           ///< FastTrak factory default.
        Centimetres       ///< FastSCAN / G4 default.
    };

    FastTrakParser() = default;

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
    bool nextSample(FastTrakSample& out);

    /** Reset accumulated buffer state. */
    void reset() { m_buffer.clear(); }

    /**
     * Parse one record line (without trailing CR/LF) — exposed for unit
     * tests and for callers that pre-frame their data.
     *
     * @return true on success; false on malformed input.
     */
    static bool parseRecord(const QByteArray& record, Units units, FastTrakSample& out);

private:
    QByteArray m_buffer;
    Units      m_units = Units::Centimetres;
};

} // namespace UTILSLIB

#endif // UTILS_FASTTRAK_PARSER_H
