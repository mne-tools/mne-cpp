//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fiff_dig_point_set.h
 * @since 2022
 * @date  May 2026
 * @brief Container for the FIFF_DIG_POINT records of a measurement (a parsed FIFFB_ISOTRAK block).
 *
 * @ref FiffDigPointSet holds the head-coordinate point cloud associated
 * with one recording: cardinal fiducials, HPI coil positions, EEG
 * electrodes and the extra head-shape samples. It is what
 * @ref FiffStream returns when asked for the contents of an
 * @c FIFFB_ISOTRAK / @c FIFFB_HPI_MEAS block, and what
 * @ref FiffDigitizerData consumes when constructing a digitization
 * view for the registration GUIs. Round-trips with the @c info['dig']
 * list in MNE-Python.
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
#include "fiff_dig_point.h"
#include <memory>

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
 * @brief Collection of @ref FiffDigPoint records as parsed from a FIFFB_ISOTRAK block.
 *
 * Indexed access plus convenience filters (cardinals only, HPI only,
 * EEG only, extras only) so registration code can pick out the subset it
 * needs without re-walking the underlying QList of dig points.
 */

class FIFFSHARED_EXPORT FiffDigPointSet
{

public:
    using SPtr = QSharedPointer<FiffDigPointSet>;            /**< Shared pointer type for FiffDigPointSet. */
    using ConstSPtr = QSharedPointer<const FiffDigPointSet>; /**< Const shared pointer type for FiffDigPointSet. */
    using UPtr = std::unique_ptr<FiffDigPointSet>;             /**< Unique pointer type for FiffDigPointSet. */
    using ConstUPtr = std::unique_ptr<const FiffDigPointSet>;  /**< Const unique pointer type for FiffDigPointSet. */

    //=========================================================================================================
    /**
     * Constructs a FiffDigPointSet object.
     */
    FiffDigPointSet();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffDigPointSet   FiffDigPointSet which should be copied.
     */
    FiffDigPointSet(const FiffDigPointSet &p_FiffDigPointSet);

    //=========================================================================================================
    /**
     * Construct FiffDigPointSet based on input pointList
     *
     * @param[in] pointList     list of digitizer points
     */
    FiffDigPointSet(QList<FIFFLIB::FiffDigPoint> pointList);

    //=========================================================================================================
    /**
     * Constructs a FiffDigPointSet by reading from a IO device.
     *
     * @param[in] p_IODevice   IO device to read the digitizer point set from.
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
     * @param[in, out] p_Stream     The opened fif file.
     * @param[in, out] p_Dig        The read digitizer point set.
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
     * @return true if FiffDigPointSet is empty.
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
     * Writes the FiffDigPointSet to a FIFF file.
     *
     * @param[in] p_IODevice   IO device to write the digitizer point set to.
     */
    void write(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Convenience overload: write the digitizer point set to a path on disk.
     *
     * Verifies that the destination directory exists and that the file was
     * actually produced. The file is opened in WriteOnly|Truncate mode.
     *
     * @param[in]  filePath      Destination .fif file path.
     * @param[out] errorMessage  Optional, populated on failure.
     *
     * @return true on success, false on error.
     */
    bool write(const QString& filePath, QString* errorMessage = nullptr);

    //=========================================================================================================
    /**
     * Writes the FiffDigPointSet to a FIFF stream.
     *
     * @param[in] p_pStream   Pointer to the FIFF stream to write to.
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
     * Apply a transformation matrix on the 3D position of the digitized points.
     *
     * @param[in] coordTrans    FiffCoordTrans which is to be applied.
     * @param[in] bApplyInverse Whether to apply the inverse. False by default.
     */
    void applyTransform(const FiffCoordTrans& coordTrans, bool bApplyInverse = false);

    //=========================================================================================================
    /**
     * Returns list of digitizer point the set contains
     *
     * @return list of digitizer points
     */
    QList<FiffDigPoint> getList();

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

#ifndef metatype_fiffdigpointset_sptr
#define metatype_fiffdigpointset_sptr
Q_DECLARE_METATYPE(FIFFLIB::FiffDigPointSet::SPtr); /**< Provides QT META type declaration of the FIFFLIB::FiffDigPointSet::SPtr type. For signal/slot usage.*/
#endif

#endif // FIFFLIB_FIFF_DIG_POINT_SET_H
