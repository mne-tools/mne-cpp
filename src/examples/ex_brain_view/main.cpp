//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Pure QRhi example with FreeSurfer brain surface rendering (Optimized)
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fs/surfaceset.h>
#include <fs/surface.h>

#include <memory>
#include <QApplication>
#include <QRhiWidget>
#include <rhi/qrhi.h>
#include <QFile>
#include <QTimer>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMatrix4x4>
#include <QVector3D>
#include <QCommandLineParser>
#include <QDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>

//=============================================================================================================
// USING
//=============================================================================================================

using namespace FSLIB;

//=============================================================================================================
// CLASS DEFINITION
//=============================================================================================================

// Vertex layout with normals (28 bytes: pos 12 + normal 12 + color 4)
struct VertexData {
    float x, y, z;          // Position
    float nx, ny, nz;       // Normal
    uint32_t color;         // Packed RGBA
};

/**
 * @brief Widget that renders brain surface using pure QRhi
 */
class BrainSurfaceWidget : public QRhiWidget
{
    Q_OBJECT

public:
    enum ShaderMode {
        Standard,
        Holographic,
        Atlas
    };

    explicit BrainSurfaceWidget(QWidget *parent = nullptr)
    : QRhiWidget(parent)
    , m_rotation(0.0f)
    , m_pitch(0.0f)
    , m_zoom(8.0f)
    , m_bSurfaceLoaded(false)
    , m_frameCount(0)
    , m_snapshotCounter(0)
    , m_vertexBuffer(nullptr)
    , m_indexBuffer(nullptr)
    , m_uniformBuffer(nullptr)
    , m_pipeline(nullptr)
    , m_pipelineBackColor(nullptr)
    , m_srb(nullptr)
    , m_shaderMode(Standard)
    , m_lightingEnabled(true)
    , m_activeSurfaceType("pial")
    {
        setMinimumSize(800, 600);
        setSampleCount(1);
        
        m_updateTimer = new QTimer(this);
        connect(m_updateTimer, &QTimer::timeout, this, QOverload<>::of(&BrainSurfaceWidget::update));
        m_updateTimer->start(0);
        
        m_fpsTimer.start();

        m_fpsLabel = new QLabel(this);
        m_fpsLabel->setStyleSheet("QLabel { color : #ffffff; font-family: 'Arial'; font-size: 14px; font-weight: bold; background-color: transparent; }");
        m_fpsLabel->move(20, 20);
        m_fpsLabel->resize(200, 50);
        m_fpsLabel->show();
        m_fpsLabel->raise();
    }
    
    ~BrainSurfaceWidget() {}

    bool loadBrainSurface(const QString &subjectPath, const QString &subject, int hemi, const QString &surfType = "pial")
    {
        try {
            SurfaceSet surfSet(subject, hemi, surfType, subjectPath);
            if (surfSet.isEmpty()) return false;
            
            Surface surf = surfSet[0];
            QVector<VertexData> vertices;
            vertices.reserve(surf.rr().rows());
            
            if (m_indexData.isEmpty()) {
                m_indexData.reserve(surf.tris().rows() * 3);
                for (int i = 0; i < surf.tris().rows(); ++i) {
                    m_indexData.append(surf.tris()(i, 0));
                    m_indexData.append(surf.tris()(i, 1));
                    m_indexData.append(surf.tris()(i, 2));
                }
            }
            
            const Eigen::VectorXf& curv = surf.curv();
            bool hasCurv = (curv.rows() == surf.rr().rows());
            
            for (int i = 0; i < surf.rr().rows(); ++i) {
                uint8_t c = 128;
                if (hasCurv) {
                    float val = curv[i];
                    c = (val > 0) ? 50 : 255;
                }
                uint32_t colorPacked = c | (c << 8) | (c << 16) | (255 << 24);
                
                vertices.append({
                    surf.rr()(i, 0), surf.rr()(i, 1), surf.rr()(i, 2),
                    surf.nn()(i, 0), surf.nn()(i, 1), surf.nn()(i, 2),
                    colorPacked
                });
            }
            
            m_availableSurfaces[surfType] = vertices;
            if (m_vertexData.isEmpty() || surfType == m_activeSurfaceType) {
                 m_vertexData = vertices;
                 m_bSurfaceLoaded = true;
                 if (rhi()) {
                    m_bDataDirty = true;
                    update();
                 }
            }
            return true;
        } catch (...) {
            return false;
        }
    }

