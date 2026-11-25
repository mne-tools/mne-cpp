//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_scene_brain_view.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests the camera, ray picking, pane layout, input handling and overlay of the 3-D brain view in
 *           mne_scene, without Qt.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/scene/brain_view_interaction.h>
#include <mne/scene/brain_view_overlay.h>
#include <mne/scene/camera.h>
#include <mne/scene/pane_layout.h>
#include <mne/scene/ray_picking.h>
#include <mne/scene/svg_renderer.h>
#include <mne/scene/text_metrics.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Geometry>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scene;

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

bool near(double a, double b, double tolerance = 1e-6)
{
    return std::abs(a - b) <= tolerance;
}

bool near_quat(const Eigen::Quaternionf& q, double w, double x, double y, double z, double tolerance = 1e-6)
{
    return near(q.w(), w, tolerance) && near(q.x(), x, tolerance) && near(q.y(), y, tolerance) && near(q.z(), z, tolerance);
}

// Golden SVG comparison as in test_scene_trace_view
bool matches_golden(const std::string& svg, const std::string& path)
{
    const char* update = std::getenv("MNE_UPDATE_GOLDEN");
    if (update && std::strcmp(update, "1") == 0) {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << svg;
        return static_cast<bool>(out);
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "missing golden file %s (run with MNE_UPDATE_GOLDEN=1)\n", path.c_str());
        return false;
    }
    const std::string expected((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (expected == svg) {
        return true;
    }
    std::size_t at = 0;
    while (at < expected.size() && at < svg.size() && expected[at] == svg[at]) {
        ++at;
    }
    const std::size_t from = at > 60 ? at - 60 : 0;
    std::fprintf(stderr, "%s differs at byte %zu\n expected: %s\n actual:   %s\n", path.c_str(), at,
                 expected.substr(from, 160).c_str(), svg.substr(from, 160).c_str());
    return false;
}

PointerEvent pointer(PointerEvent::Type type, double x, double y, MouseButton button = MouseButton::None)
{
    PointerEvent event;
    event.type = type;
    event.x = x;
    event.y = y;
    event.button = button;
    event.buttons = (type == PointerEvent::Type::Press && button == MouseButton::Left) ? MouseButtons::Left : MouseButtons::None;
    return event;
}

// A move with the left button held
PointerEvent drag(double x, double y)
{
    PointerEvent event = pointer(PointerEvent::Type::Move, x, y);
    event.buttons = MouseButtons::Left;
    return event;
}

//=============================================================================================================
// Camera
//=============================================================================================================

void test_presets()
{
    CHECK(view_preset_name(0) == "Top");
    CHECK(view_preset_name(1) == "Perspective");
    CHECK(view_preset_name(6) == "Right");
    CHECK(view_preset_name(42) == "Top");
    CHECK(clamp_view_preset(-3) == 0);
    CHECK(clamp_view_preset(9) == 6);
    CHECK(view_preset_is_perspective(1));
    CHECK(!view_preset_is_perspective(0));

    // Reference values of QQuaternion::fromEulerAngles(-45, -40, -130) and the axis-angle products
    CHECK(near_quat(perspective_preset_rotation(), 0.2482789, 0.1344045, -0.4594539, -0.8421373));
    CHECK(near_quat(view_preset_rotation(1), 0.2482789, 0.1344045, -0.4594539, -0.8421373));
    CHECK(near_quat(view_preset_rotation(0), 0.0, 0.0, 0.0, 1.0));
    CHECK(near_quat(view_preset_rotation(3), 0.5, 0.5, -0.5, -0.5));
    CHECK(near_quat(view_preset_rotation(5), std::sqrt(0.5), std::sqrt(0.5), 0.0, 0.0));
    CHECK(near_quat(view_preset_rotation(6), 0.5, 0.5, 0.5, 0.5));

    // Every preset is a unit rotation; Top looks down the z axis
    for (int preset = 0; preset < view_preset_count; ++preset) {
        CHECK(near(view_preset_rotation(preset).norm(), 1.0, 1e-6));
    }
    CHECK(near(rotation_about_axis(Eigen::Vector3f(0, 0, 2), 90).w(), std::sqrt(0.5)));
    CHECK(rotation_about_axis(Eigen::Vector3f::Zero(), 90).isApprox(Eigen::Quaternionf::Identity()));
}

void test_matrices()
{
    // 90 degrees, aspect 2, planes 1 and 3: cotan 1
    const Eigen::Matrix4f projection = perspective_matrix(90.0f, 2.0f, 1.0f, 3.0f);
    CHECK(near(projection(0, 0), 0.5));
    CHECK(near(projection(1, 1), 1.0));
    CHECK(near(projection(2, 2), -2.0));
    CHECK(near(projection(2, 3), -3.0));
    CHECK(near(projection(3, 2), -1.0));
    CHECK(near(projection(3, 3), 0.0));
    CHECK(perspective_matrix(45.0f, 0.0f, 1.0f, 3.0f).isIdentity());

    // A camera on the z axis looking at the origin only translates
    const Eigen::Matrix4f view = look_at_matrix({0, 0, 5}, {0, 0, 0}, {0, 1, 0});
    CHECK(view.isApprox(translation_matrix({0, 0, -5})));
    CHECK(look_at_matrix({1, 1, 1}, {1, 1, 1}, {0, 1, 0}).isIdentity());
    const Eigen::Vector4f mapped = view * Eigen::Vector4f(0, 0, 0, 1);
    CHECK(near(mapped.z(), -5.0));
}

void test_single_view()
{
    OrbitCamera camera;
    camera.set_scene_center({0.01f, 0.02f, 0.03f});
    camera.set_scene_size(0.3f);
    CHECK(near(camera.scene_size(), 0.3));
    const CameraMatrices m = camera.single_view(rotation_about_axis({0, 1, 0}, 30.0f), 2.0f, 4.0f / 3.0f);

    // Reference values computed with QMatrix4x4 / QQuaternion by the former CameraController
    CHECK(near(m.distance, 0.42));
    CHECK(near(m.eye.x(), -0.0515709) && near(m.eye.y(), 0.2969849) && near(m.eye.z(), 0.2924729));
    const std::array<float, 16> expected = {
        -1.3165004f, -0.9807894f, 0.7637851f, 0.0098673f, //
        1.6307524f, -1.0973067f, 1.4017808f, -0.0364148f, //
        0.1227953f, -0.7071493f, -0.6964059f, 0.4478322f, //
        0.1227880f, -0.7071069f, -0.6963642f, 0.4538052f};
    const Eigen::Matrix4f pvm = m.pvm();
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            CHECK(near(pvm(r, c), expected[static_cast<std::size_t>(4 * r + c)], 2e-5));
        }
    }

    // Zooming in moves the camera closer by 0.05 scene sizes per notch; tiny scenes count as 0.3
    CHECK(near(camera.single_view(Eigen::Quaternionf::Identity(), -4.0f, 1.0f).distance, 0.45 + 0.06));
    camera.set_scene_size(0.0f);
    CHECK(near(camera.scene_size(), 0.3));
}

