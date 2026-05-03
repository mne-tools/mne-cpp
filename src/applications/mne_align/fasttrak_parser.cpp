//=============================================================================================================
/**
 * @file     fasttrak_parser.cpp
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
 * @brief    Implementation of @ref MNEALIGN::FastTrakParser.
 */

#include "fasttrak_parser.h"

using namespace MNEALIGN;

namespace {

constexpr float kInchToMetre       = 0.0254f;
constexpr float kCentimetreToMetre = 0.01f;

float linearScale(FastTrakParser::Units units)
{
    return (units == FastTrakParser::Units::Inches) ? kInchToMetre
                                                    : kCentimetreToMetre;
}

} // namespace

//=============================================================================================================

bool FastTrakParser::parseRecord(const QByteArray& record, Units units, FastTrakSample& out)
{
    // Tokenise on whitespace; a record is "<station> <x> <y> <z> [<az> <el> <ro>]".
    const QList<QByteArray> tokens = record.simplified().split(' ');
    if (tokens.size() < 4) {
        return false;
    }

    bool ok = false;
    const int station = tokens[0].toInt(&ok);
    if (!ok) {
        return false;
    }

    auto parseFloat = [&ok](const QByteArray& t) -> float {
        bool good = false;
        const float v = t.toFloat(&good);
        ok = ok && good;
        return v;
    };

    ok = true;
    const float x = parseFloat(tokens[1]);
    const float y = parseFloat(tokens[2]);
    const float z = parseFloat(tokens[3]);
    if (!ok) {
        return false;
    }

    const float scale = linearScale(units);
    out.station        = station;
    out.position       = QVector3D(x * scale, y * scale, z * scale);
    out.hasOrientation = false;
    out.orientation    = QQuaternion();

    if (tokens.size() >= 7) {
        const float az = parseFloat(tokens[4]);
        const float el = parseFloat(tokens[5]);
        const float ro = parseFloat(tokens[6]);
        if (ok) {
            // Polhemus convention: yaw (azimuth, Z), pitch (elevation, Y), roll (X),
            // intrinsic ZYX order. QQuaternion::fromEulerAngles uses (pitch, yaw, roll)
            // around (X, Y, Z) intrinsic; pre-compose explicitly to avoid surprises.
            const QQuaternion qZ = QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 1.0f, az);
            const QQuaternion qY = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, el);
            const QQuaternion qX = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, ro);
            out.orientation    = qZ * qY * qX;
            out.hasOrientation = true;
        }
    }

    return true;
}

//=============================================================================================================

bool FastTrakParser::nextSample(FastTrakSample& out)
{
    while (true) {
        const int eol = m_buffer.indexOf('\n');
        if (eol < 0) {
            return false;
        }

        QByteArray line = m_buffer.left(eol);
        m_buffer.remove(0, eol + 1);
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        if (line.trimmed().isEmpty()) {
            continue; // skip blank line, keep scanning
        }
        if (parseRecord(line, m_units, out)) {
            return true;
        }
        // malformed line → drop and keep scanning
    }
}
