//=============================================================================================================
/**
 * @file     test_ml_models.cpp
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
 * @brief    Tests for ML library (MlLinearModel, MlOnnxModel, model I/O).
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <ml/ml_model.h>
#include <ml/ml_linear_model.h>
#include <ml/ml_onnx_model.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QTemporaryDir>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MLLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestMlModels
 *
 * @brief The TestMlModels class provides tests for ml library model classes.
 */
class TestMlModels : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // MlLinearModel
    void testLinearModelConstruction();
    void testLinearModelType();
    void testLinearModelTaskType();
    void testLinearModelPredict();
    void testLinearModelSaveLoad();
    void testLinearModelWeightsAndBias();

    // MlOnnxModel
    void testOnnxModelConstruction();
    void testOnnxModelNotLoaded();
    void testOnnxModelLoadNonexistent();

    // Model interface
    void testModelPolymorphism();

    void cleanupTestCase();

private:
    QTemporaryDir m_tempDir;
};

//=============================================================================================================

void TestMlModels::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
}

//=============================================================================================================

void TestMlModels::testLinearModelConstruction()
{
    MlLinearModel model;
    QVERIFY(!model.modelType().isEmpty());
}

//=============================================================================================================

void TestMlModels::testLinearModelType()
{
    MlLinearModel model(MlTaskType::Regression, 1.0);
    QVERIFY(model.modelType().contains("linear", Qt::CaseInsensitive));
}

//=============================================================================================================

void TestMlModels::testLinearModelTaskType()
{
    MlLinearModel regModel(MlTaskType::Regression);
    QCOMPARE(regModel.taskType(), MlTaskType::Regression);

    MlLinearModel classModel(MlTaskType::Classification);
    QCOMPARE(classModel.taskType(), MlTaskType::Classification);
}

//=============================================================================================================

void TestMlModels::testLinearModelPredict()
{
    MlLinearModel model(MlTaskType::Regression, 0.1);

    // Predicting on an untrained model should throw
    MatrixXf mat = MatrixXf::Random(5, 3);
    MlTensor input(mat);
    QVERIFY_EXCEPTION_THROWN(model.predict(input), std::runtime_error);

    // Create a trained model via save/load round-trip with known weights
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString modelPath = tmpDir.filePath("test_model.json");

    // Write a minimal model JSON: 3 features, 1 output, identity-like weights
    QJsonObject root;
    root[QStringLiteral("task_type")] = QStringLiteral("regression");
    root[QStringLiteral("regularization")] = 0.1;
    root[QStringLiteral("n_features")] = 3;
    root[QStringLiteral("n_outputs")] = 1;
    QJsonArray weights;
    weights.append(QJsonArray{1.0});
    weights.append(QJsonArray{1.0});
    weights.append(QJsonArray{1.0});
    root[QStringLiteral("weights")] = weights;
    root[QStringLiteral("bias")] = QJsonArray{0.0};

    QFile file(modelPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QJsonDocument(root).toJson());
    file.close();

    MlLinearModel trainedModel;
    QVERIFY(trainedModel.load(modelPath));

    MlTensor output = trainedModel.predict(input);
    QCOMPARE(output.rows(), 5);
}

//=============================================================================================================

void TestMlModels::testLinearModelSaveLoad()
{
    // Create a trained model via JSON with known weights (4 features, 1 output)
    QString seedPath = m_tempDir.filePath("seed_model.json");
    {
        QJsonObject root;
        root[QStringLiteral("task_type")] = QStringLiteral("regression");
        root[QStringLiteral("regularization")] = 0.5;
        root[QStringLiteral("n_features")] = 4;
        root[QStringLiteral("n_outputs")] = 1;
        QJsonArray weights;
        weights.append(QJsonArray{1.0});
        weights.append(QJsonArray{2.0});
        weights.append(QJsonArray{3.0});
        weights.append(QJsonArray{4.0});
        root[QStringLiteral("weights")] = weights;
        root[QStringLiteral("bias")] = QJsonArray{0.5};

        QFile f(seedPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(root).toJson());
        f.close();
    }

    MlLinearModel original;
    QVERIFY(original.load(seedPath));

    // Save and reload
    QString path = m_tempDir.filePath("linear_model.json");
    QVERIFY(original.save(path));

    MlLinearModel loaded;
    QVERIFY(loaded.load(path));

    QCOMPARE(loaded.modelType(), original.modelType());
    QCOMPARE(loaded.taskType(), original.taskType());
}

//=============================================================================================================

void TestMlModels::testLinearModelWeightsAndBias()
{
    MlLinearModel model(MlTaskType::Regression, 1.0);

    // After construction, weights and bias may be empty until fitted
    const MatrixXf& weights = model.weights();
    const VectorXf& bias = model.bias();

    // Just verify these are accessible without crash
    Q_UNUSED(weights);
    Q_UNUSED(bias);
    QVERIFY(true);
}

//=============================================================================================================

void TestMlModels::testOnnxModelConstruction()
{
    MlOnnxModel model;
    QVERIFY(!model.isLoaded());
}

//=============================================================================================================

void TestMlModels::testOnnxModelNotLoaded()
{
    MlOnnxModel model;
    QVERIFY(!model.isLoaded());

    // Predict on unloaded model should return empty or handle gracefully
    MatrixXf mat = MatrixXf::Random(1, 10);
    MlTensor input(mat);
    MlTensor output = model.predict(input);
    // Should not crash
    QVERIFY(true);
}

//=============================================================================================================

void TestMlModels::testOnnxModelLoadNonexistent()
{
    MlOnnxModel model;
    QVERIFY(!model.load("/nonexistent/model.onnx"));
    QVERIFY(!model.isLoaded());
}

//=============================================================================================================

void TestMlModels::testModelPolymorphism()
{
    // Test that MlLinearModel works through MlModel pointer
    MlModel::SPtr model = MlModel::SPtr(new MlLinearModel(MlTaskType::Regression));

    QVERIFY(!model->modelType().isEmpty());
    QCOMPARE(model->taskType(), MlTaskType::Regression);

    MatrixXf mat = MatrixXf::Random(3, 5);
    MlTensor input(mat);
    MlTensor output = model->predict(input);
    QVERIFY(true); // Didn't crash
}

//=============================================================================================================

void TestMlModels::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMlModels)
#include "test_ml_models.moc"
