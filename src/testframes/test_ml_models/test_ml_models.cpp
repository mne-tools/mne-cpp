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
 * @brief    Tests for ML library (MlOnnxModel).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <ml/ml_model.h>
#include <ml/ml_onnx_model.h>

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

QTEST_GUILESS_MAIN(TestMlModels)
#include "test_ml_models.moc"