void test_pane_view()
{
    OrbitCamera camera;
    camera.set_scene_size(0.3f);

    // Planar presets pan along their view plane and ignore the pane rotation
    PaneCamera top;
    top.preset = static_cast<int>(ViewPreset::Top);
    top.zoom = -1.0f;
    top.pan = {0.02f, -0.01f};
    top.rotation = rotation_about_axis({1, 0, 0}, 45.0f);
    const CameraMatrices planar = camera.pane_view(top, 1.0f);
    CHECK(near(planar.look_at.x(), -0.02) && near(planar.look_at.y(), 0.01) && near(planar.look_at.z(), 0.0));
    CHECK(near(planar.distance, 0.465));

    // The perspective preset orbits by the pane rotation and ignores the pan
    PaneCamera perspective;
    perspective.pan = {0.5f, 0.5f};
    perspective.rotation = rotation_about_axis({0, 1, 0}, 30.0f);
    const CameraMatrices orbiting = camera.pane_view(perspective, 4.0f / 3.0f);
    CHECK(orbiting.look_at.isZero());
    const CameraMatrices single = camera.single_view(perspective.rotation, 0.0f, 4.0f / 3.0f);
    CHECK(orbiting.view.isApprox(single.view));
}

void test_orbit_and_pan()
{
    // Reference value of the former CameraController::applyMouseRotation(QPoint(10, 5))
    const Eigen::Quaternionf rotated = OrbitCamera::orbit(Eigen::Quaternionf::Identity(), 10.0, 5.0);
    CHECK(near_quat(rotated, 0.9988104, 0.0059023, 0.0309535, -0.0372112));
    CHECK(OrbitCamera::orbit(Eigen::Quaternionf::Identity(), 0.0, 0.0).isApprox(Eigen::Quaternionf::Identity()));

    // A horizontal drag turns about the up axis of the camera, which keeps its up direction
    OrbitCamera camera;
    const Eigen::Quaternionf yawed = OrbitCamera::orbit(Eigen::Quaternionf::Identity(), 40.0, 0.0);
    CHECK(camera.single_view(yawed, 0, 1).up.isApprox(camera.single_view(Eigen::Quaternionf::Identity(), 0, 1).up, 1e-5f));

    // 0.002 scene sizes per pixel, the scene following the pointer
    const Eigen::Vector2f pan = OrbitCamera::panned(Eigen::Vector2f::Zero(), 10.0, -5.0, 0.3f);
    CHECK(near(pan.x(), -0.006) && near(pan.y(), -0.003));
}

