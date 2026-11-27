//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rhiframerenderer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    RhiFrameRenderer definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rhiframerenderer.h"

#include "scenepainter.h"
#include <mne/base/log.h>
#include <rhi/qrhi.h>
#include <rhi/qshader.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QFont>
#include <QImage>
#include <QPainter>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstring>
#include <format>
#include <map>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz::qt;
using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC HELPERS
//=============================================================================================================

namespace
{

constexpr auto kAllStages = QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;

QRhiVertexInputAttribute::Format toRhi(GpuVertexFormat format)
{
    switch (format) {
        case GpuVertexFormat::Float:
            return QRhiVertexInputAttribute::Float;
        case GpuVertexFormat::Float2:
            return QRhiVertexInputAttribute::Float2;
        case GpuVertexFormat::Float3:
            return QRhiVertexInputAttribute::Float3;
        case GpuVertexFormat::Float4:
            return QRhiVertexInputAttribute::Float4;
        case GpuVertexFormat::UNormByte4:
            return QRhiVertexInputAttribute::UNormByte4;
    }
    return QRhiVertexInputAttribute::Float;
}

QRhiGraphicsPipeline::Topology toRhi(GpuTopology topology)
{
    switch (topology) {
        case GpuTopology::Triangles:
            return QRhiGraphicsPipeline::Triangles;
        case GpuTopology::TriangleStrip:
            return QRhiGraphicsPipeline::TriangleStrip;
        case GpuTopology::Lines:
            return QRhiGraphicsPipeline::Lines;
        case GpuTopology::LineStrip:
            return QRhiGraphicsPipeline::LineStrip;
        case GpuTopology::Points:
            return QRhiGraphicsPipeline::Points;
    }
    return QRhiGraphicsPipeline::Triangles;
}

QRhiGraphicsPipeline::CompareOp toRhi(GpuDepthCompare compare)
{
    switch (compare) {
        case GpuDepthCompare::Less:
            return QRhiGraphicsPipeline::Less;
        case GpuDepthCompare::LessOrEqual:
            return QRhiGraphicsPipeline::LessOrEqual;
        case GpuDepthCompare::Always:
            return QRhiGraphicsPipeline::Always;
    }
    return QRhiGraphicsPipeline::Less;
}

QRhiGraphicsPipeline::TargetBlend toRhi(GpuBlend blend, bool colorWrite)
{
    QRhiGraphicsPipeline::TargetBlend target;
    if (!colorWrite) {
        target.colorWrite = {};
    }
    switch (blend) {
        case GpuBlend::None:
            break;
        case GpuBlend::Alpha:
            target.enable = true;
            target.srcColor = QRhiGraphicsPipeline::SrcAlpha;
            target.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
            target.srcAlpha = QRhiGraphicsPipeline::One;
            target.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
            break;
        case GpuBlend::Additive:
            target.enable = true;
            target.srcColor = QRhiGraphicsPipeline::SrcAlpha;
            target.dstColor = QRhiGraphicsPipeline::One;
            target.srcAlpha = QRhiGraphicsPipeline::One;
            target.dstAlpha = QRhiGraphicsPipeline::One;
            break;
    }
    return target;
}

QShader loadShader(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QShader::fromSerialized(file.readAll());
}

QColor toQColor(const Color& color)
{
    return QColor(color.r, color.g, color.b, color.a);
}

} // namespace

//=============================================================================================================
// DEFINE PRIVATE STRUCTS
//=============================================================================================================

struct RhiFrameRenderer::Impl
{
    struct Buffer
    {
        std::unique_ptr<QRhiBuffer> buffer;
        std::uint64_t revision = 0;
        int uploads = 0;
    };

    struct Texture
    {
        std::unique_ptr<QRhiTexture> texture;
        std::uint64_t revision = 0;
        QSize size;
        std::uint64_t serial = 0; /**< Changes whenever the QRhi texture is created again. */
        bool mipmaps = false;     /**< Whether the texture has mipmaps. */
        QImage image;             /**< What was uploaded last. */
    };

