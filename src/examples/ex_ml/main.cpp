//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    ML library: tensors, ONNX inference through the MlModel interface, and the Python trainer.
 *
 * Runs a bundled two-class softmax(x W + b) ONNX model (written by
 * data/make_fixture.py) on two rows and compares the probabilities with the
 * NumPy closed form, then runs a short Python script through MLTrainer. Exits
 * non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <ml/ml_model.h>
#include <ml/ml_onnx_model.h>
#include <ml/ml_tensor.h>
#include <ml/ml_trainer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdlib>
#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MLLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    //! [ml_tensor_usage]
    MatrixXf features(2, 3);
    features << 1.0f, 2.0f, 0.5f,
        0.0f, -1.0f, 1.5f;
    MlTensor batch(features);                          // owning row-major copy, shape {2, 3}
    MlTensor flat = batch.reshape({6});                // shares the buffer
    MlTensor::RowMajorMatrixMap rows = batch.matrix(); // zero-copy Eigen view
    //! [ml_tensor_usage]
    ok &= expect(batch.shape() == std::vector<int64_t>({2, 3}) && flat.ndim() == 1 && flat.data()[4] == -1.0f && rows(1, 2) == 1.5f && batch.toMatrixXd().isApprox(features.cast<double>()),
                 "MlTensor: {2, 3} from Eigen, reshape shares data, matrix() maps it back");

#ifdef MNE_USE_ONNXRUNTIME
    //! [ml_onnx_model_usage]
    std::unique_ptr<MlModel> model = std::make_unique<MlOnnxModel>();
    const bool loaded = model->load(MNE_EX_ML_DATA_DIR "/affine_softmax.onnx");
    const MlTensor probabilities = model->predict(batch); // shape {2, 2}, one row per sample
    const QString classes = static_cast<MlOnnxModel&>(*model).metadata("classes");
    //! [ml_onnx_model_usage]
    // NumPy: softmax(x @ W + b) = [[0.6681878, 0.33181223], [0.00235232, 0.9976477]]
    Matrix2f expected;
    expected << 0.6681878f, 0.33181223f, 0.00235232f, 0.9976477f;
    ok &= expect(loaded && model->modelType() == "onnx" && probabilities.shape() == std::vector<int64_t>({2, 2}) && (probabilities.toMatrixXf() - expected).cwiseAbs().maxCoeff() < 1e-6f && classes == "rest,task",
                 QString("MlOnnxModel: p(task) = %1, %2 match NumPy; classes '%3'").arg(probabilities.matrix()(0, 1)).arg(probabilities.matrix()(1, 1)).arg(classes));
#else
    qInfo().noquote() << "  skip  MlOnnxModel (built without ONNX Runtime)";
#endif

    //! [ml_trainer_usage]
    QTemporaryDir dir;
    QFile script(dir.filePath("train.py"));
    const bool written = script.open(QIODevice::WriteOnly) && script.write("import sys\nprint('trained on', sys.argv[1], 'epochs')\n") > 0;
    script.close();
    MLTrainer trainer; // runs python3; a venv and pip install are configured via UTILSLIB::PythonRunnerConfig
    const UTILSLIB::PythonRunnerResult result = trainer.run(script.fileName(), {"5"});
    //! [ml_trainer_usage]
    if (trainer.runner().isPythonAvailable()) {
        ok &= expect(written && result.success && result.stdOut.trimmed() == "trained on 5 epochs" && trainer.checkPrerequisites({"json", "no_such_package_xyz"}) == QStringList({"no_such_package_xyz"}),
                     "MLTrainer runs the script with its arguments and reports the missing package");
    } else {
        qInfo().noquote() << "  skip  MLTrainer (python3 not found)";
    }

    qInfo().noquote() << (ok ? "All ml checks passed." : "ml checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
