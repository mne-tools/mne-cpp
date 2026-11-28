//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     form_view.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of the FormView.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "form_view.h"
#include "panel.h"
#include "../models/item_list.h"
#include "../figures/canvas.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <span>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

//=============================================================================================================
// DEFINE STATIC FUNCTIONS
//=============================================================================================================

namespace
{

// Full-target quad as a strip: (x, y) in clip space, (u, v) with v = 0 at the top of the texture
constexpr std::array<float, 16> kQuad = {-1.f, 1.f, 0.f, 0.f, -1.f, -1.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f, 1.f, -1.f, 1.f, 1.f};

constexpr double kDisabledOpacity = 0.4;
constexpr double kWheelNotch = 120.0;
constexpr double kScrollPerNotch = 48.0;

bool is_text_kind(SettingField::Kind kind)
{
    return kind == SettingField::Kind::Text || kind == SettingField::Kind::Path || kind == SettingField::Kind::Directory;
}

bool is_number_kind(SettingField::Kind kind)
{
    return kind == SettingField::Kind::Int || kind == SettingField::Kind::Double;
}

std::string utf8(char32_t c)
{
    std::string out;
    if (c < 0x80) {
        out += static_cast<char>(c);
    } else if (c < 0x800) {
        out += static_cast<char>(0xC0 | (c >> 6));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        out += static_cast<char>(0xE0 | (c >> 12));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (c >> 18));
        out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    }
    return out;
}

void pop_code_point(std::string& text)
{
    while (!text.empty()) {
        const auto byte = static_cast<unsigned char>(text.back());
        text.pop_back();
        if ((byte & 0xC0) != 0x80) {
            break;
        }
    }
}

std::optional<Color> parse_color(const std::string& text)
{
    if ((text.size() != 7 && text.size() != 9) || text.front() != '#') {
        return std::nullopt;
    }
    const auto component = [&text](std::size_t at) {
        return static_cast<std::uint8_t>(std::stoi(text.substr(at, 2), nullptr, 16));
    };
    Color color{component(1), component(3), component(5), 255};
    if (text.size() == 9) {
        color.a = component(7);
    }
    return color;
}

constexpr double kCheckBoxSize = 18.0;
constexpr double kControlRadius = 4.0;
constexpr double kRowInset = 16.0;

std::string upper(std::string text)
{
    for (char& c : text) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }
    return text;
}

Color tone_color(const Theme& theme, SettingField::Tone tone)
{
    switch (tone) {
        case SettingField::Tone::Positive:
            return theme.positive;
        case SettingField::Tone::Warning:
            return theme.warning;
        case SettingField::Tone::Negative:
            return theme.negative;
        case SettingField::Tone::Normal:
            break;
    }
    return theme.muted_text;
}

