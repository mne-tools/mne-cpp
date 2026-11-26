//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brain_view_overlay.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of BrainViewOverlay.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brain_view_overlay.h"

#include "camera.h"
#include "../figures/canvas.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

namespace
{

std::vector<std::string_view> split_lines(std::string_view text)
{
    std::vector<std::string_view> lines;
    std::size_t start = 0;
    while (true) {
        const std::size_t end = text.find('\n', start);
        lines.push_back(text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start));
        if (end == std::string_view::npos) {
            return lines;
        }
        start = end + 1;
    }
}

Font label_font(const std::string& family, double pixel_size, bool bold)
{
    Font font;
    font.family = family;
    font.pixel_size = pixel_size;
    font.bold = bold;
    return font;
}

} // namespace

//=============================================================================================================

BrainViewOverlay::BrainViewOverlay(int viewport_count)
{
    m_fps.text = "FPS: --.-\nVertices: 0";
    set_viewport_count(viewport_count);
}

//=============================================================================================================

void BrainViewOverlay::set_viewport_count(int count)
{
    const auto size = static_cast<std::size_t>(std::max(0, count));
    const std::size_t old_size = m_names.size();
    m_names.resize(size);
    m_infos.resize(size);
    for (std::size_t i = old_size; i < size; ++i) {
        m_names[i].text = std::string(view_preset_name(static_cast<int>(i % view_preset_count)));
    }
}

//=============================================================================================================

BrainViewOverlay::Label* BrainViewOverlay::item_mut(Item item, int viewport)
{
    switch (item) {
        case Item::FpsLabel:
            return &m_fps;
        case Item::SingleViewInfoLabel:
            return &m_single_info;
        case Item::RegionLabel:
            return &m_region;
        case Item::ViewportNameLabel:
            return (viewport >= 0 && viewport < viewport_count()) ? &m_names[static_cast<std::size_t>(viewport)] : nullptr;
        case Item::ViewportInfoLabel:
            return (viewport >= 0 && viewport < viewport_count()) ? &m_infos[static_cast<std::size_t>(viewport)] : nullptr;
        case Item::VerticalSeparator:
            return &m_vertical;
        case Item::HorizontalSeparator:
            return &m_horizontal;
    }
    return nullptr;
}

//=============================================================================================================

const BrainViewOverlay::Label* BrainViewOverlay::item(Item item, int viewport) const
{
    return const_cast<BrainViewOverlay*>(this)->item_mut(item, viewport);
}

//=============================================================================================================

bool BrainViewOverlay::set_text(Item item, std::string text, int viewport)
{
    Label* label = item_mut(item, viewport);
    if (!label || label->text == text) {
        return false;
    }
    label->text = std::move(text);
    return true;
}

//=============================================================================================================

void BrainViewOverlay::set_region(const std::string& name)
{
    if (!name.empty()) {
        m_region.text = name;
    }
    m_region.visible = !name.empty();
}

//=============================================================================================================

BrainViewOverlay::LabelStyle BrainViewOverlay::style(Item item, bool selected) const
{
    LabelStyle style;
    switch (item) {
        case Item::FpsLabel:
            style.font = label_font(m_fonts.monospace, 13, true);
            style.pad_x = 5;
            style.pad_y = 5;
            style.align = Align::Right | Align::Top;
            break;
        case Item::SingleViewInfoLabel:
        case Item::ViewportInfoLabel:
            style.font = label_font(m_fonts.monospace, 10, false);
            style.background = {0, 0, 0, 110};
            style.radius = 3;
            style.pad_x = 4;
            style.pad_y = 2;
            style.align = Align::Left | Align::Top;
            break;
        case Item::RegionLabel:
            style.font = label_font(m_fonts.sans_serif, 16, true);
            style.pad_x = 5;
            style.pad_y = 5;
            break;
        case Item::ViewportNameLabel:
            if (selected) {
                style.font = label_font(m_fonts.sans_serif, 13, true);
                style.color = {0xFF, 0xD5, 0x4F, 255};
                style.background = {255, 213, 79, 40};
                style.border = {0xFF, 0xD5, 0x4F, 255};
                style.border_px = 1;
                style.radius = 3;
                style.pad_x = 6;
                style.pad_y = 2;
            } else {
                style.font = label_font(m_fonts.sans_serif, 12, true);
                style.pad_x = 4;
                style.pad_y = 2;
            }
            break;
        case Item::VerticalSeparator:
        case Item::HorizontalSeparator:
            break;
    }
    return style;
}

//=============================================================================================================

PixelSize BrainViewOverlay::label_size(const LabelStyle& style, std::string_view text, const TextMetrics& metrics)
{
    double width = 0.0;
    const std::vector<std::string_view> lines = split_lines(text);
    for (std::string_view line : lines) {
        width = std::max(width, metrics.advance(style.font, line));
    }
    const double height = static_cast<double>(lines.size()) * metrics.height(style.font);
    const int frame = style.border_px;
    return {static_cast<int>(std::ceil(width)) + 2 * (style.pad_x + frame),
            static_cast<int>(std::ceil(height)) + 2 * (style.pad_y + frame)};
}

//=============================================================================================================

