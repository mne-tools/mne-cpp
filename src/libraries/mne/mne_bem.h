//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     mne_bem.h
 * @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Ruben Doerfel <doerfelruben@aol.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June 2015
 * @brief    Boundary element model bundle (inner skull, outer skull, outer skin) loaded from -bem.fif.
 *
 * @ref mne::MNEBem aggregates the @ref mne::MNEBemSurface objects that
 * make up a head conductor model produced by @c mne_setup_forward_model.
 * Depending on the model it carries one (single layer) or three
 * (three-layer EEG) surfaces in MRI coordinates. It is consumed by the
 * forward solver in mne_fwd to build the BEM linear system and by the
 * 3D viewer to render the head surfaces alongside the cortex.
 */

#ifndef MNE_BEM_H
#define MNE_BEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include <memory>
#include "mne_bem_surface.h"

#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_stream.h>

#include <base/error.h>

#include <filesystem>
#include <span>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Boundary element model: aggregates the inner-skull, outer-skull and scalp
 * @ref MNEBemSurface objects of a head conductor model.
 *
 * @brief Aggregated boundary element model loaded from a @c -bem.fif file.
 *
 * @snippet ex_mne_api/main.cpp mne_bem_usage
 */
class MNESHARED_EXPORT MNEBem
{
public:
    using iterator = std::vector<MNEBemSurface>::iterator;             /**< Surface iterator. */
    using const_iterator = std::vector<MNEBemSurface>::const_iterator; /**< Const surface iterator. */

    /** Constructs an empty model. */
    MNEBem() = default;

    //=========================================================================================================
    /**
     * Reads a -bem.fif file like read(); throws mne::Error if it cannot.
     *
     * @param[in] path   The -bem.fif file.
     */
    explicit MNEBem(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Reads the BEM surfaces of a file, as mne.read_bem_surfaces does.
     *
     * @param[in] path          The -bem.fif file.
     * @param[in] patch_stats   Recompute the vertex normals from the triangles even where the file stores them.
     *
     * @return The model, or why the file cannot be read: Io if it cannot be opened, NotFound if it holds no
     *         BEM surfaces, Format if a surface is incomplete or inconsistent.
     */
    [[nodiscard]] static mne::Expected<MNEBem> read(const std::filesystem::path& path, bool patch_stats = true);

    //=========================================================================================================
    /**
     * Reads the BEM surfaces from an open FIFF stream.
     *
     * @param[in] stream        The stream; opened here if it is not open yet.
     * @param[in] patch_stats   Recompute the vertex normals from the triangles even where the file stores them.
     *
     * @return The model, or why the stream cannot be read (see read(path)).
     */
    [[nodiscard]] static mne::Expected<MNEBem> read(mne::FiffStream& stream, bool patch_stats = true);

    //=========================================================================================================
    /**
     * Writes the model, as mne.write_bem_surfaces does.
     *
     * @param[in] path   The -bem.fif file.
     *
     * @return Nothing, or why the file cannot be written (Io).
     */
    [[nodiscard]] mne::Expected<void> write(const std::filesystem::path& path) const;

    //=========================================================================================================
    /**
     * Writes the FIFFB_BEM block into an open FIFF stream (MNE-C mne_write_bem_surfaces_block).
     *
     * @param[in] stream  The stream to write to.
     */
    void write_to_stream(mne::FiffStream& stream) const;

    /** @return The number of surfaces. */
    std::size_t size() const noexcept
    {
        return m_surfaces.size();
    }

    /** @return Whether the model holds no surface. */
    bool empty() const noexcept
    {
        return m_surfaces.empty();
    }

    /** Removes all surfaces. */
    void clear() noexcept
    {
        m_surfaces.clear();
    }

    /**
     * @param[in] surface   The surface to append.
     */
    void push_back(MNEBemSurface surface)
    {
        m_surfaces.push_back(std::move(surface));
    }

    /**
     * @param[in] idx   The surface index.
     *
     * @return The surface.
     */
    const MNEBemSurface& operator[](std::size_t idx) const
    {
        return m_surfaces[idx];
    }

    /**
     * @param[in] idx   The surface index.
     *
     * @return The surface.
     */
    MNEBemSurface& operator[](std::size_t idx)
    {
        return m_surfaces[idx];
    }

    /** @return The surfaces. */
    std::span<const MNEBemSurface> surfaces() const noexcept
    {
        return m_surfaces;
    }

    /** @return Iterator to the first surface. */
    iterator begin() noexcept
    {
        return m_surfaces.begin();
    }

    /** @return Iterator past the last surface. */
    iterator end() noexcept
    {
        return m_surfaces.end();
    }

    /** @return Iterator to the first surface. */
    const_iterator begin() const noexcept
    {
        return m_surfaces.begin();
    }

    /** @return Iterator past the last surface. */
    const_iterator end() const noexcept
    {
        return m_surfaces.end();
    }

    //=========================================================================================================
    /**
     * Warps every surface with the thin-plate spline that maps the source landmarks onto the destination ones.
     *
     * @param[in] sLm       3D Landmarks of the source geometry.
     * @param[in] dLm       3D Landmarks of the destination geometry.
     */
    void warp(const Eigen::MatrixXf& sLm, const Eigen::MatrixXf& dLm);

    //=========================================================================================================
    /**
     * Applies a coordinate transformation to every surface.
     *
     * @param[in] trans     The transformation.
     */
    void transform(const mne::FiffCoordTrans& trans);

    //=========================================================================================================
    /**
     * Applies the inverse of a coordinate transformation to every surface.
     *
     * @param[in] trans     The transformation.
     */
    void inverse_transform(const mne::FiffCoordTrans& trans);

private:
    std::vector<MNEBemSurface> m_surfaces; /**< The BEM surfaces, innermost first as in the file. */
};

} // NAMESPACE

#endif // MNE_BEM_H
