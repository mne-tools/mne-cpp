//=============================================================================================================
/**
 * @file     colortable.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
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
 * @brief     Colortable class declaration.
 *
 */

#ifndef COLORTABLE_H
#define COLORTABLE_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QStringList>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE FSLIB
//=============================================================================================================

namespace FSLIB
{

//=============================================================================================================
/**
 * Vertices label based lookup table containing colorcodes and anatomical names
 *
 * @brief Vertices label based lookup table
 */
class Colortable
{
public:
    typedef QSharedPointer<Colortable> SPtr;            /**< Shared pointer type for Colortable. */
    typedef QSharedPointer<const Colortable> ConstSPtr; /**< Const shared pointer type for Colortable. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    explicit Colortable();

    //=========================================================================================================
    /**
     * Initializes colortable.
     */
    void clear();

    //=========================================================================================================
    /**
     * Ids encoded in the colortable
     *
     * @return ids
     */
    inline Eigen::VectorXi getLabelIds() const;

    //=========================================================================================================
    /**
     * Names encoded in the colortable
     *
     * @return ids
     */
    inline QStringList getNames() const;

    //=========================================================================================================
    /**
     * RGBAs encoded in the colortable
     *
     * @return RGBAs
     */
    inline Eigen::MatrixX4i getRGBAs() const;

public:
    QString orig_tab;           /**< Colortable raw data */
    qint32 numEntries;          /**< Number of entries */
    QStringList struct_names;   /**< Anatomical ROI description */
    Eigen::MatrixXi table;      /**< labels and corresponing colorcode */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline Eigen::VectorXi Colortable::getLabelIds() const
{
    Eigen::VectorXi p_vecIds;
    if (table.cols() == 5)
        p_vecIds = table.block(0,4,table.rows(),1);

    return p_vecIds;
}

//=============================================================================================================

inline QStringList Colortable::getNames() const
{
    return struct_names;
}

//=============================================================================================================

inline Eigen::MatrixX4i Colortable::getRGBAs() const
{
    Eigen::MatrixX4i p_matRGBAs;
    if (table.cols() == 5)
        p_matRGBAs = table.block(0,0,table.rows(),4);

    return p_matRGBAs;
}

} // NAMESPACE

#endif // COLORTABLE_H
