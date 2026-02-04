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
        // Mat4 is 4 columns of vec4. Layout in standard is column-major.
        // However, in shader we use row vectors for attributes to assemble mat4.
        // Let's stick to standard layout and handle in shader.
        // Actually, easiest is 4 vec4s for matrix columns/rows.
        float model[16]; 
        float color[4];
    };
    
    QByteArray m_vertexData;
    QByteArray m_indexData;
    QByteArray m_instanceData;
    
    bool m_geometryDirty = false;
    bool m_instancesDirty = false;
};

#endif // DIPOLEOBJECT_H
