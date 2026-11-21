//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_dig_montage.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Implementation of @ref FiffDigMontage: the digitizer file readers of mne.channels.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_dig_montage.h"

#include "fiff_constants.h"
#include "fiff_dig_point.h"
#include <base/xml.h>
#include <base/log.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <regex>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;
using mne::ErrorCode;
using mne::Expected;
using mne::fail;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

// Metres per file unit, as mne's _check_unit_and_get_scaling.
std::optional<double> unitScale(std::string_view unit)
{
    if (unit == "m")
        return 1.0;
    if (unit == "cm")
        return 1e-2;
    if (unit == "mm")
        return 1e-3;
    return std::nullopt;
}

// The whitespace-separated words of a text.
std::vector<std::string_view> words(std::string_view text)
{
    std::vector<std::string_view> result;
    std::size_t pos = 0;
    while ((pos = text.find_first_not_of(" \t\r\n\f\v", pos)) != std::string_view::npos) {
        const std::size_t end = std::min(text.find_first_of(" \t\r\n\f\v", pos), text.size());
        result.push_back(text.substr(pos, end - pos));
        pos = end;
    }
    return result;
}

// The whitespace-separated numbers of a text; nothing if any word is not one.
std::optional<std::vector<double>> numbers(std::string_view text)
{
    std::vector<double> values;
    for (const std::string_view word : words(text)) {
        const std::optional<double> value = mne::to_double(word);
        if (!value)
            return std::nullopt;
        values.push_back(*value);
    }
    return values;
}

// The text of a file.
Expected<std::string> readText(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return fail(ErrorCode::Io, "cannot read {}", mne::utf8_from_path(path));
    return std::string(std::istreambuf_iterator<char>(file), {});
}

// The fiducials (nasion, LPA, RPA) followed by the points, as mne's _read_isotrak_elp_points.
std::vector<Vector3d> readIsotrakElp(const std::string& text)
{
    static const std::regex coordinates(R"((-?\d+\.?\d*e?-?\d*)\s+(-?\d+\.?\d*e?-?\d*)\s+(-?\d+\.?\d*e?-?\d*)\s*$)");
    std::vector<Vector3d> points;
    for (const std::string& line : mne::split(text, '\n')) {
        std::smatch match;
        if (!std::regex_search(line, match, coordinates))
            continue;
        const std::optional<double> x = mne::to_double(match.str(1)), y = mne::to_double(match.str(2)), z = mne::to_double(match.str(3));
        if (x && y && z)
            points.emplace_back(*x, *y, *z);
    }
    return points;
}

// The same for an .hsp/.eeg file, as mne's _read_isotrak_hsp_points; empty on a malformed file.
std::vector<Vector3d> readIsotrakHsp(const std::string& text)
{
    const mne::StringList lines = mne::split(text, '\n');
    const auto fiducials = std::ranges::find_if(lines, [](const std::string& line) {
        return mne::to_lower(line).find("position of fiducials") != std::string::npos;
    });
    const std::size_t line = static_cast<std::size_t>(fiducials - lines.begin());
    std::vector<Vector3d> points;
    for (std::size_t i = 1; i <= 3; ++i) {
        if (line + i >= lines.size())
            return {};
        std::string row = lines[line + i];
        if (const std::size_t marker = row.find("%F"); marker != std::string::npos)
            row.erase(marker, 2);
        const std::optional<std::vector<double>> values = numbers(row);
        if (!values || values->size() != 3)
            return {};
        points.emplace_back((*values)[0], (*values)[1], (*values)[2]);
    }
    // A comment line, then "<number of points> <number of columns>" and the points
    const std::size_t header = line + 5;
    if (header >= lines.size())
        return points;
    const std::optional<std::vector<double>> shape = numbers(lines[header]);
    if (!shape || shape->size() != 2 || (*shape)[1] != 3.0)
        return {};
    std::string rest;
    for (std::size_t i = header + 1; i < lines.size(); ++i)
        rest.append(lines[i]).push_back('\n');
    const std::optional<std::vector<double>> values = numbers(rest);
    if (!values || static_cast<double>(values->size()) != 3.0 * (*shape)[0])
        return {};
    for (std::size_t i = 0; i + 2 < values->size(); i += 3)
        points.emplace_back((*values)[i], (*values)[i + 1], (*values)[i + 2]);
    return points;
}

// The points of an mne DigMontage in metres, before they become dig points.
struct Montage
{
    std::optional<Vector3d> nasion, lpa, rpa;
    std::vector<Vector3d> hpi, extra;
    std::vector<std::string> chNames;
    std::vector<Vector3d> chPos;