    void setActiveSurface(const QString &type)
    {
        if (m_availableSurfaces.contains(type)) {
            m_activeSurfaceType = type;
            const auto& data = m_availableSurfaces[type];
            if (m_vertexBuffer) {
                QRhiResourceUpdateBatch *u = rhi()->nextResourceUpdateBatch();
                u->updateDynamicBuffer(m_vertexBuffer.get(), 0, data.size() * sizeof(VertexData), data.constData());
                m_pendingUpload = u;
                update();
            }
        }
    }

    void setShaderMode(const QString &modeName) {
        ShaderMode newMode = Standard;
        if (modeName == "Holographic") newMode = Holographic;
        else if (modeName == "Atlas") newMode = Atlas;
        
        qDebug() << "setShaderMode: mode =" << modeName << "newMode =" << newMode << "current =" << m_shaderMode;
        if (m_shaderMode != newMode) {
            m_shaderMode = newMode;
            qDebug() << "setShaderMode: Resetting pipelines!";
            m_pipeline.reset(); 
            m_pipelineBackColor.reset();
            update();
        }
    }

    void setLightingEnabled(bool enabled) {
        m_lightingEnabled = enabled;
        update();
    }

    void render(QRhiCommandBuffer *cb) override
    {
        if (m_bSurfaceLoaded && (m_bDataDirty || !m_pipeline)) {
            qDebug() << "render: m_bDataDirty =" << m_bDataDirty << "m_pipeline =" << m_pipeline.get();
            createResources();
            m_bDataDirty = false;
        }

        if (!m_bSurfaceLoaded || !m_pipeline) return;

        m_frameCount++;
        if (m_fpsTimer.elapsed() > 500) {
            float fps = m_frameCount / (m_fpsTimer.elapsed() / 1000.0f);
            if (m_fpsLabel) {
                m_fpsLabel->setText(QString("FPS: %1\nVertices: %2\nShader: %3")
                                    .arg(QString::number(fps, 'f', 1))
                                    .arg(m_vertexData.size())
                                    .arg(m_shaderMode == Standard ? "Standard" : m_shaderMode == Holographic ? "Holographic" : "Atlas"));
            }
            m_frameCount = 0;
            m_fpsTimer.restart();
        }

        QSize outputSize = renderTarget()->pixelSize();
        QMatrix4x4 projection;
        projection.perspective(45.0f, float(outputSize.width()) / float(outputSize.height()), 0.01f, 100.0f);
        
        float distance = 0.5f - m_zoom * 0.03f;
        float azimuth = m_rotation * M_PI / 180.0f;
        float elevation = m_pitch * M_PI / 180.0f;
        QVector3D cameraPos(distance * cos(elevation) * sin(azimuth), distance * sin(elevation), distance * cos(elevation) * cos(azimuth));
        
        QMatrix4x4 view;
        view.lookAt(cameraPos, QVector3D(0,0,0), QVector3D(0,1,0));
        
        QMatrix4x4 mvp = rhi()->clipSpaceCorrMatrix();
        mvp *= projection;
        mvp *= view;
        
        QVector3D lightDir = (view.inverted() * QVector4D(0, 1, 0, 0)).toVector3D().normalized();
        
        QRhiResourceUpdateBatch *u = m_pendingUpload;
        m_pendingUpload = nullptr;
        if (!u) u = rhi()->nextResourceUpdateBatch();

        if (!m_uniformBuffer) {
            qWarning() << "render: m_uniformBuffer is NULL!";
            return;
        }
        u->updateDynamicBuffer(m_uniformBuffer.get(), 0, 64, mvp.constData());
        u->updateDynamicBuffer(m_uniformBuffer.get(), 64, 12, &cameraPos);
        u->updateDynamicBuffer(m_uniformBuffer.get(), 80, 12, &lightDir);
        float lightingEnabled = m_lightingEnabled ? 1.0f : 0.0f;
        u->updateDynamicBuffer(m_uniformBuffer.get(), 96, 4, &lightingEnabled);
        
        const QColor clearColor(0, 0, 0);
        const QRhiCommandBuffer::VertexInput vbufBinding(m_vertexBuffer.get(), 0);

        if (m_shaderMode == Holographic) {
            if (!m_pipeline) {
                qDebug() << "render: Holographic pipeline missing!";
                cb->beginPass(renderTarget(), clearColor, { 1.0f, 0 }, u);
                cb->endPass();
                return;
            }
            cb->beginPass(renderTarget(), clearColor, { 1.0f, 0 }, u);
            cb->setGraphicsPipeline(m_pipeline.get());
            cb->setViewport(QRhiViewport(0, 0, outputSize.width(), outputSize.height()));
            cb->setShaderResources(m_srb.get());
            if (!m_indexBuffer) { qWarning() << "render: m_indexBuffer NULL!"; return; }
            cb->setVertexInput(0, 1, &vbufBinding, m_indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
            cb->drawIndexed(m_indexData.size());
            cb->endPass();
        } else {
            if (!m_pipeline) {
                qDebug() << "render: m_pipeline missing!";
                cb->beginPass(renderTarget(), clearColor, { 1.0f, 0 }, u);
                cb->endPass();
                return;
            }
            cb->beginPass(renderTarget(), clearColor, { 1.0f, 0 }, u);
            cb->setGraphicsPipeline(m_pipeline.get());
            cb->setViewport(QRhiViewport(0, 0, outputSize.width(), outputSize.height()));
            cb->setShaderResources(m_srb.get());
            if (!m_indexBuffer) { qWarning() << "render: m_indexBuffer NULL!"; return; }
            cb->setVertexInput(0, 1, &vbufBinding, m_indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
            cb->drawIndexed(m_indexData.size());
            cb->endPass();
        }
    }

    void mousePressEvent(QMouseEvent *event) override { m_lastMousePos = event->pos(); }
    void mouseMoveEvent(QMouseEvent *event) override {
        QPoint delta = event->pos() - m_lastMousePos;
        m_rotation -= delta.x() * 0.5f;
        m_pitch = qBound(-89.0f, m_pitch + delta.y() * 0.5f, 89.0f);
        m_lastMousePos = event->pos();
    }
    void wheelEvent(QWheelEvent *event) override {
        m_zoom = qBound(1.0f, m_zoom + ((event->angleDelta().y() > 0) ? 0.5f : -0.5f), 10.0f);
    }
    void keyPressEvent(QKeyEvent *event) override {
        qDebug() << "keyPressEvent: key =" << event->key();
        if (event->key() == Qt::Key_S) setShaderMode(m_shaderMode == Standard ? "Holographic" : m_shaderMode == Holographic ? "Atlas" : "Standard");
        else if (event->key() == Qt::Key_1) setActiveSurface("pial");
        else if (event->key() == Qt::Key_2) setActiveSurface("inflated");
        else if (event->key() == Qt::Key_3) setActiveSurface("white");
        else if (event->key() == Qt::Key_P) saveSnapshot();
    }

    void saveSnapshot() {
        QString filename = QString("/Users/christoph.dinh/.gemini/antigravity/brain/b210b818-b4ba-4c78-86c6-b41dd5e1234b/snapshot_%1.png").arg(m_snapshotCounter++, 4, 10, QChar('0'));
        grab().save(filename);
    }

private:
    void createResources() {
        if (!rhi()) {
            qWarning() << "createResources: rhi() is NULL!";
            return;
        }
        qDebug() << "createResources: Creating buffers...";
        size_t vSize = m_vertexData.size() * sizeof(VertexData);
        if (!m_vertexBuffer || m_vertexBuffer->size() < vSize) {
            m_vertexBuffer.reset(rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, vSize));
            m_vertexBuffer->create();
        }
        
        size_t iSize = m_indexData.size() * sizeof(uint32_t);
        if (!m_indexBuffer || m_indexBuffer->size() < iSize) {
            m_indexBuffer.reset(rhi()->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, iSize));
            m_indexBuffer->create();
        }
        
        if (!m_uniformBuffer) {
            m_uniformBuffer.reset(rhi()->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 256));
            m_uniformBuffer->create();
        }
        