//=============================================================================================================
// Ray picking
//=============================================================================================================

void test_ray_picking()
{
    OrbitCamera camera;
    camera.set_scene_center({0.01f, 0.02f, 0.03f});
    const Eigen::Matrix4f pvm = camera.single_view(rotation_about_axis({0, 1, 0}, 30.0f), 2.0f, 4.0f / 3.0f).pvm();

    // Reference values of the former RayPicker::unproject
    const std::optional<Ray> ray = ray_from_pixel({250, 170}, {100, 50, 800, 600}, pvm);
    CHECK(ray.has_value());
    if (ray) {
        CHECK(near(ray->origin.x(), -0.0399460, 1e-5) && near(ray->origin.y(), 0.3150856, 1e-5) &&
              near(ray->origin.z(), 0.3203799, 1e-5));
        CHECK(near(ray->direction.x(), 0.4984232, 1e-5) && near(ray->direction.y(), -0.5825858, 1e-5) &&
              near(ray->direction.z(), -0.6420032, 1e-5));
    }

    // The centre of a pane looks down the view axis
    const Eigen::Matrix4f straight = perspective_matrix(60, 4.0f / 3.0f, 0.1f, 100) * look_at_matrix({0, 0, 5}, {0, 0, 0}, {0, 1, 0});
    const std::optional<Ray> centre = ray_from_pixel({400, 300}, {0, 0, 800, 600}, straight);
    CHECK(centre && centre->direction.isApprox(Eigen::Vector3f(0, 0, -1), 1e-5f));
    CHECK(!ray_from_pixel({0, 0}, {0, 0, 800, 600}, Eigen::Matrix4f::Zero()));

    // Triangle in the plane z = 1
    const Ray down{{0.2f, 0.2f, 5.0f}, {0, 0, -1}};
    const Eigen::Vector3f a(0, 0, 1);
    const Eigen::Vector3f b(1, 0, 1);
    const Eigen::Vector3f c(0, 1, 1);
    const std::optional<double> hit = intersect_ray_triangle(down, a, b, c);
    CHECK(hit && near(*hit, 4.0));
    CHECK(intersect_ray_triangle(Ray{{0.0f, 0.0f, 5.0f}, {0, 0, -1}}, a, b, c).has_value()); // on a corner
    CHECK(!intersect_ray_triangle(Ray{{0.8f, 0.8f, 5.0f}, {0, 0, -1}}, a, b, c));            // outside
    CHECK(!intersect_ray_triangle(Ray{{0.2f, 0.2f, 0.0f}, {0, 0, -1}}, a, b, c));            // behind
    CHECK(!intersect_ray_triangle(Ray{{0.2f, 0.2f, 5.0f}, {1, 0, 0}}, a, b, c));             // parallel

    // Sphere of radius 1 at z = -3
    const std::optional<SphereCrossing> through = intersect_ray_sphere(Ray{{0, 0, 0}, {0, 0, -1}}, {0, 0, -3}, 1.0f);
    CHECK(through && near(through->entry, 2.0) && near(through->exit, 4.0));
    const std::optional<SphereCrossing> inside = intersect_ray_sphere(Ray{{0, 0, -3}, {0, 0, -1}}, {0, 0, -3}, 1.0f);
    CHECK(inside && near(inside->entry, -1.0) && near(inside->exit, 1.0));
    CHECK(!intersect_ray_sphere(Ray{{2, 0, 0}, {0, 0, -1}}, {0, 0, -3}, 1.0f));
}

