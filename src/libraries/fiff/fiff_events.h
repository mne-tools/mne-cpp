//=============================================================================================================
/**
 * @file     fiff_events.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    FiffEvents class declaration.
 *
 */

#ifndef FIFF_EVENTS_H
#define FIFF_EVENTS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QIODevice;

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

// Forward declaration
class FiffRawData;

//=============================================================================================================
/**
 * Provides FIFF event storage and I/O operations.
 *
 * Each event is stored as a row in an Nx3 matrix: [sample, before, after].
 * The class provides methods to read/write events from/to FIFF or ASCII files,
 * and to detect events from raw data trigger channels.
 *
 * @brief FIFF event container with reading, writing and detection.
 */
class FIFFSHARED_EXPORT FiffEvents
{

public:
    //=========================================================================================================
    /**
     * Default constructor. Creates an empty event set.
     */
    FiffEvents();

    //=========================================================================================================
    /**
     * Constructs FiffEvents by reading from an I/O device.
     *
     * Attempts to read as FIFF first; if that fails, tries ASCII format.
     *
     * @param[in] p_IODevice    The I/O device to read from.
     */
    explicit FiffEvents(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Read events from a FIFF or ASCII event file.
     *
     * If t_sEventName is empty, the event file name is derived from the raw file name
     * (replacing ".fif" with "-eve.fif").
     *
     * @param[in] t_sEventName  Path to the event file (empty = derive from raw file name).
     * @param[in] t_fileRawName Path to the raw data file (used to derive event file name if needed).
     * @param[out] p_Events     The loaded events.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read(const QString &t_sEventName,
                     const QString &t_fileRawName,
                     FiffEvents &p_Events);

    //=========================================================================================================
    /**
     * Read an event list from a FIFF file.
     *
     * @param[in] p_IODevice    The I/O device to read from.
     * @param[out] p_Events     The loaded events.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read_from_fif(QIODevice &p_IODevice,
                              FiffEvents &p_Events);

    //=========================================================================================================
    /**
     * Read a list of events from an ASCII event file.
     *
     * @param[in] p_IODevice    The I/O device to read from.
     * @param[out] p_Events     The loaded events.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool read_from_ascii(QIODevice &p_IODevice,
                                FiffEvents &p_Events);

    //=========================================================================================================
    /**
     * Write the event list to a FIFF file.
     *
     * @param[in] p_IODevice    The I/O device to write to.
     *
     * @return true if succeeded, false otherwise.
     */
    bool write_to_fif(QIODevice &p_IODevice) const;

    //=========================================================================================================
    /**
     * Write the event list to a text file (MNE-C compatible format).
     *
     * @param[in] p_IODevice    The I/O device to write to.
     * @param[in] sfreq         Sampling frequency for time column computation.
     *
     * @return true if succeeded, false otherwise.
     */
    bool write_to_ascii(QIODevice &p_IODevice,
                        float sfreq = 0.0f) const;

    //=========================================================================================================
    /**
     * Detect trigger events from raw data using a stimulus channel.
     * Ported from detect_events (MNE-C).
     *
     * @param[in] raw           The raw data.
     * @param[out] p_Events     The detected events.
     * @param[in] triggerCh     Name of the trigger channel (empty = default "STI 014").
     * @param[in] triggerMask   Bitmask to apply to trigger values.
     * @param[in] leadingEdge   If true, only detect rising edges (default = true).
     *
     * @return true if events were detected, false otherwise.
     */
    static bool detect_from_raw(const FiffRawData &raw,
                                FiffEvents &p_Events,
                                const QString &triggerCh = QString("STI 014"),
                                unsigned int triggerMask = 0xFFFFFFFF,
                                bool leadingEdge = true);

    //=========================================================================================================
    /**
     * Returns the number of events.
     *
     * @return Number of events.
     */
    inline int num_events() const;

    //=========================================================================================================
    /**
     * Returns true if no events are stored.
     *
     * @return true if the event matrix has zero rows.
     */
    inline bool is_empty() const;

    Eigen::MatrixXi events;     /**< Event matrix (nEvents x 3): [sample, before, after]. */
};

} // NAMESPACE

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline int FIFFLIB::FiffEvents::num_events() const
{
    return static_cast<int>(events.rows());
}

//=============================================================================================================

inline bool FIFFLIB::FiffEvents::is_empty() const
{
    return events.rows() == 0;
}

#endif // FIFF_EVENTS_H