        QRhiResourceUpdateBatch *u = rhi()->nextResourceUpdateBatch();
        u->updateDynamicBuffer(m_vertexBuffer.get(), 0, m_vertexData.size() * sizeof(VertexData), m_vertexData.constData());
        u->uploadStaticBuffer(m_indexBuffer.get(), m_indexData.constData());
        
        auto getShader = [](const QString &name) {
            QFile f(name);
            if (!f.open(QIODevice::ReadOnly)) {
                qWarning() << "getShader: Failed to open" << name;
                return QShader();
            }
            return QShader::fromSerialized(f.readAll());
        };
        
        m_pipeline.reset(rhi()->newGraphicsPipeline());
        m_pipelineBackColor.reset();
        
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = (m_shaderMode != Standard);
        if (blend.enable) {
            blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
            blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
            blend.srcAlpha = QRhiGraphicsPipeline::One;
            blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        }
        
        m_pipeline->setTargetBlends({ blend });
        m_pipeline->setDepthTest(true);
        m_pipeline->setDepthWrite(true);
        m_pipeline->setDepthOp(QRhiGraphicsPipeline::Less);
        m_pipeline->setCullMode(QRhiGraphicsPipeline::Back);
        
        m_pipeline->setSampleCount(sampleCount());
        
