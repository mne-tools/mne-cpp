//=============================================================================================================
/**
 * @file     ml_linear_model.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    MlLinearModel class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_linear_model.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <stdexcept>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MLLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MlLinearModel::MlLinearModel(MlTaskType type, double regularization)
: m_taskType(type)
, m_regularization(regularization)
{
}

//=============================================================================================================

MlTensor MlLinearModel::predict(const MlTensor& input) const
{
    if (!m_trained) {
        throw std::runtime_error("MlLinearModel::predict – model has not been trained.");
    }

    auto X = input.matrix();
    MatrixXf result = (X * m_weights).rowwise() + m_bias.transpose();

    if (m_taskType == MlTaskType::Classification) {
        // Apply sigmoid: p = 1 / (1 + exp(-z))
        result = (1.0f + (-result.array()).exp()).inverse().matrix();
    }

    return MlTensor(result);
}

//=============================================================================================================

bool MlLinearModel::save(const QString& path) const
{
    if (!m_trained) {
        qWarning() << "MlLinearModel::save – model not trained; nothing to save.";
        return false;
    }

    QJsonObject root;
    root[QStringLiteral("model_type")] = QStringLiteral("linear");

    switch (m_taskType) {
        case MlTaskType::Regression:        root[QStringLiteral("task_type")] = QStringLiteral("regression"); break;
        case MlTaskType::Classification:    root[QStringLiteral("task_type")] = QStringLiteral("classification"); break;
        case MlTaskType::FeatureExtraction: root[QStringLiteral("task_type")] = QStringLiteral("feature_extraction"); break;
    }

    root[QStringLiteral("regularization")] = m_regularization;
    root[QStringLiteral("n_features")]     = static_cast<int>(m_weights.rows());
    root[QStringLiteral("n_outputs")]      = static_cast<int>(m_weights.cols());

    // Weights: array of arrays (row-major)
    QJsonArray wArr;
    for (int r = 0; r < m_weights.rows(); ++r) {
        QJsonArray row;
        for (int c = 0; c < m_weights.cols(); ++c) {
            row.append(static_cast<double>(m_weights(r, c)));
        }
        wArr.append(row);
    }
    root[QStringLiteral("weights")] = wArr;

    // Bias
    QJsonArray bArr;
    for (int i = 0; i < m_bias.size(); ++i) {
        bArr.append(static_cast<double>(m_bias(i)));
    }
    root[QStringLiteral("bias")] = bArr;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "MlLinearModel::save – cannot open file" << path;
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

//=============================================================================================================

bool MlLinearModel::load(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "MlLinearModel::load – cannot open file" << path;
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        qWarning() << "MlLinearModel::load – invalid JSON in" << path;
        return false;
    }

    QJsonObject root = doc.object();

    // Task type
    QString tt = root[QStringLiteral("task_type")].toString();
    if (tt == QStringLiteral("regression"))             m_taskType = MlTaskType::Regression;
    else if (tt == QStringLiteral("classification"))     m_taskType = MlTaskType::Classification;
    else if (tt == QStringLiteral("feature_extraction")) m_taskType = MlTaskType::FeatureExtraction;

    m_regularization = root[QStringLiteral("regularization")].toDouble(1.0);

    int nFeatures = root[QStringLiteral("n_features")].toInt();
    int nOutputs  = root[QStringLiteral("n_outputs")].toInt();

    // Weights
    QJsonArray wArr = root[QStringLiteral("weights")].toArray();
    m_weights.resize(nFeatures, nOutputs);
    for (int r = 0; r < nFeatures; ++r) {
        QJsonArray row = wArr[r].toArray();
        for (int c = 0; c < nOutputs; ++c) {
            m_weights(r, c) = static_cast<float>(row[c].toDouble());
        }
    }

    // Bias
    QJsonArray bArr = root[QStringLiteral("bias")].toArray();
    m_bias.resize(nOutputs);
    for (int i = 0; i < nOutputs; ++i) {
        m_bias(i) = static_cast<float>(bArr[i].toDouble());
    }

    m_trained = true;
    return true;
}

//=============================================================================================================

QString MlLinearModel::modelType() const
{
    return QStringLiteral("linear");
}

//=============================================================================================================

MlTaskType MlLinearModel::taskType() const
{
    return m_taskType;
}

//=============================================================================================================

const MatrixXf& MlLinearModel::weights() const
{
    return m_weights;
}

//=============================================================================================================

const VectorXf& MlLinearModel::bias() const
{
    return m_bias;
}
