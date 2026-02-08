#ifndef DIPOLEOBJECT_H
#define DIPOLEOBJECT_H

#include <rhi/qrhi.h>
#include <QMatrix4x4>
#include <QVector3D>
#include <vector>
#include <memory>
#include <inverse/dipoleFit/ecd_set.h>

class DipoleObject
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