//=============================================================================================================
// Pane layout
//=============================================================================================================

void test_layout()
{
    PaneLayout layout;
    const PixelSize size{800, 600};

    CHECK(layout.slot_rect(0, 1, size) == (PixelRect{0, 0, 800, 600}));
    CHECK(layout.slot_rect(0, 2, size) == (PixelRect{0, 0, 400, 600}));
    CHECK(layout.slot_rect(1, 2, size) == (PixelRect{400, 0, 400, 600}));
    CHECK(layout.slot_rect(0, 3, size) == (PixelRect{0, 0, 800, 300}));
    CHECK(layout.slot_rect(2, 3, size) == (PixelRect{400, 300, 400, 300}));
    CHECK(layout.slot_rect(3, 4, size) == (PixelRect{400, 300, 400, 300}));

    // The right and bottom edges belong to the next pane
    const std::array<int, 4> viewports = {0, 1, 2, 3};
    CHECK(layout.viewport_index_at({399, 299}, viewports, size) == 0);
    CHECK(layout.viewport_index_at({400, 299}, viewports, size) == 1);
    CHECK(layout.viewport_index_at({400, 300}, viewports, size) == 3);
    CHECK(layout.viewport_index_at({900, 300}, viewports, size) == -1);
    const std::array<int, 2> two = {2, 5};
    CHECK(layout.viewport_index_at({500, 10}, two, size) == 5);

    // Splitters grab within 6 pixels; with three panes the vertical one only divides the bottom row
    CHECK(layout.hit_test_splitter({400, 100}, 1, size) == SplitterHit::None);
    CHECK(layout.hit_test_splitter({405, 100}, 2, size) == SplitterHit::Vertical);
    CHECK(layout.hit_test_splitter({407, 100}, 2, size) == SplitterHit::None);
    CHECK(layout.hit_test_splitter({400, 100}, 3, size) == SplitterHit::None);
    CHECK(layout.hit_test_splitter({400, 400}, 3, size) == SplitterHit::Vertical);
    CHECK(layout.hit_test_splitter({100, 303}, 4, size) == SplitterHit::Horizontal);
    CHECK(layout.hit_test_splitter({402, 298}, 4, size) == SplitterHit::Both);
    CHECK(PaneLayout::cursor_for_hit(SplitterHit::Both) == CursorShape::SizeAll);
    CHECK(PaneLayout::cursor_for_hit(SplitterHit::Horizontal) == CursorShape::SizeVertical);
    CHECK(PaneLayout::cursor_for_hit(SplitterHit::None) == CursorShape::Default);

    // The 3-D scene leaves the separator lines to the right and below a pane
    CHECK(layout.inset_for_separator(layout.slot_rect(0, 4, size), 0, 4) == (PixelRect{0, 0, 398, 298}));
    CHECK(layout.inset_for_separator(layout.slot_rect(1, 3, size), 1, 3) == (PixelRect{0, 300, 398, 300}));
    CHECK(layout.inset_for_separator(layout.slot_rect(3, 4, size), 3, 4) == (PixelRect{400, 300, 400, 300}));
    CHECK(layout.inset_for_separator(layout.slot_rect(1, 3, size), 2, 3) == layout.slot_rect(1, 3, size));

    PaneLayout::Separators separators = layout.separator_geometries(3, size);
    CHECK(separators.horizontal == (PixelRect{0, 299, 800, 2}));
    CHECK(separators.vertical == (PixelRect{399, 300, 2, 300}));
    CHECK(layout.separator_geometries(1, size).vertical.empty());
    CHECK(layout.separator_geometries(2, size).horizontal.empty());

    // Dragging keeps the minimum pane size and the 0.15 .. 0.85 range
    layout.drag_splitter({20, 450}, SplitterHit::Both, size);
    CHECK(near(layout.split_x(), 0.15) && near(layout.split_y(), 0.75));
    CHECK(layout.slot_rect(0, 2, size).width == 120);
    layout.drag_splitter({790, 10}, SplitterHit::Vertical, size);
    CHECK(near(layout.split_x(), 0.85) && near(layout.split_y(), 0.75));
    layout.reset_splits();
    CHECK(near(layout.split_x(), 0.5));
}

