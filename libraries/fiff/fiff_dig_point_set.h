//=============================================================================================================
/**
 * @file     fiff_dig_point_set.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     Jul, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief     FiffDigPointSet class declaration.
 *
 */

#ifndef FIFFLIB_FIFF_DIG_POINT_SET_H
#define FIFFLIB_FIFF_DIG_POINT_SET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QIODevice>
#include <QList>

#include "fiff_stream.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB {

//=============================================================================================================
// FIFFLIB FORWARD DECLARATIONS
//=============================================================================================================

class FiffDigPoint;
class FiffDirNode;

//=============================================================================================================
/**
 * The FiffDigPointSet hold a set of Digitizer Points and read, write and transform function.
 *
 * @brief Holds a set of digitizer points.
 */

class FIFFSHARED_EXPORT FiffDigPointSet
{

public:
    typedef QSharedPointer<FiffDigPointSet> SPtr;            /**< Shared pointer type for FiffDigPointSet. */
    typedef QSharedPointer<const FiffDigPointSet> ConstSPtr; /**< Const shared pointer type for FiffDigPointSet. */

    //=========================================================================================================
    /**
     * Constructs a FiffDigPointSet object.
     */
    FiffDigPointSet();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffDigPointSet.
     */
    FiffDigPointSet(const FiffDigPointSet &p_FiffDigPointSet);

    FiffDigPointSet(QList<FIFFLIB::FiffDigPoint> pointList);

    //=========================================================================================================
    /**
     * Default constructor
     */
    FiffDigPointSet(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Destroys the FiffDigPointSet
     */
    ~FiffDigPointSet();

    //=========================================================================================================
    /**
     * Reads FiffDigPointSet from a fif file
     *
     * @param+[\[]in, out] p_Stream     The opened fif file.
     * @param[in, out] p_Dig            The read digitizer point set.
     *
     * @return true if succeeded, false otherwise.
     */
    static bool readFromStream(FiffStream::SPtr& p_Stream, FiffDigPointSet& p_Dig);

    //=========================================================================================================
    /**
     * Initializes FiffDigPointSet
     */
    inline void clear();

    //=========================================================================================================
    /**
     * True if FiffDigPointSet is empty.
     *
     * @return true if MNE Bem is empty.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * Returns the number of stored FiffDigPoints
     *
     * @return number of stored FiffDigPoints.
     */
    inline qint32 size() const;

    //=========================================================================================================
    /**
     * @brief write
     * @param[in] p_IODevice.
     */
    void write(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * @brief writeToStream
     * @param[in] p_pStream.
     */
    void writeToStream(FiffStream* p_pStream);

    //=========================================================================================================
    /**
     * Subscript operator [] to access FiffDigPoint by index
     *
     * @param[in] idx    the FiffDigPoint index.
     *
     * @return FiffDigPoint related to the parameter index.
     */
    const FiffDigPoint& operator[] (qint32 idx) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access FiffDigPoint by index
     *
     * @param[in] idx    the FiffDigPoint index.
     *
     * @return FiffDigPoint related to the parameter index.
     */
    FiffDigPoint& operator[] (qint32 idx);

    //=========================================================================================================
    /**
     * Pick the wanted types from this set and returns them
     *
     * @param[in] includeTypes    The include types (FIFFV_POINT_HPI, FIFFV_POINT_CARDINAL, FIFFV_POINT_EEG, FIFFV_POINT_ECG, FIFFV_POINT_EXTRA, FIFFV_POINT_LPA, FIFFV_POINT_NASION, FIFFV_POINT_RPA).
     *
     * @return FiffDigPointSet.
     */
    FiffDigPointSet pickTypes(QList<int> includeTypes) const;

    //=========================================================================================================
    /**
     * Subscript operator << to add a new FiffDigPoint
     *
     * @param[in] dig    FiffDigPoint to be added.
     *
     * @return FiffDigPointSet.
     */
    FiffDigPointSet& operator<< (const FiffDigPoint& dig);

    //=========================================================================================================
    /**
     * Subscript operator << to add a new FiffDigPoint
     *
     * @param[in] dig    FiffDigPoint to be added.
     *
     * @return FiffDigPointSet.
     */
    FiffDigPointSet& operator<< (const FiffDigPoint* dig);

    //=========================================================================================================
    /**
     * Apply a transforamtion matrix on the 3D position of the digitzed points.
     *
     * @param[in] coordTrans    FiffCoordTrans which is to be applied.
     * @param[in] bApplyInverse Whether to apply the inverse. False by default.
     */
    void applyTransform(const FiffCoordTrans& coordTrans, bool bApplyInverse = false);

    QList<FiffDigPoint> getList();

//    ToDo:
//    //=========================================================================================================
//    /**
//    * Write the FiffDigPointSet to a FIF file
//    *
//    * @param[in] p_IODevice   IO device to write the FiffDigPointSet to.
//    */
//    void write(QIODevice &p_IODevice);

protected:

private:
    QList<FiffDigPoint> m_qListDigPoint;    /**< List of digitizer Points. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline void FiffDigPointSet::clear()
{
    m_qListDigPoint.clear();
}

//=============================================================================================================

inline bool FiffDigPointSet::isEmpty() const
{
    return m_qListDigPoint.size() == 0;
}

//=============================================================================================================

inline qint32 FiffDigPointSet::size() const
{
    return m_qListDigPoint.size();
}
} // namespace FIFFLIB

#ifndef metatype_fiffdigpointset
#define metatype_fiffdigpointset
Q_DECLARE_METATYPE(FIFFLIB::FiffDigPointSet); /**< Provides QT META type declaration of the FIFFLIB::FiffDigPointSet type. For signal/slot usage.*/
#endif

#ifndef metatype_fiffdigpointset
#define metatype_fiffdigpointset
Q_DECLARE_METATYPE(FIFFLIB::FiffDigPointSet::SPtr); /**< Provides QT META type declaration of the FIFFLIB::FiffDigPointSet type. For signal/slot usage.*/
#endif

#endif // FIFFLIB_FIFF_DIG_POINT_SET_H
