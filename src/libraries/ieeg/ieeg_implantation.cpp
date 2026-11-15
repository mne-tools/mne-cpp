//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ieeg_implantation.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Lead grouping, ordering and typing of sEEG / ECoG contacts, and BIDS iEEG reading.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ieeg_implantation.h"

#include <bids/bids_coordinate_system.h>
#include <bids/bids_electrode.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <unordered_map>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE LOCAL FUNCTIONS
//=============================================================================================================

namespace
{

struct NameParts
{
    bool numbered = false;
    std::string prefix;
    int number = 0;
};

bool isDigit(char c)
{
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

bool isSpace(char c)
{
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

// "LA 12" -> ("LA", 12): the trailing number and the name before it without separators
NameParts splitName(const std::string& name)
{
    std::size_t digits = name.size();
    while (digits > 0 && isDigit(name[digits - 1]))
        --digits;
    if (digits == name.size())
        return {};
    std::size_t prefixEnd = digits;
    while (prefixEnd > 0 && (isSpace(name[prefixEnd - 1]) || name[prefixEnd - 1] == '_' || name[prefixEnd - 1] == '-'))
        --prefixEnd;
    return {true, name.substr(0, prefixEnd), mne::toInt(std::string_view(name).substr(digits)).value_or(0)};
}

// "[4x8]", "4 x 8" -> (4, 8)
std::optional<std::pair<int, int>> parseDimension(std::string_view text)
{
    std::size_t i = 0;
    auto skipSpace = [&] {
        while (i < text.size() && isSpace(text[i]))
            ++i;
    };
    auto number = [&]() -> std::optional<int> {
        const std::size_t start = i;
        while (i < text.size() && isDigit(text[i]))
            ++i;
        if (i == start)
            return std::nullopt;
        return mne::toInt(text.substr(start, i - start)).value_or(0);
    };
    if (i < text.size() && text[i] == '[')
        ++i;
    skipSpace();
    const std::optional<int> rows = number();
    skipSpace();
    if (!rows || i >= text.size() || text[i] != 'x')
        return std::nullopt;
    ++i;
    skipSpace();
    const std::optional<int> cols = number();
    skipSpace();
    if (cols && i < text.size() && text[i] == ']')
        ++i;
    if (!cols || i != text.size())
        return std::nullopt;
    return std::make_pair(*rows, *cols);
}

std::optional<float> toCoordinate(std::string_view text)
{
    const std::optional<double> value = mne::toDouble(mne::trimmed(text));
    if (!value || !std::isfinite(static_cast<float>(*value)))
        return std::nullopt;
    return static_cast<float>(*value);
}

bool endsWithIgnoreCase(const std::string& text, std::string_view suffix)
{
    return text.size() >= suffix.size() && mne::equalsIgnoreCase(std::string_view(text).substr(text.size() - suffix.size()), suffix);
}

void setError(std::string* error, std::string message)
{
    if (error)
        *error = std::move(message);
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

IeegImplantation::IeegImplantation(const std::vector<IeegContact>& contacts, const std::string& coordinateSystem)
: m_contacts(contacts)
, m_coordinateSystem(coordinateSystem)
{
    std::unordered_map<std::string, std::size_t> leadIndex;
    std::vector<std::vector<std::pair<int, int>>> members; // per lead: (number along the lead, contact index)
    for (std::size_t i = 0; i < m_contacts.size(); ++i) {
        IeegContact& contact = m_contacts[i];
        const NameParts parts = splitName(contact.name);
        if (contact.lead.empty())
            contact.lead = (parts.numbered && !parts.prefix.empty()) ? parts.prefix : contact.name;
        auto [it, inserted] = leadIndex.try_emplace(contact.lead, m_leads.size());
        if (inserted) {
            IeegLead lead;
            lead.name = contact.lead;
            m_leads.push_back(lead);
            members.emplace_back();
        }
        members[it->second].emplace_back(parts.number, static_cast<int>(i));
    }

    for (std::size_t l = 0; l < m_leads.size(); ++l) {
        std::vector<std::pair<int, int>>& lead = members[l];
        std::stable_sort(lead.begin(), lead.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        float spacing = std::numeric_limits<float>::max();
        m_leads[l].contacts.reserve(lead.size());
        for (std::size_t k = 0; k < lead.size(); ++k) {
            m_leads[l].contacts.push_back(lead[k].second);
            if (k > 0)
                spacing = std::min(spacing, (m_contacts[std::size_t(lead[k].second)].position - m_contacts[std::size_t(lead[k - 1].second)].position).norm());
        }
        m_leads[l].type = spacing < kMaxDepthSpacing ? IeegLeadType::Depth : IeegLeadType::Strip;
    }
}

//=============================================================================================================

IeegImplantation IeegImplantation::readBids(const std::string& electrodesTsv, std::string* error)
{
    const std::filesystem::path tsvPath = mne::pathFromUtf8(electrodesTsv);
    const std::string tsvName = mne::utf8FromPath(tsvPath.filename());
    // The coordsystem sidecar sharing the longest name prefix, so sessions are never mixed
    std::vector<std::pair<std::string, std::string>> candidates; // (lower-case name, name), sorted like QDir
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(tsvPath.parent_path().empty() ? "." : tsvPath.parent_path(), ec)) {
        std::error_code typeError;
        const std::string name = mne::utf8FromPath(entry.path().filename());
        if (entry.is_regular_file(typeError) && endsWithIgnoreCase(name, "_coordsystem.json"))
            candidates.emplace_back(mne::toLower(name), name);
    }
    std::sort(candidates.begin(), candidates.end());
    std::string coordPath;
    std::ptrdiff_t bestPrefix = -1;
    for (const auto& [lower, candidate] : candidates) {
        const auto prefix = std::mismatch(candidate.begin(), candidate.end(), tsvName.begin(), tsvName.end()).first - candidate.begin();
        if (prefix > bestPrefix) {
            bestPrefix = prefix;
            coordPath = mne::utf8FromPath(tsvPath.parent_path() / mne::pathFromUtf8(candidate));
        }
    }
    if (coordPath.empty()) {
        setError(error, "no *_coordsystem.json next to " + electrodesTsv + ", so the units are unknown");
        return {};
    }
    const mne::BidsCoordinateSystem coordSystem = mne::BidsCoordinateSystem::read_json(mne::pathFromUtf8(coordPath));
    const std::string& units = coordSystem.units;
    const float scale = units == "m" ? 1.0f
        : units == "cm"              ? 0.01f
        : units == "mm"              ? 0.001f
                                     : 0.0f;
    if (scale == 0.0f) {
        setError(error, "unsupported iEEGCoordinateUnits \"" + units + "\" in " + coordPath);
        return {};
    }

    std::vector<IeegContact> contacts;
    std::unordered_map<std::string, std::string> declaredTypes;
    std::unordered_map<std::string, std::string> declaredDimensions;
    for (const mne::BidsElectrode& e : mne::BidsElectrode::read_tsv(mne::pathFromUtf8(electrodesTsv))) {
        const std::optional<float> x = toCoordinate(e.x);
        const std::optional<float> y = toCoordinate(e.y);
        const std::optional<float> z = toCoordinate(e.z);
        if (!x || !y || !z)
            continue;
        IeegContact contact;
        contact.name = e.name;
        contact.position = Eigen::Vector3f(*x, *y, *z) * scale;
        contact.lead = e.group;
        // Declarations are per lead; contacts without a group take their lead from the name later
        const std::string key = contact.lead.empty() ? contact.name : contact.lead;
        if (!e.type.empty())
            declaredTypes.insert_or_assign(key, mne::toLower(e.type));
        if (!e.dimension.empty())
            declaredDimensions.insert_or_assign(key, e.dimension);
        contacts.push_back(std::move(contact));
    }
    if (contacts.empty()) {
        setError(error, "no contacts with coordinates in " + electrodesTsv);
        return {};
    }

    auto lookup = [](const std::unordered_map<std::string, std::string>& map, const std::string& key) {
        const auto it = map.find(key);
        return it == map.end() ? std::string() : it->second;
    };
    IeegImplantation implantation(contacts, coordSystem.system);
    for (std::size_t l = 0; l < implantation.m_leads.size(); ++l) {
        const IeegLead lead = implantation.m_leads[l];
        // A declaration on any contact of the lead applies to the lead
        std::string type = lookup(declaredTypes, lead.name);
        std::string shape = lookup(declaredDimensions, lead.name);
        for (const int c : lead.contacts) {
            const std::string& name = implantation.m_contacts[std::size_t(c)].name;
            if (type.empty())
                type = lookup(declaredTypes, name);
            if (shape.empty())
                shape = lookup(declaredDimensions, name);
        }
        const std::optional<std::pair<int, int>> dimension = parseDimension(shape);
        const int rows = dimension ? dimension->first : 0;
        const int cols = dimension ? dimension->second : 0;
        IeegLeadType leadType = (rows > 1 && cols > 1 && std::int64_t{rows} * cols == std::int64_t(lead.contacts.size())) ? IeegLeadType::Grid : lead.type;
        if (type.find("depth") != std::string::npos || type.find("seeg") != std::string::npos)
            leadType = IeegLeadType::Depth;
        else if (type.find("strip") != std::string::npos)
            leadType = IeegLeadType::Strip;
        else if (type.find("grid") != std::string::npos)
            leadType = IeegLeadType::Grid;
        implantation.setLeadType(lead.name, leadType, rows, cols);
    }
    return implantation;
}

//=============================================================================================================

bool IeegImplantation::setLeadType(const std::string& lead, IeegLeadType type, int rows, int cols)
{
    for (IeegLead& l : m_leads) {
        if (l.name != lead)
            continue;
        l.type = type;
        const bool shapeFits = rows > 0 && cols > 0 && std::int64_t{rows} * cols == std::int64_t(l.contacts.size());
        l.rows = shapeFits ? rows : 0;
        l.cols = shapeFits ? cols : 0;
        return true;
    }
    return false;
}

//=============================================================================================================

int IeegImplantation::indexOf(const std::string& name) const
{
    const auto it = std::find_if(m_contacts.begin(), m_contacts.end(), [&](const IeegContact& c) { return c.name == name; });
    return it == m_contacts.end() ? -1 : static_cast<int>(it - m_contacts.begin());
}

//=============================================================================================================

bool IeegImplantation::isScannerRas() const
{
    return mne::equalsIgnoreCase(m_coordinateSystem, "ACPC") || mne::equalsIgnoreCase(m_coordinateSystem, "ScanRAS");
}

//=============================================================================================================

IeegImplantation IeegImplantation::transformed(const Eigen::Matrix4f& trans, const std::string& frame) const
{
    IeegImplantation result = *this;
    for (IeegContact& contact : result.m_contacts)
        contact.position = trans.topLeftCorner<3, 3>() * contact.position + trans.topRightCorner<3, 1>();
    result.m_coordinateSystem = frame;
    return result;
}

//=============================================================================================================

Eigen::MatrixX3f IeegImplantation::positions() const
{
    Eigen::MatrixX3f rr(Eigen::Index(m_contacts.size()), 3);
    for (std::size_t i = 0; i < m_contacts.size(); ++i)
        rr.row(Eigen::Index(i)) = m_contacts[i].position.transpose();
    return rr;
}
