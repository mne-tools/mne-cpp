//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_hpi_model_parameters.h
 * @since March 2026
 * @brief Immutable configuration for the HPI signal model — coil drive frequencies, sample rate, line frequency and model variant.
 *
 * @ref INVLIB::InvHpiModelParameters captures the four quantities that
 * fully determine the design of the sinusoidal HPI signal model:
 * the list of HPI coil drive frequencies (typically four MEGIN /
 * Elekta coils between 154 and 304 Hz), the sampling frequency of the
 * acquisition, the local power-line frequency (50 / 60 Hz) and a
 * @c bBasic flag that selects whether the line component is included
 * in the regressor matrix. Value-typed with copy semantics and
 * equality operators so the cache logic in @ref InvHpiDataUpdater and
 * @ref InvSignalModel can detect changes cheaply.
 */

#ifndef INV_HPI_MODEL_PARAMETERS_H
#define INV_HPI_MODEL_PARAMETERS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include "../inv_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QVector>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE HPILIBE
//=============================================================================================================

namespace INVLIB {

//=============================================================================================================
// HPILIBE FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief Configuration parameters for the HPI signal model (line frequency, coil frequencies, sample rate, buffer size)
 */
class INVSHARED_EXPORT InvHpiModelParameters
{
public:
    typedef QSharedPointer<InvHpiModelParameters> SPtr;            /**< Shared pointer type for InvHpiModelParameters. */
    typedef QSharedPointer<const InvHpiModelParameters> ConstSPtr; /**< Const shared pointer type for InvHpiModelParameters. */

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
    explicit InvHpiModelParameters(const QVector<int> vecHpiFreqs,
                                const int iSampleFreq,
                                const int iLineFreq,
                                const bool bBasic);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] hpiModelParameter   InvHpiModelParameters which should be copied.
     */
    InvHpiModelParameters(const InvHpiModelParameters &hpiModelParameter);

    //=========================================================================================================

    InvHpiModelParameters operator= (const InvHpiModelParameters& other);
    inline bool operator== (const InvHpiModelParameters &b) const;
    inline bool operator!= (const InvHpiModelParameters &b) const;

    //=========================================================================================================
    /**
    * Inline functions to get acces to parameters.
    */

    inline QVector<int> vecHpiFreqs() const;
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

    QVector<int> m_vecHpiFreqs{QVector<int>()};
    int m_iNHpiCoils{0};
    int m_iSampleFreq{0};
    int m_iLineFreq{0};
    bool m_bBasic{true};
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QVector<int> InvHpiModelParameters::vecHpiFreqs() const
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

//=============================================================================================================

inline bool InvHpiModelParameters::operator== (const InvHpiModelParameters &b) const
{
    return (this->vecHpiFreqs() == b.vecHpiFreqs() &&
            this->iNHpiCoils() == b.iNHpiCoils() &&
            this->iSampleFreq() == b.iSampleFreq() &&
            this->iLineFreq() == b.iLineFreq() &&
            this->bBasic() == b.bBasic());
}

//=============================================================================================================

inline bool InvHpiModelParameters::operator!= (const InvHpiModelParameters &b) const
{
    return !(*this==b);
}

} // namespace INVLIBE

#endif // INV_HPI_MODEL_PARAMETERS_H

