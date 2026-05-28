//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fine_calibration.h
 * @since May 2026
 * @brief Fine calibration data for SSS.
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
    int     chNumber = 0;               /**< MEG channel number (e.g., 0113 for MEG0113). */
    double  dGain = 1.0;                /**< Gain correction factor (nominal = 1.0). */
    Eigen::Vector3d imbalance{0,0,0};   /**< Cross-talk imbalance correction (x, y, z). */
};

//=============================================================================================================
/**
 * @brief Fine calibration data for SSS.
 *
 * A fine calibration file (.dat) contains one line per MEG sensor with:
 *   channel_number  gain  imbalance_x  imbalance_y  imbalance_z
 *
 * Usage:
 * @code
 *   FineCalibration cal = FineCalibration::read("/path/to/sss_cal.dat");
 *   cal.write("/path/to/output.dat");
 * @endcode
 */
class DSPSHARED_EXPORT FineCalibration
{
public:
    FineCalibration() = default;

    //=========================================================================================================
    /**
     * @brief Read a fine calibration file (.dat format).
     *
     * Expected format: whitespace-separated columns per line:
     *   channel_number  gain  imbalance_x  imbalance_y  imbalance_z
     * Lines starting with '#' are comments.
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
     */
    const QList<FineCalEntry>& entries() const { return m_entries; }

    //=========================================================================================================
    /**
     * @brief Get the number of entries.
     */
    int size() const { return m_entries.size(); }

    //=========================================================================================================
    /**
     * @brief Check if empty.
     */
    bool isEmpty() const { return m_entries.isEmpty(); }

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
     */
    void addEntry(const FineCalEntry& entry) { m_entries.append(entry); }

    //=========================================================================================================
    /**
     * @brief Build gain correction diagonal matrix for MEG channels.
     *
     * Returns a diagonal matrix where each diagonal element is the gain
     * factor for the corresponding MEG channel (in the order of m_entries).
     *
     * @return Diagonal gain matrix (n_entries × n_entries).
     */
    Eigen::VectorXd gainVector() const;

    //=========================================================================================================
    /**
     * @brief Build imbalance matrix.
     *
     * Returns a matrix of cross-talk imbalance vectors (n_entries × 3).
     */
    Eigen::MatrixXd imbalanceMatrix() const;

private:
    QList<FineCalEntry> m_entries;
};

} // namespace UTILSLIB

#endif // FINE_CALIBRATION_DSP_H
