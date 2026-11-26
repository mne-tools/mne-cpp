//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     gpu_frame.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    GpuResources definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gpu_frame.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

namespace
{

/** The resource of a handle if it has type T, else nullptr; const if @p resources is. */
template<typename T, typename Resources>
auto find(Resources& resources, GpuHandle handle) -> decltype(std::get_if<T>(resources.data()))
{
    if (handle == 0 || handle > resources.size()) {
        return nullptr;
    }
    return std::get_if<T>(&resources[handle - 1]);
}

} // namespace

//=============================================================================================================

GpuHandle GpuResources::create_buffer(GpuBufferUsage usage)
{
    GpuBuffer buffer;
    buffer.usage = usage;
    buffer.revision = ++m_revision;
    m_resources.emplace_back(std::move(buffer));
    return static_cast<GpuHandle>(m_resources.size());
}

//=============================================================================================================

void GpuResources::set_buffer_data(GpuHandle buffer, std::span<const std::byte> bytes)
{
    if (GpuBuffer* target = find<GpuBuffer>(m_resources, buffer)) {
        target->data.assign(bytes.begin(), bytes.end());
        target->revision = ++m_revision;
    }
}

//=============================================================================================================

GpuHandle GpuResources::create_texture(GpuTexture texture)
{
    texture.revision = ++m_revision;
    m_resources.emplace_back(std::move(texture));
    return static_cast<GpuHandle>(m_resources.size());
}

//=============================================================================================================

void GpuResources::set_texture(GpuHandle texture, std::variant<Image, DisplayList> content, double scale)
{
    if (GpuTexture* target = find<GpuTexture>(m_resources, texture)) {
        target->content = std::move(content);
        target->scale = scale;
        target->revision = ++m_revision;
    }
}

//=============================================================================================================

GpuHandle GpuResources::create_pipeline(GpuPipeline pipeline)
{
    m_resources.emplace_back(std::move(pipeline));
    return static_cast<GpuHandle>(m_resources.size());
}

//=============================================================================================================

void GpuResources::release(GpuHandle handle)
{
    if (handle != 0 && handle <= m_resources.size()) {
        m_resources[handle - 1] = std::monostate{};
    }
}

//=============================================================================================================

const GpuBuffer* GpuResources::buffer(GpuHandle handle) const
{
    return find<GpuBuffer>(m_resources, handle);
}

//=============================================================================================================

const GpuTexture* GpuResources::texture(GpuHandle handle) const
{
    return find<GpuTexture>(m_resources, handle);
}

//=============================================================================================================

const GpuPipeline* GpuResources::pipeline(GpuHandle handle) const
{
    return find<GpuPipeline>(m_resources, handle);
}
