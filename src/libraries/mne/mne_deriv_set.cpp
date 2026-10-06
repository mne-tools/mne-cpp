//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_deriv_set.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref MNELIB::MNEDerivSet (MNE-C mne_derivations.c and mne_make_derivations/interpret.c).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_deriv_set.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_stream.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QTextStream>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;

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
QStringList tokenize(QTextStream& in)
{
    QStringList tokens;
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.trimmed().startsWith('#')) {
            continue;
        }
        int k = 0;
        while (k < line.size()) {
            if (line[k].isSpace()) {
                ++k;
                continue;
            }
            int end = k + 1;
            if (line[k] == '"') {
                end = line.indexOf('"', k + 1);
                end = end < 0 ? line.size() : end;
                tokens << line.mid(k + 1, end - k - 1);
                k = end + 1;
                continue;
            }
            while (end < line.size() && !line[end].isSpace()) {
                ++end;
            }
            tokens << line.mid(k, end - k);
            k = end;
        }
    }
    return tokens;
}

//=============================================================================================================

std::unique_ptr<MNEDeriv> makeDeriv(const QList<MNEDerivSet::Definition>& definitions, const QString& filename, const QString& shortname)
{
    QStringList inputs;
    QStringList outputs;
    std::vector<Eigen::Triplet<float>> triplets;
    for (const MNEDerivSet::Definition& definition : definitions) {
        QList<QPair<int, double>> row;
        for (auto it = definition.second.cbegin(); it != definition.second.cend(); ++it) {
            if (std::fabs(it.value()) <= kZero) {
                continue;
            }
            if (!inputs.contains(it.key())) {
                inputs << it.key();
            }
            row.append({static_cast<int>(inputs.indexOf(it.key())), it.value()});
        }
        if (row.isEmpty()) {
            qInfo("MNEDerivSet - Empty derivation \"%s\" omitted", qPrintable(definition.first));
            continue;
        }
        for (const auto& [column, weight] : row) {
            triplets.emplace_back(static_cast<int>(outputs.size()), column, static_cast<float>(weight));
        }
        outputs << definition.first;
    }
    Eigen::SparseMatrix<float> data(outputs.size(), inputs.size());
    data.setFromTriplets(triplets.begin(), triplets.end());
    auto deriv = std::make_unique<MNEDeriv>();
    deriv->filename = filename;
    deriv->shortname = shortname;
    deriv->deriv_data = std::make_unique<MNESparseNamedMatrix>();
    deriv->deriv_data->nrow = static_cast<int>(outputs.size());
    deriv->deriv_data->ncol = static_cast<int>(inputs.size());
    deriv->deriv_data->rowlist = outputs;
    deriv->deriv_data->collist = inputs;
    deriv->deriv_data->data = std::make_unique<FiffSparseMatrix>(std::move(data));
    return deriv;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::optional<MNEDerivSet> MNEDerivSet::read(const QString& path)
{
    QFile file(path);
    FiffStream::SPtr stream(new FiffStream(&file));
    if (!stream->open()) {
        return std::nullopt;
    }
    MNEDerivSet set;
    for (const FiffDirNode::SPtr& block : stream->dirtree()->dir_tree_find(FIFFB_MNE_DERIVATIONS)) {
        for (const FiffDirNode::SPtr& matrix : block->dir_tree_find(FIFFB_MNE_NAMED_MATRIX)) {
            auto data = MNESparseNamedMatrix::read(stream, matrix, FIFF_MNE_DERIVATION_DATA);
            if (!data) {
                qWarning("MNEDerivSet::read - Bad derivation data in %s", qPrintable(path));
                return std::nullopt;
            }
            auto deriv = std::make_unique<MNEDeriv>();
            deriv->filename = path;
            deriv->deriv_data = std::move(data);
            set.derivs.push_back(std::move(deriv));
        }
    }
    return set;
}

//=============================================================================================================

std::optional<MNEDerivSet> MNEDerivSet::readText(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("MNEDerivSet::readText - Cannot open %s", qPrintable(path));
        return std::nullopt;
    }
    QTextStream in(&file);
    const QStringList tokens = tokenize(in);

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
    QList<Definition> definitions;
    Kind prev = Kind::Channel;
    bool havePrev = false;
    double number = 0.0;
    for (const QString& token : tokens) {
        bool isNumber = false;
        const double value = token.toDouble(&isNumber);
        const Kind kind = token == "=" ? Kind::Equal : token == "+" ? Kind::Plus
            : token == "-"                                          ? Kind::Minus
            : token == "*"                                          ? Kind::Mult
            : isNumber                                              ? Kind::Number
                                                                    : Kind::Channel;
        if (kind == Kind::Mult) {
            continue;
        }
        if (kind == Kind::Channel && (!havePrev || prev == Kind::Channel)) {
            definitions.append({token, {}});
        } else if (kind == Kind::Equal) {
            if (definitions.isEmpty() || !definitions.last().second.isEmpty()) {
                qWarning("MNEDerivSet::readText - Misplaced equal sign in %s", qPrintable(path));
                return std::nullopt;
            }
        } else if (kind == Kind::Channel) {
            const double weight = prev == Kind::Minus ? -1.0 : prev == Kind::Number ? number
                                                                                    : 1.0;
            if (definitions.isEmpty()) {
                qWarning("MNEDerivSet::readText - Misplaced channel name in %s", qPrintable(path));
                return std::nullopt;
            }
            definitions.last().second[token] += weight;
        } else if (kind == Kind::Number) {
            number = prev == Kind::Minus ? -value : value;
        }
        prev = kind;
        havePrev = true;
    }
    if (definitions.isEmpty()) {
        qWarning("MNEDerivSet::readText - No derivations in %s", qPrintable(path));
        return std::nullopt;
    }
    MNEDerivSet set;
    set.derivs.push_back(makeDeriv(definitions, path, QString()));
    if (set.derivs.front()->deriv_data->nrow == 0) {
        return std::nullopt;
    }
    return set;
}

