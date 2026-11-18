//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     connectivitysettings.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     March 2017
 * @brief    Input-data and parameter container shared by every functional-connectivity metric in @c mne_connectivity.
 *
 * @ref mne::ConnectivitySettings carries the per-trial time-domain matrices, the
 * derived per-trial cross-spectral data (DPSS tapered FFTs, CSDs and the
 * imaginary-part variants needed by PLI / wPLI / dwPLI), and the
 * accumulated cross-trial sums used to normalise the final estimates.
 * Holding all of this in one object lets the dispatcher in @ref mne::compute_connectivity
 * compute several metrics in one pass without re-tapering and re-FFT'ing
 * the same trials: the @ref mne::ConnectivitySettings::IntermediateTrialData fields are filled lazily
 * by the first metric that needs them and reused by everyone else.
 *
 * The container also stores the spectral-domain parameters (sampling rate,
 * FFT length, taper window name) and the 3D node positions used to lay out
 * the resulting @ref mne::Network - the latter can be sourced either from a
 * @c FiffInfo channel set (sensor space) or from a forward solution and
 * @c FsSurfaceSet pair (source space).
 */

#ifndef CONNECTIVITYSETTINGS_H
#define CONNECTIVITYSETTINGS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivity_global.h"

#include <vector>
#include <memory>
#include <span>
#include <string>
#include <utility>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class MNEForwardSolution;
}

namespace mne
{
class FsSurfaceSet;
}

namespace mne
{
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// mne FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Mutable container that aggregates the inputs and intermediate spectral
 * results required by every metric implementation in @c mne_connectivity.
 *
 * Each call to @c mne::ConnectivitySettings::append "append" adds one trial; the trial matrices live in
 * @ref m_trialData, while DPSS tapered spectra, cross-spectral densities
 * and their imaginary-part derivatives are filled in lazily by whichever
 * metric runs first. Cross-trial sums accumulate in @ref m_intermediateSumData
 * so that a second metric run on the same data set reuses those sums and
 * skips the FFT pass entirely. Node positions can be derived from a
 * @c FiffInfo (sensor space) or from a forward solution + @c FsSurfaceSet
 * (source space) and are propagated into the resulting @ref Network nodes.
 *
 * @brief Aggregates trial data, spectral cache and node geometry shared by all @c mne_connectivity metrics.
 *
 * @snippet ex_connectivity_metrics/main.cpp connectivity_settings_setup
 *
 * @snippet ex_connectivity_metrics/main.cpp connectivity_frequency_bins
 *
 * @snippet ex_connectivity_metrics/main.cpp connectivity_calculate
 */
class CONNECTIVITYSHARED_EXPORT ConnectivitySettings
{
public:
    /**
     * @brief Per-trial intermediate frequency-domain data used during connectivity computation
     */
    struct IntermediateTrialData
    {
        Eigen::MatrixXd matData;
        Eigen::MatrixXd matPsd;
        std::vector<Eigen::MatrixXcd> vecTapSpectra;
        std::vector<std::pair<int, Eigen::MatrixXcd>> vecPairCsd;
        std::vector<std::pair<int, Eigen::MatrixXcd>> vecPairCsdNormalized;
        std::vector<std::pair<int, Eigen::MatrixXd>> vecPairCsdImagSign;
        std::vector<std::pair<int, Eigen::MatrixXd>> vecPairCsdImagAbs;
        std::vector<std::pair<int, Eigen::MatrixXd>> vecPairCsdImagSqrd;
    };

    /**
     * @brief Accumulated cross-spectral and auto-spectral sums across trials for final metric normalization
     */
    struct IntermediateSumData
    {
        Eigen::MatrixXd matPsdSum;
        std::vector<std::pair<int, Eigen::MatrixXcd>> vecPairCsdSum;
        std::vector<std::pair<int, Eigen::MatrixXcd>> vecPairCsdNormalizedSum;
        std::vector<std::pair<int, Eigen::MatrixXd>> vecPairCsdImagSignSum;
        std::vector<std::pair<int, Eigen::MatrixXd>> vecPairCsdImagAbsSum;
        std::vector<std::pair<int, Eigen::MatrixXd>> vecPairCsdImagSqrdSum;
    };

