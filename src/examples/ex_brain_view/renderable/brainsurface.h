//=============================================================================================================
/**
 * @file     brainsurface.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     January, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    BrainSurface class declaration.
 *
 */

#ifndef BRAINSURFACE_H
#define BRAINSURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QVector>
#include <QVector3D>
#include <QColor>
#include <memory>
#include <rhi/qrhi.h>
#include <fs/surface.h>
#include <fs/annotation.h>
#include <mne/mne_bem.h>

#include <Eigen/Core>
#include <atomic>

//=============================================================================================================
// STRUCTS
//=============================================================================================================

struct VertexData {
    QVector3D pos;
    QVector3D norm;
    uint32_t color; // ABGR or RGBA depending on RHI
};

//=============================================================================================================
/**
 * BrainSurface manages the geometry and visual properties of a single brain mesh.
 *
 * @brief    BrainSurface class.
 */
class BrainSurface
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

    enum VisualizationMode {
        ModeSurface,
        ModeAnnotation,
        ModeScientific, // Curvature
        ModeSourceEstimate // Source estimate overlay
    };

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
    void fromSurface(const FSLIB::Surface &surf);

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
     * @param[in] color      Surface color.
     */
    void createFromData(const Eigen::MatrixX3f &vertices, const Eigen::MatrixX3i &triangles, const QColor &color);

    //=========================================================================================================
    /**
     * Create surface from raw vertex, normal and triangle data.
     *
     * @param[in] vertices   Nx3 matrix of vertex positions.
     * @param[in] normals    Nx3 matrix of vertex normals.
     * @param[in] triangles  Mx3 matrix of triangle indices.
     * @param[in] color      Surface color.
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
    void addAnnotation(const FSLIB::Annotation &annotation);

    //=========================================================================================================
    /**
     * Set the visualization mode (Surface, Annotation, Scientific).
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
    
    //=========================================================================================================
    /**
     * Update graphics buffers (vertex/index) on the GPU.
     *
     * @param[in] rhi        Pointer to QRhi instance.
     * @param[in] u          Resource update batch.
     */
    void updateBuffers(QRhi *rhi, QRhiResourceUpdateBatch *u);
    
    QRhiBuffer* vertexBuffer() const { return m_vertexBuffer.get(); }
    QRhiBuffer* indexBuffer() const { return m_indexBuffer.get(); }
    uint32_t indexCount() const { return m_indexCount; }
    uint32_t vertexCount() const { return m_vertexData.size(); }
    
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
     * @return Label name.
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
    QVector<QVector<int>> computeNeighbors() const;

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

    QVector<VertexData> m_vertexData;
    QVector<VertexData> m_originalVertexData;
    QVector<uint32_t> m_indexData;
    uint32_t m_indexCount = 0;
    
    QColor m_defaultColor = Qt::white;
    QColor m_baseColor = Qt::white;
    
    FSLIB::Annotation m_annotation;
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

    std::unique_ptr<QRhiBuffer> m_vertexBuffer;
    std::unique_ptr<QRhiBuffer> m_indexBuffer;
    bool m_bBuffersDirty = true;
    
    mutable QVector3D m_aabbMin;
    mutable QVector3D m_aabbMax;
    mutable bool m_bAABBDirty = true;
};

#endif // BRAINSURFACE_H