        QString vert = (m_shaderMode == Holographic) ? ":/brain_exp.vert.qsb" : (m_shaderMode == Atlas) ? ":/brain_atlas.vert.qsb" : ":/brain.vert.qsb";
        QString frag = (m_shaderMode == Holographic) ? ":/brain_exp.frag.qsb" : (m_shaderMode == Atlas) ? ":/brain_atlas.frag.qsb" : ":/brain.frag.qsb";

        QShader vS = getShader(vert);
        QShader fS = getShader(frag);
        if (!vS.isValid() || !fS.isValid()) {
            qWarning() << "createResources: Shaders are invalid!";
            return;
        }

        m_srb.reset(rhi()->newShaderResourceBindings());
        m_srb->setBindings({ QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_uniformBuffer.get()) });
        m_srb->create();

        auto setup = [&](std::unique_ptr<QRhiGraphicsPipeline>& p) {
            if (!p) {
                 qWarning() << "setup: Pipeline pointer is NULL!";
                 return;
            }
            qDebug() << "setup: Configuring pipeline stages...";
            p->setShaderStages({{ QRhiShaderStage::Vertex, vS }, { QRhiShaderStage::Fragment, fS }});
            QRhiVertexInputLayout il;
            il.setBindings({{ 28 }});
            il.setAttributes({{ 0, 0, QRhiVertexInputAttribute::Float3, 0 }, { 0, 1, QRhiVertexInputAttribute::Float3, 12 }, { 0, 2, QRhiVertexInputAttribute::UNormByte4, 24 }});
            p->setVertexInputLayout(il);
            p->setShaderResourceBindings(m_srb.get());
            if (!renderTarget()) { qWarning() << "setup: renderTarget is NULL!"; return; }
            p->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
            
            qDebug() << "setup: Calling p->create()...";
            if (!p->create()) {
                qWarning() << "setup: Failed to create pipeline!";
                p.reset();
            } else {
                qDebug() << "setup: Pipeline created successfully!";
            }
        };

        qDebug() << "createResources: Setting up pipelines for mode" << m_shaderMode;
        if (m_shaderMode == Holographic) setup(m_pipelineBackColor);
        setup(m_pipeline);
        if (!m_pipeline) {
            qWarning() << "createResources: m_pipeline is STILL NULL after setup!";
        }
        m_pendingUpload = u;
    }

    float m_rotation, m_pitch, m_zoom;
    QPoint m_lastMousePos;
    bool m_bSurfaceLoaded;
    int m_frameCount;
    QElapsedTimer m_fpsTimer;
    QLabel *m_fpsLabel;
    QTimer *m_updateTimer;

    QMap<QString, QVector<VertexData>> m_availableSurfaces;
    QString m_activeSurfaceType;
    ShaderMode m_shaderMode;
    bool m_lightingEnabled;
    int m_snapshotCounter;

    QVector<VertexData> m_vertexData;
    QVector<uint32_t> m_indexData;
    bool m_bDataDirty = false;
    
    std::unique_ptr<QRhiBuffer> m_vertexBuffer;
    std::unique_ptr<QRhiBuffer> m_indexBuffer;
    std::unique_ptr<QRhiBuffer> m_uniformBuffer;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipelineBackColor;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    QRhiResourceUpdateBatch *m_pendingUpload = nullptr;
};

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    QCommandLineParser parser;
    parser.setApplicationDescription("QRhi Brain View");
    parser.addHelpOption();
    
    QCommandLineOption subjectPathOption("subjectPath", "Path", "path", 
        QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects");
    QCommandLineOption subjectOption("subject", "Subject", "name", "sample");
    QCommandLineOption hemiOption("hemi", "Hemi", "hemi", "0");
    parser.addOptions({subjectPathOption, subjectOption, hemiOption});
    parser.process(app);
    
    QWidget mainWindow;
    mainWindow.setWindowTitle("MNE-CPP Brain View (QRhi)");
    mainWindow.resize(1280, 720);
    
    // Main Layout (Horizontal)
    QHBoxLayout *mainLayout = new QHBoxLayout(&mainWindow);
    
    // Side Panel (Controls)
    QWidget *sidePanel = new QWidget;
    sidePanel->setFixedWidth(250);
    QVBoxLayout *sideLayout = new QVBoxLayout(sidePanel);
    
    QGroupBox *controlGroup = new QGroupBox("Controls");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);
    
    // Controls: Surface Selector
    QLabel *surfLabel = new QLabel("Surface Type:");
    QComboBox *surfCombo = new QComboBox;
    surfCombo->addItems({"pial", "inflated", "white"});
    
    // Controls: Shader Mode
    QLabel *shaderLabel = new QLabel("Shader:");
    QComboBox *shaderCombo = new QComboBox();
    shaderCombo->addItem("Standard");
    shaderCombo->addItem("Holographic");
    shaderCombo->addItem("Atlas");
    
    // Controls: Lighting Toggle
    QCheckBox *lightingCheck = new QCheckBox("Enable Lighting");
    lightingCheck->setChecked(true); // Default on
    
    controlLayout->addWidget(surfLabel);
    controlLayout->addWidget(surfCombo);
    controlLayout->addWidget(shaderLabel);
    controlLayout->addWidget(shaderCombo);
    controlLayout->addWidget(lightingCheck);
    controlLayout->addStretch();
    
    sideLayout->addWidget(controlGroup);
    sideLayout->addStretch();
    
    // Brain Widget
    BrainSurfaceWidget *brainWidget = new BrainSurfaceWidget();
    brainWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Add to main layout
    mainLayout->addWidget(sidePanel);
    mainLayout->addWidget(brainWidget);
    
    QString path = parser.value(subjectPathOption);
    QString subj = parser.value(subjectOption);
    int hemi = parser.value(hemiOption).toInt();
    
    // Load surfaces
    QStringList types = {"pial", "inflated", "white"};
    bool anyLoaded = false;
    
    for (const auto& type : types) {
         if (brainWidget->loadBrainSurface(path, subj, hemi, type)) {
             anyLoaded = true;
         }
    }

    if (anyLoaded) {
        qDebug() << "Surfaces loaded.";
    } else {
        QMessageBox::critical(&mainWindow, "Error", "Failed to load surface.");
    }
    
    // Connect Controls
    QObject::connect(surfCombo, &QComboBox::currentTextChanged, brainWidget, &BrainSurfaceWidget::setActiveSurface);
    QObject::connect(shaderCombo, &QComboBox::currentTextChanged, brainWidget, &BrainSurfaceWidget::setShaderMode);
    QObject::connect(lightingCheck, &QCheckBox::toggled, brainWidget, &BrainSurfaceWidget::setLightingEnabled);
    
    mainWindow.show();
    return app.exec();
}

#include "main.moc"
