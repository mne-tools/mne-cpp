//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_channels_layout_selection.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Tests for LayoutLoader, LayoutMaker, and SelectionIO — file I/O
 *           for 2D/3D electrode layouts and channel selection groups.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <channels/layoutloader.h>
#include <channels/layoutmaker.h>
#include <channels/selectionio.h>
#include <base/log.h>

#include <filesystem>
#include <fstream>
#include <sstream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QObject>
#include <QTemporaryDir>
#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

namespace fs = std::filesystem;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

std::string read_text(const fs::path& path)
{
    std::ifstream file(path);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

void write_text(const fs::path& path, std::string_view text)
{
    std::ofstream(path) << text;
}

// Four points on the equator of the unit sphere, scaled to 0.1
std::vector<Vector3d> equator()
{
    return {{0.1, 0.0, 0.0}, {0.0, 0.1, 0.0}, {-0.1, 0.0, 0.0}, {0.0, -0.1, 0.0}};
}

} // anonymous namespace

//=============================================================================================================
/**
 * @brief Tests for LayoutLoader, LayoutMaker, and SelectionIO.
 */
class TestChannelsLayoutSelection : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // LayoutLoader
    void testReadElcNamedPositions();
    void testReadElcLabelsAndPlainPositions();
    void testReadElcResourceFile();
    void testReadElcRejects();

    // LayoutMaker
    void testMakeLayoutNoFit();
    void testMakeLayoutWithFit();
    void testMakeLayoutMirror();
    void testMakeLayoutRejects();
    void testWriteLout();

    // SelectionIO
    void testSelRoundTrip();
    void testReadSelSkipsComments();
    void testReadSelRejects();
    void testMonRoundTrip();
    void testWriteMonRejectsPathNames();

private:
    fs::path tempPath(std::string_view name) const;

    QTemporaryDir m_tempDir;
};

//=============================================================================================================

