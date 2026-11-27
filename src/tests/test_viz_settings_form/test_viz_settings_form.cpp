//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_viz_settings_form.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests SettingsForm, the toolkit-neutral description of settings panels, without Qt.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/viz/forms/settings_form.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdio>
#include <functional>
#include <optional>
#include <string>
#include <utility>
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

void test_fields_and_values()
{
    SettingsForm form("Averaging");
    form.add_int("trials", "Number of trials", 10, 1, 100, 1, " trials", "General");
    form.add_double("prestim", "Pre-stimulus", 0.1, 0.0, 1.0, 0.01, 2, " s", "General");
    form.add_bool("baseline", "Baseline correction", false, "Baseline");
    form.add_choice("mode", "Mode", {{"mean", "Mean"}, {"median", "Median"}}, "median");
    form.add_text("name", "Name", "avg");
    form.add_action("reset", "Reset");
    form.add_custom("trials_view");
    CHECK(form.title() == "Averaging" && form.fields().size() == 7);
    CHECK((form.groups() == std::vector<std::string>{"General", "Baseline", ""}));

    // Defaults; actions and custom fields hold no value
    CHECK(form.get<int>("trials") == 10 && form.get<double>("prestim") == 0.1 && !form.get<bool>("baseline"));
    CHECK(form.get<std::string>("mode") == "median" && form.get<std::string>("name") == "avg");
    CHECK(!form.value("reset") && !form.value("trials_view") && !form.value("missing"));
    // Ints read as doubles; the wrong type gives the fallback
    CHECK(form.get<double>("trials") == 10.0 && form.get<int>("prestim", -1) == -1);

    // Numbers are clamped and rounded, choices must exist, types must fit
    CHECK(form.set_value("trials", 500) && form.get<int>("trials") == 100);
    CHECK(form.set_value("trials", 7.6) && form.get<int>("trials") == 8);
    CHECK(!form.set_value("trials", 8));
    CHECK(form.set_value("prestim", -2) && form.get<double>("prestim") == 0.0);
    CHECK(!form.set_value("mode", std::string("max")) && form.get<std::string>("mode") == "median");
    CHECK(!form.set_value("baseline", 1) && !form.set_value("missing", true));

    // Ranges and choices change; values follow
    form.set_range("trials", 20, 50);
    CHECK(form.get<int>("trials") == 20 && form.field("trials")->minimum == 20.0);
    form.set_range("trials", 30, 10);
    CHECK(form.field("trials")->maximum == 30.0);
    form.set_choices("mode", {{"mean", "Mean"}});
    CHECK(form.get<std::string>("mode") == "mean");
    form.set_label("trials", "Trials");
    CHECK(form.field("trials")->label == "Trials");

    // Bulk values and reset
    form.set_values({{"baseline", true}, {"name", std::string("evoked")}, {"unknown", 3}});
    CHECK(form.get<bool>("baseline") && form.get<std::string>("name") == "evoked");
    CHECK(form.values().size() == 5);
    form.reset();
    CHECK(!form.get<bool>("baseline") && form.get<std::string>("name") == "avg" && form.get<int>("trials") == 30);

    // A field with the key of another replaces it
    form.add_bool("name", "Named", true);
    CHECK(form.fields().size() == 7 && form.get<bool>("name"));
}

void test_conditions()
{
    SettingsForm form;
    form.add_bool("baseline", "Baseline correction", false);
    SettingField from;
    from.key = "from";
    from.kind = SettingField::Kind::Int;
    from.enabled_if = "baseline";
    from.visible_if = "!hidden";
    form.add(from);
    form.add_bool("hidden", "Hide", false);
    CHECK(!form.is_enabled("from") && form.is_visible("from"));
    form.set_value("baseline", true);
    form.set_value("hidden", true);
    CHECK(form.is_enabled("from") && !form.is_visible("from"));
    CHECK(form.is_enabled("baseline") && !form.is_enabled("missing"));
}

void test_flags()
{
    SettingsForm form;
    form.add_flag("offline");
    SettingField compute;
    compute.key = "compute";
    compute.kind = SettingField::Kind::Action;
    compute.visible_if = "offline";
    form.add(compute);
    form.add_bool("baseline", "Baseline correction", false);

    // A flag holds a value conditions read, but is neither shown nor stored
    const SettingField* flag = form.field("offline");
    CHECK(flag && flag->kind == SettingField::Kind::Bool && flag->hidden && !flag->persistent);
    CHECK(!form.is_visible("offline") && !form.is_visible("compute"));
    CHECK(form.set_value("offline", true) && form.get<bool>("offline"));
    CHECK(form.is_visible("compute") && !form.is_visible("offline"));
    CHECK(form.add_flag("live", true).default_value == SettingValue(true) && form.get<bool>("live"));
    // Ordinary fields are shown and stored
    CHECK(!form.field("baseline")->hidden && form.field("baseline")->persistent && form.is_visible("baseline"));
}

void test_listeners()
{
    SettingsForm form;
    form.add_int("n", "N", 1, 0, 10);
    form.add_action("go", "Go");
    std::vector<std::pair<std::string, std::optional<SettingValue>>> heard;
    std::vector<std::string> changedFields;
    const int id = form.add_listener([&heard](const std::string& key, const std::optional<SettingValue>& value) {
        heard.emplace_back(key, value);
    });
    form.add_structure_listener([&changedFields](const std::string& key) {
        changedFields.push_back(key);
    });

    form.set_value("n", 5);
    form.set_value("n", 5);
    form.trigger("go");
    form.trigger("n");
    CHECK(heard.size() == 2 && heard[0].first == "n" && heard[0].second == SettingValue(5));
    CHECK(heard[1].first == "go" && !heard[1].second);

    // A range change that clamps the value is heard by both kinds of listeners
    form.set_range("n", 0, 3);
    CHECK(heard.size() == 3 && heard[2].second == SettingValue(3));
    form.add_bool("flag", "Flag", false);
    CHECK((changedFields == std::vector<std::string>{"n", "flag"}));

    form.remove_listener(id);
    form.set_value("n", 1);
    CHECK(heard.size() == 3);
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main()
{
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"fields_and_values", test_fields_and_values},
        {"conditions", test_conditions},
        {"flags", test_flags},
        {"listeners", test_listeners},
    };
    for (const auto& [name, test] : tests) {
        const int before = g_failures;
        test();
        std::printf("%s %s\n", g_failures == before ? "PASS" : "FAIL", name);
    }
    std::printf("%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
