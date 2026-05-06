//=============================================================================================================
/**
 * @file     polhemus_coregistration.h
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
 * @brief    Head–device coregistration using the Polhemus Fastrak.
 *
 *           Two Polhemus stations work together:
 *
 *           - **Tracker station** — a rigid sensor fixed to an external
 *             device (e.g. microscope) at a known calibration offset.
 *             Its continuous stream provides the live device pose in
 *             the Polhemus world (transmitter) frame.
 *
 *           - **Pen station** — the digitization stylus. The operator
 *             touches anatomical landmarks (NAS, LPA, RPA) and free
 *             head-surface points to build up an @ref AcquiredPoints store.
 *
 *           After capturing all three fiducials the caller invokes
 *           @ref computeRegistration to obtain the head→device rigid
 *           transform (@ref headToDevice). This matrix can be used
 *           directly by a neuro-navigation pipeline.
 *
 *           The head coordinate frame follows the standard MEG/EEG convention:
 *             - Origin at the midpoint of LPA and RPA.
 *             - +X toward the nasion.
 *             - +Y toward LPA (patient's left).
 *             - +Z superior (up).
 *
 *           The tracker-to-device calibration offset
 *           (@ref setTrackerToDeviceOffset) describes the rigid transform
 *           from the tracker sensor body frame to the device frame.
 *           It is determined once by a pivot-calibration procedure and
 *           stored in the application settings.
 */

#ifndef UTILS_POLHEMUS_COREGISTRATION_H
#define UTILS_POLHEMUS_COREGISTRATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../utils_global.h"
#include "acquired_points.h"
#include "polhemus_connection.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMatrix4x4>
#include <QObject>
#include <QQuaternion>
#include <QVector3D>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Head–device coregistration engine driven by a Polhemus Fastrak.
 *
 * The class wires itself to a @ref PolhemusConnection and maintains:
 *   - A continuously updated device-to-world transform (from the tracker
 *     station + user-supplied calibration offset).
 *   - An @ref AcquiredPoints store filled by explicit capture calls or
 *     automatically on pen-button press.
 *   - The computed head–device transform after @ref computeRegistration.
 */
class UTILSSHARED_EXPORT PolhemusCoregistration : public QObject
{
    Q_OBJECT

public:
    explicit PolhemusCoregistration(QObject* parent = nullptr);
    ~PolhemusCoregistration() override = default;

    //=========================================================================================================
    // Station assignment
    //=========================================================================================================

    void setTrackerStation(int station);
    void setPenStation(int station);

    int trackerStation() const { return m_trackerStation; }
    int penStation()     const { return m_penStation; }

    //=========================================================================================================
    // Tracker-to-device calibration offset
    //=========================================================================================================

    /**
     * @brief Set the rigid offset from the tracker sensor body frame to the
     *        device frame.
     *
     * @param translation  Translation in metres, expressed in the tracker body frame.
     * @param rotation     Rotation from tracker body frame to device frame.
     */
    void setTrackerToDeviceOffset(const QVector3D& translation,
                                  const QQuaternion& rotation);

    QVector3D   trackerToDeviceTranslation() const { return m_offsetTranslation; }
    QQuaternion trackerToDeviceRotation()    const { return m_offsetRotation; }

    //=========================================================================================================
    // Digitizer connection
    //=========================================================================================================

    void setConnection(PolhemusConnection* conn);
    PolhemusConnection* connection() const { return m_pConn; }

    //=========================================================================================================
    // Digitized-point store
    //=========================================================================================================

    AcquiredPoints* acquiredPoints() const { return m_pPoints; }

    //=========================================================================================================
    // Explicit capture API
    //=========================================================================================================

    bool captureCurrentPenPositionAsFiducial(FiducialId id);
    bool captureCurrentPenPositionAsHeadShape();

    //=========================================================================================================
    // Registration
    //=========================================================================================================

    /**
     * @brief Compute the head→device rigid transform from the three
     *        captured fiducials (NAS, LPA, RPA).
     *
     * @return @c true on success; @c false if any fiducial is missing or the
     *         fiducials are degenerate (collinear).
     */
    bool computeRegistration();

    bool registrationValid() const { return m_registrationValid; }

    //=========================================================================================================
    // Current state accessors
    //=========================================================================================================

    QMatrix4x4  deviceToWorld()    const { return m_deviceToWorld; }
    QMatrix4x4  headToWorld()      const { return m_headToWorld; }
    QMatrix4x4  headToDevice()     const { return m_headToDevice; }
    QVector3D   penPosition()      const { return m_penPosition; }
    QQuaternion penOrientation()   const { return m_penOrientation; }
    bool        haveLivePenPosition() const { return m_havePenPos; }

signals:
    void devicePoseChanged(const QMatrix4x4& deviceToWorld);
    void penPoseChanged(const QVector3D& position, const QQuaternion& orientation);
    void penButtonPressed(const QVector3D& position, const QQuaternion& orientation);
    void registrationChanged();

private slots:
    void onPointReceived(int station, const QVector3D& position, const QQuaternion& orientation);
    void onPenButtonPressedFromConn(int station, const QVector3D& position, const QQuaternion& orientation);

private:
    QMatrix4x4 buildDevicePose(const QVector3D& trackerPos,
                               const QQuaternion& trackerOri) const;
    QMatrix4x4 buildHeadFrame() const;

    int m_trackerStation = 1;
    int m_penStation     = 2;

    QVector3D   m_offsetTranslation;
    QQuaternion m_offsetRotation;

    PolhemusConnection* m_pConn   = nullptr;
    AcquiredPoints*     m_pPoints = nullptr;

    QVector3D   m_penPosition;
    QQuaternion m_penOrientation;
    bool        m_havePenPos = false;

    QMatrix4x4  m_deviceToWorld;
    QMatrix4x4  m_headToWorld;
    QMatrix4x4  m_headToDevice;
    bool        m_registrationValid = false;
};

} // namespace UTILSLIB

#endif // UTILS_POLHEMUS_COREGISTRATION_H
