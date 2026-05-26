//=============================================================================================================
/**
 * @file     fastrak_parser.cpp
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
 * @brief    Implementation of @ref UTILSLIB::FastrakParser.
 */

#include "fastrak_parser.h"

using namespace UTILSLIB;

namespace {

constexpr float kInchToMetre       = 0.0254f;
constexpr float kCentimetreToMetre = 0.01f;

float linearScale(FastrakParser::Units units)
{
    return (units == FastrakParser::Units::Inches) ? kInchToMetre
                                                    : kCentimetreToMetre;
}

} // namespace

//=============================================================================================================

bool FastrakParser::parseRecord(const QByteArray& record, Units units, FastrakSample& out)
{
    // Fastrak uses fixed-width ASCII fields.  When a negative value
    // immediately follows a positive one the minus sign eats the
    // separating space, e.g. "0.8829-0.3055".  Insert a space before
    // every '-' that is preceded by a digit so tokenisation works.
    QByteArray fixed = record.simplified();
    for (int i = fixed.size() - 1; i > 0; --i) {
        const char c  = fixed.at(i);
        const char pc = fixed.at(i - 1);
        if (c == '-' && pc >= '0' && pc <= '9') {
            fixed.insert(i, ' ');
        }
    }

    const QList<QByteArray> tokens = fixed.split(' ');
    if (tokens.size() < 4) {
        return false;
    }

    // One-shot debug: log the first record to confirm output format
    static bool s_firstRecord = true;
    if (s_firstRecord) {
        qInfo() << "FastrakParser: raw record:" << record.simplified();
        qInfo() << "FastrakParser: fixed →" << tokens.size() << "tokens:" << fixed;
        s_firstRecord = false;
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

    if (tokens.size() >= 8) {
        // Quaternion output (O-item 11): q0 q1 q2 q3 (scalar-first).
        // No gimbal lock — preferred over Euler angles.
        const float q0 = parseFloat(tokens[4]);
        const float q1 = parseFloat(tokens[5]);
        const float q2 = parseFloat(tokens[6]);
        const float q3 = parseFloat(tokens[7]);
        if (ok) {
            out.orientation    = QQuaternion(q0, q1, q2, q3).normalized();
            out.hasOrientation = true;
        }
    } else if (tokens.size() >= 7) {
        // Euler angle output (O-item 4): azimuth, elevation, roll.
        // Kept for backward compatibility; suffers from gimbal lock at elevation ±90°.
        const float az = parseFloat(tokens[4]);
        const float el = parseFloat(tokens[5]);
        const float ro = parseFloat(tokens[6]);
        if (ok) {
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

bool FastrakParser::nextSample(FastrakSample& out)
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
