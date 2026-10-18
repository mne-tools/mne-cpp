//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainsurface.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Renderable cortical / BEM mesh: CPU geometry, colour and annotation streams for the renderer.
 *
 * BrainSurface is the core 3-D primitive of disp3D. It owns the CPU
 * streams the renderer uploads: geometry (@ref DISP3DLIB::VertexData,
 * position and normal), packed-ABGR colour, packed-ABGR annotation
 * colour and the triangle indices. Each stream has its own generation,
 * so a source-estimate frame re-uploads only the 4-byte colour stream.
 *
 * Per-vertex colour is computed once on the CPU from one of four
 * sources selected by @ref DISP3DLIB::VisualizationMode &mdash; flat base colour,
 * FsAnnotation parcellation, scientific curvature shading (Lambertian
 * ambient + sulcal darkening), or a source-time-course value mapped
 * through the active colormap &mdash; then packed into the second
 * colour slot so a single fragment shader covers every mode by
 * blending the two colour streams.
 *
 * The class accepts geometry from a @ref FSLIB::FsSurface, an
 * @ref MNELIB::MNEBemSurface or raw Eigen matrices, and can carry a
 * @c BrainSurface::TissueType "TissueType" tag so the renderer knows whether the mesh is a
 * cortex, skin, skull or generic shell for alpha / lighting tuning.
 */

#ifndef BRAINSURFACE_H
#define BRAINSURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include "../core/rendertypes.h"

#include <QVector>
#include <QVector3D>
#include <QColor>
#include <memory>
#include <optional>
#include <vector>
#include <fs/fs_surface.h>
#include <fs/fs_annotation.h>
#include <mne/mne_bem.h>

#include <Eigen/Core>

//=============================================================================================================
// STRUCTS
//=============================================================================================================

namespace DISP3DLIB
{

/**
 * @brief Geometry of one brain surface vertex: position and normal.
 *
 * @snippet ex_disp3d_scene/main.cpp brain_surface_usage
 */
struct VertexData
{
    QVector3D pos;
    QVector3D norm;
};

//=============================================================================================================
/**
 * BrainSurface manages the geometry and visual properties of a single brain mesh.
 *
 * @brief Renderable cortical surface mesh with per-vertex color, curvature data, and GPU buffer management.
 *
 * @snippet ex_disp3d_scene/main.cpp brain_surface_usage
 */
class DISP3DSHARED_EXPORT BrainSurface
{
public:
    //=========================================================================================================
    /**
     * Default Constructor
     */
    BrainSurface();

    //=========================================================================================================
    /**
     * Destructor
     */
    ~BrainSurface();

    // VisualizationMode is defined in core/rendertypes.h for lightweight inclusion.
    // These aliases preserve backward compatibility.
    using VisualizationMode = DISP3DLIB::VisualizationMode;
    static constexpr VisualizationMode ModeSurface = DISP3DLIB::ModeSurface;
    static constexpr VisualizationMode ModeAnnotation = DISP3DLIB::ModeAnnotation;
    static constexpr VisualizationMode ModeScientific = DISP3DLIB::ModeScientific;
    static constexpr VisualizationMode ModeSourceEstimate = DISP3DLIB::ModeSourceEstimate;

    enum TissueType
    {
        TissueUnknown = 0,
        TissueBrain = 1,
        TissueSkin = 2,       // Head/Scalp surface
        TissueOuterSkull = 3, // Outer skull bone
        TissueInnerSkull = 4  // Inner skull bone
    };

    //=========================================================================================================
    /**
     * Set the visibility of the surface.
     *
     * @param[in] visible    True if visible.
     */
    void setVisible(bool visible);

    //=========================================================================================================
    /**
     * Check if the surface is visible.
     *
     * @return True if visible.
     */
    bool isVisible() const
    {
        return m_visible;
    }

    //=========================================================================================================
    /**
     * Set the hemisphere index.
     *
     * @param[in] hemi       0 for Left, 1 for Right.
     */
    void setHemi(int hemi)
    {
        m_hemi = hemi;
    }

    //=========================================================================================================
    /**
     * Set the tissue type (Brain, Skin, Skull, etc.).
     *
     * @param[in] type       TissueType enum value.
     */
    void setTissueType(TissueType type)
    {
        m_tissueType = type;
    }

    //=========================================================================================================
    /**
     * Get the tissue type.
     *
     * @return TissueType enum value.
     */
    TissueType tissueType() const
    {
        return m_tissueType;
    }

    //=========================================================================================================
    /**
     * Get the hemisphere index.
     *
     * @return Hemisphere index (0=LH, 1=RH).
     */
    int hemi() const
    {
        return m_hemi;
    }

    //=========================================================================================================
    /**
     * Load geometry from a FreeSurfer surface.
     *
     * @param[in] surf       Input FreeSurfer surface.
     */
    void fromSurface(const FSLIB::FsSurface& surf);

    //=========================================================================================================
    /**
     * Load geometry from a MNE BEM surface.
     *
     * @param[in] surf       Input BEM surface.
     * @param[in] color      Base color for the surface.
     */
    void fromBemSurface(const MNELIB::MNEBemSurface& surf, const QColor& color = Qt::white);