void BrainViewOverlay::update_layout(const LayoutInput& input, const TextMetrics& metrics)
{
    const PaneLayout default_layout;
    const PaneLayout& layout = input.layout ? *input.layout : default_layout;
    const PixelSize area = input.size;

    m_fps.visible = input.info_visible;
    const PixelSize fps_size = label_size(style(Item::FpsLabel), m_fps.text, metrics);
    m_fps.geometry = {area.width - fps_size.width - 10, area.height - fps_size.height - 2, fps_size.width, fps_size.height};

    m_single_info.visible = !input.multi_view && input.info_visible;
    if (m_single_info.visible) {
        const PixelSize info_size = label_size(style(Item::SingleViewInfoLabel), m_single_info.text, metrics);
        m_single_info.geometry = {area.width - info_size.width - 8, 8, info_size.width, info_size.height};
    }

    // The region label keeps a fixed size; long names are clipped
    m_region.geometry = {10, input.multi_view ? 38 : 10, 300, 30};

    for (std::size_t i = 0; i < m_names.size(); ++i) {
        m_names[i].visible = false;
        m_names[i].selected = input.multi_view && input.edit_target == static_cast<int>(i);
        m_infos[i].visible = false;
    }
    m_vertical.visible = false;
    m_horizontal.visible = false;

    const int pane_count = static_cast<int>(input.viewports.size());
    if (!input.multi_view) {
        return;
    }

    if (pane_count > 1) {
        const PaneLayout::Separators separators = layout.separator_geometries(pane_count, area);
        m_vertical.geometry = separators.vertical;
        m_vertical.visible = !separators.vertical.empty();
        m_horizontal.geometry = separators.horizontal;
        m_horizontal.visible = !separators.horizontal.empty();
    }

    for (int slot = 0; slot < pane_count; ++slot) {
        const int vp = input.viewports[static_cast<std::size_t>(slot)];
        if (vp < 0 || vp >= viewport_count()) {
            continue;
        }
        Label& name = m_names[static_cast<std::size_t>(vp)];
        Label& info = m_infos[static_cast<std::size_t>(vp)];

        const int preset = vp < static_cast<int>(input.presets.size()) ? input.presets[static_cast<std::size_t>(vp)] : 0;
        name.text = std::string(view_preset_name(clamp_view_preset(preset)));

        const PixelRect pane = layout.slot_rect(slot, pane_count, area);
        const PixelSize name_size = label_size(style(Item::ViewportNameLabel, name.selected), name.text, metrics);
        name.geometry = {pane.x + 8, pane.y + 8, name_size.width, name_size.height};
        name.visible = true;

        const PixelSize info_size = label_size(style(Item::ViewportInfoLabel), info.text, metrics);
        info.geometry = {pane.x + pane.width - info_size.width - 8, pane.y + 8, info_size.width, info_size.height};
        info.visible = input.info_visible;
    }
}

//=============================================================================================================

void BrainViewOverlay::draw_label(Canvas& canvas, const LabelStyle& style, const Label& label, const TextMetrics& metrics) const
{
    const Rect box = label.geometry.to_rect();
    canvas.save();
    canvas.clip_rect(box);
    if (style.background.a > 0 || style.border_px > 0) {
        const double inset = style.border_px * 0.5;
        canvas.set_pen(style.border_px > 0 ? Pen{style.border, static_cast<double>(style.border_px)} : Pen::none());
        canvas.set_brush(Brush::solid(style.background));
        canvas.draw_rounded_rect(box.adjusted(inset, inset, -inset, -inset), style.radius, style.radius);
    }
    const double inset_x = style.pad_x + style.border_px;
    const double inset_y = style.pad_y + style.border_px;
    const Rect text_box = box.adjusted(inset_x, inset_y, -inset_x, -inset_y);
    canvas.set_pen(style.color);
    canvas.set_font(style.font);

    const std::vector<std::string_view> lines = split_lines(label.text);
    if (lines.size() == 1) {
        canvas.draw_text(text_box, style.align, label.text);
    } else {
        // One text per line keeps the display list free of line breaks
        const double line_height = metrics.height(style.font);
        const Align horizontal = has_flag(style.align, Align::Right) ? Align::Right
            : has_flag(style.align, Align::HCenter)                  ? Align::HCenter
                                                                     : Align::Left;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            const Rect line_box{text_box.x, text_box.y + static_cast<double>(i) * line_height, text_box.width, line_height};
            canvas.draw_text(line_box, horizontal | Align::Top, lines[i]);
        }
    }
    canvas.restore();
}

//=============================================================================================================

DisplayList BrainViewOverlay::draw(int width, int height, const TextMetrics& metrics) const
{
    Canvas canvas(width, height);
    canvas.set_antialiasing(true);

    // Separators below the labels
    for (const Label* separator : {&m_vertical, &m_horizontal}) {
        if (separator->visible) {
            canvas.fill_rect(separator->geometry.to_rect(), Brush::solid(m_separator_color));
        }
    }
    if (m_fps.visible) {
        draw_label(canvas, style(Item::FpsLabel), m_fps, metrics);
    }
    if (m_single_info.visible) {
        draw_label(canvas, style(Item::SingleViewInfoLabel), m_single_info, metrics);
    }
    if (m_region.visible) {
        draw_label(canvas, style(Item::RegionLabel), m_region, metrics);
    }
    for (std::size_t i = 0; i < m_names.size(); ++i) {
        if (m_names[i].visible) {
            draw_label(canvas, style(Item::ViewportNameLabel, m_names[i].selected), m_names[i], metrics);
        }
        if (m_infos[i].visible) {
            draw_label(canvas, style(Item::ViewportInfoLabel), m_infos[i], metrics);
        }
    }
    return canvas.take();
}

} // namespace mne::viz
