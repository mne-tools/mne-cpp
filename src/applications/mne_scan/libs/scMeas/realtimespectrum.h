//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     realtimespectrum.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the RealTimeSpectrum class.
 */

#ifndef REALTIMESPECTRUM_H
#define REALTIMESPECTRUM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"
#include "measurement.h"
#include "realtimesamplearraychinfo.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>
#include <QList>
#include <QColor>

//=============================================================================================================
// DEFINE NAMESPACE SCMEASLIB
//=============================================================================================================

namespace SCMEASLIB
{

//=========================================================================================================
/**
 * DECLARE CLASS RealTimeSpectrum
 *
 * @brief The RealTimeSpectrum class is the base class of every RealTimeSpectrum Measurement.
 */
class SCMEASSHARED_EXPORT RealTimeSpectrum : public Measurement
{
    Q_OBJECT

public:
    typedef QSharedPointer<RealTimeSpectrum> SPtr;               /**< Shared pointer type for RealTimeSpectrum. */
    typedef QSharedPointer<const RealTimeSpectrum> ConstSPtr;    /**< Const shared pointer type for RealTimeSpectrum. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeSpectrum.
     */
    explicit RealTimeSpectrum(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTimeSpectrum.
     */
    virtual ~RealTimeSpectrum();

    //=========================================================================================================
    /**
     * Init channel infos using fiff info
     *
     * @param[in] p_pFiffInfo     Info to init from.
     */
    void initFromFiffInfo(FIFFLIB::FiffInfo::SPtr &p_pFiffInfo);

    //=========================================================================================================
    /**
     * Init Scale Type
     *
     * @param[in] ScaleType     Scale type to init from.
     */
    void initScaleType(qint8 ScaleType);

    //=========================================================================================================
    /**
     * Returns whether channel info is initialized
     *
     * @return true whether the channel info is available.
     */
    inline bool isInit() const;

    //=========================================================================================================
    /**
     * Returns the reference to the orig FiffInfo.
     *
     * @return the reference to the orig FiffInfo.
     */
    inline FIFFLIB::FiffInfo::SPtr& getFiffInfo();

    //=========================================================================================================
    /**
     * Returns the scale type.
     *
     * @return the scale type.
     */
    inline qint8 getScaleType();

    //=========================================================================================================
    /**
     * New data block to distribute
     *
     * @param[in] v the value which is should be distributed.
     */
    virtual void setValue(Eigen::MatrixXd& v);

    //=========================================================================================================
    /**
     * Returns the current value set.
     * This method is inherited by Measurement.
     *
     * @return the last attached value.
     */
    virtual Eigen::MatrixXd getValue() const;

    //=========================================================================================================
    /**
     * Returns whether RealTimeSpectrum contains values
     *
     * @return whether RealTimeSpectrum contains values.
     */
    inline bool containsValues() const;

private:
    FIFFLIB::FiffInfo::SPtr         m_pFiffInfo;    /**< Original Fiff Info if initialized by fiff info. */

    Eigen::MatrixXd                 m_matValue;     /**< The current attached sample vector.*/

    bool m_bIsInit;             /**< If channel info is initialized.*/
    bool m_bContainsValues;     /**< If values are stored.*/

    qint8 m_xScaleType;         /**< The scale type of x axis: 0-normal; 1-log. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool RealTimeSpectrum::isInit() const
{
    return m_bIsInit;
}

//=============================================================================================================

inline FIFFLIB::FiffInfo::SPtr& RealTimeSpectrum::getFiffInfo()
{
    return m_pFiffInfo;
}

//=============================================================================================================

inline qint8 RealTimeSpectrum::getScaleType()
{
    return m_xScaleType;
}

//=============================================================================================================

inline bool RealTimeSpectrum::containsValues() const
{
    return m_bContainsValues;
}
} // NAMESPACE

Q_DECLARE_METATYPE(SCMEASLIB::RealTimeSpectrum::SPtr)

#endif // REALTIMESPECTRUM_H