    //=========================================================================================================
    /**
     * Create surface from raw vertex and triangle data. Vertex normals are the sums of the adjacent face normals.
     *
     * @param[in] vertices   Nx3 matrix of vertex positions.
     * @param[in] triangles  Mx3 matrix of triangle indices.
     * @param[in] color      FsSurface color.
     */
    void createFromData(const Eigen::MatrixX3f& vertices, const Eigen::MatrixX3i& triangles, const QColor& color);

    //=========================================================================================================
    /**
     * Create surface from raw vertex, normal and triangle data.
     *
     * @param[in] vertices   Nx3 matrix of vertex positions.
     * @param[in] normals    Nx3 matrix of vertex normals.
     * @param[in] triangles  Mx3 matrix of triangle indices.
     * @param[in] color      FsSurface color.
     */
    void createFromData(const Eigen::MatrixX3f& vertices, const Eigen::MatrixX3f& normals, const Eigen::MatrixX3i& triangles, const QColor& color);

    //=========================================================================================================
    /**
     * Load annotation data from file.
     *
     * @param[in] path       Path to the .annot file.
     * @return True if successful.
     */
    bool loadAnnotation(const QString& path);

    //=========================================================================================================
    /**
     * Add annotation data directly.
     *
     * @param[in] annotation Input annotation data.
     */
    void addAnnotation(const FSLIB::FsAnnotation& annotation);

    //=========================================================================================================
    /**
     * Give each run of vertices its own label key, so one element of a batched mesh (e.g. one
     * digitizer sphere) can be highlighted on its own.
     *
     * @param[in] verticesPerElement Number of consecutive vertices per element.
     */
    void labelElements(int verticesPerElement);

    //=========================================================================================================
    /**
     * Set the visualization mode (FsSurface, FsAnnotation, Scientific).
     *
     * @param[in] mode       VisualizationMode enum.
     */
    void setVisualizationMode(VisualizationMode mode);

    //=========================================================================================================
    /**
     * Apply source estimate colors to vertices.
     *
     * @param[in] colors     Vector of packed ABGR colors, one per vertex.
     */
    void applySourceEstimateColors(const QVector<uint32_t>& colors);

    /**
     * Clear source estimate overlay colors and restore curvature-based coloring.
     */
    void clearSourceEstimateColors();

    //=========================================================================================================
    /**
     * @return Geometry stream (@ref DISP3DLIB::VertexData per vertex).
     */
    GpuStream geometryStream() const;

    /**
     * @return Colour stream (packed ABGR per vertex; alpha carries the curvature grey on cortex).
     */
    GpuStream colorStream() const;

    /**
     * @return Annotation colour stream (packed ABGR per vertex, 0 where unlabelled).
     */
    GpuStream annotationStream() const;

    /**
     * @return Triangle index stream (three uint32 per triangle).
     */
    GpuStream indexStream() const;

    /**
     * @return Identity of this surface in the renderer's GPU cache.
     */
    const RenderKey& renderKey() const
    {
        return m_renderKey;
    }

    uint32_t indexCount() const
    {
        return m_indexCount;
    }
    uint32_t vertexCount() const
    {
        return m_vertexData.size();
    }

    //=========================================================================================================
    /**
     * Get a copy of the current vertex positions.
     *
     * @return Nx3 matrix of the current (possibly transformed) vertex positions.
     */
    Eigen::MatrixX3f vertexPositions() const;

    //=========================================================================================================
    /**
     * Get a copy of the current vertex normals.
     *
     * @return Nx3 matrix of the current vertex normals.
     */
    Eigen::MatrixX3f vertexNormals() const;

    //=========================================================================================================
    /**
     * Get the triangle index buffer.
     *
     * @return Copy of the index data, three vertex indices per triangle.
     */
    QVector<uint32_t> triangleIndices() const
    {
        return m_indexData;
    }

    /**
     * @return Current vertex geometry, valid while this surface lives.
     */
    const QVector<VertexData>& vertexDataRef() const
    {
        return m_vertexData;
    }

    /**
     * @return Per-vertex packed ABGR colours, valid while this surface lives.
     */
    const QVector<uint32_t>& colorsRef() const
    {
        return m_colors;
    }

    /**
     * @return Per-vertex packed ABGR annotation colours, valid while this surface lives.
     */
    const QVector<uint32_t>& annotationColorsRef() const
    {
        return m_annotationColors;
    }

    //=========================================================================================================
    /**
     * Get minimum X coordinate.
     *
     * @return Minimum X value.
     */
    float minX() const;

    //=========================================================================================================
    /**
     * Get maximum X coordinate.
     *
     * @return Maximum X value.
     */
    float maxX() const;

    //=========================================================================================================
    /**
     * Get bounding box of the surface.
     * 
     * @param[out] min       Minimum coordinates.
     * @param[out] max       Maximum coordinates.
     */
    void boundingBox(QVector3D& min, QVector3D& max) const;