    struct Bindings
    {
        std::unique_ptr<QRhiShaderResourceBindings> srb;
        bool used = false;
    };

    struct Pipeline
    {
        std::unique_ptr<QRhiGraphicsPipeline> pipeline;
        std::vector<std::uint32_t> renderPassFormat;
        int sampleCount = 0;
    };

    std::vector<std::string> prefixes;
    QFont font;
    QRhi* rhi = nullptr;
    std::map<GpuHandle, Buffer> buffers;
    std::map<GpuHandle, Texture> textures;
    std::map<GpuHandle, Pipeline> pipelines;
    std::map<std::string, Bindings> bindings; /**< By pipeline, uniform buffer, blocks and textures. */
    std::unique_ptr<QRhiBuffer> uniforms;
    std::uint64_t serial = 0; /**< Last serial handed to a uniform buffer or texture. */
    std::uint64_t uniformsSerial = 0;
    std::unique_ptr<QRhiSampler> linearSampler;
    std::unique_ptr<QRhiSampler> nearestSampler;
    std::unique_ptr<QRhiSampler> mipmapSampler;
    std::map<std::string, std::pair<QShader, QShader>> shaders;

    void release()
    {
        bindings.clear();
        pipelines.clear();
        buffers.clear();
        textures.clear();
        uniforms.reset();
        linearSampler.reset();
        nearestSampler.reset();
        mipmapSampler.reset();
        rhi = nullptr;
    }

    QRhiBuffer* buffer(GpuHandle handle, const GpuResources& resources, QRhiResourceUpdateBatch* batch);
    QRhiTexture* texture(GpuHandle handle, const GpuResources& resources, QRhiResourceUpdateBatch* batch);
    const std::pair<QShader, QShader>* shaderPair(const std::string& id);
    QRhiShaderResourceBindings* shaderBindings(const GpuDraw& draw, const std::vector<quint32>& blockSizes,
                                               const GpuResources& resources, QRhiResourceUpdateBatch* batch);
    QRhiGraphicsPipeline* pipeline(GpuHandle handle, const GpuResources& resources, QRhiRenderTarget* target,
                                   QRhiShaderResourceBindings* layout);
};

//=============================================================================================================

QRhiBuffer* RhiFrameRenderer::Impl::buffer(GpuHandle handle, const GpuResources& resources, QRhiResourceUpdateBatch* batch)
{
    const GpuBuffer* source = resources.buffer(handle);
    if (!source || source->data.empty()) {
        return nullptr;
    }
    Buffer& cached = buffers[handle];
    if (cached.buffer && cached.revision == source->revision) {
        return cached.buffer.get();
    }
    const auto size = static_cast<quint32>(source->data.size());
    const QRhiBuffer::UsageFlags usage = source->usage == GpuBufferUsage::Vertex ? QRhiBuffer::VertexBuffer : QRhiBuffer::IndexBuffer;
    // Buffers that change after their first upload become dynamic, e.g. streamed trace vertices
    const QRhiBuffer::Type type = cached.uploads > 0 ? QRhiBuffer::Dynamic : QRhiBuffer::Static;
    if (!cached.buffer || cached.buffer->size() < size || cached.buffer->type() != type) {
        cached.buffer.reset(rhi->newBuffer(type, usage, size));
        if (!cached.buffer->create()) {
            mne::Log::warning("RhiFrameRenderer: cannot create a buffer of {} bytes", size);
            cached.buffer.reset();
            return nullptr;
        }
    }
    if (type == QRhiBuffer::Dynamic) {
        batch->updateDynamicBuffer(cached.buffer.get(), 0, size, source->data.data());
    } else {
        batch->uploadStaticBuffer(cached.buffer.get(), 0, size, source->data.data());
    }
    cached.revision = source->revision;
    ++cached.uploads;
    return cached.buffer.get();
}

//=============================================================================================================

