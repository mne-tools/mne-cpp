//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_hpi_model_parameters.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Immutable configuration for the HPI signal model — coil drive frequencies, sample rate, line frequency and model variant.
 *
 * @ref mne::InvHpiModelParameters captures the four quantities that
 * fully determine the design of the sinusoidal HPI signal model:
 * the list of HPI coil drive frequencies (typically four MEGIN /
 * Elekta coils between 154 and 304 Hz), the sampling frequency of the
 * acquisition, the local power-line frequency (50 / 60 Hz) and a
 * @c bBasic flag that selects whether the line component is included
 * in the regressor matrix. Value-typed with copy semantics and
 * equality operators so the cache logic in @ref mne::InvHpiDataUpdater and
 * @ref mne::InvSignalModel can detect changes cheaply.
 */

#ifndef INV_HPI_MODEL_PARAMETERS_H
#define INV_HPI_MODEL_PARAMETERS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include "../inv_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

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
 * Description of what this class is intended to do (in detail).
 *
 * @brief Configuration parameters for the HPI signal model (line frequency, coil frequencies, sample rate, buffer size)
 *
 * @snippet ex_inv_hpi/main.cpp inv_hpi_usage
 */
class INVSHARED_EXPORT InvHpiModelParameters
{
public:
    //=========================================================================================================
    /**
    * Defaul Constructor.
    */
    InvHpiModelParameters() = default;

    //=========================================================================================================
    /**
    * Constructs a InvHpiModelParameters object.
    *
    * @param[in] vecHpiFreqs     The Hpi frequencies.
    * @param[in] iSampleFreq     The sampling frequency.
    * @param[in] iLineFreq       The line Frequency
    * @param[in] bBasic          Create a basic model without line frequeny or not.
    */
    explicit InvHpiModelParameters(std::vector<int> vecHpiFreqs,
                                   const int iSampleFreq,
                                   const int iLineFreq,
                                   const bool bBasic);

    //=========================================================================================================
    /**
     * @param[in] other   The parameters to compare with.
     *
     * @return Whether all parameters are equal.
     */
    bool operator==(const InvHpiModelParameters& other) const = default;

    //=========================================================================================================
    /**
    * Inline functions to get acces to parameters.
    *
    * @return HPI coil frequencies, in Hz.
    */

    inline const std::vector<int>& vecHpiFreqs() const;
    inline int iNHpiCoils() const;
    inline int iSampleFreq() const;
    inline int iLineFreq() const;
    inline bool bBasic() const;

private:
    //=========================================================================================================
    /**
    * Compute the number of coils.
    */
    void computeNumberOfCoils();

    //=========================================================================================================
    /**
    * Check line frequencies and set model type accordingly.
    */
    void checkForLineFreq();

    std::vector<int> m_vecHpiFreqs;
    int m_iNHpiCoils{0};
    int m_iSampleFreq{0};
    int m_iLineFreq{0};
    bool m_bBasic{true};
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline const std::vector<int>& InvHpiModelParameters::vecHpiFreqs() const
{
    return m_vecHpiFreqs;
}

//=============================================================================================================

inline int InvHpiModelParameters::iNHpiCoils() const
{
    return m_iNHpiCoils;
}

//=============================================================================================================

inline int InvHpiModelParameters::iSampleFreq() const
{
    return m_iSampleFreq;
}

//=============================================================================================================

inline int InvHpiModelParameters::iLineFreq() const
{
    return m_iLineFreq;
}

//=============================================================================================================

inline bool InvHpiModelParameters::bBasic() const
{
    return m_bBasic;
}


} // namespace INVLIBE

#endif // INV_HPI_MODEL_PARAMETERS_H
