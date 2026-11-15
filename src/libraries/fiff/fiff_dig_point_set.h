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
 * @ref mne::FiffDigPointSet holds the head-coordinate point cloud associated
 * with one recording: cardinal fiducials, HPI coil positions, EEG
 * electrodes and the extra head-shape samples. It is what
 * @ref mne::FiffStream returns when asked for the contents of an
 * @c FIFFB_ISOTRAK / @c FIFFB_HPI_MEAS block, and what
 * @ref mne::FiffDigitizerData consumes when constructing a digitization
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


#include "fiff_stream.h"
#include "fiff_dig_point.h"

#include <base/error.h>

#include <cstddef>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <span>
#include <string>
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

class FiffDigPoint;
class FiffDirNode;

//=============================================================================================================
/**
 * @brief Collection of @ref FiffDigPoint records as parsed from a FIFFB_ISOTRAK block.
 *
 * A contiguous range of points with the container interface of std::vector, plus
 * the selection, transformation and file operations of mne.io.Info["dig"].
 *
 * @snippet ex_fiff_api/main.cpp fiff_dig_point_set_usage
 */

class FIFFSHARED_EXPORT FiffDigPointSet
{
public:
    using value_type = FiffDigPoint;                                  /**< Element type. */
    using iterator = std::vector<FiffDigPoint>::iterator;             /**< Iterator. */
    using const_iterator = std::vector<FiffDigPoint>::const_iterator; /**< Constant iterator. */

    //=========================================================================================================
    /**
     * Constructs an empty set.
     */
    FiffDigPointSet() = default;

    //=========================================================================================================
    /**
     * Constructs a set holding @p points.
     *
     * @param[in] points   The digitizer points.
     */
    explicit FiffDigPointSet(std::vector<FiffDigPoint> points);

    //=========================================================================================================
    /**
     * Reads the digitizer points of a FIFF file, like read(); throws mne::Error if it cannot.
     *
     * @param[in] path   The file.
     */
    explicit FiffDigPointSet(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads the digitizer points of a FIFF file, like mne.io.read_info(path)["dig"].
     *
     * @param[in] path   The file.
     *
     * @return The points, or why the file cannot be read.
     */
    [[nodiscard]] static mne::Expected<FiffDigPointSet> read(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads the digitizer points of the isotrak block of a FIFF stream, opening it if needed.
     *
     * @param[in, out] stream   The stream.
     *
     * @return The points, which are none if the stream holds no isotrak block, or why the stream cannot be read.
     */
    [[nodiscard]] static mne::Expected<FiffDigPointSet> read(FiffStream& stream);

    /** @return Whether the set holds no point. */
    bool empty() const noexcept
    {
        return m_points.empty();
    }

    /** @return The number of points. */
    std::size_t size() const noexcept
    {
        return m_points.size();
    }

    /** Removes every point. */
    void clear() noexcept
    {
        m_points.clear();
    }

    /**
     * Appends a point.
     *
     * @param[in] point   The point.
     */
    void push_back(const FiffDigPoint& point)
    {
        m_points.push_back(point);
    }

    /** @return The first point; the set must not be empty. */
    const FiffDigPoint& front() const
    {
        return m_points.front();
    }

    /** @return The last point; the set must not be empty. */
    const FiffDigPoint& back() const
    {
        return m_points.back();
    }

    /**
     * @param[in] idx   Index below size().
     *
     * @return Point @p idx.
     */
    const FiffDigPoint& operator[](std::size_t idx) const
    {
        return m_points[idx];
    }

    /**
     * @param[in] idx   Index below size().
     *
     * @return Point @p idx.
     */
    FiffDigPoint& operator[](std::size_t idx)
    {
        return m_points[idx];
    }

    iterator begin() noexcept
    {
        return m_points.begin();
    } /**< @return First point. */
    iterator end() noexcept
    {
        return m_points.end();
    } /**< @return Past the last point. */
    const_iterator begin() const noexcept
    {
        return m_points.begin();
    } /**< @return First point. */
    const_iterator end() const noexcept
    {
        return m_points.end();
    } /**< @return Past the last point. */

    /** @return The points, contiguous in memory. */
    std::span<const FiffDigPoint> points() const noexcept
    {
        return m_points;
    }

    //=========================================================================================================
    /**
     * Writes the points to a FIFF file, replacing it, like mne.io.write_info writes info["dig"].
     *
     * @param[in] path   The file.
     *
     * @return Nothing, or why the file cannot be written.
     */
    [[nodiscard]] mne::Expected<void> write(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * Writes the points as an isotrak block inside a measurement info block.
     *
     * @param[in, out] stream   The stream.
     */
    void write_to_stream(FiffStream& stream) const;

    //=========================================================================================================
    /**
     * @param[in] kinds   Point kinds to keep: FIFFV_POINT_CARDINAL, FIFFV_POINT_HPI, FIFFV_POINT_EEG,
     *                    FIFFV_POINT_EXTRA or FIFFV_POINT_ECG.
     *
     * @return The points of those kinds, in their order in this set.
     */
    FiffDigPointSet pick_types(std::span<const int> kinds) const;

    /** @copydoc pick_types(std::span<const int>) const */
    FiffDigPointSet pick_types(std::initializer_list<int> kinds) const
    {
        return pick_types(std::span<const int>(kinds.begin(), kinds.size()));
    }

    //=========================================================================================================
    /**
     * Applies a coordinate transformation to every point.
     *
     * @param[in] coordTrans   The transformation.
     * @param[in] inverse      Apply its inverse instead.
     */
    void apply_transform(const FiffCoordTrans& coordTrans, bool inverse = false);

    //=========================================================================================================
    /**
     * Moves every point that shares the frame of the LPA, nasion and RPA points into the Neuromag head
     * frame defined by these fiducials (mne.channels.transform_to_head).
     *
     * @return false if a fiducial is missing or the fiducials are in different frames.
     */
    bool transform_to_head();

private:
    std::vector<FiffDigPoint> m_points;
};

} // namespace mne

#endif // FIFFLIB_FIFF_DIG_POINT_SET_H