QRhiTexture* RhiFrameRenderer::Impl::texture(GpuHandle handle, const GpuResources& resources, QRhiResourceUpdateBatch* batch)
{
    const GpuTexture* source = resources.texture(handle);
    if (!source) {
        return nullptr;
    }
    Texture& cached = textures[handle];
    if (cached.texture && cached.revision == source->revision) {
        return cached.texture.get();
    }
    const QSize size(std::max(source->pixel_width(), 1), std::max(source->pixel_height(), 1));
    QImage image;
    if (const Image* pixels = std::get_if<Image>(&source->content)) {
        image = QImage(size, QImage::Format_RGBA8888);
        image.fill(Qt::transparent);
        if (pixels->width > 0 && pixels->height > 0) {
            for (int y = 0; y < pixels->height; ++y) {
                std::memcpy(image.scanLine(y), pixels->rgba.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(pixels->width) * 4,
                            static_cast<std::size_t>(pixels->width) * 4);
            }
        }
    } else {
        image = QImage(size, QImage::Format_RGBA8888);
        image.setDevicePixelRatio(source->scale);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, false);
        paintScene(painter, std::get<DisplayList>(source->content), font);
    }
    if (!cached.texture || cached.size != size || cached.mipmaps != source->mipmaps) {
        const QRhiTexture::Flags flags = source->mipmaps ? QRhiTexture::Flags(QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips) : QRhiTexture::Flags();
        cached.texture.reset(rhi->newTexture(QRhiTexture::RGBA8, size, 1, flags));
        if (!cached.texture->create()) {
            cached.texture.reset();
            return nullptr;
        }
        cached.size = size;
        cached.mipmaps = source->mipmaps;
        cached.serial = ++serial;
    }
    batch->uploadTexture(cached.texture.get(), QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(image)));
    if (source->mipmaps) {
        batch->generateMips(cached.texture.get());
    }
    cached.revision = source->revision;
    cached.image = std::move(image);
    return cached.texture.get();
}

//=============================================================================================================

const std::pair<QShader, QShader>* RhiFrameRenderer::Impl::shaderPair(const std::string& id)
{
    const auto found = shaders.find(id);
    if (found != shaders.end()) {
        return found->second.first.isValid() ? &found->second : nullptr;
    }
    std::pair<QShader, QShader> pair;
    for (const std::string& prefix : prefixes) {
        pair = {loadShader(QString::fromStdString(prefix + id + ".vert.qsb")), loadShader(QString::fromStdString(prefix + id + ".frag.qsb"))};
        if (pair.first.isValid() && pair.second.isValid()) {
            break;
        }
    }
    if (!pair.first.isValid() || !pair.second.isValid()) {
        mne::Log::warning("RhiFrameRenderer: no shaders for '{}'", id);
        pair = {};
    }
    const auto& stored = shaders[id] = pair;
    return stored.first.isValid() ? &stored : nullptr;
}

//=============================================================================================================

QRhiShaderResourceBindings* RhiFrameRenderer::Impl::shaderBindings(const GpuDraw& draw, const std::vector<quint32>& blockSizes,
                                                                   const GpuResources& resources, QRhiResourceUpdateBatch* batch)
{
    std::string key = std::to_string(draw.pipeline) + "|b" + std::to_string(uniformsSerial);
    std::vector<QRhiShaderResourceBinding> list;
    for (std::size_t i = 0; i < draw.uniforms.size(); ++i) {
        key += "|u" + std::to_string(draw.uniforms[i].binding) + ":" + std::to_string(blockSizes[i]);
        list.push_back(QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(draw.uniforms[i].binding, kAllStages, uniforms.get(), blockSizes[i]));
    }
    for (const GpuTextureBinding& binding : draw.textures) {
        QRhiTexture* tex = texture(binding.texture, resources, batch);
        const GpuTexture* source = resources.texture(binding.texture);
        if (!tex || !source) {
            return nullptr;
        }
        key += "|t" + std::to_string(binding.binding) + ":" + std::to_string(textures[binding.texture].serial);
        list.push_back(QRhiShaderResourceBinding::sampledTexture(binding.binding, kAllStages, tex,
                                                                 source->mipmaps             ? mipmapSampler.get()
                                                                     : source->linear_filter ? linearSampler.get()
                                                                                             : nearestSampler.get()));
    }
    Bindings& entry = bindings[key];
    entry.used = true;
    std::unique_ptr<QRhiShaderResourceBindings>& srb = entry.srb;
    if (!srb) {
        srb.reset(rhi->newShaderResourceBindings());
        srb->setBindings(list.begin(), list.end());
        if (!srb->create()) {
            srb.reset();
            return nullptr;
        }
    }
    return srb.get();
}

