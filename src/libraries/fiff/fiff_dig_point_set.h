//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     fiff_dig_point_set.h
 * @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     July 2016
 * @brief    Container for the FIFF_DIG_POINT records of a measurement (a parsed FIFFB_ISOTRAK block).
 *
 * @ref FIFFLIB::FiffDigPointSet holds the head-coordinate point cloud associated
 * with one recording: cardinal fiducials, HPI coil positions, EEG
 * electrodes and the extra head-shape samples. It is what
 * @ref FIFFLIB::FiffStream returns when asked for the contents of an
 * @c FIFFB_ISOTRAK / @c FIFFB_HPI_MEAS block, and what
 * @ref FIFFLIB::FiffDigitizerData consumes when constructing a digitization
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
#include <QStringList>

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

namespace FIFFLIB
{

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
 *
 * @snippet ex_fiff_api/main.cpp fiff_dig_point_set_usage
 */

class FIFFSHARED_EXPORT FiffDigPointSet
{
public:
    using SPtr = QSharedPointer<FiffDigPointSet>;             /**< Shared pointer type for FiffDigPointSet. */
    using ConstSPtr = QSharedPointer<const FiffDigPointSet>;  /**< Const shared pointer type for FiffDigPointSet. */
    using UPtr = std::unique_ptr<FiffDigPointSet>;            /**< Unique pointer type for FiffDigPointSet. */
    using ConstUPtr = std::unique_ptr<const FiffDigPointSet>; /**< Const unique pointer type for FiffDigPointSet. */

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
    FiffDigPointSet(const FiffDigPointSet& p_FiffDigPointSet);

    //=========================================================================================================
    /**
     * Copy assignment. Every member has value semantics, so the
     * compiler-generated member-wise assignment is correct. It is declared
     * explicitly because declaring a copy constructor and/or a destructor
     * deprecates the implicitly generated one.
     *
     * @param[in] other   Object to assign from.
     *
     * @return reference to this object.
     */
    FiffDigPointSet& operator=(const FiffDigPointSet& other) = default;

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
    FiffDigPointSet(QIODevice& p_IODevice);

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
     * Reads a Polhemus Isotrak digitizer file, as mne.channels.read_dig_polhemus_isotrak does.
     *
     * The three fiducials become cardinal points (LPA, nasion, RPA). The remaining
     * points become HPI coils for an .elp file and head shape points for .hsp/.eeg
     * files, or EEG electrodes when @p chNames names them. Electrodes are numbered by
     * the last three characters of their names when all of those are numbers, and
     * 1..n otherwise. All points are in digitizer coordinates (FIFFV_COORD_UNKNOWN).
     *
     * @param[in]  path     The .elp, .hsp or .eeg file.
     * @param[out] dig      The digitizer points.
     * @param[in]  chNames  Electrode names, one per non-fiducial point, or empty.
     * @param[in]  unit     Unit of the file: "m", "cm" or "mm".
     *
     * @return false if the file cannot be read, the extension or unit is unknown, or
     *         the number of names does not match the number of points.
     */
    static bool readPolhemusIsotrak(const QString& path,
                                    FiffDigPointSet& dig,
                                    const QStringList& chNames = QStringList(),
                                    const QString& unit = QStringLiteral("m"));

    //=========================================================================================================
    /**
     * Reads the points of a Polhemus FastSCAN .txt file in metres, as
     * mne.channels.read_polhemus_fastscan does.
     *
     * @param[in]  path           The .txt file.
     * @param[out] points         The points, one per row, in digitizer coordinates.
     * @param[in]  unit           Unit of the file: "m", "cm" or "mm".
     * @param[in]  requireHeader  Reject files whose "%" header does not name FastSCAN.
     *
     * @return false if the file cannot be read, the extension, unit or header is
     *         wrong, or a row does not hold three numbers.
     */
    static bool readPolhemusFastscan(const QString& path,
                                     Eigen::MatrixX3d& points,
                                     const QString& unit = QStringLiteral("mm"),
                                     bool requireHeader = true);