//=============================================================================================================
// Interaction
//=============================================================================================================

struct Recorder
{
    int redraws = 0;
    int layout_changes = 0;
    int saves = 0;
    int snapshots = 0;
    CursorShape cursor = CursorShape::Default;
    std::vector<int> selected;
    std::vector<int> menus;
    std::vector<Point> hovers;
    std::vector<Point> clicks;
    PixelRect label{8, 8, 40, 20};

    BrainViewInteraction::Effects effects()
    {
        BrainViewInteraction::Effects e;
        e.redraw = [this] {
            ++redraws;
        };
        e.layout_changed = [this] {
            ++layout_changes;
        };
        e.settings_changed = [this] {
            ++saves;
        };
        e.cursor_changed = [this](CursorShape shape) {
            cursor = shape;
        };
        e.viewport_selected = [this](int vp) {
            selected.push_back(vp);
        };
        e.hits_viewport_label = [this](int vp, Point p) {
            return vp == 0 && label.contains(p);
        };
        e.preset_menu_requested = [this](int vp, Point) {
            menus.push_back(vp);
        };
        e.hovered = [this](Point p) {
            hovers.push_back(p);
        };
        e.clicked = [this](Point p) {
            clicks.push_back(p);
        };
        e.double_clicked = [](Point p) {
            return p.x < 100;
        };
        e.snapshot_requested = [this] {
            ++snapshots;
        };
        return e;
    }
};