    // A repeated name keeps its first place and its last position, as mne's _check_dupes_odict.
    void addElectrode(std::string name, const Vector3d& r)
    {
        if (const auto it = std::ranges::find(chNames, name); it != chNames.end()) {
            mne::Log::warning("[FiffDigMontage] duplicate electrode {} - the last position is used", name);
            chPos[static_cast<std::size_t>(it - chNames.begin())] = r;
            return;
        }
        chNames.push_back(std::move(name));
        chPos.push_back(r);
    }

    // Removes the electrode @p name and makes it @p fiducial; false if there is none.
    bool takeFiducial(std::string_view name, std::optional<Vector3d>& fiducial)
    {
        if (name.empty())
            return true;
        const auto it = std::ranges::find(chNames, name);
        if (it == chNames.end())
            return false;
        const auto index = it - chNames.begin();
        fiducial = chPos[static_cast<std::size_t>(index)];
        chPos.erase(chPos.begin() + index);
        chNames.erase(it);
        return true;
    }
};

// The dig points of mne's make_dig_montage: cardinals, HPI coils, head shape, then electrodes,
// which keep the number at the end of their names when every name has one (1..n otherwise).
FiffDigMontage makeDigMontage(Montage montage)
{
    std::vector<FiffDigPoint> points;
    const auto append = [&points](int kind, int ident, const Vector3d& r) {
        FiffDigPoint point;
        point.kind = kind;
        point.ident = ident;
        point.coord_frame = FIFFV_COORD_UNKNOWN;
        for (int c = 0; c < 3; ++c)
            point.r[c] = static_cast<float>(r[c]);
        points.push_back(point);
    };
    if (montage.lpa)
        append(FIFFV_POINT_CARDINAL, FIFFV_POINT_LPA, *montage.lpa);
    if (montage.nasion)
        append(FIFFV_POINT_CARDINAL, FIFFV_POINT_NASION, *montage.nasion);
    if (montage.rpa)
        append(FIFFV_POINT_CARDINAL, FIFFV_POINT_RPA, *montage.rpa);
    for (std::size_t i = 0; i < montage.hpi.size(); ++i)
        append(FIFFV_POINT_HPI, static_cast<int>(i) + 1, montage.hpi[i]);
    for (std::size_t i = 0; i < montage.extra.size(); ++i)
        append(FIFFV_POINT_EXTRA, static_cast<int>(i) + 1, montage.extra[i]);

    std::vector<int> idents;
    for (const std::string& name : montage.chNames) {
        const std::optional<int> ident = mne::to_int(std::string_view(name).substr(name.size() - std::min<std::size_t>(name.size(), 3)));
        if (!ident) {
            idents.clear();
            break;
        }
        idents.push_back(*ident);
    }
    for (std::size_t i = 0; i < montage.chPos.size(); ++i)
        append(FIFFV_POINT_EEG, idents.empty() ? static_cast<int>(i) + 1 : idents[i], montage.chPos[i]);
    return {FiffDigPointSet(std::move(points)), std::move(montage.chNames)};
}

using XmlRecord = std::map<std::string, std::string, std::less<>>;

// The records of every element named @p element, outside other records, in document order.
void collectXmlRecords(const XmlElement& node, std::string_view element, std::vector<XmlRecord>& records)
{
    if (node.local_name() != element) {
        for (const XmlElement& child : node.children)
            collectXmlRecords(child, element, records);
        return;
    }
    XmlRecord record;
    for (const XmlElement& child : node.children)
        record.insert_or_assign(std::string(child.local_name()), child.text);
    records.push_back(std::move(record));
}

// The text of each child element of every <element> in an XML file (namespaces ignored).
Expected<std::vector<XmlRecord>> readXmlRecords(const std::filesystem::path& path, std::string_view element)
{
    const Expected<XmlElement> root = read_xml_file(path);
    if (!root)
        return std::unexpected(root.error());
    std::vector<XmlRecord> records;
    collectXmlRecords(*root, element, records);
    return records;
}

// A field of an XML record, empty if it has none.
std::string_view field(const XmlRecord& record, std::string_view key)
{
    const auto it = record.find(key);
    return it == record.end() ? std::string_view() : std::string_view(it->second);
}