    //=========================================================================================================
    /**
     * Reads a BrainVision CapTrak .bvct file, as mne.channels.read_dig_captrak does.
     *
     * The electrodes named Nasion, LPA and RPA become the cardinal points, every other
     * electrode an EEG point; millimetres are converted to metres. The points are in
     * digitizer coordinates (FIFFV_COORD_UNKNOWN), numbered as by
     * readPolhemusIsotrak; a repeated name keeps its first place and its last position.
     *
     * @param[in]  path     The .bvct file.
     * @param[out] dig      The digitizer points.
     * @param[out] chNames  If not null, the electrode names in the order of the EEG points.
     *
     * @return false if the file cannot be parsed or lacks one of the fiducials.
     */
    static bool readCaptrak(const QString& path, FiffDigPointSet& dig, QStringList* chNames = nullptr);

    //=========================================================================================================
    /**
     * Reads the coordinates.xml of an EGI MFF recording, as mne.channels.read_dig_egi does.
     *
     * Sensors of type 0 become EEG points named "EEG <number>", the reference (type 1)
     * is numbered after the electrodes before it, and the nasion and periauricular
     * points (type 2) become the cardinal points. Unlike mne.channels.read_dig_egi,
     * which keeps the file's centimetres as if they were metres, the positions are
     * converted to metres (as mne.io.read_raw_egi does).
     *
     * @param[in]  path     The coordinates.xml file.
     * @param[out] dig      The digitizer points, in digitizer coordinates.
     * @param[out] chNames  If not null, the electrode names in the order of the EEG points.
     *
     * @return false if the file cannot be parsed or lacks one of the fiducials.
     */
    static bool readEgi(const QString& path, FiffDigPointSet& dig, QStringList* chNames = nullptr);

    //=========================================================================================================
    /**
     * Reads a Localite .csv file ("#,name,x,y,z" in millimetres after a header row), as
     * mne.channels.read_dig_localite does.
     *
     * @param[in]  path     The .csv file.
     * @param[out] dig      The digitizer points, in digitizer coordinates.
     * @param[out] chNames  If not null, the electrode names in the order of the EEG points.
     * @param[in]  nasion   Name of the point that is the nasion, or empty.
     * @param[in]  lpa      Name of the point that is the left preauricular point, or empty.
     * @param[in]  rpa      Name of the point that is the right preauricular point, or empty.
     *
     * @return false if the file cannot be read, a row is malformed or a named fiducial is missing.
     */
    static bool readLocalite(const QString& path,
                             FiffDigPointSet& dig,
                             QStringList* chNames = nullptr,
                             const QString& nasion = QString(),
                             const QString& lpa = QString(),
                             const QString& rpa = QString());

    //=========================================================================================================
    /**
     * Reads a Neuroscan .dat electrode file ("name number x y z" per line), as
     * mne.channels.read_dig_dat does: point numbers 78, 76 and 82 are the nasion,
     * LPA and RPA, 67 (the centroid) is skipped, and the coordinates are kept as they are.
     *
     * @param[in]  path     The .dat file.
     * @param[out] dig      The digitizer points, in digitizer coordinates.
     * @param[out] chNames  If not null, the electrode names in the order of the EEG points.
     *
     * @return false if the file cannot be read or a line does not hold five entries.
     */
    static bool readNeuroscanDat(const QString& path, FiffDigPointSet& dig, QStringList* chNames = nullptr);

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
     *
     * @return false if the device cannot be opened for writing.
     */
    bool write(QIODevice& p_IODevice);

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
    const FiffDigPoint& operator[](qint32 idx) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access FiffDigPoint by index
     *
     * @param[in] idx    the FiffDigPoint index.
     *
     * @return FiffDigPoint related to the parameter index.
     */
    FiffDigPoint& operator[](qint32 idx);

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
    FiffDigPointSet& operator<<(const FiffDigPoint& dig);

    //=========================================================================================================
    /**
     * Subscript operator << to add a new FiffDigPoint
     *
     * @param[in] dig    FiffDigPoint to be added.
     *
     * @return FiffDigPointSet.
     */
    FiffDigPointSet& operator<<(const FiffDigPoint* dig);

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
    QList<FiffDigPoint> m_qListDigPoint; /**< List of digitizer Points. */
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
