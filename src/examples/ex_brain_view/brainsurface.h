//=============================================================================================================
/**
 * @file     brainsurface.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
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
        ModeScientific // Curvature
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
     * Load annotation data from file.
     *
     * @param[in] path       Path to the .annot file.
     * @return True if successful.
     */
    bool loadAnnotation(const QString &path);

    //=========================================================================================================
    /**
     * Set the visualization mode (Surface, Annotation, Scientific).
     *
     * @param[in] mode       VisualizationMode enum.
     */
    void setVisualizationMode(VisualizationMode mode);
    
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
     * Translate all vertices along the X axis.
     *
     * @param[in] offset     Amount to translate.
     */
    void translateX(float offset);

private:
    void updateVertexColors();

    QVector<VertexData> m_vertexData;
    QVector<uint32_t> m_indexData;
    uint32_t m_indexCount = 0;
    
    FSLIB::Annotation m_annotation;
    bool m_hasAnnotation = false;
    VisualizationMode m_visMode = ModeSurface;
    QVector<float> m_curvature;
    
    std::atomic<bool> m_visible{true};
    int m_hemi = -1; // 0=lh, 1=rh

    std::unique_ptr<QRhiBuffer> m_vertexBuffer;
    std::unique_ptr<QRhiBuffer> m_indexBuffer;
    bool m_bBuffersDirty = true;
};

#endif // BRAINSURFACE_H