    //=========================================================================================================
    /**
     * Test ray intersection with this surface.
     * 
     * @param[in] rayOrigin  Ray origin in world/local space (object is assumed at 0,0,0 usually unless transformed externally).
     *                       Note: The BrainSurface vertices are typically already transformed (e.g. by View3D model matrix logic? 
     *                       No, BrainView applies matrix in shader. The vertices here are raw. 
     *                       So ray must be transformed to local space OR vertices transformed to world.
     * @param[in] rayDir     Ray direction (normalized).
     * @param[out] dist      Distance to intersection.
     * @param[in, out] vertexIdx Index of the vertex closest to the intersection.
     * @return True if intersected.
     */
    bool intersects(const QVector3D& rayOrigin, const QVector3D& rayDir, float& dist, int& vertexIdx) const;

    //=========================================================================================================
    /**
     * Get the annotation label name for a given vertex.
     * 
     * @param[in] vertexIdx  Index of the vertex.
     * @return FsLabel name.
     */
    QString getAnnotationLabel(int vertexIdx) const;
    int getAnnotationLabelId(int vertexIdx) const;

    //=========================================================================================================
    /**
     * Set the selected region for highlighting.
     * 
     * @param[in] regionId   The ID of the region to highlight.
     */
    void setSelectedRegion(int regionId);
    /**
     * Translate all vertices along the X axis.
     *
     * @param[in] offset     Amount to translate.
     */
    void translateX(float offset);

    //=========================================================================================================
    /**
     * Apply a generic 4x4 transformation matrix to all vertices and normals.
     *
     * @param[in] m          Transformation matrix.
     */
    void transform(const QMatrix4x4& m);

    //=========================================================================================================
    /**
     * Apply a transformation to the surface starting from the original data.
     * Note: This prevents transformation accumulation.
     *
     * @param[in] m          Transformation matrix.
     */
    void applyTransform(const QMatrix4x4& m);

    //=========================================================================================================
    /**
     * Compute neighbor vertices from triangle connectivity.
     * Required for surface-constrained distance calculations.
     *
     * @return Vector of neighbor indices for each vertex.
     */
    std::vector<Eigen::VectorXi> computeNeighbors() const;

    //=========================================================================================================
    /**
     * Get vertex positions as Eigen matrix.
     *
     * @return Nx3 matrix of vertex positions.
     */
    Eigen::MatrixX3f verticesAsMatrix() const;

    //=========================================================================================================
    /**
     * Set/Get whether to use the default surface color.
     *
     * @param[in] useDefault   True to use the surface's default color as base, false for white.
     */
    void setUseDefaultColor(bool useDefault);

    //=========================================================================================================
    /**
     * Set the colour of a surface without curvature (BEM, sensors, digitizers, source spaces).
     *
     * @param[in] color   New colour; its alpha is the opacity of the holographic shader.
     */
    void setColor(const QColor& color);

    void setSelected(bool selected);
    bool isSelected() const
    {
        return m_selected;
    }
    int selectedRegionId() const
    {
        return m_selectedRegionId;
    }
    int selectedVertexStart() const
    {
        return m_selectedVertexStart;
    }

    /**
     * Highlight a contiguous range of vertices (e.g. a single sphere in a batched mesh).
     * Pass start=-1 to clear the vertex range highlight.
     *
     * @param[in] start  First vertex index to highlight (-1 to clear).
     * @param[in] count  Number of vertices to highlight.
     */
    void setSelectedVertexRange(int start, int count);

    /**
     * @return Annotation-stream value of the highlighted vertices (selected region or vertex
     *         range), or nothing when no part of the surface is highlighted.
     */
    std::optional<uint32_t> selectedLabel() const;

private:
    void updateVertexColors();
    void setGeometry(QVector<VertexData> vertices, QVector<uint32_t> indices, const QColor& color);
    void markGeometryDirty();

    QVector<VertexData> m_vertexData;
    QVector<VertexData> m_originalVertexData;
    QVector<uint32_t> m_colors;
    QVector<uint32_t> m_annotationColors;
    QVector<int> m_vertexLabelIds;
    QVector<uint32_t> m_indexData;
    uint32_t m_indexCount = 0;

    RenderKey m_renderKey;
    quint64 m_geometryGeneration = 0;
    quint64 m_colorGeneration = 0;
    quint64 m_annotationGeneration = 0;
    quint64 m_indexGeneration = 0;

    QColor m_defaultColor = Qt::white;
    QColor m_baseColor = Qt::white;

    FSLIB::FsAnnotation m_annotation;
    bool m_hasAnnotation = false;
    VisualizationMode m_visMode = ModeSurface;
    QVector<float> m_curvature;
    QVector<uint32_t> m_stcColors;

    bool m_visible = true;
    bool m_selected = false;
    int m_selectedRegionId = -1;
    int m_selectedVertexStart = -1;
    int m_selectedVertexCount = 0;
    int m_hemi = -1; // 0=lh, 1=rh
    TissueType m_tissueType = TissueUnknown;

    mutable QVector3D m_aabbMin;
    mutable QVector3D m_aabbMax;
    mutable bool m_bAABBDirty = true;
};

} // namespace DISP3DLIB

#endif // BRAINSURFACE_H