void test_interaction_single_view()
{
    PaneCamera single;
    std::vector<PaneCamera> panes(4);
    std::vector<PaneCamera*> pointers;
    pointers.reserve(panes.size());
    for (PaneCamera& pane : panes) {
        pointers.push_back(&pane);
    }
    Recorder recorder;
    BrainViewInteraction interaction;
    interaction.set_effects(recorder.effects());
    interaction.attach_cameras(&single, pointers);
    interaction.set_view_size({400, 300});
    interaction.set_panes(false, {0});

    // A drag orbits the single view and is not a click
    interaction.handle_pointer(pointer(PointerEvent::Type::Press, 200, 150, MouseButton::Left));
    interaction.handle_pointer(drag(210.4, 155));
    CHECK(near_quat(single.rotation, 0.9988104, 0.0059023, 0.0309535, -0.0372112));
    CHECK(recorder.redraws == 1);
    interaction.handle_pointer(pointer(PointerEvent::Type::Release, 210, 155, MouseButton::Left));
    CHECK(recorder.clicks.empty());

    // Without a held button a move hovers, also when a release went missing
    interaction.handle_pointer(pointer(PointerEvent::Type::Press, 210, 155, MouseButton::Left));
    interaction.handle_pointer(pointer(PointerEvent::Type::Move, 240, 155));
    CHECK(recorder.hovers.size() == 1);
    CHECK(recorder.redraws == 1);
    recorder.hovers.clear();
    CHECK(recorder.saves == 0);

    // A click without motion reports the point; hovering reports moves without a button
    interaction.handle_pointer(pointer(PointerEvent::Type::Press, 50, 60, MouseButton::Left));
    interaction.handle_pointer(pointer(PointerEvent::Type::Release, 50, 60, MouseButton::Left));
    CHECK(recorder.clicks.size() == 1 && recorder.clicks[0] == (Point{50, 60}));
    interaction.handle_pointer(pointer(PointerEvent::Type::Move, 70.6, 80.2));
    CHECK(recorder.hovers.size() == 1 && recorder.hovers[0] == (Point{71, 80}));
    CHECK(interaction.handle_pointer(pointer(PointerEvent::Type::DoubleClick, 50, 60, MouseButton::Left)));
    CHECK(!interaction.handle_pointer(pointer(PointerEvent::Type::DoubleClick, 150, 60, MouseButton::Left)));
    interaction.handle_pointer(pointer(PointerEvent::Type::Release, 150, 60, MouseButton::Left));

    // The right button does not orbit
    const Eigen::Quaternionf before = single.rotation;
    interaction.handle_pointer(pointer(PointerEvent::Type::Press, 50, 60, MouseButton::Right));
    PointerEvent right_drag = pointer(PointerEvent::Type::Move, 90, 60);
    right_drag.buttons = MouseButtons::Right;
    interaction.handle_pointer(right_drag);
    CHECK(single.rotation.isApprox(before));
    interaction.handle_pointer(pointer(PointerEvent::Type::Release, 90, 60, MouseButton::Right));

    // The wheel zooms one step per notch
    WheelEvent wheel;
    wheel.x = 10;
    wheel.y = 10;
    wheel.angle_delta_y = 240;
    interaction.handle_wheel(wheel);
    CHECK(near(single.zoom, 2.0));
    CHECK(near(panes[0].zoom, 0.0));
    CHECK(recorder.saves == 1);

    // R resets the rotation, S asks for a snapshot, other keys are not used
    KeyEvent key;
    key.key = Key::Character;
    key.character = U'r';
    CHECK(interaction.handle_key(key));
    CHECK(single.rotation.isApprox(Eigen::Quaternionf::Identity()));
    key.character = U'S';
    CHECK(interaction.handle_key(key));
    CHECK(recorder.snapshots == 1);
    key.character = U'x';
    CHECK(!interaction.handle_key(key));

    // The automated rotation orbits the single view
    interaction.orbit_perspective_views(10, 5);
    CHECK(near_quat(single.rotation, 0.9988104, 0.0059023, 0.0309535, -0.0372112));
}

