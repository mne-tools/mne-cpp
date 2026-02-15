//=============================================================================================================
/**
 * @file     dipoleobject.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    DipoleObject class declaration.
 *
 */

#ifndef DIPOLEOBJECT_H
#define DIPOLEOBJECT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_rhi_global.h"

#include <rhi/qrhi.h>
#include <QMatrix4x4>
#include <QVector3D>
#include <vector>
#include <memory>
#include <inverse/dipoleFit/ecd_set.h>

class DISP3DRHISHARED_EXPORT DipoleObject
{
public:
    DipoleObject();
    ~DipoleObject();

    void load(const INVERSELIB::ECDSet &ecdSet);
    
    // Apply a transformation matrix to all dipoles
    void applyTransform(const QMatrix4x4 &trans);

    void updateBuffers(QRhi *rhi, QRhiResourceUpdateBatch *u);
    
    QRhiBuffer* vertexBuffer() const { return m_vertexBuffer.get(); }
    QRhiBuffer* indexBuffer() const { return m_indexBuffer.get(); }
    QRhiBuffer* instanceBuffer() const { return m_instanceBuffer.get(); }
    
    int indexCount() const { return m_indexCount; }
    int instanceCount() const { return m_instanceCount; }
    
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    
    QVector3D debugFirstDipolePosition() const; // For debugging

    void setSelected(int index, bool selected);

    //=========================================================================================================
    /**
     * Test ray intersection with dipoles.
     * 
     * @param[in] rayOrigin  Ray origin in world space.
     * @param[in] rayDir     Ray direction (normalized).
     * @param[out] dist      Distance to intersection.
     * @return Index of intersected dipole, or -1 if none.
     */
    int intersect(const QVector3D &rayOrigin, const QVector3D &rayDir, float &dist) const;

private:
    void createGeometry();
    
    std::unique_ptr<QRhiBuffer> m_vertexBuffer;
    std::unique_ptr<QRhiBuffer> m_indexBuffer;
    std::unique_ptr<QRhiBuffer> m_instanceBuffer;
    
    int m_indexCount = 0;
    int m_instanceCount = 0;
    bool m_visible = true;
    
    struct VertexData {
        float x, y, z;
        float nx, ny, nz;
    };
    
    // Instance data: Model Matrix (4x4) + Color (vec4)
    struct InstanceData {
        float model[16]; 
        float color[4];
        float isSelected; // 1.0 = selected, 0.0 = not
    };
    
    QByteArray m_vertexData;
    QByteArray m_indexData;
    QByteArray m_instanceData;
    
    bool m_geometryDirty = false;
    bool m_instancesDirty = false;
    
    std::vector<QVector4D> m_originalColors;
};

#endif // DIPOLEOBJECT_H
