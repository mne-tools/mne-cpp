//=============================================================================================================
/**
 * @file     hpimodelparameters.h
 * @author   Ruben Doerfel <doerfelruben@aol.com>
 * @since    0.1.0
 * @date     February, 2022
 *
 * @section  LICENSE
 *
 * Copyright (C) 2022, Ruben Doerfel. All rights reserved.
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
 * @brief     HpiModelParameters class declaration.
 *
 */

#ifndef INVERSELIBE_HPIMODELPARAMETERS_H
#define INVERSELIBE_HPIMODELPARAMETERS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include "../inverse_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE INVERSELIBE
//=============================================================================================================


namespace INVERSELIB {


//=============================================================================================================
// INVERSELIBE FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief Brief description of this class.
 */
class INVERSESHARED_EXPORT HpiModelParameters
{
public:
    typedef QSharedPointer<HpiModelParameters> SPtr;            /**< Shared pointer type for HpiModelParameters. */
    typedef QSharedPointer<const HpiModelParameters> ConstSPtr; /**< Const shared pointer type for HpiModelParameters. */

    //=========================================================================================================
    /**
    * Defaul Constructor.
    *
    */
    HpiModelParameters();

    //=========================================================================================================
    /**
    * Constructs a HpiModelParameters object.
    *
    * @param[in] vecHpiFreqs     The Hpi frequencies.
    * @param[in] iSampleFreq     The sampling frequency.
    * @param[in] iLineFreq       The line Frequency
    * @param[in] bBasic          Create a basic model without line frequeny or not.
    */
    explicit HpiModelParameters(const QVector<int> vecHpiFreqs,
                                const int iSampleFreq,
                                const int iLineFreq,
                                const bool bBasic);

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] hpiModelParameter   HpiModelParameters which should be copied.
     */
    HpiModelParameters(const HpiModelParameters &hpiModelParameter);

    //=========================================================================================================

    HpiModelParameters operator= (const HpiModelParameters& other);
    inline bool operator== (const HpiModelParameters &b) const;
    inline bool operator!= (const HpiModelParameters &b) const;

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

    QVector<int> m_vecHpiFreqs;
    int m_iNHpiCoils = 0;
    int m_iSampleFreq = 0;
    int m_iLineFreq = 0;
    bool m_bBasic = true;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QVector<int> HpiModelParameters::vecHpiFreqs() const
{
    return m_vecHpiFreqs;
}

//=============================================================================================================

inline int HpiModelParameters::iNHpiCoils() const
{
    return m_iNHpiCoils;
}

//=============================================================================================================

inline int HpiModelParameters::iSampleFreq() const

//=============================================================================================================

{
    return m_iSampleFreq;
}

//=============================================================================================================

inline int HpiModelParameters::iLineFreq() const
{
    return m_iLineFreq;
}

//=============================================================================================================

inline bool HpiModelParameters::bBasic() const
{
    return m_bBasic;
}

//=============================================================================================================

inline bool HpiModelParameters::operator== (const HpiModelParameters &b) const
{
    return (this->vecHpiFreqs() == b.vecHpiFreqs() &&
            this->iNHpiCoils() == b.iNHpiCoils() &&
            this->iSampleFreq() == b.iSampleFreq() &&
            this->iLineFreq() == b.iLineFreq() &&
            this->bBasic() == b.bBasic());
}

//=============================================================================================================

inline bool HpiModelParameters::operator!= (const HpiModelParameters &b) const
{
    return !(*this==b);
}

} // namespace INVERSELIBE

#endif // INVERSELIBE_HPIMODELPARAMETERS_H

