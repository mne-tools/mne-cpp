//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_deriv_set.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::MNEDerivSet (MNE-C mne_derivations.c and mne_make_derivations/interpret.c).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_deriv_set.h"

#include <fiff/fiff_ch_name.h>
#include <base/log.h>
#include <base/string_utils.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_stream.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <vector>
#include <utility>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <optional>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

// Coefficients smaller than this are zero (MNE-C ZERO_THRESH and the mne_make_derivations default).
constexpr double kZero = 1e-6;

//=============================================================================================================
/**
 * Splits MNE-C derivation text into tokens: white space separates, a token starting with a
 * double quote runs to the next double quote, and a line starting with # is skipped.
 */
StringList tokenize(std::istream& in)
{
    StringList tokens;
    const auto isSpace = [](char c) {
        return std::isspace(static_cast<unsigned char>(c)) != 0;
    };
    std::string line;
    while (std::getline(in, line)) {
        if (mne::trimmed(line).starts_with('#')) {
            continue;
        }
        std::size_t k = 0;
        while (k < line.size()) {
            if (isSpace(line[k])) {
                ++k;
                continue;
            }
            if (line[k] == '"') {
                const std::size_t end = std::min(line.find('"', k + 1), line.size());
                tokens.emplace_back(line, k + 1, end - k - 1);
                k = end + 1;
                continue;
            }
            std::size_t end = k + 1;
            while (end < line.size() && !isSpace(line[end])) {
                ++end;
            }
            tokens.emplace_back(line, k, end - k);
            k = end;
        }
    }
    return tokens;
}

//=============================================================================================================

std::unique_ptr<MNEDeriv> makeDeriv(const std::vector<MNEDerivSet::Definition>& definitions, std::string filename, std::string shortname)
{
    StringList inputs;
    StringList outputs;
    std::vector<Eigen::Triplet<float>> triplets;
    for (const MNEDerivSet::Definition& definition : definitions) {
        std::vector<std::pair<int, double>> row;
        for (auto it = definition.second.cbegin(); it != definition.second.cend(); ++it) {
            if (std::fabs(it->second) <= kZero) {
                continue;
            }
            if (!mne::has_channel(inputs, it->first)) {
                inputs.push_back(it->first);
            }
            row.push_back({static_cast<int>(mne::channel_index(inputs, it->first)), it->second});
        }
        if (row.empty()) {
            mne::Log::info("MNEDerivSet - Empty derivation \"{}\" omitted", definition.first);
            continue;
        }
        for (const auto& [column, weight] : row) {
            triplets.emplace_back(static_cast<int>(outputs.size()), column, static_cast<float>(weight));
        }
        outputs.push_back(definition.first);
    }
    Eigen::SparseMatrix<float> data(outputs.size(), inputs.size());
    data.setFromTriplets(triplets.begin(), triplets.end());
    auto deriv = std::make_unique<MNEDeriv>();
    deriv->filename = std::move(filename);
    deriv->shortname = std::move(shortname);
    deriv->deriv_data = std::make_unique<MNESparseNamedMatrix>();
    deriv->deriv_data->rowlist = outputs;
    deriv->deriv_data->collist = inputs;
    deriv->deriv_data->data = FiffSparseMatrix(std::move(data));
    return deriv;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::optional<MNEDerivSet> MNEDerivSet::read(const std::filesystem::path& path)
{
    FiffStream stream(path);
    if (!stream.open()) {
        return std::nullopt;
    }
    MNEDerivSet set;
    for (const std::shared_ptr<FiffDirNode>& block : stream.dirtree()->dir_tree_find(FIFFB_MNE_DERIVATIONS)) {
        for (const std::shared_ptr<FiffDirNode>& matrix : block->dir_tree_find(FIFFB_MNE_NAMED_MATRIX)) {
            auto data = MNESparseNamedMatrix::read(stream, matrix, FIFF_MNE_DERIVATION_DATA);
            if (!data) {
                mne::Log::warning("MNEDerivSet::read - Bad derivation data in {}", mne::utf8_from_path(path));
                return std::nullopt;
            }
            auto deriv = std::make_unique<MNEDeriv>();
            deriv->filename = utf8_from_path(path);
            deriv->deriv_data = std::move(data);
            set.derivs.push_back(std::move(deriv));
        }
    }
    return set;
}

//=============================================================================================================

std::optional<MNEDerivSet> MNEDerivSet::readText(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file) {
        mne::Log::warning("MNEDerivSet::readText - Cannot open {}", mne::utf8_from_path(path));
        return std::nullopt;
    }
    const StringList tokens = tokenize(file);

    // A channel token right after another channel token (or first) names a new derivation;
    // otherwise it is an input whose weight comes from the preceding =, +, - or number.
    enum class Kind
    {
        Channel,
        Equal,
        Plus,
        Minus,
        Mult,
        Number
    };
    std::vector<Definition> definitions;
    Kind prev = Kind::Channel;
    bool havePrev = false;
    double number = 0.0;
    for (const std::string& token : tokens) {
        const std::optional<double> parsed = mne::to_double(token);
        const bool isNumber = parsed.has_value();
        const double value = parsed.value_or(0.0);
        const Kind kind = token == "=" ? Kind::Equal : token == "+" ? Kind::Plus
            : token == "-"                                          ? Kind::Minus
            : token == "*"                                          ? Kind::Mult
            : isNumber                                              ? Kind::Number
                                                                    : Kind::Channel;
        if (kind == Kind::Mult) {
            continue;
        }
        if (kind == Kind::Channel && (!havePrev || prev == Kind::Channel)) {
            definitions.push_back({token, {}});
        } else if (kind == Kind::Equal) {
            if (definitions.empty() || !definitions.back().second.empty()) {
                mne::Log::warning("MNEDerivSet::readText - Misplaced equal sign in {}", mne::utf8_from_path(path));
                return std::nullopt;
            }
        } else if (kind == Kind::Channel) {
            const double weight = prev == Kind::Minus ? -1.0 : prev == Kind::Number ? number
                                                                                    : 1.0;
            if (definitions.empty()) {
                mne::Log::warning("MNEDerivSet::readText - Misplaced channel name in {}", mne::utf8_from_path(path));
                return std::nullopt;
            }
            definitions.back().second[token] += weight;
        } else if (kind == Kind::Number) {
            number = prev == Kind::Minus ? -value : value;
        }
        prev = kind;
        havePrev = true;
    }
    if (definitions.empty()) {
        mne::Log::warning("MNEDerivSet::readText - No derivations in {}", mne::utf8_from_path(path));
        return std::nullopt;
    }
    MNEDerivSet set;
    set.derivs.push_back(makeDeriv(definitions, mne::utf8_from_path(path), {}));
    if (set.derivs.front()->deriv_data->nrow() == 0) {
        return std::nullopt;
    }
    return set;
}