//=============================================================================================================

QRhiGraphicsPipeline* RhiFrameRenderer::Impl::pipeline(GpuHandle handle, const GpuResources& resources, QRhiRenderTarget* target,
                                                       QRhiShaderResourceBindings* layout)
{
    const GpuPipeline* source = resources.pipeline(handle);
    if (!source) {
        return nullptr;
    }
    Pipeline& cached = pipelines[handle];
    const QVector<quint32> serialized = target->renderPassDescriptor()->serializedFormat();
    const std::vector<std::uint32_t> format(serialized.begin(), serialized.end());
    if (cached.pipeline && cached.renderPassFormat == format && cached.sampleCount == target->sampleCount()) {
        return cached.pipeline.get();
    }
    const std::pair<QShader, QShader>* stages = shaderPair(source->shader);
    if (!stages) {
        return nullptr;
    }

    std::unique_ptr<QRhiGraphicsPipeline> created(rhi->newGraphicsPipeline());
    created->setShaderStages({{QRhiShaderStage::Vertex, stages->first}, {QRhiShaderStage::Fragment, stages->second}});
    QRhiVertexInputLayout inputLayout;
    std::vector<QRhiVertexInputBinding> inputBindings;
    inputBindings.reserve(source->bindings.size());
    for (const GpuVertexBinding& binding : source->bindings) {
        inputBindings.emplace_back(binding.stride, binding.per_instance ? QRhiVertexInputBinding::PerInstance : QRhiVertexInputBinding::PerVertex);
    }
    std::vector<QRhiVertexInputAttribute> attributes;
    attributes.reserve(source->attributes.size());
    for (const GpuVertexAttribute& attribute : source->attributes) {
        attributes.emplace_back(attribute.binding, attribute.location, toRhi(attribute.format), attribute.offset);
    }
    inputLayout.setBindings(inputBindings.begin(), inputBindings.end());
    inputLayout.setAttributes(attributes.begin(), attributes.end());
    created->setVertexInputLayout(inputLayout);
    created->setTopology(toRhi(source->topology));
    created->setTargetBlends({toRhi(source->blend, source->color_write)});
    created->setCullMode(source->cull == GpuCull::Back        ? QRhiGraphicsPipeline::Back
                             : source->cull == GpuCull::Front ? QRhiGraphicsPipeline::Front
                                                              : QRhiGraphicsPipeline::None);
    created->setFrontFace(source->front_counter_clockwise ? QRhiGraphicsPipeline::CCW : QRhiGraphicsPipeline::CW);
    created->setDepthTest(source->depth_test);
    created->setDepthWrite(source->depth_write);
    created->setDepthOp(toRhi(source->depth_compare));
    created->setFlags(QRhiGraphicsPipeline::UsesScissor);
    created->setSampleCount(target->sampleCount());
    created->setShaderResourceBindings(layout);
    created->setRenderPassDescriptor(target->renderPassDescriptor());
    if (!created->create()) {
        mne::Log::warning("RhiFrameRenderer: cannot create the pipeline of '{}'", source->shader);
        return nullptr;
    }
    cached.pipeline = std::move(created);
    cached.renderPassFormat = format;
    cached.sampleCount = target->sampleCount();
    return cached.pipeline.get();
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RhiFrameRenderer::RhiFrameRenderer()
: m_pImpl(std::make_unique<Impl>())
{
}

//=============================================================================================================

RhiFrameRenderer::~RhiFrameRenderer() = default;

//=============================================================================================================

void RhiFrameRenderer::addShaderPrefix(const std::string& prefix)
{
    if (std::find(m_pImpl->prefixes.begin(), m_pImpl->prefixes.end(), prefix) == m_pImpl->prefixes.end()) {
        m_pImpl->prefixes.push_back(prefix);
    }
}

//=============================================================================================================

void RhiFrameRenderer::setFont(const QFont& font)
{
    if (font != m_pImpl->font) {
        m_pImpl->font = font;
        // Figure textures are painted again with the new font
        for (auto& [handle, texture] : m_pImpl->textures) {
            texture.revision = 0;
        }
    }
}

//=============================================================================================================

GpuFrameTarget RhiFrameRenderer::frameTarget(QRhi* rhi, QRhiRenderTarget* target, double devicePixelRatio)
{
    GpuFrameTarget result;
    if (target) {
        result.pixel_width = target->pixelSize().width();
        result.pixel_height = target->pixelSize().height();
    }
    result.device_pixel_ratio = devicePixelRatio;
    if (rhi) {
        const QMatrix4x4 correction = rhi->clipSpaceCorrMatrix();
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                result.clip_correction(row, column) = correction(row, column);
            }
        }
    }
    return result;
}

