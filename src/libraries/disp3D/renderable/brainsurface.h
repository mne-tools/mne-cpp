//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainsurface.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Renderable cortical / BEM mesh with interleaved vertex attributes and Qt-RHI buffer management.
 *
 * BrainSurface is the core 3-D primitive of disp3D. It owns the
 * interleaved @ref VertexData stream (position, normal, packed-ABGR
 * base / annotation colour, surface-id) and the matching index
 * buffer, plus the QRhi vertex / index buffer pair that the renderer
 * binds for every draw.
 *
 * Per-vertex colour is computed once on the CPU from one of four
 * sources selected by @ref VisualizationMode &mdash; flat base colour,
 * FsAnnotation parcellation, scientific curvature shading (Lambertian
 * ambient + sulcal darkening), or a source-time-course value mapped
 * through the active colormap &mdash; then packed into the second
 * colour slot so a single fragment shader covers every mode by
 * blending the two colour streams.
 *
 * The class accepts geometry from a @ref FSLIB::FsSurface, an
 * @ref MNELIB::MNEBemSurface or raw Eigen matrices, and can carry a
 * @ref TissueType tag so the renderer knows whether the mesh is a
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
#include <vector>
#include <fs/fs_surface.h>
#include <fs/fs_annotation.h>
#include <mne/mne_bem.h>

#include <Eigen/Core>

// Forward-declare QRhi types so that this header stays QRhi-free
class QRhi;
class QRhiBuffer;
class QRhiResourceUpdateBatch;

//=============================================================================================================
// STRUCTS
//=============================================================================================================

/**
 * @brief Interleaved vertex attributes (position, normal, color, curvature) for brain surface GPU upload.
 */
struct VertexData {
    QVector3D pos;
    QVector3D norm;
    uint32_t color;           // curvature / base / STC color  (ABGR packed)
    uint32_t colorAnnotation; // annotation region color       (ABGR packed)
};

