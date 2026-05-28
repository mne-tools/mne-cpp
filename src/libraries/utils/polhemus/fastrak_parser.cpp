//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fastrak_parser.cpp
 * @since May 2026
 * @brief Streaming ASCII record decoder that converts Polhemus Fastrak / FastSCAN bytes into @ref UTILSLIB::FastrakSample values.
 *
 * Bytes are accumulated in an internal buffer; complete
 * CR/LF terminated records are tokenised on demand by
 * @c nextSample. Token-count heuristics auto-detect the seven-
 * field Euler record (used by FastSCAN-firmware devices) and
 * the eight-field quaternion record (Fastrak with the
 * @c O,7 output command active), so the same parser drives both
 * generations of hardware. Positions are normalised to SI
 * metres on the fly so downstream coregistration code never
 * needs to know which physical units the device reports.
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