//=============================================================================================================

void RhiFrameRenderer::render(QRhi* rhi, QRhiRenderTarget* target, QRhiCommandBuffer* cb, const GpuFrame& frame, const GpuResources& resources)
{
    Impl& d = *m_pImpl;
    if (d.rhi != rhi) {
        d.release();
        d.rhi = rhi;
    }
    if (!d.linearSampler) {
        d.linearSampler.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None, QRhiSampler::ClampToEdge,
                                              QRhiSampler::ClampToEdge));
        d.linearSampler->create();
        d.nearestSampler.reset(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None, QRhiSampler::ClampToEdge,
                                               QRhiSampler::ClampToEdge));
        d.nearestSampler->create();
        d.mipmapSampler.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::ClampToEdge,
                                              QRhiSampler::ClampToEdge));
        d.mipmapSampler->create();
    }
    QRhiResourceUpdateBatch* batch = rhi->nextResourceUpdateBatch();

    // The uniform blocks of all draws in one dynamic buffer, each at an aligned offset
    const auto align = [rhi](quint32 size) {
        const auto alignment = static_cast<quint32>(rhi->ubufAlignment());
        return (size + alignment - 1) & ~(alignment - 1);
    };
    quint32 uniformBytes = 0;
    for (const GpuPass& pass : frame.passes) {
        for (const GpuDraw& draw : pass.draws) {
            for (const GpuUniformBlock& block : draw.uniforms) {
                uniformBytes += align(std::max<quint32>(static_cast<quint32>(block.data.size()), 16));
            }
        }
    }
    if (uniformBytes > 0 && (!d.uniforms || d.uniforms->size() < uniformBytes)) {
        d.uniforms.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, std::max(uniformBytes * 2, 4096u)));
        if (!d.uniforms->create()) {
            d.uniforms.reset();
        }
        d.uniformsSerial = ++d.serial;
    }

    // Shader resource bindings unused in the last frame refer to replaced buffers or textures
    std::erase_if(d.bindings, [](const auto& entry) {
        return !entry.second.used;
    });
    for (auto& [key, entry] : d.bindings) {
        entry.used = false;
    }

    // Upload everything and resolve the QRhi objects of the draws before the passes start
    struct Prepared
    {
        const GpuDraw* draw = nullptr;
        QRhiGraphicsPipeline* pipeline = nullptr;
        QRhiShaderResourceBindings* srb = nullptr;
        std::vector<QRhiCommandBuffer::DynamicOffset> offsets;
        std::vector<QRhiCommandBuffer::VertexInput> inputs;
        QRhiBuffer* indices = nullptr;
    };
    std::vector<std::vector<Prepared>> passes(frame.passes.size());
    quint32 uniformOffset = 0;
    for (std::size_t p = 0; p < frame.passes.size(); ++p) {
        for (const GpuDraw& draw : frame.passes[p].draws) {
            Prepared prepared;
            prepared.draw = &draw;
            std::vector<quint32> blockSizes;
            for (const GpuUniformBlock& block : draw.uniforms) {
                const auto size = static_cast<quint32>(block.data.size());
                blockSizes.push_back(std::max<quint32>(size, 16));
                if (d.uniforms && size > 0) {
                    batch->updateDynamicBuffer(d.uniforms.get(), uniformOffset, size, block.data.data());
                }
                prepared.offsets.push_back({block.binding, uniformOffset});
                uniformOffset += align(std::max<quint32>(size, 16));
            }
            bool complete = !draw.uniforms.empty() ? d.uniforms != nullptr : true;
            for (const GpuHandle handle : draw.vertex_buffers) {
                QRhiBuffer* vertexBuffer = d.buffer(handle, resources, batch);
                complete = complete && vertexBuffer;
                prepared.inputs.emplace_back(vertexBuffer, 0);
            }
            if (draw.index_buffer != 0) {
                prepared.indices = d.buffer(draw.index_buffer, resources, batch);
                complete = complete && prepared.indices;
            }
            prepared.srb = complete ? d.shaderBindings(draw, blockSizes, resources, batch) : nullptr;
            prepared.pipeline = prepared.srb ? d.pipeline(draw.pipeline, resources, target, prepared.srb) : nullptr;
            if (prepared.pipeline) {
                passes[p].push_back(std::move(prepared));
            }
        }
    }

    const QSize size = target->pixelSize();
    bool inPass = false;
    for (std::size_t p = 0; p < frame.passes.size(); ++p) {
        const GpuPass& pass = frame.passes[p];
        if (!inPass || pass.clear_color) {
            if (inPass) {
                cb->endPass();
            }
            cb->beginPass(target, toQColor(pass.clear_color.value_or(Color::transparent())), {1.f, 0}, batch);
            batch = nullptr;
            inPass = true;
        }
        for (const Prepared& prepared : passes[p]) {
            const GpuDraw& draw = *prepared.draw;
            cb->setGraphicsPipeline(prepared.pipeline);
            const GpuRect viewport = draw.viewport.value_or(GpuRect{0, 0, size.width(), size.height()});
            // QRhi viewports and scissors have their origin at the bottom left
            cb->setViewport(QRhiViewport(static_cast<float>(viewport.x), static_cast<float>(size.height() - viewport.y - viewport.height),
                                         static_cast<float>(viewport.width), static_cast<float>(viewport.height)));
            const GpuRect scissor = draw.scissor.value_or(GpuRect{0, 0, size.width(), size.height()});
            cb->setScissor(QRhiScissor(scissor.x, size.height() - scissor.y - scissor.height, scissor.width, scissor.height));
            cb->setShaderResources(prepared.srb, static_cast<int>(prepared.offsets.size()), prepared.offsets.data());
            if (prepared.indices) {
                const GpuBuffer* indices = resources.buffer(draw.index_buffer);
                cb->setVertexInput(0, static_cast<int>(prepared.inputs.size()), prepared.inputs.data(), prepared.indices, 0,
                                   indices->usage == GpuBufferUsage::Index16 ? QRhiCommandBuffer::IndexUInt16 : QRhiCommandBuffer::IndexUInt32);
                cb->drawIndexed(draw.count, draw.instance_count, draw.first);
            } else {
                cb->setVertexInput(0, static_cast<int>(prepared.inputs.size()), prepared.inputs.data());
                cb->draw(draw.count, draw.instance_count, draw.first);
            }
        }
    }
    if (!inPass) {
        cb->beginPass(target, toQColor(Color::transparent()), {1.f, 0}, batch);
    }
    cb->endPass();
}

