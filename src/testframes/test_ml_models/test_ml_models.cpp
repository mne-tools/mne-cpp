//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_ml_models.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for ML library (MlOnnxModel).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <ml/ml_model.h>
#include <ml/ml_onnx_model.h>
#include <ml/ml_tensor.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MLLIB;
using namespace Eigen;

//=============================================================================================================

class TestMlModels : public QObject
{
    Q_OBJECT

private slots:
    void testOnnxModelConstruction();
    void testOnnxModelNotLoaded();
    void testOnnxModelLoadNonexistent();
    void testTensor();
};

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

    MatrixXf mat = MatrixXf::Random(1, 10);
    MlTensor input(mat);
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, static_cast<void>(model.predict(input)));
}

//=============================================================================================================

void TestMlModels::testOnnxModelLoadNonexistent()
{
    MlOnnxModel model;
    QVERIFY(!model.load("/nonexistent/model.onnx"));
    QVERIFY(!model.isLoaded());
}

//=============================================================================================================

void TestMlModels::testTensor()
{
    // Row-major storage of a column-major matrix, negative dimension indices, and reshaping in place
    MatrixXd mat(2, 3);
    mat << 1, 2, 3, 4, 5, 6;
    const MlTensor owned(mat);
    QCOMPARE(owned.data()[1], 2.0f);
    QCOMPARE(owned.shape(-1), int64_t(3));
    QVERIFY(!owned.isView());
    const MlTensor flat = owned.reshape({6});
    QCOMPARE(flat.ndim(), 1);
    QCOMPARE(flat.data(), owned.data());
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, static_cast<void>(owned.reshape({4})));
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, MlTensor(std::vector<float>(5), {2, 3}));

    // A view borrows the buffer
    std::vector<float> buffer{1.0f, 2.0f};
    MlTensor view = MlTensor::view(buffer.data(), {1, 2});
    QVERIFY(view.isView());
    view.data()[0] = 7.0f;
    QCOMPARE(buffer[0], 7.0f);

    // A scalar (shape []) holds one element, as in ONNX and NumPy; only a default tensor is empty
    const MlTensor scalar(std::vector<float>{2.5f}, {});
    QCOMPARE(scalar.ndim(), 0);
    QCOMPARE(scalar.size(), int64_t(1));
    QVERIFY(!scalar.empty());
    QCOMPARE(scalar.data()[0], 2.5f);
    QVERIFY(MlTensor().empty());
    QVERIFY(!MlTensor().isView());
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMlModels)
#include "test_ml_models.moc"
