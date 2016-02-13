
#include "shadermaterial.h"

using namespace DISP3DLIB;
using namespace Qt3DRender;

ShaderMaterial::ShaderMaterial(QNode *parent)
: QMaterial(parent)
, m_vertexEffect(new QEffect())
, m_vertexGL3Technique(new QTechnique())
, m_vertexGL2Technique(new QTechnique())
, m_vertexES2Technique(new QTechnique())
, m_vertexGL3RenderPass(new QRenderPass())
, m_vertexGL2RenderPass(new QRenderPass())
, m_vertexES2RenderPass(new QRenderPass())
, m_vertexGL3Shader(new QShaderProgram())
, m_vertexGL2ES2Shader(new QShaderProgram())
{
    this->init();
}

ShaderMaterial::~ShaderMaterial()
{
}

// TODO: Define how lights are proties are set in the shaders. Ideally using a QShaderData
void ShaderMaterial::init()
{
    m_vertexGL3Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/materials/shaders/gl3/brain.vert"))));
    m_vertexGL3Shader->setFragmentShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/materials/shaders/gl3/brain.frag"))));
    m_vertexGL2ES2Shader->setVertexShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/materials/shaders/es2/brain.vert"))));
    m_vertexGL2ES2Shader->setFragmentShaderCode(QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/materials/shaders/es2/brain.frag"))));

    m_vertexGL3Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_vertexGL3Technique->graphicsApiFilter()->setMajorVersion(3);
    m_vertexGL3Technique->graphicsApiFilter()->setMinorVersion(1);
    m_vertexGL3Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::CoreProfile);

    m_vertexGL2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGL);
    m_vertexGL2Technique->graphicsApiFilter()->setMajorVersion(2);
    m_vertexGL2Technique->graphicsApiFilter()->setMinorVersion(0);
    m_vertexGL2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    m_vertexES2Technique->graphicsApiFilter()->setApi(QGraphicsApiFilter::OpenGLES);
    m_vertexES2Technique->graphicsApiFilter()->setMajorVersion(2);
    m_vertexES2Technique->graphicsApiFilter()->setMinorVersion(0);
    m_vertexES2Technique->graphicsApiFilter()->setProfile(QGraphicsApiFilter::NoProfile);

    m_vertexGL3RenderPass->setShaderProgram(m_vertexGL3Shader);
    m_vertexGL2RenderPass->setShaderProgram(m_vertexGL2ES2Shader);
    m_vertexES2RenderPass->setShaderProgram(m_vertexGL2ES2Shader);

    m_vertexGL3Technique->addPass(m_vertexGL3RenderPass);
    m_vertexGL2Technique->addPass(m_vertexGL2RenderPass);
    m_vertexES2Technique->addPass(m_vertexES2RenderPass);

    m_vertexEffect->addTechnique(m_vertexGL3Technique);
    m_vertexEffect->addTechnique(m_vertexGL2Technique);
    m_vertexEffect->addTechnique(m_vertexES2Technique);

    this->setEffect(m_vertexEffect);
}

QT_END_NAMESPACE