void test_interaction_multi_view()
{
    PaneCamera single;
    std::vector<PaneCamera> panes(4);
    std::vector<PaneCamera*> pointers;
    for (int i = 0; i < 4; ++i) {
        panes[static_cast<std::size_t>(i)].preset = i; // Top, Perspective, Front, Left
        pointers.push_back(&panes[static_cast<std::size_t>(i)]);
    }
    Recorder recorder;
    BrainViewInteraction interaction;
    interaction.set_effects(recorder.effects());
    interaction.attach_cameras(&single, pointers);
    interaction.set_view_size({800, 600});
    interaction.set_panes(true, {0, 1, 2, 3});
    interaction.set_scene_size(0.3f);
    CHECK(interaction.viewport_at({500, 100}) == 1);

    // A press on a pane name selects the pane and asks for the preset menu
    interaction.handle_pointer(pointer(PointerEvent::Type::Press, 20, 15, MouseButton::Left));
    CHECK(recorder.menus.size() == 1 && recorder.menus[0] == 0);
    CHECK(recorder.selected.size() == 1 && recorder.selected[0] == 0);
    interaction.handle_pointer(pointer(PointerEvent::Type::Release, 20, 15, MouseButton::Left));
    recorder.clicks.clear();

    // Dragging in the Top pane pans it, in the perspective pane orbits it
    interaction.handle_pointer(pointer(PointerEvent::Type::Press, 100, 100, MouseButton::Left));
    interaction.handle_pointer(drag(110, 95));
    CHECK(near(panes[0].pan.x(), -0.006) && near(panes[0].pan.y(), -0.003));
    CHECK(panes[0].rotation.isApprox(Eigen::Quaternionf::Identity()));
    const int saves = recorder.saves;
    interaction.handle_pointer(pointer(PointerEvent::Type::Release, 110, 95, MouseButton::Left));
    CHECK(recorder.saves == saves + 1);
    CHECK(recorder.clicks.empty());

    interaction.handle_pointer(pointer(PointerEvent::Type::Press, 600, 100, MouseButton::Left));
    CHECK(recorder.selected.back() == 1);
    interaction.handle_pointer(drag(610, 105));
    CHECK(near_quat(panes[1].rotation, 0.9988104, 0.0059023, 0.0309535, -0.0372112));
    CHECK(panes[1].pan.isZero());
    interaction.handle_pointer(pointer(PointerEvent::Type::Release, 610, 105, MouseButton::Left));

    // Hovering a splitter changes the cursor; dragging it resizes the panes and is saved at the end
    interaction.handle_pointer(pointer(PointerEvent::Type::Move, 402, 100));
    CHECK(recorder.cursor == CursorShape::SizeHorizontal);
    interaction.handle_pointer(pointer(PointerEvent::Type::Move, 402, 299));
    CHECK(recorder.cursor == CursorShape::SizeAll);
    interaction.handle_pointer(pointer(PointerEvent::Type::Press, 402, 100, MouseButton::Left));
    CHECK(interaction.dragging_splitter());
    interaction.handle_pointer(drag(480, 100));
    CHECK(recorder.layout_changes == 1);
    CHECK(near(interaction.layout().split_x(), 0.6));
    const int saves_before_drop = recorder.saves;
    interaction.handle_pointer(pointer(PointerEvent::Type::Release, 480, 100, MouseButton::Left));
    CHECK(!interaction.dragging_splitter());
    CHECK(recorder.saves == saves_before_drop + 1);
    CHECK(recorder.clicks.empty());

    // The wheel zooms the pane under the pointer only
    WheelEvent wheel;
    wheel.x = 700;
    wheel.y = 500;
    wheel.angle_delta_y = -120;
    interaction.handle_wheel(wheel);
    CHECK(near(panes[3].zoom, -1.0) && near(panes[0].zoom, 0.0) && near(single.zoom, 0.0));

    // The automated rotation orbits the perspective panes only
    const Eigen::Quaternionf top = panes[0].rotation;
    interaction.orbit_perspective_views(10, 0);
    CHECK(panes[0].rotation.isApprox(top));
    CHECK(!panes[1].rotation.isApprox(Eigen::Quaternionf(0.9988104f, 0.0059023f, 0.0309535f, -0.0372112f)));

    interaction.handle_pointer(pointer(PointerEvent::Type::Press, 480, 100, MouseButton::Left));
    interaction.cancel_splitter_drag();
    CHECK(!interaction.dragging_splitter());
    CHECK(recorder.cursor == CursorShape::Default);
}

//=============================================================================================================
// Overlay
//=============================================================================================================

