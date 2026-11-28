//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_viz_form_view.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests FormView, the toolkit-free view of settings forms, with synthetic input and a golden image.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../common/scene_golden.h"
#include <mne/viz/figures/font_library.h>
#include <mne/viz/figures/font_text_metrics.h>
#include <mne/viz/forms/settings_form.h>
#include <mne/viz/gpu/software_renderer.h>
#include <mne/viz/host/surface.h>
#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/viz/models/item_list.h>
#include <mne/viz/panels/channel_selection_panel.h>
#include <mne/viz/panels/form_view.h>
#include <mne/viz/panels/projectors_panel.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <filesystem>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::viz;

namespace
{

int g_failures = 0;

#define CHECK(condition)                                                                       \
    do {                                                                                       \
        if (!(condition)) {                                                                    \
            std::fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #condition); \
            ++g_failures;                                                                      \
        }                                                                                      \
    } while (false)

class CountingSurface : public Surface
{
public:
    void request_update() override
    {
        ++updates;
    }

    std::int64_t now_ms() const override
    {
        return 0;
    }

    int updates = 0;
};

bool click(FormView& view, const std::string& key, FormView::Part part = FormView::Part::Editor)
{
    const std::optional<Rect> rect = view.editor_rect(key, part);
    if (!rect) {
        return false;
    }
    PointerEvent press;
    press.type = PointerEvent::Type::Press;
    press.x = rect->center().x;
    press.y = rect->center().y;
    press.button = MouseButton::Left;
    press.buttons = MouseButtons::Left;
    const bool used = view.handle_pointer(press);
    PointerEvent release = press;
    release.type = PointerEvent::Type::Release;
    release.buttons = MouseButtons::None;
    view.handle_pointer(release);
    return used;
}

void type(FormView& view, const std::string& text)
{
    for (const char c : text) {
        KeyEvent key;
        key.key = Key::Character;
        key.character = static_cast<char32_t>(c);
        view.handle_key(key);
    }
}

void press_key(FormView& view, Key k)
{
    KeyEvent key;
    key.key = k;
    view.handle_key(key);
}

void build_form(SettingsForm& form)
{
    form.add_bool("filter", "Band-pass filter", true, "Filter");
    form.add_double("highpass", "High-pass", 1.0, 0.1, 40.0, 0.5, 1, " Hz", "Filter");
    form.add_int("order", "Order", 4, 1, 8, 1, {}, "Filter");
    form.add_choice("design", "Design", {{"fir", "FIR"}, {"iir", "Butterworth"}, {"cheby", "Chebyshev"}}, "fir", "Filter");
    form.add_text("name", "Name", "raw_sss", "Output");
    form.add_path("file", "File", "/data/sample_audvis_raw.fif", "FIFF files (*.fif)", "Output");
    form.add_color("color", "Trace colour", "#2f6fde", "Output");
    form.add_info("status", "Status", "Ready", "Output");
    form.set_tone("status", SettingField::Tone::Positive);
    form.add_action("apply", "Apply", "Output");
}

void test_bool_toggles()
{
    SettingsForm form("Preprocessing");
    build_form(form);
    FormView view(form);
    view.set_view_size(420, 600);
    CHECK(form.get<bool>("filter"));
    CHECK(click(view, "filter"));
    CHECK(!form.get<bool>("filter"));
    CHECK(click(view, "filter"));
    CHECK(form.get<bool>("filter"));
}

void test_steppers()
{
    SettingsForm form("Preprocessing");
    build_form(form);
    FormView view(form);
    view.set_view_size(420, 600);

    // + and - step by the field's step and the form clamps
    CHECK(click(view, "order", FormView::Part::Increment) && form.get<int>("order") == 5);
    CHECK(click(view, "order", FormView::Part::Decrement) && form.get<int>("order") == 4);
    for (int i = 0; i < 10; ++i) {
        click(view, "order", FormView::Part::Increment);
    }
    CHECK(form.get<int>("order") == 8);
    for (int i = 0; i < 20; ++i) {
        click(view, "order", FormView::Part::Decrement);
    }
    CHECK(form.get<int>("order") == 1);
    // A click on the value itself does nothing
    CHECK(click(view, "order") && form.get<int>("order") == 1);

    CHECK(click(view, "highpass", FormView::Part::Increment) && form.get<double>("highpass") == 1.5);

    // The wheel over a stepper steps it
    const Rect rect = *view.editor_rect("highpass");
    WheelEvent wheel;
    wheel.x = rect.center().x;
    wheel.y = rect.center().y;
    wheel.angle_delta_y = -240.0;
    CHECK(view.handle_wheel(wheel) && form.get<double>("highpass") == 0.5);
}

void test_choice_cycles()
{
    SettingsForm form("Preprocessing");
    build_form(form);
    FormView view(form);
    view.set_view_size(420, 600);
    CHECK(click(view, "design", FormView::Part::Increment) && form.get<std::string>("design") == "iir");
    CHECK(click(view, "design", FormView::Part::Increment) && form.get<std::string>("design") == "cheby");
    CHECK(click(view, "design", FormView::Part::Increment) && form.get<std::string>("design") == "fir");
    CHECK(click(view, "design", FormView::Part::Decrement) && form.get<std::string>("design") == "cheby");
    CHECK(!view.editor_rect("filter", FormView::Part::Increment));
}

void test_text_editing()
{
    SettingsForm form("Preprocessing");
    build_form(form);
    SettingField finished;
    finished.key = "subject";
    finished.kind = SettingField::Kind::Text;
    finished.label = "Subject";
    finished.default_value = std::string("sample");
    finished.commit = SettingField::Commit::WhenFinished;
    form.add(finished);
    FormView view(form);
    view.set_view_size(420, 800);
    std::vector<std::string> heard;
    form.add_listener([&heard](const std::string& key, const std::optional<SettingValue>&) {
        heard.push_back(key);
    });

    // Typing commits on Enter
    CHECK(click(view, "subject") && view.focused_key() == "subject");
    press_key(view, Key::Backspace);
    type(view, "Xy");
    CHECK(view.edit_text() == "samplXy" && form.get<std::string>("subject") == "sample" && heard.empty());
    press_key(view, Key::Return);
    CHECK(view.focused_key().empty() && form.get<std::string>("subject") == "samplXy" && heard.size() == 1);

    // Escape cancels
    click(view, "subject");
    type(view, "zz");
    press_key(view, Key::Escape);
    CHECK(view.focused_key().empty() && form.get<std::string>("subject") == "samplXy");

    // Immediate fields commit every keystroke; Escape restores the value
    click(view, "name");
    type(view, "2");
    CHECK(form.get<std::string>("name") == "raw_sss2");
    press_key(view, Key::Escape);
    CHECK(form.get<std::string>("name") == "raw_sss");

    // A click elsewhere finishes editing
    click(view, "subject");
    type(view, "!");
    click(view, "filter");
    CHECK(view.focused_key().empty() && form.get<std::string>("subject") == "samplXy!");

    // Without focus, characters are not used
    KeyEvent key;
    key.key = Key::Character;
    key.character = U'q';
    CHECK(!view.handle_key(key));
}

void test_action_and_disabled()
{
    SettingsForm form("Preprocessing");
    build_form(form);
    form.add_bool("advanced", "Advanced", false);
    SettingField taps;
    taps.key = "taps";
    taps.kind = SettingField::Kind::Int;
    taps.label = "Taps";
    taps.default_value = 64;
    taps.minimum = 16;
    taps.maximum = 512;
    taps.enabled_if = "advanced";
    form.add(taps);
    FormView view(form);
    view.set_view_size(420, 800);

    int triggered = 0;
    form.add_listener([&triggered](const std::string& key, const std::optional<SettingValue>& value) {
        triggered += key == "apply" && !value ? 1 : 0;
    });
    CHECK(click(view, "apply") && triggered == 1);

    // Disabled fields ignore clicks and the wheel until their condition holds
    click(view, "taps", FormView::Part::Increment);
    CHECK(form.get<int>("taps") == 64);
    const Rect rect = *view.editor_rect("taps");
    WheelEvent wheel;
    wheel.x = rect.center().x;
    wheel.y = rect.center().y;
    wheel.angle_delta_y = 120.0;
    view.handle_wheel(wheel);
    CHECK(form.get<int>("taps") == 64);
    click(view, "advanced");
    click(view, "taps", FormView::Part::Increment);
    CHECK(form.get<int>("taps") == 65);
}

void test_structure_and_visibility()
{
    SettingsForm form("Preprocessing");
    build_form(form);
    form.add_bool("advanced", "Advanced", false);
    FormView view(form);
    CountingSurface surface;
    view.attach(&surface);
    view.set_view_size(420, 200);
    const double before = view.content_height();
    CHECK(!view.editor_rect("extra"));

    // Added fields are laid out and repainted
    const int updates = surface.updates;
    SettingField extra;
    extra.key = "extra";
    extra.kind = SettingField::Kind::Bool;
    extra.label = "Extra";
    extra.default_value = false;
    extra.visible_if = "advanced";
    form.add(extra);
    CHECK(surface.updates > updates);
    CHECK(!view.editor_rect("extra") && view.content_height() == before);
    form.set_value("advanced", true);
    CHECK(view.editor_rect("extra") && view.content_height() > before);
    form.remove("extra");
    CHECK(!view.editor_rect("extra") && view.content_height() == before);

    // Hidden fields and custom editors have no editor; content taller than the view scrolls
    form.add_flag("mode");
    CHECK(!view.editor_rect("mode"));
    CHECK(view.content_height() > 200);
    const Rect top = *view.editor_rect("filter");
    WheelEvent wheel;
    wheel.x = 10;
    wheel.y = 10;
    wheel.angle_delta_y = -120.0;
    CHECK(view.handle_wheel(wheel) && view.scroll_offset() > 0.0);
    CHECK(view.editor_rect("filter")->y < top.y);
    wheel.angle_delta_y = -12000.0;
    view.handle_wheel(wheel);
    CHECK(view.scroll_offset() == view.content_height() - 200);
    view.attach(nullptr);
}

void test_item_list()
{
    ItemList single;
    int items = 0;
    int selections = 0;
    single.add_listener([&](ItemList::Change change) {
        (change == ItemList::Change::Items ? items : selections) += 1;
    });
    single.set_items({"All", "Left temporal", "Vertex"});
    single.click(1);
    single.click(2, true);
    CHECK(single.selected() == std::vector<int>({2}) && single.current_item() == "Vertex");
    single.set_selected({0, 2});
    CHECK(single.selected() == std::vector<int>({0}) && items == 1 && selections == 3);
    // Not removable: Delete does nothing; removable: the protected item stays
    CHECK(!single.remove_selected());
    single.set_removable(true);
    single.set_protected_items({"All"});
    CHECK(!single.remove_selected());
    single.click(1);
    CHECK(single.remove_selected() && single.items() == std::vector<std::string>({"All", "Vertex"}) && single.selected().empty());
    single.click(5);
    CHECK(single.current() == -1 && single.find("Vertex") == 1 && single.find("x") == -1);

    ItemList multi(ItemList::SelectionMode::Multi);
    multi.set_items({"MEG 0111", "MEG 0112", "MEG 0113"});
    multi.click(0);
    multi.click(2, true);
    CHECK(multi.selected() == std::vector<int>({0, 2}));
    multi.click(0, true);
    CHECK(multi.selected() == std::vector<int>({2}));
    multi.set_selected({2, 1, 1, 7});
    CHECK(multi.selected() == std::vector<int>({1, 2}) && multi.is_selected(1) && !multi.is_selected(0));

    ItemList none(ItemList::SelectionMode::None);
    none.set_items({"a"});
    none.click(0);
    CHECK(none.selected().empty());
}

void test_lists_in_form_view()
{
    // The channel selection panel: its three lists in FormView, clicked, scrolled and edited with the keyboard
    ChannelSelectionPanel panel;
    std::vector<std::vector<std::string>> shown;
    ChannelSelectionPanel::Callbacks callbacks;
    callbacks.channels_shown = [&shown](const std::vector<std::string>& channels) {
        shown.push_back(channels);
    };
    panel.set_callbacks(std::move(callbacks));
    CHECK(!panel.layout_map().empty() && panel.selection_groups().size() > 2);
    panel.set_mapped_channels({"MEG 0113", "MEG 0112", "MEG 0111"});
    CHECK(panel.groups().current_item() == "All" && panel.group_channels().count() == 3 && !shown.empty());
    CHECK(panel.selected_channels() == std::vector<std::string>({"MEG 0113", "MEG 0112", "MEG 0111"}));

    // As drawn with the bundled fonts, with a channel picked
    {
        panel.select_channels({"MEG 0112"});
        const auto fonts = std::make_shared<FontLibrary>();
        const FontTextMetrics metrics(fonts);
        FormView golden(panel);
        golden.set_text_metrics(&metrics);
        GpuFrameTarget target;
        target.pixel_width = 380;
        target.pixel_height = 900;
        SoftwareRenderer renderer;
        renderer.set_fonts(fonts);
        CHECK(scene_golden::matches_png(renderer.render(golden.frame(target), golden.resources()), GOLDEN_DIR "/channel_selection.png"));
        panel.select_channels({});
    }

    FormView view(panel);
    view.set_view_size(380, 2000);
    const std::string groups = ChannelSelectionPanel::groups_key;
    const std::string picked = ChannelSelectionPanel::picked_key;
    CHECK(view.editor_rect(groups).has_value() && view.item_rect(groups, 0).has_value());
    // A click on a group chooses it and shows its channels
    const int vertex = panel.groups().find("Vertex");
    CHECK(vertex >= 0);
    auto clickItem = [&view](const std::string& key, int row, Modifier modifiers = Modifier::None) {
        const std::optional<Rect> item = view.item_rect(key, row);
        if (!item) {
            return false;
        }
        PointerEvent press;
        press.type = PointerEvent::Type::Press;
        press.x = item->center().x;
        press.y = item->center().y;
        press.button = MouseButton::Left;
        press.modifiers = modifiers;
        return view.handle_pointer(press);
    };
    // The wheel scrolls a list whose groups do not fit, until the group shows
    for (int notch = 0; notch < 20 && !view.item_rect(groups, vertex); ++notch) {
        WheelEvent wheel;
        const Rect box = *view.editor_rect(groups);
        wheel.x = box.center().x;
        wheel.y = box.center().y;
        wheel.angle_delta_y = -120.0;
        view.handle_wheel(wheel);
    }
    CHECK(clickItem(groups, vertex) && panel.groups().current_item() == "Vertex");
    CHECK(panel.group_channels().count() > 0 && panel.sensors().channel_kind() == FIFFV_MEG_CH);

    // The arrow keys move the choice; Delete removes a group, but never "All"
    KeyEvent key;
    key.key = Key::Up;
    CHECK(view.handle_key(key) && panel.groups().current() == vertex - 1);
    const std::string removed = panel.groups().current_item();
    key.key = Key::Delete;
    CHECK(view.handle_key(key) && panel.groups().find(removed) < 0 && !panel.selection_groups().contains(removed));
    panel.groups().set_selected({panel.groups().find("All")});
    CHECK(!view.handle_key(key) && panel.groups().find("All") >= 0);

    // Picking sensors fills the picked list; a group keeps them under a name
    panel.select_channels({"MEG 0111", "MEG 0112"});
    CHECK(panel.picked_channels().items() == std::vector<std::string>({"MEG 0111", "MEG 0112"}));
    CHECK(shown.back() == std::vector<std::string>({"MEG 0111", "MEG 0112"}) && !panel.selection_empty());
    panel.form().set_value("groupName", std::string("Mine"));
    panel.form().trigger("addGroup");
    CHECK(panel.selection_groups().at("Mine") == std::vector<std::string>({"MEG 0111", "MEG 0112"}));
    CHECK(panel.groups().current_item() == "All");

    // Ctrl+click extends the picked selection; Delete drops those channels, in the layout too
    CHECK(clickItem(picked, 0) && clickItem(picked, 1, Modifier::Ctrl));
    CHECK(panel.picked_channels().selected() == std::vector<int>({0, 1}));
    CHECK(view.handle_key(key) && panel.picked_channels().count() == 0 && panel.sensors().selected_names().empty());
    CHECK(panel.selection_empty());

    // Saving and reading the groups back
    const std::filesystem::path file = std::filesystem::temp_directory_path() / "test_viz_form_view_groups.sel";
    CHECK(panel.save_selection_groups(file));
    CHECK(panel.load_selection_groups(mne::utf8_from_path(file)) && panel.selection_groups().contains("Mine"));
    CHECK(panel.group_file() == mne::utf8_from_path(file));
    std::filesystem::remove(file);
    CHECK(!panel.load_layout("missing.lout") && panel.layout_map().empty());
    CHECK(panel.form().field("selectedLayoutFile")->choices.back().value == "missing.lout");
    CHECK(panel.load_layout("Vectorview-mag.lout") && !panel.layout_map().empty());
}

void test_panel_and_golden()
{
    ProjectorsPanel panel;
    std::vector<mne::FiffProj> projs(2);
    projs[0].desc = "Average EEG reference";
    projs[0].active = true;
    projs[1].desc = "PCA-v1";
    FormView panelView(panel);
    panelView.set_view_size(360, 240);
    panel.set_projectors(projs);
    CHECK(click(panelView, ProjectorsPanel::field_key(1)) && panel.projectors()[1].active);

    SettingsForm form("Preprocessing");
    build_form(form);
    form.add_custom("plot", "Response");
    // The bundled typefaces, identical on every machine, measured as drawn
    const auto fonts = std::make_shared<FontLibrary>();
    const FontTextMetrics metrics(fonts);
    FormView view(form);
    view.set_text_metrics(&metrics);
    GpuFrameTarget target;
    target.pixel_width = 420;
    target.pixel_height = 560;
    const GpuFrame frame = view.frame(target);
    CHECK(frame.passes.size() == 1 && frame.passes[0].draws.size() == 1);
    // Focus a field so the golden shows a caret
    click(view, "name");
    SoftwareRenderer renderer;
    renderer.set_fonts(fonts);
    CHECK(scene_golden::matches_png(renderer.render(view.frame(target), view.resources()), GOLDEN_DIR "/form_view.png"));

    // The dark theme clears to its background
    FormView dark(form, Theme::dark());
    const Image image = renderer.render(dark.frame(target), dark.resources());
    CHECK(image.width == 420 && image.pixel(2, 2) == Theme::dark().background);
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main()
{
    test_bool_toggles();
    test_steppers();
    test_choice_cycles();
    test_text_editing();
    test_action_and_disabled();
    test_structure_and_visibility();
    test_item_list();
    test_lists_in_form_view();
    test_panel_and_golden();
    if (g_failures != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    std::printf("All FormView tests passed\n");
    return 0;
}