    //=========================================================================================================
    /**
     * Constructs a ConnectivitySettings object.
     */
    explicit ConnectivitySettings();

    void clearAllData();

    void clearIntermediateData();

    void append(std::span<const Eigen::MatrixXd> matInputData);

    void append(const Eigen::MatrixXd& matInputData);

    void append(const ConnectivitySettings::IntermediateTrialData& inputData);

    const IntermediateTrialData& at(int i) const;

    int size() const;

    bool empty() const;

    void removeFirst(int iAmount = 1);

    void removeLast(int iAmount = 1);

    void setConnectivityMethods(std::vector<std::string> sConnectivityMethods);

    const std::vector<std::string>& getConnectivityMethods() const;

    void setSamplingFrequency(int iSFreq);

    int getSamplingFrequency() const;

    void setFFTSize(int iNfft);

    int getFFTSize() const;

    //=========================================================================================================
    /**
     * Restricts the spectral metrics to a range of frequency bins of the half spectrum. Changing it drops
     * the kept per-trial spectra.
     *
     * @param[in] start   First bin; -1 for the whole spectrum.
     * @param[in] count   Number of bins; -1 for the whole spectrum.
     */
    void set_frequency_bins(int start, int count);

    //=========================================================================================================
    /**
     * @return The first frequency bin and the number of bins the spectral metrics use: the whole half
     *         spectrum of the FFT size unless a range inside it was set.
     */
    [[nodiscard]] std::pair<int, int> frequency_bins() const;

    //=========================================================================================================
    /**
     * Keeps the tapered spectra and cross-spectra of each trial, so that the next metric call computes only
     * those of the trials appended since. Off by default to save memory.
     *
     * @param[in] keep   Whether to keep them.
     */
    void set_keep_trial_data(bool keep);

    //=========================================================================================================
    /**
     * @return Whether the per-trial spectra are kept between metric calls.
     */
    [[nodiscard]] bool keeps_trial_data() const;

    void setWindowType(std::string sWindowType);

    const std::string& getWindowType() const;

    void setNodePositions(const mne::FiffInfo& fiffInfo,
                          const Eigen::RowVectorXi& picks);

    void setNodePositions(const mne::MNEForwardSolution& forwardSolution,
                          const mne::FsSurfaceSet& surfSet);

    void setNodePositions(const Eigen::MatrixX3f& matNodePositions);

    const Eigen::MatrixX3f& getNodePositions() const;

    std::vector<IntermediateTrialData>& getTrialData();

    IntermediateSumData& getIntermediateSumData();

protected:
    std::vector<std::string> m_sConnectivityMethods; /**< The connectivity methods. */
    std::string m_sWindowType;                       /**< The window type used to compute tapered spectra. */

    float m_fSFreq;               /**< The sampling frequency. */
    int m_binStart = -1;          /**< First frequency bin of the spectral metrics; -1 for all. */
    int m_binCount = -1;          /**< Number of frequency bins; -1 for all. */
    bool m_keepTrialData = false; /**< Whether the per-trial spectra are kept between calls. */
    int m_iNfft;                  /**< The FFT length. Also includes the negativ frequencies. Gets recalculated if the sFreq or spectrum resolution change. */
    float m_fFreqResolution;      /**< The spectrum's resolution. */

    Eigen::MatrixX3f m_matNodePositions; /**< The node position in 3D space. */

    IntermediateSumData m_intermediateSumData;      /**< The intermediate sum data holds data calculated over all trials as a whole. */
    std::vector<IntermediateTrialData> m_trialData; /**< The trial data holds the actual and intermediate data calcualted for each trial. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace mne

#endif // CONNECTIVITYSETTINGS_H