void test_overlay()
{
    const ApproximateTextMetrics metrics;
    BrainViewOverlay overlay(4);
    CHECK(overlay.item(BrainViewOverlay::Item::ViewportNameLabel, 4) == nullptr);
    CHECK(overlay.set_text(BrainViewOverlay::Item::FpsLabel, "FPS: 60.0\nVertices: 327680"));
    CHECK(!overlay.set_text(BrainViewOverlay::Item::FpsLabel, "FPS: 60.0\nVertices: 327680"));
    overlay.set_text(BrainViewOverlay::Item::SingleViewInfoLabel, "Shader: Standard\nSurface: pial\nOverlay: Surface\nDrawn: lh_pial, rh_pial");
    for (int vp = 0; vp < 4; ++vp) {
        overlay.set_text(BrainViewOverlay::Item::ViewportInfoLabel, "Shader: Anatomical\nSurface: pial", vp);
    }

    // Single view: FPS bottom right, info top right, no pane labels or separators
    const std::array<int, 1> single = {0};
    const std::array<int, 4> presets = {0, 1, 2, 3};
    BrainViewOverlay::LayoutInput input;
    input.size = {640, 480};
    input.viewports = single;
    input.presets = presets;
    overlay.update_layout(input, metrics);
    const BrainViewOverlay::Label* fps = overlay.item(BrainViewOverlay::Item::FpsLabel);
    CHECK(fps->visible);
    CHECK(fps->geometry.x + fps->geometry.width == 640 - 10);
    CHECK(fps->geometry.y + fps->geometry.height == 480 - 2);
    const PixelSize fps_size = BrainViewOverlay::label_size(overlay.style(BrainViewOverlay::Item::FpsLabel), fps->text, metrics);
    CHECK(fps_size.height == static_cast<int>(std::ceil(2 * metrics.height(overlay.style(BrainViewOverlay::Item::FpsLabel).font))) + 10);
    const BrainViewOverlay::Label* info = overlay.item(BrainViewOverlay::Item::SingleViewInfoLabel);
    CHECK(info->visible && info->geometry.y == 8 && info->geometry.x + info->geometry.width == 640 - 8);
    CHECK(!overlay.item(BrainViewOverlay::Item::ViewportNameLabel, 0)->visible);
    CHECK(!overlay.item(BrainViewOverlay::Item::VerticalSeparator)->visible);

    // The region label shows the last non-empty name
    overlay.set_region("Region: precentral (lh)");
    CHECK(overlay.item(BrainViewOverlay::Item::RegionLabel)->visible);
    overlay.set_region("");
    CHECK(!overlay.item(BrainViewOverlay::Item::RegionLabel)->visible);
    CHECK(overlay.item(BrainViewOverlay::Item::RegionLabel)->text == "Region: precentral (lh)");

    // Multi view with four panes, the second being edited
    PaneLayout layout;
    const std::array<int, 4> four = {0, 1, 2, 3};
    input.multi_view = true;
    input.viewports = four;
    input.edit_target = 1;
    input.layout = &layout;
    overlay.set_region("Left Hemisphere");
    overlay.update_layout(input, metrics);
    CHECK(!overlay.item(BrainViewOverlay::Item::SingleViewInfoLabel)->visible);
    const BrainViewOverlay::Label* name = overlay.item(BrainViewOverlay::Item::ViewportNameLabel, 3);
    CHECK(name->visible && name->text == "Left" && name->geometry.x == 328 && name->geometry.y == 248);
    CHECK(overlay.item(BrainViewOverlay::Item::ViewportNameLabel, 1)->selected);
    CHECK(!overlay.item(BrainViewOverlay::Item::ViewportNameLabel, 0)->selected);
    CHECK(overlay.item(BrainViewOverlay::Item::RegionLabel)->geometry == (PixelRect{10, 38, 300, 30}));
    CHECK(overlay.item(BrainViewOverlay::Item::VerticalSeparator)->geometry == (PixelRect{319, 0, 2, 480}));
    CHECK(overlay.item(BrainViewOverlay::Item::HorizontalSeparator)->visible);

    const std::string svg = to_svg(overlay.draw(640, 480, metrics));
    CHECK(svg.find(">Perspective<") != std::string::npos);
    CHECK(svg.find(">Vertices: 327680<") != std::string::npos);
    CHECK(matches_golden(svg, GOLDEN_DIR "/brain_view_overlay.svg"));

    // Without the info panel only the pane names, separators and region remain
    input.info_visible = false;
    overlay.update_layout(input, metrics);
    CHECK(!overlay.item(BrainViewOverlay::Item::FpsLabel)->visible);
    CHECK(!overlay.item(BrainViewOverlay::Item::ViewportInfoLabel, 2)->visible);
    CHECK(to_svg(overlay.draw(640, 480, metrics)).find("Vertices") == std::string::npos);
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main()
{
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"presets", test_presets},
        {"matrices", test_matrices},
        {"single_view", test_single_view},
        {"pane_view", test_pane_view},
        {"orbit_and_pan", test_orbit_and_pan},
        {"ray_picking", test_ray_picking},
        {"layout", test_layout},
        {"interaction_single_view", test_interaction_single_view},
        {"interaction_multi_view", test_interaction_multi_view},
        {"overlay", test_overlay},
    };
    for (const auto& [name, test] : tests) {
        const int before = g_failures;
        test();
        std::printf("%s %s\n", g_failures == before ? "PASS" : "FAIL", name);
    }
    std::printf("%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