void TestChannelsLayoutSelection::initTestCase()
{
    mne::Log::install_qt_message_handler();
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================

fs::path TestChannelsLayoutSelection::tempPath(std::string_view name) const
{
    return fs::path(m_tempDir.path().toStdString()) / name;
}

//=============================================================================================================
// LayoutLoader
//=============================================================================================================

void TestChannelsLayoutSelection::testReadElcNamedPositions()
{
    // ANT layout: "name : x y z" lines, the Labels block (upper case) wins over the prefixes
    const auto layout = LayoutLoader::parse_elc("# comment\n"
                                                "NumberPositions=\t2\n"
                                                "UnitPosition\tmm\n"
                                                "Positions\n"
                                                "Fp1:\t94.9\t30.7\t14.0\n"
                                                "Cz :\t0.0\t0.0\t87.0\n"
                                                "Labels\n"
                                                "FP1\tCZ\n"
                                                "Positions2D\n"
                                                "Fp1: 82.7\t29.37\n"
                                                "Cz : 0\t0\n");
    QVERIFY(layout);
    QCOMPARE(layout->names, StringList({"FP1", "CZ"}));
    QCOMPARE(layout->unit, std::string("mm"));
    QCOMPARE(layout->positions_3d.size(), std::size_t(2));
    QVERIFY((layout->positions_3d[0] - Vector3d(94.9, 30.7, 14.0)).norm() < 1e-12);
    QVERIFY((layout->positions_3d[1] - Vector3d(0.0, 0.0, 87.0)).norm() < 1e-12);
    QCOMPARE(layout->positions_2d.size(), std::size_t(2));
    QVERIFY((layout->positions_2d[0] - Vector2d(82.7, 29.37)).norm() < 1e-12);

    // Without a Labels block the names come from the position lines
    const auto unlabelled = LayoutLoader::parse_elc("Positions\nFp1 : 1 2 3\nCz : 4 5 6\n");
    QVERIFY(unlabelled);
    QCOMPARE(unlabelled->names, StringList({"Fp1", "Cz"}));
}

//=============================================================================================================

void TestChannelsLayoutSelection::testReadElcLabelsAndPlainPositions()
{
    // MNE-Python layout: plain "x y z" lines, then one label per line, Windows line ends
    const fs::path path = tempPath("plain.ELC");
    write_text(path, "ReferenceLabel\tavg\r\nUnitPosition\tm\r\nPositions\r\n1 2 3\r\n4 5 6\r\nLabels\r\nA1\r\nA2\r\n");
    const auto layout = LayoutLoader::read_elc(path);
    QVERIFY(layout);
    QCOMPARE(layout->names, StringList({"A1", "A2"}));
    QCOMPARE(layout->unit, std::string("m"));
    QVERIFY((layout->positions_3d[1] - Vector3d(4.0, 5.0, 6.0)).norm() < 1e-12);
    QVERIFY(layout->positions_2d.empty());
}

//=============================================================================================================

void TestChannelsLayoutSelection::testReadElcResourceFile()
{
    const fs::path path = fs::path(QCoreApplication::applicationDirPath().toStdString()) / "../resources/general/3DLayouts/standard_waveguard64.elc";
    if (!fs::exists(path))
        QSKIP("ELC file not available");

    const auto layout = LayoutLoader::read_elc(path);
    QVERIFY(layout);
    QCOMPARE(layout->names.size(), std::size_t(64));
    QCOMPARE(layout->positions_3d.size(), std::size_t(64));
    QCOMPARE(layout->positions_2d.size(), std::size_t(64));
    QCOMPARE(layout->names.front(), std::string("FP1"));
    QCOMPARE(layout->unit, std::string("mm"));

    // Every cap shipped with mne-cpp reads, with a name and a 2-D position for each electrode where given
    int files = 0;
    for (const auto& entry : fs::recursive_directory_iterator(path.parent_path().parent_path().parent_path())) {
        if (entry.path().extension() != ".elc")
            continue;
        ++files;
        const auto cap = LayoutLoader::read_elc(entry.path());
        QVERIFY2(cap, entry.path().string().c_str());
        QVERIFY2(cap->positions_2d.empty() || cap->positions_2d.size() == cap->positions_3d.size(), entry.path().string().c_str());
    }
    QVERIFY(files >= 9);
}

//=============================================================================================================

void TestChannelsLayoutSelection::testReadElcRejects()
{
    QVERIFY(!LayoutLoader::read_elc("/nonexistent.elc"));
    const fs::path wrongExtension = tempPath("cap.txt");
    write_text(wrongExtension, "Positions\nCz : 0 0 1\n");
    QVERIFY(!LayoutLoader::read_elc(wrongExtension));

    QVERIFY(!LayoutLoader::parse_elc(""));
    QVERIFY(!LayoutLoader::parse_elc("Positions\nCz : 0 zero 1\n"));
    QVERIFY(!LayoutLoader::parse_elc("Positions\n0 0 1\n1 0 0\nLabels\nCz\n"));
}

//=============================================================================================================
// LayoutMaker
//=============================================================================================================

void TestChannelsLayoutSelection::testMakeLayoutNoFit()
{
    const LayoutOptions options{.fit_sphere = false, .radius = 0.2, .box_width = 0.5, .box_height = 0.5};
    const auto corners = LayoutMaker::make_layout(equator(), options);
    QVERIFY(corners);
    QCOMPARE(corners->size(), std::size_t(4));

    // Equatorial points lie at 90 degrees from the pole, i.e. at radius prad, in their own direction
    // (box corner = centre - (w/2, h/2)), as in MNE-C mne_make_eeg_layout
    const Vector2d expected[4] = {{0.2, 0.0}, {0.0, 0.2}, {-0.2, 0.0}, {0.0, -0.2}};
    for (std::size_t i = 0; i < 4; ++i)
        QVERIFY(((*corners)[i] + Vector2d(0.25, 0.25) - expected[i]).norm() < 1e-12);
}

//=============================================================================================================

void TestChannelsLayoutSelection::testMakeLayoutWithFit()
{
    const std::vector<Vector3d> cap{{0.1, 0.0, 0.05}, {0.0, 0.1, 0.05}, {-0.1, 0.0, 0.05}, {0.0, -0.1, 0.05}, {0.05, 0.05, 0.07}};
    const auto corners = LayoutMaker::make_layout(cap, {.radius = 0.3, .box_width = 0.5, .box_height = 0.5});
    QVERIFY(corners);
    QCOMPARE(corners->size(), std::size_t(5));
}

//=============================================================================================================

void TestChannelsLayoutSelection::testMakeLayoutMirror()
{
    const LayoutOptions plain{.fit_sphere = false, .radius = 0.2, .box_width = 0.5, .box_height = 0.4};
    LayoutOptions mirrored = plain;
    mirrored.mirror_x = true;
    mirrored.mirror_y = true;

    const auto normal = LayoutMaker::make_layout(equator(), plain);
    const auto flipped = LayoutMaker::make_layout(equator(), mirrored);
    QVERIFY(normal && flipped);
    for (std::size_t i = 0; i < normal->size(); ++i)
        QVERIFY(((*flipped)[i] + (*normal)[i]).norm() < 1e-12);
}

//=============================================================================================================

void TestChannelsLayoutSelection::testMakeLayoutRejects()
{
    QVERIFY(!LayoutMaker::make_layout(std::vector<Vector3d>{}));
    const std::vector<Vector3d> coincident(3, Vector3d(0.0, 0.0, 0.1));
    QVERIFY(!LayoutMaker::make_layout(coincident, {.fit_sphere = false}));
}

//=============================================================================================================

void TestChannelsLayoutSelection::testWriteLout()
{
    const std::vector<Vector2d> corners{{-0.25, 1.5}, {2.0, -3.125}};
    const fs::path path = tempPath("layout.lout");
    QVERIFY(LayoutMaker::write_lout(path, corners, {"EEG 001"}, {.box_width = 0.5, .box_height = 0.4}));
    QCOMPARE(read_text(path), std::string("0.000000 0.000000 0.000000 0.000000\n"
                                          "1 -0.250000 1.500000 0.500000 0.400000 EEG 001\n"
                                          "2 2.000000 -3.125000 0.500000 0.400000\n"));

    QVERIFY(!LayoutMaker::write_lout(tempPath("missing/dir/layout.lout"), corners, {}));
}

//=============================================================================================================
// SelectionIO
//=============================================================================================================

void TestChannelsLayoutSelection::testSelRoundTrip()
{
    // File order survives, also when it is not alphabetical
    const std::vector<ChannelSelection> groups{{"Vertex", {"MEG 0711"}},
                                               {"Left-temporal", {"MEG 0111", "MEG 0121", "MEG 0131"}},
                                               {"Empty", {}}};
    const fs::path path = tempPath("groups.sel");
    QVERIFY(SelectionIO::write_sel(path, groups));
    const auto read = SelectionIO::read_sel(path);
    QVERIFY(read);
    QVERIFY(*read == groups);
}

//=============================================================================================================

void TestChannelsLayoutSelection::testReadSelSkipsComments()
{
    const fs::path path = tempPath("mne_browse_raw.sel");
    write_text(path, "%\r\n% Vectorview: comment with a colon\r\n\r\nVertex:MEG 0711|MEG 0712|\r\n");
    const auto read = SelectionIO::read_sel(path);
    QVERIFY(read);
    QCOMPARE(read->size(), std::size_t(1));
    QVERIFY(read->front() == ChannelSelection({"Vertex", {"MEG 0711", "MEG 0712"}}));
}

//=============================================================================================================

void TestChannelsLayoutSelection::testReadSelRejects()
{
    QVERIFY(!SelectionIO::read_sel("/nonexistent.sel"));
    const fs::path wrongExtension = tempPath("groups.txt");
    write_text(wrongExtension, "Vertex:MEG 0711|\n");
    QVERIFY(!SelectionIO::read_sel(wrongExtension));
    QVERIFY(!SelectionIO::write_sel(wrongExtension, {}));
}

//=============================================================================================================

void TestChannelsLayoutSelection::testMonRoundTrip()
{
    const std::vector<ChannelSelection> groups{{"Frontal", {"Fp1", "Fp2"}}, {"Occipital", {"O1"}}};
    const fs::path directory = tempPath("");
    QVERIFY(SelectionIO::write_mon(directory, groups));
    QCOMPARE(read_text(directory / "Frontal.mon"), std::string("Frontal\nFp1 : Fp1\nFp2 : Fp2\n"));

    const auto occipital = SelectionIO::read_mon(directory / "Occipital.mon");
    QVERIFY(occipital);
    QVERIFY(*occipital == groups[1]);
    QVERIFY(!SelectionIO::read_mon(directory / "Missing.mon"));
    QVERIFY(!SelectionIO::read_mon(directory / "groups.sel"));
}

//=============================================================================================================

void TestChannelsLayoutSelection::testWriteMonRejectsPathNames()
{
    // A group name is a file name; it must not escape the target directory
    const fs::path directory = tempPath("mon");
    fs::create_directories(directory);
    QVERIFY(!SelectionIO::write_mon(directory, std::vector<ChannelSelection>{{"../escape", {"Cz"}}}));
    QVERIFY(!fs::exists(tempPath("escape.mon")));
    QVERIFY(!SelectionIO::write_mon(directory, std::vector<ChannelSelection>{{"", {"Cz"}}}));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestChannelsLayoutSelection)
#include "test_channels_layout_selection.moc"
