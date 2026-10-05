//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fine_calibration.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Fine calibration data for SSS.
 *
 * Stores per-sensor calibration coefficients (gain and cross-talk imbalance)
 * used to refine the SSS forward model. Equivalent to MNE-Python's
 * mne.preprocessing.read_fine_calibration / write_fine_calibration.
 */

#ifndef FINE_CALIBRATION_DSP_H
#define FINE_CALIBRATION_DSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Per-sensor fine calibration entry.
 */
struct DSPSHARED_EXPORT FineCalEntry
{
    int chNumber = 0;                                         /**< MEG channel number (e.g., 113 for MEG0113). */
    Eigen::Vector3d position{0, 0, 0};                        /**< Coil position in device coordinates (metres). */
    Eigen::Matrix3d orientation{Eigen::Matrix3d::Identity()}; /**< Rows: coil x, y and z (normal) axes. */
    Eigen::VectorXd imbalance{Eigen::VectorXd::Ones(1)};      /**< One term (magnetometer calibration) or three (gradiometer imbalance). */
};

//=============================================================================================================
/**
 * @brief Fine calibration data for SSS.
 *
 * A fine calibration file (.dat) has one line per MEG sensor, as read and written by
 * mne.preprocessing.read_fine_calibration / write_fine_calibration:
 *   channel_number  position(3)  x_axis(3)  y_axis(3)  z_axis(3)  imbalance(1 or 3)
 *
 * @snippet ex_dsp_maxwell/main.cpp fine_calibration_usage
 */
class DSPSHARED_EXPORT FineCalibration
{
public:
    FineCalibration() = default;

    //=========================================================================================================
    /**
     * @brief Read a fine calibration file (.dat format).
     *
     * Each line has 14 or 16 whitespace-separated columns (see the class description);
     * lines starting with '#' are comments and malformed lines are skipped.
     *
     * @param[in] sPath  Path to .dat file.
     *
     * @return FineCalibration with loaded entries.
     */
    static FineCalibration read(const QString& sPath);

    //=========================================================================================================
    /**
     * @brief Write fine calibration to a .dat file.
     *
     * @param[in] sPath  Output file path.
     *
     * @return true if successful.
     */
    bool write(const QString& sPath) const;

    //=========================================================================================================
    /**
     * @brief Get the calibration entries.
     *
     * @return Const reference to the list of per-channel calibration entries.
     */
    const QList<FineCalEntry>& entries() const
    {
        return m_entries;
    }

    //=========================================================================================================
    /**
     * @brief Get the number of entries.
     *
     * @return Number of calibration entries (one per MEG channel).
     */
    int size() const
    {
        return m_entries.size();
    }

    //=========================================================================================================
    /**
     * @brief Check if empty.
     *
     * @return True if no calibration entries are stored.
     */
    bool isEmpty() const
    {
        return m_entries.isEmpty();
    }

    //=========================================================================================================
    /**
     * @brief Find entry by channel number.
     *
     * @param[in] chNumber  Channel number to find.
     * @param[out] entry    Found entry (if any).
     *
     * @return true if found.
     */
    bool findEntry(int chNumber, FineCalEntry& entry) const;

    //=========================================================================================================
    /**
     * @brief Add an entry.
     *
     * @param[in] entry  Calibration entry appended to the end of the list.
     */
    void addEntry(const FineCalEntry& entry)
    {
        m_entries.append(entry);
    }

    //=========================================================================================================
    /**
     * @brief Magnetometer calibration factors.
     *
     * @return One factor per entry: the single calibration term of a magnetometer
     *         (channel number ending in 1), 1.0 for gradiometers.
     */
    Eigen::VectorXd gainVector() const;

    //=========================================================================================================
    /**
     * @brief Gradiometer imbalance terms.
     *
     * @return Matrix (n_entries × 3): the one or three imbalance terms of each
     *         gradiometer, zero-padded; zero rows for magnetometers.
     */
    Eigen::MatrixXd imbalanceMatrix() const;

private:
    QList<FineCalEntry> m_entries;
};

} // namespace UTILSLIB

#endif // FINE_CALIBRATION_DSP_H
