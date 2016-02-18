

#ifndef SHADERMATERIAL_H
#define SHADERMATERIAL_H

#include "../disp3D_global.h"

#include <Qt3DRender/qmaterial.h>
#include <QColor>
#include <Qt3DRender/qmaterial.h>
#include <Qt3DRender/qeffect.h>
#include <Qt3DRender/qtechnique.h>
#include <Qt3DRender/qshaderprogram.h>
#include <Qt3DRender/qparameter.h>
#include <Qt3DRender/qrenderpass.h>
#include <Qt3DRender/qgraphicsapifilter.h>
#include <QUrl>
#include <QVector3D>
#include <QVector4D>


QT_BEGIN_NAMESPACE

namespace DISP3DLIB {


class DISP3DNEWSHARED_EXPORT ShaderMaterial : public Qt3DRender::QMaterial
{
    Q_OBJECT

public:
    explicit ShaderMaterial(Qt3DCore::QNode *parent = 0);
    ~ShaderMaterial();

private:
    void init();

    Qt3DRender::QEffect*            m_vertexEffect;
    Qt3DRender::QTechnique*         m_vertexGL3Technique;
    Qt3DRender::QTechnique*         m_vertexGL2Technique;
    Qt3DRender::QTechnique*         m_vertexES2Technique;
    Qt3DRender::QRenderPass*        m_vertexGL3RenderPass;
    Qt3DRender::QRenderPass*        m_vertexGL2RenderPass;
    Qt3DRender::QRenderPass*        m_vertexES2RenderPass;
    Qt3DRender::QShaderProgram*     m_vertexGL3Shader;
    Qt3DRender::QShaderProgram*     m_vertexGL2ES2Shader;
};

} // namespace DISP3DLIB

#endif // SHADERMATERIAL_H