//=============================================================================================================
/**
 * BrainSurface manages the geometry and visual properties of a single brain mesh.
 *
 * @brief Renderable cortical surface mesh with per-vertex color, curvature data, and GPU buffer management.
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
    using VisualizationMode = ::VisualizationMode;
    static constexpr VisualizationMode ModeSurface        = ::ModeSurface;
    static constexpr VisualizationMode ModeAnnotation     = ::ModeAnnotation;
    static constexpr VisualizationMode ModeScientific     = ::ModeScientific;
    static constexpr VisualizationMode ModeSourceEstimate = ::ModeSourceEstimate;

    enum TissueType {
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
    bool isVisible() const { return m_visible; }
    
    //=========================================================================================================
    /**
     * Set the hemisphere index.
     *
     * @param[in] hemi       0 for Left, 1 for Right.
     */
    void setHemi(int hemi) { m_hemi = hemi; }

    //=========================================================================================================
    /**
     * Set the tissue type (Brain, Skin, Skull, etc.).
     *
     * @param[in] type       TissueType enum value.
     */
    void setTissueType(TissueType type) { m_tissueType = type; }

    //=========================================================================================================
    /**
     * Get the tissue type.
     *
     * @return TissueType enum value.
     */
    TissueType tissueType() const { return m_tissueType; }

    //=========================================================================================================
    /**
     * Get the hemisphere index.
     *
     * @return Hemisphere index (0=LH, 1=RH).
     */
    int hemi() const { return m_hemi; }

    //=========================================================================================================
    /**
     * Load geometry from a FreeSurfer surface.
     *
     * @param[in] surf       Input FreeSurfer surface.
     */
    void fromSurface(const FSLIB::FsSurface &surf);

    //=========================================================================================================
    /**
     * Load geometry from a MNE BEM surface.
     *
     * @param[in] surf       Input BEM surface.
     * @param[in] color      Base color for the surface.
     */
    void fromBemSurface(const MNELIB::MNEBemSurface &surf, const QColor &color = Qt::white);

    //=========================================================================================================
    /**
     * Create surface from raw vertex and triangle data.
     *
     * @param[in] vertices   Nx3 matrix of vertex positions.
     * @param[in] triangles  Mx3 matrix of triangle indices.
     * @param[in] color      FsSurface color.
     */
    void createFromData(const Eigen::MatrixX3f &vertices, const Eigen::MatrixX3i &triangles, const QColor &color);

    //=========================================================================================================
    /**
     * Create surface from raw vertex, normal and triangle data.
     *
     * @param[in] vertices   Nx3 matrix of vertex positions.
     * @param[in] normals    Nx3 matrix of vertex normals.
     * @param[in] triangles  Mx3 matrix of triangle indices.
     * @param[in] color      FsSurface color.
     */
    void createFromData(const Eigen::MatrixX3f &vertices, const Eigen::MatrixX3f &normals, const Eigen::MatrixX3i &triangles, const QColor &color);

    //=========================================================================================================
    /**
     * Load annotation data from file.
     *
     * @param[in] path       Path to the .annot file.
     * @return True if successful.
     */
    bool loadAnnotation(const QString &path);

    //=========================================================================================================
    /**
     * Add annotation data directly.
     *
     * @param[in] annotation Input annotation data.
     */
    void addAnnotation(const FSLIB::FsAnnotation &annotation);

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
    void applySourceEstimateColors(const QVector<uint32_t> &colors);

    /**
     * Clear source estimate overlay colors and restore curvature-based coloring.
     */
    void clearSourceEstimateColors();
    
    //=========================================================================================================
    /**
     * Update graphics buffers (vertex/index) on the GPU.
     *
     * @param[in] rhi        Pointer to QRhi instance.
     * @param[in] u          Resource update batch.
     */
    void updateBuffers(QRhi *rhi, QRhiResourceUpdateBatch *u);

    QRhiBuffer* vertexBuffer() const;
    QRhiBuffer* indexBuffer() const;
    uint32_t indexCount() const { return m_indexCount; }
    uint32_t vertexCount() const { return m_vertexData.size(); }

    //=========================================================================================================
    /**
     * Get a copy of the current vertex positions.
     */
    Eigen::MatrixX3f vertexPositions() const;

    //=========================================================================================================
    /**
     * Get a copy of the current vertex normals.
     */
    Eigen::MatrixX3f vertexNormals() const;

    //=========================================================================================================
    /**
     * Get the triangle index buffer.
     */
    QVector<uint32_t> triangleIndices() const { return m_indexData; }

    /** @brief Const-ref access to CPU-side vertex data (used by merged rendering). */
    const QVector<VertexData>& vertexDataRef() const { return m_vertexData; }
    /** @brief Const-ref access to CPU-side index data (used by merged rendering). */
    const QVector<uint32_t>& indexDataRef() const { return m_indexData; }

    /** @brief Monotonically increasing counter bumped whenever vertex data changes. */
    quint64 vertexGeneration() const { return m_vertexGeneration; }
    
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
    void boundingBox(QVector3D &min, QVector3D &max) const;

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
     * @return True if intersected.
     */
    bool intersects(const QVector3D &rayOrigin, const QVector3D &rayDir, float &dist, int &vertexIdx) const;

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
    void transform(const QMatrix4x4 &m);

    //=========================================================================================================
    /**
     * Apply a transformation to the surface starting from the original data.
     * Note: This prevents transformation accumulation.
     *
     * @param[in] m          Transformation matrix.
     */
    void applyTransform(const QMatrix4x4 &m);

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
     */
    void setUseDefaultColor(bool useDefault);
    
    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }
    int selectedRegionId() const { return m_selectedRegionId; }
    int selectedVertexStart() const { return m_selectedVertexStart; }

    /**
     * Highlight a contiguous range of vertices (e.g. a single sphere in a batched mesh).
     * Pass start=-1 to clear the vertex range highlight.
     *
     * @param[in] start  First vertex index to highlight (-1 to clear).
     * @param[in] count  Number of vertices to highlight.
     */
    void setSelectedVertexRange(int start, int count);
    
private:
    void updateVertexColors();
    void markVertexDirty();

    QVector<VertexData> m_vertexData;
    QVector<VertexData> m_originalVertexData;
    QVector<uint32_t> m_indexData;
    uint32_t m_indexCount = 0;
    
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

    /** @brief QRhi vertex, index, and uniform buffers for brain surface GPU rendering. */
    struct GpuBuffers;
    std::unique_ptr<GpuBuffers> m_gpu;

    quint64 m_vertexGeneration = 0;

    mutable QVector3D m_aabbMin;
    mutable QVector3D m_aabbMax;
    mutable bool m_bAABBDirty = true;
};

#endif // BRAINSURFACE_H