//=============================================================================================================

std::vector<std::string> RhiFrameRenderer::checkShaders(const GpuFrame& frame, const GpuResources& resources) const
{
    std::vector<std::string> problems;
    for (const GpuPass& pass : frame.passes) {
        for (const GpuDraw& draw : pass.draws) {
            const GpuPipeline* pipeline = resources.pipeline(draw.pipeline);
            if (!pipeline) {
                problems.emplace_back("draw without pipeline");
                continue;
            }
            const std::string& id = pipeline->shader;
            const std::pair<QShader, QShader>* stages = m_pImpl->shaderPair(pipeline->shader);
            if (!stages) {
                problems.push_back(std::format("{}: no shaders", id));
                continue;
            }
            const QShaderDescription vertex = stages->first.description();
            const QShaderDescription fragment = stages->second.description();

            // Uniform blocks of both stages by binding
            std::map<int, int> blocks;
            std::map<int, int> samplers;
            for (const QShaderDescription& description : {vertex, fragment}) {
                for (const QShaderDescription::UniformBlock& block : description.uniformBlocks()) {
                    blocks[block.binding] = block.size;
                }
                for (const QShaderDescription::InOutVariable& sampler : description.combinedImageSamplers()) {
                    samplers[sampler.binding] = 1;
                }
            }
            for (const GpuUniformBlock& block : draw.uniforms) {
                const auto found = blocks.find(block.binding);
                if (found == blocks.end()) {
                    problems.push_back(std::format("{}: no uniform block at binding {}", id, block.binding));
                } else if (found->second != static_cast<int>(block.data.size())) {
                    problems.push_back(std::format("{}: uniform block {} has {} bytes, the shader {}", id, block.binding, block.data.size(), found->second));
                }
                blocks.erase(block.binding);
            }
            for (const auto& [binding, size] : blocks) {
                problems.push_back(std::format("{}: uniform block {} not set", id, binding));
            }
            for (const GpuTextureBinding& texture : draw.textures) {
                if (samplers.erase(texture.binding) == 0) {
                    problems.push_back(std::format("{}: no sampler at binding {}", id, texture.binding));
                }
            }
            for (const auto& [binding, unused] : samplers) {
                problems.push_back(std::format("{}: sampler {} not set", id, binding));
            }

            // Vertex inputs
            std::map<int, int> inputs;
            for (const QShaderDescription::InOutVariable& input : vertex.inputVariables()) {
                // BATCHABLE shaders get an input for Qt Quick's batching that only Qt Quick feeds
                if (input.name.startsWith("_qt_")) {
                    continue;
                }
                const int components = input.type == QShaderDescription::Float ? 1
                    : input.type == QShaderDescription::Vec2                   ? 2
                    : input.type == QShaderDescription::Vec3                   ? 3
                    : input.type == QShaderDescription::Vec4                   ? 4
                                                                               : 0;
                inputs[input.location] = components;
            }
            for (const GpuVertexAttribute& attribute : pipeline->attributes) {
                const int components = attribute.format == GpuVertexFormat::Float ? 1
                    : attribute.format == GpuVertexFormat::Float2                 ? 2
                    : attribute.format == GpuVertexFormat::Float3                 ? 3
                                                                                  : 4;
                const auto found = inputs.find(attribute.location);
                if (found == inputs.end()) {
                    problems.push_back(std::format("{}: no vertex input at location {}", id, attribute.location));
                } else if (found->second != components) {
                    problems.push_back(std::format("{}: vertex input {} has {} components, the attribute {}", id, attribute.location, found->second, components));
                }
                inputs.erase(attribute.location);
            }
            for (const auto& [location, components] : inputs) {
                problems.push_back(std::format("{}: vertex input {} not fed", id, location));
            }
        }
    }
    std::sort(problems.begin(), problems.end());
    problems.erase(std::unique(problems.begin(), problems.end()), problems.end());
    return problems;
}

//=============================================================================================================

QImage RhiFrameRenderer::textureImage(GpuHandle texture) const
{
    const auto found = m_pImpl->textures.find(texture);
    return found != m_pImpl->textures.end() ? found->second.image : QImage();
}

//=============================================================================================================

void RhiFrameRenderer::releaseResources()
{
    m_pImpl->release();
}
