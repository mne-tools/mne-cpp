//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_viewmodels.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Tests the toolkit-independent view models ChannelTable, SpectrumTable and BidsTree without Qt.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_info.h>
#include <mne/viz/models/bids_tree.h>
#include <mne/viz/models/channel_table.h>
#include <mne/viz/models/spectrum_table.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
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

std::shared_ptr<FiffInfo> make_info()
{
    auto info = std::make_shared<FiffInfo>();
    info->sfreq = 200.0;
    const auto add = [&](const std::string& name, int kind, int unit) {
        FiffChInfo ch;
        ch.ch_name = name;
        ch.kind = kind;
        ch.unit = unit;
        ch.chpos.r0 = Eigen::Vector3f(0.01f, -0.02f, 0.035f);
        info->chs.push_back(ch);
        info->ch_names.push_back(name);
    };
    add("MEG0113", FIFFV_MEG_CH, FIFF_UNIT_T_M);
    add("MEG 0111", FIFFV_MEG_CH, FIFF_UNIT_T);
    add("EEG-001", FIFFV_EEG_CH, FIFF_UNIT_V);
    add("STI014", FIFFV_STIM_CH, FIFF_UNIT_NONE);
    info->nchan = static_cast<int>(info->chs.size());
    info->bads = {"EEG-001"};
    return info;
}

void test_channel_table()
{
    ChannelTable table;
    CHECK(table.row_count() == 0);
    CHECK(table.text(0, ChannelTable::Column::OriginalName).empty());

    table.set_info(make_info());
    CHECK(table.row_count() == 4);
    CHECK((table.mapped_names() == std::vector<std::string>{"MEG 0113", "MEG 0111", "EEG 001", "STI014"}));
    CHECK(table.aliases() == table.info()->ch_names);
    CHECK(table.index_of_original("STI014") == 3);
    CHECK(table.index_of_mapped("EEG 001") == 2);
    CHECK(table.index_of_mapped("EEG-001") == -1);

    CHECK(table.meg_type(0) == "MEG_grad" && table.meg_type(1) == "MEG_mag" && table.meg_type(2) == "non_MEG");
    CHECK(table.is_bad(2) && !table.is_bad(0));
    CHECK(table.bad_channels() == StringList{"EEG-001"});
    CHECK(table.text(3, ChannelTable::Column::Number) == "3");
    CHECK(table.text(2, ChannelTable::Column::Bad) == "true");
    CHECK(table.text(0, ChannelTable::Column::Digitizer) == "(1|-2|3.5)");
    CHECK(table.text(0, ChannelTable::Column::Position) == "(0|0)");
    CHECK(ChannelTable::header(ChannelTable::Column::Compensators) == "# of Compensators");

    table.set_layout({{"MEG 0113", {0.25, -0.5}}});
    CHECK(table.text(0, ChannelTable::Column::Position) == "(0.25|-0.5)");
    CHECK(table.position(1)[0] == 0.0);

    table.clear();
    CHECK(table.row_count() == 0 && table.mapped_names().empty());
}