// A position from the x/y/z fields of an XML record.
std::optional<Vector3d> recordPosition(const XmlRecord& record, const std::array<std::string_view, 3>& keys, double scale)
{
    Vector3d r;
    for (int c = 0; c < 3; ++c) {
        const std::optional<double> value = mne::to_double(field(record, keys[static_cast<std::size_t>(c)]));
        if (!value)
            return std::nullopt;
        r[c] = scale * *value;
    }
    return r;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Expected<FiffDigMontage> FiffDigMontage::read_polhemus_isotrak(const std::filesystem::path& path,
                                                               std::span<const std::string> ch_names,
                                                               std::string_view unit)
{
    const std::string file = mne::utf8_from_path(path);
    const std::optional<double> scale = unitScale(unit);
    if (!scale)
        return fail(ErrorCode::Format, "unknown unit {} (use m, cm or mm)", unit);
    const std::filesystem::path extension = path.extension();
    if (extension != ".hsp" && extension != ".elp" && extension != ".eeg")
        return fail(ErrorCode::Format, "{} is not an Isotrak .elp, .hsp or .eeg file", file);
    const Expected<std::string> text = readText(path);
    if (!text)
        return std::unexpected(text.error());
    std::vector<Vector3d> points = extension == ".elp" ? readIsotrakElp(*text) : readIsotrakHsp(*text);
    if (points.size() < 3)
        return fail(ErrorCode::Format, "{} does not hold the three fiducials", file);
    const std::size_t count = points.size() - 3;
    if (!ch_names.empty() && ch_names.size() != count)
        return fail(ErrorCode::Format, "{} holds {} points besides the fiducials, but {} names were given", file, count, ch_names.size());
    for (Vector3d& point : points)
        point *= *scale;

    Montage montage;
    montage.nasion = points[0];
    montage.lpa = points[1];
    montage.rpa = points[2];
    const std::span<const Vector3d> rest = std::span(points).subspan(3);
    if (!ch_names.empty()) {
        for (std::size_t i = 0; i < count; ++i)
            montage.addElectrode(ch_names[i], rest[i]);
    } else if (extension == ".elp") {
        montage.hpi.assign(rest.begin(), rest.end());
    } else {
        montage.extra.assign(rest.begin(), rest.end());
    }
    return makeDigMontage(std::move(montage));
}

//=============================================================================================================

Expected<MatrixX3d> FiffDigMontage::read_polhemus_fastscan(const std::filesystem::path& path, std::string_view unit, bool require_header)
{
    const std::string file = mne::utf8_from_path(path);
    const std::optional<double> scale = unitScale(unit);
    if (!scale)
        return fail(ErrorCode::Format, "unknown unit {} (use m, cm or mm)", unit);
    if (path.extension() != ".txt")
        return fail(ErrorCode::Format, "{} is not a FastSCAN .txt file", file);
    const Expected<std::string> text = readText(path);
    if (!text)
        return std::unexpected(text.error());
    const mne::StringList lines = mne::split(*text, '\n');

    std::string header;
    for (auto line = lines.begin(); line != lines.end() && line->starts_with('%'); ++line)
        header += *line;
    if (require_header && header.find("FastSCAN") == std::string::npos)
        return fail(ErrorCode::Format, "{} does not contain a valid Polhemus FastSCAN header", file);

    std::vector<double> values;
    for (const std::string& line : lines) {
        const std::optional<std::vector<double>> row = numbers(std::string_view(line).substr(0, line.find('%')));
        if (!row || (!row->empty() && row->size() != 3))
            return fail(ErrorCode::Format, "{} has a row that is not three numbers: {}", file, line);
        values.insert(values.end(), row->begin(), row->end());
    }
    MatrixX3d points(static_cast<Index>(values.size() / 3), 3);
    for (std::size_t i = 0; i < values.size(); ++i)
        points(static_cast<Index>(i / 3), static_cast<Index>(i % 3)) = *scale * values[i];
    return points;
}

//=============================================================================================================

Expected<FiffDigMontage> FiffDigMontage::read_captrak(const std::filesystem::path& path)
{
    const Expected<std::vector<XmlRecord>> electrodes = readXmlRecords(path, "CapTrakElectrode");
    if (!electrodes)
        return std::unexpected(electrodes.error());
    Montage montage;
    for (const XmlRecord& electrode : *electrodes) {
        const std::optional<Vector3d> r = recordPosition(electrode, {"X", "Y", "Z"}, 1e-3);
        if (!r)
            return fail(ErrorCode::Format, "electrode without a position in {}", mne::utf8_from_path(path));
        const std::string_view name = field(electrode, "Name");
        if (name == "Nasion")
            montage.nasion = r;
        else if (name == "LPA")
            montage.lpa = r;
        else if (name == "RPA")
            montage.rpa = r;
        else
            montage.addElectrode(std::string(name), *r);
    }
    if (!montage.nasion || !montage.lpa || !montage.rpa)
        return fail(ErrorCode::Format, "{} lacks the Nasion, LPA or RPA electrode", mne::utf8_from_path(path));
    return makeDigMontage(std::move(montage));
}

//=============================================================================================================

Expected<FiffDigMontage> FiffDigMontage::read_egi(const std::filesystem::path& path)
{
    const Expected<std::vector<XmlRecord>> sensors = readXmlRecords(path, "sensor");
    if (!sensors)
        return std::unexpected(sensors.error());
    Montage montage;
    for (const XmlRecord& sensor : *sensors) {
        const std::optional<int> number = mne::to_int(field(sensor, "number"));
        const std::optional<Vector3d> r = recordPosition(sensor, {"x", "y", "z"}, 1e-2);
        if (!number || !r)
            return fail(ErrorCode::Format, "sensor without a number or position in {}", mne::utf8_from_path(path));
        const std::string_view name = field(sensor, "name");
        switch (mne::to_int(field(sensor, "type")).value_or(-1)) {
            case 0: // EEG
                montage.addElectrode(std::format("EEG {:03}", *number), *r);
                break;
            case 1: // reference, numbered after the electrodes before it
                montage.addElectrode(std::format("EEG {:03}", montage.chNames.size() + 1), *r);
                break;
            case 2:
                if (name == "Nasion")
                    montage.nasion = r;
                else if (name == "Left periauricular point")
                    montage.lpa = r;
                else if (name == "Right periauricular point")
                    montage.rpa = r;
                break;
            default:
                mne::Log::warning("[FiffDigMontage::read_egi] skipping sensor {} of unknown type", *number);
        }
    }
    if (!montage.nasion || !montage.lpa || !montage.rpa)
        return fail(ErrorCode::Format, "{} lacks the nasion or a periauricular point", mne::utf8_from_path(path));
    return makeDigMontage(std::move(montage));
}

//=============================================================================================================

Expected<FiffDigMontage> FiffDigMontage::read_localite(const std::filesystem::path& path,
                                                       std::string_view nasion,
                                                       std::string_view lpa,
                                                       std::string_view rpa)
{
    const std::string file = mne::utf8_from_path(path);
    const Expected<std::string> text = readText(path);
    if (!text)
        return std::unexpected(text.error());
    const mne::StringList lines = mne::split(*text, '\n');
    Montage montage;
    for (std::size_t i = 1; i < lines.size(); ++i) {
        if (mne::trimmed(lines[i]).empty())
            continue;
        mne::StringList fields = mne::split(lines[i], ',');
        Vector3d r;
        bool ok = fields.size() == 5;
        for (int c = 0; ok && c < 3; ++c) {
            const std::optional<double> value = mne::to_double(fields[static_cast<std::size_t>(c) + 2]);
            ok = value.has_value();
            r[c] = value.value_or(0.0) / 1000.0;
        }
        if (!ok)
            return fail(ErrorCode::Format, "{} line {} is not '#,name,x,y,z': {}", file, i + 1, lines[i]);
        montage.addElectrode(std::move(fields[1]), r);
    }
    if (!montage.takeFiducial(nasion, montage.nasion) || !montage.takeFiducial(lpa, montage.lpa) || !montage.takeFiducial(rpa, montage.rpa))
        return fail(ErrorCode::NotFound, "{} has no point named {}, {} or {}", file, nasion, lpa, rpa);
    return makeDigMontage(std::move(montage));
}

//=============================================================================================================

Expected<FiffDigMontage> FiffDigMontage::read_neuroscan_dat(const std::filesystem::path& path)
{
    const std::string file = mne::utf8_from_path(path);
    const Expected<std::string> text = readText(path);
    if (!text)
        return std::unexpected(text.error());
    const mne::StringList lines = mne::split(*text, '\n');
    Montage montage;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::vector<std::string_view> items = words(lines[i]);
        if (items.empty())
            continue;
        Vector3d r;
        bool ok = items.size() == 5;
        for (int c = 0; ok && c < 3; ++c) {
            const std::optional<double> value = mne::to_double(items[static_cast<std::size_t>(c) + 2]);
            ok = value.has_value();
            r[c] = value.value_or(0.0);
        }
        if (!ok)
            return fail(ErrorCode::Format, "{} line {} is not 'name number x y z': {}", file, i, lines[i]);
        // The point number marks the fiducials (78 nasion, 76 LPA, 82 RPA) and the centroid (67)
        const std::string_view number = items[1];
        if (number == "78")
            montage.nasion = r;
        else if (number == "76")
            montage.lpa = r;
        else if (number == "82")
            montage.rpa = r;
        else if (number != "67")
            montage.addElectrode(std::string(items[0]), r);
    }
    return makeDigMontage(std::move(montage));
}