//=============================================================================================================

MNEDerivSet MNEDerivSet::fromDefinitions(const std::vector<Definition>& definitions, const std::string& name)
{
    MNEDerivSet set;
    set.derivs.push_back(makeDeriv(definitions, {}, name));
    return set;
}

//=============================================================================================================

bool MNEDerivSet::write(const std::filesystem::path& path) const
{
    if (derivs.empty()) {
        mne::Log::warning("MNEDerivSet::write - No derivations to write");
        return false;
    }
    auto stream = FiffStream::start_file(path);
    if (!stream) {
        return false;
    }
    stream->start_block(FIFFB_MNE_DERIVATIONS);
    for (const auto& deriv : derivs) {
        deriv->deriv_data->write(*stream, FIFF_MNE_DERIVATION_DATA);
    }
    stream->end_block(FIFFB_MNE_DERIVATIONS);
    stream->end_file();
    return true;
}

//=============================================================================================================

std::vector<MNEDerivSet::Definition> MNEDerivSet::definitions() const
{
    std::vector<Definition> result;
    for (const auto& deriv : derivs) {
        const MNESparseNamedMatrix& m = *deriv->deriv_data;
        const Eigen::SparseMatrix<float, Eigen::RowMajor> rows = m.data.eigen();
        for (int j = 0; j < m.nrow(); ++j) {
            Definition definition{m.rowlist[j], {}};
            for (Eigen::SparseMatrix<float, Eigen::RowMajor>::InnerIterator it(rows, j); it; ++it) {
                definition.second.emplace(m.collist[static_cast<std::size_t>(it.col())], it.value());
            }
            result.push_back(std::move(definition));
        }
    }
    return result;
}

//=============================================================================================================

void MNEDerivSet::append(const MNEDerivSet& other)
{
    for (const auto& deriv : other.derivs) {
        derivs.push_back(std::make_unique<MNEDeriv>(*deriv));
    }
}

//=============================================================================================================

std::unique_ptr<MNEDeriv> MNEDerivSet::match(const StringList& chNames) const
{
    std::vector<Eigen::Triplet<float>> triplets;
    StringList outputs;
    int ntot = 0;
    for (const Definition& definition : definitions()) {
        ++ntot;
        bool available = true;
        for (auto it = definition.second.cbegin(); it != definition.second.cend() && available; ++it) {
            available = std::fabs(it->second) <= kZero || mne::has_channel(chNames, it->first);
        }
        if (!available) {
            continue;
        }
        for (auto it = definition.second.cbegin(); it != definition.second.cend(); ++it) {
            if (std::fabs(it->second) > kZero) {
                triplets.emplace_back(static_cast<int>(outputs.size()), static_cast<int>(mne::channel_index(chNames, it->first)), static_cast<float>(it->second));
            }
        }
        outputs.push_back(definition.first);
    }
    if (outputs.empty()) {
        return nullptr;
    }
    mne::Log::info("MNEDerivSet::match - {} of {} derivations were matched.", outputs.size(), ntot);
    Eigen::SparseMatrix<float> data(outputs.size(), chNames.size());
    data.setFromTriplets(triplets.begin(), triplets.end());

    auto matched = std::make_unique<MNEDeriv>();
    matched->shortname = "Matched derivations";
    // in_use counts, per recorded channel, the derived channels it enters (MNE-C convention).
    matched->in_use = Eigen::VectorXi::Zero(chNames.size());
    for (int k = 0; k < data.outerSize(); ++k) {
        for (Eigen::SparseMatrix<float>::InnerIterator it(data, k); it; ++it) {
            ++matched->in_use[it.col()];
        }
    }
    matched->deriv_data = std::make_unique<MNESparseNamedMatrix>();
    matched->deriv_data->rowlist = outputs;
    matched->deriv_data->collist = chNames;
    matched->deriv_data->data = FiffSparseMatrix(std::move(data));
    return matched;
}

//=============================================================================================================

int MNEDerivSet::count() const
{
    int total = 0;
    for (const auto& deriv : derivs) {
        total += deriv->deriv_data ? deriv->deriv_data->nrow() : 0;
    }
    return total;
}