Pen line_pen(Color color, double width)
{
    Pen pen;
    pen.color = color;
    pen.width = width;
    pen.cap = LineCap::Round;
    pen.join = LineJoin::Round;
    return pen;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FormView::FormView(SettingsForm& form, Theme theme)
: m_form(form)
, m_theme(std::move(theme))
{
    m_listener = m_form.add_listener([this](const std::string&, const std::optional<SettingValue>&) {
        // Values may show or enable other fields
        invalidate();
    });
    m_structureListener = m_form.add_structure_listener([this](const std::string& key) {
        if (key == m_focus && !m_form.field(key)) {
            m_focus.clear();
        }
        invalidate();
    });

    GpuPipeline overlay;
    overlay.shader = "screen_overlay";
    overlay.bindings = {{4 * sizeof(float), false}};
    overlay.attributes = {{0, 0, GpuVertexFormat::Float2, 0}, {1, 0, GpuVertexFormat::Float2, 2 * sizeof(float)}};
    overlay.topology = GpuTopology::TriangleStrip;
    overlay.blend = GpuBlend::Alpha;
    m_pipeline = m_resources.create_pipeline(std::move(overlay));
    m_quad = m_resources.create_buffer(GpuBufferUsage::Vertex);
    m_resources.set_buffer_values(m_quad, std::span<const float>(kQuad));
}

//=============================================================================================================

FormView::FormView(Panel& panel, Theme theme)
: FormView(panel.form(), std::move(theme))
{
    m_panel = &panel;
    for (const auto& [key, list] : panel.lists()) {
        const int id = list->add_listener([this](ItemList::Change change) {
            if (change == ItemList::Change::Items) {
                invalidate();
            } else {
                update();
            }
        });
        m_listListeners.emplace_back(list.get(), id);
    }
}

//=============================================================================================================

FormView::~FormView()
{
    for (const auto& [list, id] : m_listListeners) {
        list->remove_listener(id);
    }
    m_form.remove_listener(m_listener);
    m_form.remove_listener(m_structureListener);
}

//=============================================================================================================

void FormView::set_theme(Theme theme)
{
    m_theme = std::move(theme);
    invalidate();
}

//=============================================================================================================

void FormView::set_text_metrics(const TextMetrics* metrics)
{
    m_metrics = metrics ? metrics : &m_approximateMetrics;
    invalidate();
}

//=============================================================================================================

void FormView::set_view_size(double width, double height)
{
    m_width = std::max(0.0, width);
    m_height = std::max(0.0, height);
    invalidate();
}

//=============================================================================================================

double FormView::content_height() const
{
    layout();
    return m_contentHeight;
}

//=============================================================================================================

void FormView::set_scroll_offset(double offset)
{
    const double clamped = clamped_scroll(offset);
    if (clamped != m_scroll) {
        m_scroll = clamped;
        update();
    }
}

//=============================================================================================================

double FormView::clamped_scroll(double offset) const
{
    return std::clamp(offset, 0.0, std::max(0.0, content_height() - m_height));
}

//=============================================================================================================

std::optional<Rect> FormView::editor_rect(const std::string& key, Part part) const
{
    const Row* found = row(key);
    if (!found) {
        return std::nullopt;
    }
    const Rect rect = part == Part::Decrement ? found->decrement : part == Part::Increment ? found->increment
                                                                                           : found->editor;
    if (rect.empty()) {
        return std::nullopt;
    }
    return Rect{rect.x, rect.y - m_scroll, rect.width, rect.height};
}

//=============================================================================================================

void FormView::invalidate()
{
    m_dirty = true;
    m_scroll = std::clamp(m_scroll, 0.0, std::max(0.0, content_height() - m_height));
    update();
}

//=============================================================================================================

void FormView::layout() const
{
    if (!m_dirty) {
        return;
    }
    m_dirty = false;
    m_rows.clear();

    const Theme& t = m_theme;
    const double width = std::max(m_width - 2.0 * t.padding, 1.0);
    const double inset = kRowInset;
    const double editorHeight = t.control_height;
    double y = t.padding;

    if (!m_form.title().empty()) {
        Row title;
        title.type = Row::Type::Title;
        title.key = m_form.title();
        title.rect = Rect{t.padding, y, width, std::round(t.title_font_size * 1.6)};
        m_rows.push_back(title);
        y += title.rect.height + t.spacing;
    }

    bool anyCard = false;
    for (const std::string& group : m_form.groups()) {
        std::vector<const SettingField*> fields;
        for (const SettingField& f : m_form.fields()) {
            if (f.group == group && m_form.is_visible(f.key)) {
                fields.push_back(&f);
            }
        }
        if (fields.empty()) {
            continue;
        }
        if (anyCard) {
            y += 2.0 * t.spacing;
        }
        anyCard = true;
        if (!group.empty()) {
            Row header;
            header.type = Row::Type::Group;
            header.key = group;
            header.rect = Rect{t.padding + 2.0, y, width - 2.0, std::round(t.small_font_size * 2.3)};
            m_rows.push_back(header);
            y += header.rect.height;
        }
        Row card;
        card.type = Row::Type::Card;
        card.rect = Rect{t.padding, y, width, 0.0};
        const std::size_t cardIndex = m_rows.size();
        m_rows.push_back(card);

        for (const SettingField* f : fields) {
            Row r;
            r.key = f->key;
            r.rect = Rect{t.padding, y, width, t.row_height};
            // An item list: its label above a box of its visible rows
            if (const ItemList* items = f->kind == SettingField::Kind::Custom ? list_of(f->key) : nullptr) {
                const double labelHeight = f->label.empty() ? inset : std::round(t.row_height * 0.75);
                const double box = items->visible_rows() * list_row_height() + 2.0;
                r.rect.height = labelHeight + box + inset;
                r.editor = Rect{r.rect.x + inset, y + labelHeight, width - 2.0 * inset, box};
                m_rows.push_back(r);
                y += r.rect.height;
                continue;
            }
            const double right = r.rect.right() - inset;
            const double inner = width - 2.0 * inset;
            const double editorWidth = std::min(t.editor_width, std::round(inner * 0.55));
            const double top = y + (t.row_height - editorHeight) / 2.0;
            switch (f->kind) {
                case SettingField::Kind::Bool: {
                    const double h = kCheckBoxSize;
                    r.editor = Rect{right - h, y + std::round((t.row_height - h) / 2.0), h, h};
                    break;
                }
                case SettingField::Kind::Int:
                case SettingField::Kind::Double:
                case SettingField::Kind::Choice:
                    r.editor = Rect{right - editorWidth, top, editorWidth, editorHeight};
                    r.decrement = Rect{r.editor.x, top, editorHeight, editorHeight};
                    r.increment = Rect{r.editor.right() - editorHeight, top, editorHeight, editorHeight};
                    break;
                case SettingField::Kind::Text:
                case SettingField::Kind::Path:
                case SettingField::Kind::Directory:
                    r.editor = Rect{right - editorWidth, top, editorWidth, editorHeight};
                    break;
                case SettingField::Kind::Action: {
                    // As wide as its text, like a native button
                    const double w = std::clamp(m_metrics->advance(t.font(t.font_size, true), f->label) + 4.0 * t.spacing, 96.0, inner);
                    r.editor = Rect{right - w, top, w, editorHeight};
                    break;
                }
                case SettingField::Kind::Color:
                    r.editor = Rect{right - 36.0, y + std::round((t.row_height - 20.0) / 2.0), 36.0, 20.0};
                    break;
                case SettingField::Kind::Info: {
                    const double labelWidth = f->label.empty() ? 0.0 : m_metrics->advance(t.font(t.font_size), f->label) + 2.0 * t.spacing;
                    const double w = std::max(editorWidth, inner - labelWidth);
                    r.editor = Rect{right - w, top, w, editorHeight};
                    break;
                }
                case SettingField::Kind::Custom:
                    break;
            }
            m_rows.push_back(r);
            y += t.row_height;
        }
        m_rows.back().last = true;
        m_rows[cardIndex].rect.height = y - m_rows[cardIndex].rect.y;
    }
    m_contentHeight = y + t.padding;
}

//=============================================================================================================

const FormView::Row* FormView::row(const std::string& key) const
{
    layout();
    for (const Row& r : m_rows) {
        if (r.type == Row::Type::Field && r.key == key) {
            return &r;
        }
    }
    return nullptr;
}

//=============================================================================================================

const FormView::Row* FormView::hit(double x, double y, Part* part) const
{
    layout();
    const Point p{x, y + m_scroll};
    for (const Row& r : m_rows) {
        if (r.type != Row::Type::Field || !r.rect.contains(p)) {
            continue;
        }
        const SettingField* f = m_form.field(r.key);
        if (!f) {
            return nullptr;
        }
        *part = Part::Editor;
        if (!r.decrement.empty() && r.decrement.contains(p)) {
            *part = Part::Decrement;
            return &r;
        }
        if (!r.increment.empty() && r.increment.contains(p)) {
            *part = Part::Increment;
            return &r;
        }
        // A switch also toggles from its label
        if (f->kind == SettingField::Kind::Bool || (!r.editor.empty() && r.editor.contains(p))) {
            return &r;
        }
        return nullptr;
    }
    return nullptr;
}

//=============================================================================================================

void FormView::resize(const SurfaceSize& size)
{
    set_view_size(size.width, size.height);
}

//=============================================================================================================

bool FormView::handle_pointer(const PointerEvent& event)
{
    if (event.button != MouseButton::Left) {
        return false;
    }
    if (event.type == PointerEvent::Type::Press || event.type == PointerEvent::Type::DoubleClick) {
        Part part = Part::Editor;
        const Row* r = hit(event.x, event.y, &part);
        const SettingField* f = r ? m_form.field(r->key) : nullptr;
        if (!f || !m_form.is_enabled(f->key)) {
            commit();
            m_pressedKey.clear();
            m_listFocus.clear();
            return f != nullptr;
        }
        if (ItemList* items = f->kind == SettingField::Kind::Custom ? list_of(f->key) : nullptr) {
            commit();
            m_pressedKey.clear();
            m_listFocus = f->key;
            int clicked = -1;
            for (int i = 0; i < items->count(); ++i) {
                if (const std::optional<Rect> item = item_rect(f->key, i); item && item->contains({event.x, event.y})) {
                    clicked = i;
                }
            }
            if (clicked >= 0) {
                items->click(clicked, has_flag(event.modifiers, Modifier::Ctrl) || has_flag(event.modifiers, Modifier::Meta));
            }
            return true;
        }
        m_listFocus.clear();
        if (is_text_kind(f->kind)) {
            focus(f->key);
            m_pressedKey.clear();
            return true;
        }
        commit();
        m_pressedKey = r->key;
        m_pressedPart = part;
        // Steppers repeat on fast clicks
        if (event.type == PointerEvent::Type::DoubleClick && part != Part::Editor) {
            const Row target = *r;
            click(target, part);
            m_pressedKey.clear();
        }
        return true;
    }
    if (event.type == PointerEvent::Type::Release && !m_pressedKey.empty()) {
        Part part = Part::Editor;
        const Row* r = hit(event.x, event.y, &part);
        const std::string pressed = std::exchange(m_pressedKey, std::string());
        if (r && r->key == pressed && part == m_pressedPart && m_form.is_enabled(pressed)) {
            const Row target = *r;
            click(target, part);
        }
        return true;
    }
    return false;
}

//=============================================================================================================

void FormView::click(const Row& r, Part part)
{
    const SettingField* f = m_form.field(r.key);
    if (!f) {
        return;
    }
    const int direction = part == Part::Decrement ? -1 : 1;
    switch (f->kind) {
        case SettingField::Kind::Bool:
            m_form.set_value(f->key, !m_form.get<bool>(f->key));
            break;
        case SettingField::Kind::Int:
        case SettingField::Kind::Double:
            if (part != Part::Editor) {
                step(*f, direction);
            }
            break;
        case SettingField::Kind::Choice:
            step(*f, direction);
            break;
        case SettingField::Kind::Action:
            m_form.trigger(f->key);
            break;
        default:
            break;
    }
}

//=============================================================================================================

void FormView::step(const SettingField& field, int steps)
{
    if (field.kind == SettingField::Kind::Int) {
        const int stride = std::max(1, static_cast<int>(std::lround(field.step)));
        m_form.set_value(field.key, m_form.get<int>(field.key) + steps * stride);
    } else if (field.kind == SettingField::Kind::Double) {
        const double scale = std::pow(10.0, std::clamp(field.decimals, 0, 12));
        const double value = m_form.get<double>(field.key) + steps * field.step;
        m_form.set_value(field.key, std::round(value * scale) / scale);
    } else if (field.kind == SettingField::Kind::Choice && !field.choices.empty()) {
        const std::string current = m_form.get<std::string>(field.key);
        const auto count = static_cast<int>(field.choices.size());
        if (count <= 0) {
            return;
        }
        int index = 0;
        for (int i = 0; i < count; ++i) {
            if (field.choices[static_cast<std::size_t>(i)].value == current) {
                index = i;
            }
        }
        index = ((index + steps) % count + count) % count;
        m_form.set_value(field.key, field.choices[static_cast<std::size_t>(index)].value);
    }
}

//=============================================================================================================

bool FormView::handle_wheel(const WheelEvent& event)
{
    Part part = Part::Editor;
    const Row* r = hit(event.x, event.y, &part);
    const SettingField* f = r ? m_form.field(r->key) : nullptr;
    if (f && f->kind == SettingField::Kind::Custom && list_of(f->key)) {
        const double before = m_listScroll[f->key];
        scroll_list(f->key, before - event.angle_delta_y / kWheelNotch * 3.0 * list_row_height());
        if (m_listScroll[f->key] != before) {
            update();
            return true;
        }
        // At either end the wheel scrolls the form
    } else if (f && is_number_kind(f->kind) && m_form.is_enabled(f->key)) {
        const auto steps = static_cast<int>(event.angle_delta_y / kWheelNotch);
        const int direction = event.angle_delta_y > 0.0 ? 1 : event.angle_delta_y < 0.0 ? -1
                                                                                        : 0;
        step(*f, steps != 0 ? steps : direction);
        return direction != 0;
    }
    const double before = m_scroll;
    set_scroll_offset(m_scroll - event.angle_delta_y / kWheelNotch * kScrollPerNotch);
    return m_scroll != before;
}

//=============================================================================================================

bool FormView::handle_key(const KeyEvent& event)
{
    if (ItemList* items = m_focus.empty() ? list_of(m_listFocus) : nullptr) {
        switch (event.key) {
            case Key::Delete:
            case Key::Backspace:
                return items->remove_selected();
            case Key::Up:
            case Key::Down: {
                const int step = event.key == Key::Up ? -1 : 1;
                const int next = std::clamp(items->current() < 0 ? 0 : items->current() + step, 0, items->count() - 1);
                items->click(next);
                // Keep the current item in the box
                if (const Row* r = row_of_list(m_listFocus)) {
                    const double top = next * list_row_height();
                    const double scroll = m_listScroll[m_listFocus];
                    if (top < scroll) {
                        scroll_list(m_listFocus, top);
                    } else if (top + list_row_height() > scroll + r->editor.height - 2.0) {
                        scroll_list(m_listFocus, top + list_row_height() - (r->editor.height - 2.0));
                    }
                }
                update();
                return true;
            }
            default:
                break;
        }
    }
    if (!m_focus.empty()) {
        switch (event.key) {
            case Key::Character:
                if (!has_flag(event.modifiers, Modifier::Ctrl) && !has_flag(event.modifiers, Modifier::Meta) && event.character >= 0x20) {
                    type_text(utf8(event.character));
                }
                break;
            case Key::Backspace: {
                pop_code_point(m_editText);
                type_text({});
                break;
            }
            case Key::Return:
            case Key::Tab:
                commit();
                break;
            case Key::Escape:
                cancel();
                break;
            default:
                break;
        }
        return true;
    }
    const double before = m_scroll;
    switch (event.key) {
        case Key::Up:
            set_scroll_offset(m_scroll - kScrollPerNotch);
            break;
        case Key::Down:
            set_scroll_offset(m_scroll + kScrollPerNotch);
            break;
        case Key::PageUp:
            set_scroll_offset(m_scroll - std::max(m_height - kScrollPerNotch, kScrollPerNotch));
            break;
        case Key::PageDown:
            set_scroll_offset(m_scroll + std::max(m_height - kScrollPerNotch, kScrollPerNotch));
            break;
        case Key::Home:
            set_scroll_offset(0.0);
            break;
        case Key::End:
            set_scroll_offset(content_height());
            break;
        default:
            break;
    }
    return m_scroll != before;
}

//=============================================================================================================

void FormView::focus(const std::string& key)
{
    if (m_focus == key) {
        return;
    }
    commit();
    m_focus = key;
    m_editText = m_form.get<std::string>(key);
    m_editOriginal = m_editText;
    update();
}

//=============================================================================================================

void FormView::commit()
{
    if (m_focus.empty()) {
        return;
    }
    const std::string key = std::exchange(m_focus, std::string());
    m_form.set_value(key, m_editText);
    update();
}

//=============================================================================================================

void FormView::cancel()
{
    if (m_focus.empty()) {
        return;
    }
    const std::string key = std::exchange(m_focus, std::string());
    // Fields that commit immediately already hold the typed text
    m_form.set_value(key, m_editOriginal);
    update();
}

//=============================================================================================================

void FormView::type_text(const std::string& text)
{
    m_editText += text;
    const SettingField* f = m_form.field(m_focus);
    if (f && f->commit == SettingField::Commit::Immediately) {
        m_form.set_value(m_focus, m_editText);
    }
    update();
}

//=============================================================================================================

std::string FormView::value_text(const SettingField& field) const
{
    switch (field.kind) {
        case SettingField::Kind::Int:
            return field.prefix + std::to_string(m_form.get<int>(field.key)) + field.unit;
        case SettingField::Kind::Double: {
            std::array<char, 64> buffer{};
            std::snprintf(buffer.data(), buffer.size(), "%.*f", std::clamp(field.decimals, 0, 12), m_form.get<double>(field.key));
            return field.prefix + buffer.data() + field.unit;
        }
        case SettingField::Kind::Choice: {
            std::string current = m_form.get<std::string>(field.key);
            for (const SettingChoice& choice : field.choices) {
                if (choice.value == current) {
                    return choice.label;
                }
            }
            return current;
        }
        default:
            return m_form.get<std::string>(field.key);
    }
}

//=============================================================================================================

std::string FormView::elided(const std::string& text, const Font& font, double width) const
{
    if (m_metrics->advance(font, text) <= width) {
        return text;
    }
    std::string result = text;
    while (!result.empty() && m_metrics->advance(font, result + "...") > width) {
        pop_code_point(result);
    }
    return result + "...";
}

//=============================================================================================================

DisplayList FormView::draw() const
{
    layout();
    Canvas canvas(m_width, m_height, m_theme.background);
    canvas.set_antialiasing(true);
    canvas.translate(0.0, -m_scroll);
    for (const Row& r : m_rows) {
        if (r.rect.bottom() < m_scroll || r.rect.top() > m_scroll + m_height) {
            continue;
        }
        draw_row(canvas, r);
    }
    return canvas.take();
}

//=============================================================================================================

void FormView::draw_row(Canvas& canvas, const Row& r) const
{
    const Theme& t = m_theme;
    switch (r.type) {
        case Row::Type::Title:
            canvas.set_font(t.font(t.title_font_size, true));
            canvas.set_pen(t.text);
            canvas.draw_text(r.rect, Align::Left | Align::VCenter, elided(r.key, canvas.font(), r.rect.width));
            return;
        case Row::Type::Group:
            canvas.set_font(t.font(t.small_font_size, true));
            canvas.set_pen(t.muted_text);
            canvas.draw_text(r.rect, Align::Left | Align::VCenter, elided(r.key, canvas.font(), r.rect.width));
            return;
        case Row::Type::Card:
            canvas.set_pen(Pen{t.border, 1.0});
            canvas.set_brush(Brush::solid(t.surface));
            canvas.draw_rounded_rect(r.rect.adjusted(0.5, 0.5, -0.5, -0.5), t.corner_radius, t.corner_radius);
            return;
        case Row::Type::Field:
            break;
    }

    const SettingField* f = m_form.field(r.key);
    if (!f) {
        return;
    }
    const double inset = kRowInset;
    if (!r.last) {
        canvas.set_pen(Pen{t.separator, 1.0});
        canvas.draw_line({r.rect.x + inset, r.rect.bottom() - 0.5}, {r.rect.right() - inset, r.rect.bottom() - 0.5});
    }

    canvas.save();
    if (!m_form.is_enabled(f->key)) {
        canvas.set_opacity(kDisabledOpacity);
    }
    if (const ItemList* items = f->kind == SettingField::Kind::Custom ? list_of(f->key) : nullptr) {
        canvas.set_font(t.font(t.font_size));
        canvas.set_pen(t.text);
        const double left = r.rect.x + inset;
        canvas.draw_text(Rect{left, r.rect.y, r.rect.width - 2.0 * inset, r.editor.y - r.rect.y}, Align::Left | Align::VCenter,
                         elided(f->label, canvas.font(), r.rect.width - 2.0 * inset));
        draw_list(canvas, r, *items);
        canvas.restore();
        return;
    }
    if (f->kind != SettingField::Kind::Action) {
        const double left = r.rect.x + inset;
        const double right = r.editor.empty() ? r.rect.right() - inset : r.editor.x - t.spacing;
        canvas.set_font(t.font(t.font_size));
        canvas.set_pen(t.text);
        canvas.draw_text(Rect{left, r.rect.y, right - left, r.rect.height}, Align::Left | Align::VCenter,
                         elided(f->label, canvas.font(), right - left));
    }
    draw_editor(canvas, r, *f);
    canvas.restore();
}

//=============================================================================================================

void FormView::draw_editor(Canvas& canvas, const Row& r, const SettingField& f) const
{
    const Theme& t = m_theme;
    const Rect& e = r.editor;
    const Font font = t.font(t.font_size);
    canvas.set_font(font);

    switch (f.kind) {
        case SettingField::Kind::Bool: {
            // A check box, as clinical applications use: square, quiet when off, a white tick on the accent when on
            const bool on = m_form.get<bool>(f.key);
            if (on) {
                canvas.set_pen(Pen::none());
                canvas.set_brush(Brush::solid(t.accent));
                canvas.draw_rounded_rect(e, kControlRadius, kControlRadius);
                canvas.set_pen(line_pen(t.accent_text, 2.0));
                const std::array<Point, 3> tick = {Point{e.x + 0.25 * e.width, e.y + 0.52 * e.height}, Point{e.x + 0.43 * e.width, e.y + 0.70 * e.height},
                                                   Point{e.x + 0.76 * e.width, e.y + 0.32 * e.height}};
                canvas.draw_polyline(tick);
            } else {
                canvas.set_pen(Pen{t.field_border, 1.5});
                canvas.set_brush(Brush::solid(t.field));
                canvas.draw_rounded_rect(e.adjusted(0.75, 0.75, -0.75, -0.75), kControlRadius, kControlRadius);
            }
            return;
        }
        case SettingField::Kind::Int:
        case SettingField::Kind::Double:
        case SettingField::Kind::Choice: {
            canvas.set_pen(Pen{t.field_border, 1.0});
            canvas.set_brush(Brush::solid(t.field));
            canvas.draw_rounded_rect(e.adjusted(0.5, 0.5, -0.5, -0.5), kControlRadius, kControlRadius);
            canvas.set_pen(Pen{t.separator, 1.0});
            canvas.draw_line({r.decrement.right() - 0.5, e.y + 1.0}, {r.decrement.right() - 0.5, e.bottom() - 1.0});
            canvas.draw_line({r.increment.x + 0.5, e.y + 1.0}, {r.increment.x + 0.5, e.bottom() - 1.0});

            const Point minus = r.decrement.center();
            const Point plus = r.increment.center();
            const double arm = 4.0;
            if (f.kind == SettingField::Kind::Choice) {
                canvas.set_pen(line_pen(t.muted_text, 1.5));
                const std::array<Point, 3> previous = {Point{minus.x + 2.0, minus.y - arm}, Point{minus.x - 2.0, minus.y}, Point{minus.x + 2.0, minus.y + arm}};
                const std::array<Point, 3> next = {Point{plus.x - 2.0, plus.y - arm}, Point{plus.x + 2.0, plus.y}, Point{plus.x - 2.0, plus.y + arm}};
                canvas.draw_polyline(previous);
                canvas.draw_polyline(next);
            } else {
                const double value = m_form.get<double>(f.key);
                canvas.set_pen(line_pen(value <= f.minimum ? t.field_border : t.muted_text, 1.5));
                canvas.draw_line({minus.x - arm, minus.y}, {minus.x + arm, minus.y});
                canvas.set_pen(line_pen(value >= f.maximum ? t.field_border : t.muted_text, 1.5));
                canvas.draw_line({plus.x - arm, plus.y}, {plus.x + arm, plus.y});
                canvas.draw_line({plus.x, plus.y - arm}, {plus.x, plus.y + arm});
            }
            const Rect middle{r.decrement.right(), e.y, r.increment.x - r.decrement.right(), e.height};
            canvas.set_pen(t.text);
            canvas.draw_text(middle, Align::Center, elided(value_text(f), font, middle.width - t.spacing));
            return;
        }
        case SettingField::Kind::Text:
        case SettingField::Kind::Path:
        case SettingField::Kind::Directory: {
            const bool focused = m_focus == f.key;
            if (focused) {
                // A soft focus ring around the accent outline
                Color ring = t.accent;
                ring.a = 48;
                canvas.set_pen(Pen{ring, 3.0});
                canvas.set_brush(Brush{});
                canvas.draw_rounded_rect(e.adjusted(-1.5, -1.5, 1.5, 1.5), kControlRadius + 1.5, kControlRadius + 1.5);
            }
            canvas.set_pen(Pen{focused ? t.accent : t.field_border, 1.0});
            canvas.set_brush(Brush::solid(t.field));
            canvas.draw_rounded_rect(e.adjusted(0.5, 0.5, -0.5, -0.5), kControlRadius, kControlRadius);
            const Rect box = e.adjusted(10.0, 0.0, -10.0, 0.0);
            std::string text = focused ? m_editText : value_text(f);
            if (focused) {
                // The end of the text stays in view while typing
                while (!text.empty() && m_metrics->advance(font, text) > box.width - 2.0) {
                    text.erase(0, 1);
                    while (!text.empty() && (static_cast<unsigned char>(text.front()) & 0xC0) == 0x80) {
                        text.erase(0, 1);
                    }
                }
                const double caret = std::round(box.x + m_metrics->advance(font, text)) + 1.5;
                canvas.set_pen(line_pen(t.accent, 1.5));
                canvas.draw_line({caret, e.y + 7.0}, {caret, e.bottom() - 7.0});
            } else {
                text = elided(text, font, box.width);
            }
            canvas.set_pen(f.kind == SettingField::Kind::Text || focused ? t.text : t.muted_text);
            canvas.draw_text(box, Align::Left | Align::VCenter, text);
            return;
        }
        case SettingField::Kind::Action: {
            canvas.set_pen(Pen::none());
            canvas.set_brush(Brush::solid(t.accent));
            canvas.draw_rounded_rect(e, kControlRadius, kControlRadius);
            const Font bold = t.font(t.font_size, true);
            canvas.set_font(bold);
            canvas.set_pen(t.accent_text);
            canvas.draw_text(e, Align::Center, elided(f.label, bold, e.width - t.spacing));
            return;
        }
        case SettingField::Kind::Color: {
            const std::string text = m_form.get<std::string>(f.key);
            canvas.set_pen(Pen{t.field_border, 1.0});
            canvas.set_brush(Brush::solid(parse_color(text).value_or(Color::transparent())));
            canvas.draw_rounded_rect(e.adjusted(0.5, 0.5, -0.5, -0.5), kControlRadius, kControlRadius);
            const Font mono = t.mono_font(t.small_font_size);
            canvas.set_font(mono);
            canvas.set_pen(t.muted_text);
            canvas.draw_text(Rect{e.x - 120.0, r.rect.y, 120.0 - t.spacing, r.rect.height}, Align::Right | Align::VCenter, upper(text));
            return;
        }
        case SettingField::Kind::Info: {
            const std::string text = m_form.get<std::string>(f.key);
            if (f.tone == SettingField::Tone::Normal) {
                canvas.set_pen(t.muted_text);
                canvas.draw_text(e, (f.label.empty() ? Align::Left : Align::Right) | Align::VCenter, elided(text, font, e.width));
                return;
            }
            // A status chip: tinted pill with a dot in the tone's colour
            const Color color = tone_color(t, f.tone);
            const Font chipFont = t.font(t.small_font_size, true);
            const double dot = 6.0;
            const double textWidth = std::min(m_metrics->advance(chipFont, text), e.width - 3.0 * t.spacing - dot);
            const double chipWidth = textWidth + 3.0 * t.spacing + dot;
            const double chipHeight = std::round(t.small_font_size * 1.9);
            const Rect chip{f.label.empty() ? e.x : e.right() - chipWidth, std::round(e.center().y - chipHeight / 2.0), chipWidth, chipHeight};
            Color tint = color;
            tint.a = 26;
            canvas.set_pen(Pen::none());
            canvas.set_brush(Brush::solid(tint));
            canvas.draw_rounded_rect(chip, chipHeight / 2.0, chipHeight / 2.0);
            canvas.set_brush(Brush::solid(color));
            canvas.draw_ellipse({chip.x + t.spacing + dot / 2.0, chip.center().y}, dot / 2.0, dot / 2.0);
            canvas.set_font(chipFont);
            canvas.set_pen(color);
            canvas.draw_text(Rect{chip.x + 1.5 * t.spacing + dot, chip.y, textWidth + 1.0, chip.height}, Align::Left | Align::VCenter,
                             elided(text, chipFont, textWidth + 1.0));
            return;
        }
        case SettingField::Kind::Custom:
            return;
    }
}

//=============================================================================================================

GpuFrame FormView::frame(const GpuFrameTarget& target)
{
    const double ratio = target.device_pixel_ratio > 0.0 ? target.device_pixel_ratio : 1.0;
    if (m_width <= 0.0 || m_height <= 0.0) {
        set_view_size(target.pixel_width / ratio, target.pixel_height / ratio);
    }

    GpuFrame frame;
    frame.pixel_width = target.pixel_width;
    frame.pixel_height = target.pixel_height;
    frame.device_pixel_ratio = ratio;
    GpuPass pass;
    pass.clear_color = m_theme.background;

    DisplayList list = draw();
    if (m_texture == 0) {
        GpuTexture texture;
        texture.content = std::move(list);
        texture.scale = ratio;
        texture.linear_filter = false;
        m_texture = m_resources.create_texture(std::move(texture));
    } else {
        m_resources.set_texture(m_texture, std::move(list), ratio);
    }
    GpuDraw quad;
    quad.pipeline = m_pipeline;
    quad.vertex_buffers = {m_quad};
    quad.count = 4;
    quad.textures = {{0, m_texture}};
    pass.draws.push_back(std::move(quad));
    frame.passes.push_back(std::move(pass));
    return frame;
}

//=============================================================================================================

const GpuResources& FormView::resources() const
{
    return m_resources;
}

//=============================================================================================================

ItemList* FormView::list_of(const std::string& key) const
{
    return m_panel && !key.empty() ? m_panel->list(key) : nullptr;
}

//=============================================================================================================

const FormView::Row* FormView::row_of_list(const std::string& key) const
{
    return list_of(key) ? row(key) : nullptr;
}

//=============================================================================================================

double FormView::list_row_height() const
{
    return std::round(m_theme.font_size * 1.8);
}

//=============================================================================================================

void FormView::scroll_list(const std::string& key, double offset) const
{
    const ItemList* items = list_of(key);
    const Row* r = items ? row(key) : nullptr;
    if (!r) {
        return;
    }
    const double overflow = std::max(0.0, items->count() * list_row_height() - (r->editor.height - 2.0));
    m_listScroll[key] = std::clamp(offset, 0.0, overflow);
}

//=============================================================================================================

std::optional<Rect> FormView::item_rect(const std::string& key, int index) const
{
    const ItemList* items = list_of(key);
    const Row* r = items ? row(key) : nullptr;
    if (!r || index < 0 || index >= items->count()) {
        return std::nullopt;
    }
    const Rect box = r->editor.adjusted(1.0, 1.0, -1.0, -1.0);
    const double scroll = m_listScroll.contains(key) ? m_listScroll.at(key) : 0.0;
    const Rect item{box.x, box.y + index * list_row_height() - scroll, box.width, list_row_height()};
    if (item.bottom() <= box.y || item.y >= box.bottom()) {
        return std::nullopt;
    }
    return Rect{item.x, item.y - m_scroll, item.width, item.height};
}

//=============================================================================================================

void FormView::draw_list(Canvas& canvas, const Row& r, const ItemList& list) const
{
    const Theme& t = m_theme;
    const Rect& e = r.editor;
    canvas.set_pen(Pen{r.key == m_listFocus ? t.accent : t.field_border, 1.0});
    canvas.set_brush(Brush::solid(t.field));
    canvas.draw_rounded_rect(e.adjusted(0.5, 0.5, -0.5, -0.5), kControlRadius, kControlRadius);

    const Rect box = e.adjusted(1.0, 1.0, -1.0, -1.0);
    const double rowHeight = list_row_height();
    const double scroll = m_listScroll.contains(r.key) ? m_listScroll.at(r.key) : 0.0;
    const Font font = t.font(t.font_size);
    canvas.save();
    canvas.clip_rect(box);
    canvas.set_font(font);
    const int first = std::max(0, static_cast<int>(scroll / rowHeight));
    for (int index = first; index < list.count(); ++index) {
        const Rect item{box.x, box.y + index * rowHeight - scroll, box.width, rowHeight};
        if (item.y >= box.bottom()) {
            break;
        }
        const bool selected = list.is_selected(index);
        if (selected) {
            canvas.set_pen(Pen::none());
            canvas.set_brush(Brush::solid(t.accent));
            canvas.draw_rect(item);
        }
        canvas.set_pen(selected ? t.accent_text : t.text);
        const Rect text = item.adjusted(t.spacing, 0.0, -t.spacing, 0.0);
        canvas.draw_text(text, Align::Left | Align::VCenter, elided(list.items()[static_cast<std::size_t>(index)], font, text.width));
    }
    canvas.restore();
}

//=============================================================================================================

void FormView::update() const
{
    if (surface()) {
        surface()->request_update();
    }
}