void test_spectrum_table()
{
    SpectrumTable table;
    CHECK(!table.set_bounds(0.0f, 10.0f));

    table.set_info(make_info());
    CHECK(table.row_count() == 4 && table.channel(3) == 3);
    CHECK(table.channel_name(1) == "MEG 0111");
    CHECK(!table.spectrum(0).has_value());

    Eigen::MatrixXd data = Eigen::MatrixXd::Random(4, 5);
    table.set_data(data);
    CHECK(table.bins() == 5);
    // 100 Hz Nyquist over 5 bins: 0, 20, ..., 80 Hz, normalised to the last bin
    CHECK(std::abs(table.freq_scale()[1] - 0.25) < 1e-12 && table.freq_scale()[4] == 1.0);
    CHECK(table.spectrum(2)->isApprox(data.row(2)));

    table.select({3, 0, 7});
    CHECK(table.row_count() == 2 && table.channel(0) == 3 && table.channel(1) == 0);

    table.toggle_frozen();
    table.set_data(Eigen::MatrixXd::Zero(4, 5));
    CHECK(table.frozen() && table.spectrum(0)->isApprox(data.row(3)));
    table.toggle_frozen();
    CHECK(table.spectrum(0)->isZero());

    // The normalised axis spans 0 to the Nyquist frequency: 0, 25, 50, 75, 100 Hz. Band 30-70 Hz runs from the
    // last bin below 30 Hz to the first one above 70 Hz.
    CHECK(table.set_bounds(30.0f, 70.0f));
    CHECK(table.lower_bin() == 1 && table.upper_bin() == 3);
    CHECK(table.freq_scale_bound()[1] == 0.0 && std::abs(table.freq_scale_bound()[3] - 1.0) < 1e-12);

    SpectrumTable log;
    log.set_info(make_info());
    log.set_scale(SpectrumTable::Scale::Log);
    log.set_data(Eigen::MatrixXd::Ones(1, 3));
    CHECK(std::abs(log.freq_scale()[1] - std::log10(100.0 / 3.0 + 1.0) / std::log10(2.0 * 100.0 / 3.0 + 1.0)) < 1e-12);
}

void test_bids_tree()
{
    CHECK(BidsTree::subject_name("01 a") == "sub-01a" && BidsTree::subject_name("sub-02") == "sub-02");
    CHECK(BidsTree::session_name("x") == "ses-x");
    CHECK(BidsTree::folder_name(BidsTree::Behavioral) == "beh" && BidsTree::folder_name(20) == "unknown");

    BidsTree tree;
    const int subject = tree.add_subject("01");
    const int session = tree.add_session(subject, "01");
    CHECK(tree.node(session).subject == subject && tree.node(session).parent == subject);

    const BidsTree::Placement raw = tree.add_data_to_session(session, "raw.fif", BidsTree::Functional);
    CHECK(raw.folder_created && tree.node(raw.folder).name == "func");
    const BidsTree::Placement raw2 = tree.add_data_to_session(session, "raw2.fif", BidsTree::Functional);
    CHECK(!raw2.folder_created && raw2.folder == raw.folder);
    CHECK((tree.path(raw2.item) == std::vector<int>{0, 0, 0, 1}));

    const int average = tree.add_to_data(raw.item, "avg", 20);
    CHECK(tree.node(average).session == session && tree.node(average).subject == subject);

    // Sessions go to every subject with the name, which are then numbered
    const int other = tree.add_subject("sub-01");
    const std::vector<int> sessions = tree.add_session("sub-01", "02");
    CHECK(sessions.size() == 2 && tree.node(subject).name == "sub-011" && tree.node(other).name == "sub-012");
    CHECK(tree.add_session("sub-01", "03").empty());

    // Moving the last item out of a folder removes the folder; the item keeps its id and sub-items
    const BidsTree::Placement moved = tree.move_data(raw.item, sessions[1]);
    CHECK(moved.item == raw.item && moved.folder_created);
    CHECK(tree.node(raw.item).session == sessions[1] && tree.node(average).subject == other);
    CHECK(tree.contains(raw.folder));
    tree.move_data(raw2.item, sessions[1]);
    CHECK(!tree.contains(raw.folder));
    CHECK(tree.node(moved.folder).children.size() == 2);

    tree.move_session(sessions[1], subject);
    CHECK(tree.node(raw.item).subject == subject && tree.node(sessions[1]).parent == subject);

    tree.rename(other, "sub-x");
    CHECK(tree.node(other).name == "sub-x");
    tree.remove(subject);
    CHECK(!tree.contains(subject) && !tree.contains(raw.item) && !tree.contains(average));
    CHECK((tree.subjects() == std::vector<int>{other}));
    CHECK(tree.node(other).children.empty() && (tree.path(other) == std::vector<int>{0}));
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main()
{
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"channel_table", test_channel_table},
        {"spectrum_table", test_spectrum_table},
        {"bids_tree", test_bids_tree},
    };
    for (const auto& [name, test] : tests) {
        const int before = g_failures;
        test();
        std::printf("%s %s\n", g_failures == before ? "PASS" : "FAIL", name);
    }
    std::printf("%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