//=============================================================================================================

MNEDerivSet MNEDerivSet::fromDefinitions(const QList<Definition>& definitions, const QString& name)
{
    MNEDerivSet set;
    set.derivs.push_back(makeDeriv(definitions, QString(), name));
    return set;
}

//=============================================================================================================

bool MNEDerivSet::write(const QString& path) const
{
    if (derivs.empty()) {
        qWarning("MNEDerivSet::write - No derivations to write");
        return false;
    }
    QFile file(path);
    FiffStream::SPtr stream = FiffStream::start_file(file);
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

QList<MNEDerivSet::Definition> MNEDerivSet::definitions() const
{
    QList<Definition> result;
    for (const auto& deriv : derivs) {
        const MNESparseNamedMatrix& m = *deriv->deriv_data;
        const Eigen::SparseMatrix<float, Eigen::RowMajor> rows = m.data->eigen();
        for (int j = 0; j < m.nrow; ++j) {
            Definition definition{m.rowlist.value(j), {}};
            for (Eigen::SparseMatrix<float, Eigen::RowMajor>::InnerIterator it(rows, j); it; ++it) {
                definition.second.insert(m.collist.value(static_cast<int>(it.col())), it.value());
            }
            result.append(definition);
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

std::unique_ptr<MNEDeriv> MNEDerivSet::match(const QStringList& chNames) const
{
    std::vector<Eigen::Triplet<float>> triplets;
    QStringList outputs;
    int ntot = 0;
    for (const Definition& definition : definitions()) {
        ++ntot;
        bool available = true;
        for (auto it = definition.second.cbegin(); it != definition.second.cend() && available; ++it) {
            available = std::fabs(it.value()) <= kZero || chNames.contains(it.key());
        }
        if (!available) {
            continue;
        }
        for (auto it = definition.second.cbegin(); it != definition.second.cend(); ++it) {
            if (std::fabs(it.value()) > kZero) {
                triplets.emplace_back(static_cast<int>(outputs.size()), static_cast<int>(chNames.indexOf(it.key())), static_cast<float>(it.value()));
            }
        }
        outputs << definition.first;
    }
    if (outputs.isEmpty()) {
        return nullptr;
    }
    qInfo("MNEDerivSet::match - %lld of %d derivations were matched.", static_cast<long long>(outputs.size()), ntot);
    Eigen::SparseMatrix<float> data(outputs.size(), chNames.size());
    data.setFromTriplets(triplets.begin(), triplets.end());

    auto matched = std::make_unique<MNEDeriv>();
    matched->shortname = QStringLiteral("Matched derivations");
    // in_use counts, per recorded channel, the derived channels it enters (MNE-C convention).
    matched->in_use = Eigen::VectorXi::Zero(chNames.size());
    for (int k = 0; k < data.outerSize(); ++k) {
        for (Eigen::SparseMatrix<float>::InnerIterator it(data, k); it; ++it) {
            ++matched->in_use[it.col()];
        }
    }
    matched->deriv_data = std::make_unique<MNESparseNamedMatrix>();
    matched->deriv_data->nrow = static_cast<int>(outputs.size());
    matched->deriv_data->ncol = static_cast<int>(chNames.size());
    matched->deriv_data->rowlist = outputs;
    matched->deriv_data->collist = chNames;
    matched->deriv_data->data = std::make_unique<FiffSparseMatrix>(std::move(data));
    return matched;
}

//=============================================================================================================

int MNEDerivSet::count() const
{
    int total = 0;
    for (const auto& deriv : derivs) {
        total += deriv->deriv_data ? deriv->deriv_data->nrow : 0;
    }
    return total;
}
