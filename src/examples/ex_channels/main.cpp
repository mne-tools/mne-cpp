//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Channels library: montages, layouts and selections.
 *
 * Every step checks its result (round trips, known electrode positions) and the
 * example exits non-zero on a mismatch.
 * All files are written to a temporary directory.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <channels/layoutloader.h>
#include <channels/layoutmaker.h>
#include <channels/standard_montage.h>
#include <channels/selectionio.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <format>
#include <optional>
#include <print>
#include <random>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

bool expect(bool condition, std::string_view what)
{
    std::println("{} {}", condition ? "  ok  " : "  FAIL", what);
    return condition;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main()
{
    const std::filesystem::path dir = std::filesystem::temp_directory_path() / std::format("mne_cpp_ex_channels_{}", std::random_device{}());
    std::filesystem::create_directories(dir);
    bool ok = true;

    //! [selection_io_usage]
    // MNE .sel: one "name:ch1|ch2|..." group per line, read back in file order
    const std::vector<ChannelSelection> selections{{"Left-temporal", {"MEG 0111", "MEG 0112", "MEG 0113"}}, {"Vertex", {"MEG 0711"}}};
    SelectionIO::write_sel(dir / "groups.sel", selections);
    const auto reread = SelectionIO::read_sel(dir / "groups.sel");
    //! [selection_io_usage]
    ok &= expect(reread && *reread == selections, "SelectionIO .sel round trip");

    //! [standard_montage_usage]
    const std::vector<ElectrodePosition> montage1020 = StandardMontage::montage(StandardMontage::System::Standard_1020);
    const std::optional<Vector3d> cz = StandardMontage::find_electrode("Cz"); // mne's standard_1005 positions, metres, MNI frame
    //! [standard_montage_usage]
    ok &= expect(montage1020.size() == StandardMontage::electrode_count(StandardMontage::System::Standard_1020) && cz && (*cz - Vector3d(0.0004009, -0.009167, 0.100244)).norm() < 1e-12,
                 std::format("10-20 montage: {} electrodes, Cz at (0.4, -9.2, 100.2) mm", montage1020.size()));

    //! [layout_maker_usage]
    // Azimuthal equidistant projection around the fitted sphere: Cz lands near the centre, the nose points up
    std::vector<Vector3d> positions;
    StringList names;
    for (const ElectrodePosition& electrode : montage1020) {
        positions.push_back(electrode.pos);
        names.push_back(electrode.name);
    }
    const LayoutOptions options{.radius = 20.0, .box_width = 5.0, .box_height = 4.0};
    const auto layout = LayoutMaker::make_layout(positions, options);
    if (layout)
        LayoutMaker::write_lout(dir / "1020.lout", *layout, names, options);
    //! [layout_maker_usage]
    if (layout) {
        const auto box = [&](std::string_view name) {
            return (*layout)[static_cast<std::size_t>(index_of(names, name))];
        };
        // Box corners: the box of an electrode at (x, y) starts at (x - w/2, y - h/2)
        const Vector2d czCentre = box("Cz") + Vector2d(2.5, 2.0);
        ok &= expect(czCentre.norm() < 1.0 && box("Fp1").y() > box("Cz").y() && box("O1").y() < box("Cz").y() && box("Fp1").x() < box("Cz").x() && box("T8").x() > box("Cz").x(),
                     std::format("LayoutMaker: Cz at ({:g}, {:g}), Fp1 front-left, O1 back, T8 right", czCentre.x(), czCentre.y()));
    } else {
        ok &= expect(false, "LayoutMaker projects the 10-20 montage");
    }

    std::ofstream(dir / "cap.elc") << "NumberPositions=\t2\nUnitPosition\tmm\nPositions\nFp1 :\t-29.4\t83.9\t-7.0\nCz :\t0.0\t0.0\t87.0\n"
                                      "Labels\nFp1\tCz\n";
    //! [layout_loader_usage]
    // ANT .elc electrode file: names, 3-D positions and their unit
    const std::optional<ElectrodeLayout> cap = LayoutLoader::read_elc(dir / "cap.elc");
    //! [layout_loader_usage]
    ok &= expect(cap && cap->names == StringList({"Fp1", "Cz"}) && cap->unit == "mm" && cap->positions_3d.size() == 2 && cap->positions_3d[1] == Vector3d(0.0, 0.0, 87.0),
                 "LayoutLoader reads the electrode names, positions and unit of an .elc file");

    std::filesystem::remove_all(dir);
    std::println("{}", ok ? "All channels checks passed." : "channels checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
