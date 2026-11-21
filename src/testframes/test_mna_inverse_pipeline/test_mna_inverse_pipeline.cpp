//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mna_inverse_pipeline.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Real-data integration test for the dSPM inverse pipeline as an MNA project.
 *
 *           Loads resources/examples/mna_inverse_dspm.mna, reconstructs the
 *           six-node DAG, registers real operator functions that use the
 *           MNE sample dataset, executes the graph, and verifies every
 *           intermediate result (FiffInfo, FiffEvoked, FiffCov,
 *           MNEForwardSolution, MNEInverseOperator, InvSourceEstimate).
 *
 *               load_raw ──────┐
 *               load_cov ──────┤
 *               load_fwd ──────┴── make_inv ── apply_dspm
 *               load_evoked ──────────────────┘
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mna/mna_graph.h>
#include <mna/mna_graph_executor.h>
#include <mna/mna_op_registry.h>
#include <mna/mna_node.h>
#include <mna/mna_port.h>
#include <mna/mna_types.h>
#include <mna/mna_io.h>
#include <mna/mna_project.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <mne/mne_inverse_operator.h>
#include <mne/mne_forward_solution.h>
#include <inv/minimum_norm/inv_minimum_norm.h>
#include <inv/inv_source_estimate.h>
#include <base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QCoreApplication>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <any>
#include <memory>
#include <string>
#include <string_view>
#include <filesystem>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


using namespace mne;
using namespace Eigen;

//=============================================================================================================
// HELPERS
//=============================================================================================================

namespace
{

/** @return The object of type T that @p values holds under @p key, or nullptr. */
template<class T>
std::shared_ptr<T> result(const MnaValues& values, std::string_view key)
{
    const auto it = values.find(key);
    if (it == values.end()) {
        return nullptr;
    }
    const auto* object = std::any_cast<std::shared_ptr<T>>(&it->second);
    return object ? *object : nullptr;
}

/** @return The path attribute @p key with ${TEST_DATA} replaced by @p dataPath. */
std::string dataFile(const JsonObject& attrs, std::string_view key, const std::string& dataPath)
{
    std::string path = attrs[key].to_std_string();
    constexpr std::string_view placeholder = "${TEST_DATA}";
    if (const std::size_t at = path.find(placeholder); at != std::string::npos) {
        path.replace(at, placeholder.size(), dataPath);
    }
    return path;
}

/** @return The position of @p id in @p order, or -1. */
long position(const StringList& order, std::string_view id)
{
    const auto it = std::ranges::find(order, id);
    return it == order.end() ? -1 : it - order.begin();
}

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestMnaInversePipeline
 */
class TestMnaInversePipeline : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testLoadMnaFile();
    void testGraphTopology();
    void testExecuteRealPipeline();
    void testMnaFileRoundTrip();

    void cleanupTestCase();

private:
    void registerRealOps();
    QString findMnaFile() const;
    QString findTestDataPath() const;

    MnaProject m_project;
    MnaGraph m_graph;
    QString m_mnaFilePath;
    QString m_testDataPath;
};

//=============================================================================================================

void TestMnaInversePipeline::initTestCase()
{
    m_testDataPath = findTestDataPath();
    QVERIFY2(!m_testDataPath.isEmpty(),
             "Could not find mne-cpp-test-data (expected at ../resources/data/mne-cpp-test-data)");

    m_mnaFilePath = findMnaFile();
    QVERIFY2(!m_mnaFilePath.isEmpty(),
             "Could not find resources/examples/mna_inverse_dspm.mna");

    registerRealOps();
}

//=============================================================================================================

QString TestMnaInversePipeline::findTestDataPath() const
{
    QString base = QCoreApplication::applicationDirPath() + QStringLiteral("/../resources/data/mne-cpp-test-data");
    if (QFile::exists(base + QStringLiteral("/MEG/sample/sample_audvis_trunc_raw.fif")))
        return base;
    return {};
}

//=============================================================================================================

QString TestMnaInversePipeline::findMnaFile() const
{
    const QString relPath = QStringLiteral("resources/examples/mna_inverse_dspm.mna");
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 5; ++i) {
        QString candidate = dir.absoluteFilePath(relPath);
        if (QFile::exists(candidate))
            return candidate;
        dir.cdUp();
    }
    if (QFile::exists(relPath))
        return QDir::currentPath() + QStringLiteral("/") + relPath;
    return {};
}

//=============================================================================================================

void TestMnaInversePipeline::registerRealOps()
{
    MnaOpRegistry& reg = MnaOpRegistry::instance();
    const std::string dataPath = m_testDataPath.toStdString();

    // load_fiff_raw — reads raw FIFF and outputs FiffInfo
    reg.register_op_func("load_fiff_raw", [dataPath](const MnaValues&, const JsonObject& attrs) -> MnaValues {
        FiffRawData raw(dataFile(attrs, "raw_path", dataPath));
        return {{"raw_data", std::any()}, {"info", std::make_shared<FiffInfo>(raw.info)}};
    });

    // load_fiff_evoked — reads evoked response
    reg.register_op_func("load_fiff_evoked", [dataPath](const MnaValues&, const JsonObject& attrs) -> MnaValues {
        const QPair<float, float> noBaseline(-1.0f, -1.0f);
        return {{"evoked", std::make_shared<FiffEvoked>(dataFile(attrs, "evoked_path", dataPath), attrs["setno"].to_int(0), noBaseline)}};
    });

    // load_fiff_cov — reads noise covariance matrix
    reg.register_op_func("load_fiff_cov", [dataPath](const MnaValues&, const JsonObject& attrs) -> MnaValues {
        return {{"noise_cov", std::make_shared<FiffCov>(dataFile(attrs, "cov_path", dataPath))}};
    });

    // load_forward — reads pre-computed forward solution
    reg.register_op_func("load_forward", [dataPath](const MnaValues&, const JsonObject& attrs) -> MnaValues {
        return {{"forward", std::make_shared<MNEForwardSolution>(path_from_utf8(dataFile(attrs, "fwd_path", dataPath)))}};
    });

    // make_inverse_operator — builds MNEInverseOperator from FiffInfo + forward + covariance
    reg.register_op_func("make_inverse_operator", [](const MnaValues& inputs, const JsonObject& attrs) -> MnaValues {
        const auto info = result<FiffInfo>(inputs, "info");
        const auto fwd = result<MNEForwardSolution>(inputs, "forward");
        const auto cov = result<FiffCov>(inputs, "noise_cov");
        const auto loose = static_cast<float>(attrs["loose"].to_double(0.2));
        const auto depth = static_cast<float>(attrs["depth"].to_double(0.8));
        return {{"inverse_operator", std::make_shared<MNEInverseOperator>(MNEInverseOperator::make_inverse_operator(*info, *fwd, *cov, loose, depth, false, true))}};
    });

    // apply_inverse — applies dSPM/MNE/sLORETA to evoked data, produces source estimate
    reg.register_op_func("apply_inverse", [](const MnaValues& inputs, const JsonObject& attrs) -> MnaValues {
        const auto invOp = result<MNEInverseOperator>(inputs, "inverse_operator");
        const auto evoked = result<FiffEvoked>(inputs, "evoked");
        const auto snr = static_cast<float>(attrs["snr"].to_double(3.0));
        const float lambda2 = 1.0f / (snr * snr);
        const bool pickNormal = attrs["pick_normal"].to_bool(false);

        // Pick channels that match the inverse operator's noise covariance
        const FiffEvoked picked = evoked->pick_channels(invOp->noise_cov.names);
        const float tmin = picked.times(0);
        const auto tstep = static_cast<float>(1.0 / picked.info.sfreq);

        InvMinimumNorm mn(*invOp, lambda2, std::string(attrs["method"].to_string("dSPM")));
        mn.doInverseSetup(evoked->nave, pickNormal);
        return {{"source_estimate", std::make_shared<InvSourceEstimate>(mn.calculateInverse(picked.data, tmin, tstep, pickNormal))}};
    });
}

//=============================================================================================================
// Load and parse the .mna project file
//=============================================================================================================

void TestMnaInversePipeline::testLoadMnaFile()
{
    qInfo() << "Loading .mna file:" << m_mnaFilePath;
    m_project = MnaIO::read(path_from_utf8(m_mnaFilePath.toStdString())).value();

    QCOMPARE(m_project.name, std::string("dSPM Inverse Pipeline"));
    QVERIFY(!m_project.description.empty());
    QCOMPARE(m_project.pipeline.size(), std::size_t(6));

    StringList nodeIds;
    for (const auto& node : m_project.pipeline)
        nodeIds.push_back(node.id);
    QCOMPARE(nodeIds, (StringList{"load_raw", "load_evoked", "load_cov", "load_fwd", "make_inv", "apply_dspm"}));

    // Verify inverse parameters from the file
    const auto applyNode = std::ranges::find(m_project.pipeline, std::string_view("apply_dspm"), &MnaNode::id);
    QVERIFY(applyNode != m_project.pipeline.end());
    QCOMPARE(applyNode->op_type, std::string("apply_inverse"));
    QCOMPARE(applyNode->attributes["method"].to_string(), std::string_view("dSPM"));
    QCOMPARE(applyNode->attributes["snr"].to_double(), 3.0);

    // Build the execution graph directly from pipeline nodes
    for (const MnaNode& node : m_project.pipeline)
        m_graph.add_node(node);
    QCOMPARE(m_graph.nodes().size(), std::size_t(6));
}

//=============================================================================================================
// Verify topological ordering of the DAG
//=============================================================================================================

void TestMnaInversePipeline::testGraphTopology()
{
    if (m_graph.nodes().empty())
        QSKIP("Graph not built — testLoadMnaFile must run first");

    const StringList order = m_graph.topological_sort();
    QCOMPARE(order.size(), std::size_t(6));
    for (const char* id : {"load_raw", "load_evoked", "load_cov", "load_fwd", "make_inv", "apply_dspm"}) {
        QVERIFY2(position(order, id) >= 0, id);
    }

    // Dependency ordering
    QVERIFY(position(order, "load_raw") < position(order, "make_inv"));
    QVERIFY(position(order, "load_fwd") < position(order, "make_inv"));
    QVERIFY(position(order, "load_cov") < position(order, "make_inv"));
    QVERIFY(position(order, "make_inv") < position(order, "apply_dspm"));
    QVERIFY(position(order, "load_evoked") < position(order, "apply_dspm"));
}

//=============================================================================================================
// Execute the full pipeline with real FIFF data and verify every intermediate result
//=============================================================================================================

void TestMnaInversePipeline::testExecuteRealPipeline()
{
    if (m_graph.nodes().empty())
        QSKIP("Graph not built — testLoadMnaFile must run first");

    qInfo() << "Executing real dSPM inverse pipeline with sample_audvis data...";
    const MnaGraphExecutor::Context ctx = MnaGraphExecutor::execute(m_graph);

    // ── Stage 1: Verify FiffInfo from load_raw ──────────────────────────────
    qInfo() << "  [1/6] Verifying load_raw::info (FiffInfo)...";
    {
        const auto info = result<FiffInfo>(ctx.results, "load_raw::info");
        QVERIFY2(info != nullptr, "FiffInfo is null");
        QVERIFY2(info->nchan > 0,
                 qPrintable(QString("nchan = %1, expected > 0").arg(info->nchan)));
        QVERIFY(info->sfreq > 0.0);
        qInfo() << "        nchan =" << info->nchan << ", sfreq =" << info->sfreq << "Hz";
    }

    // ── Stage 2: Verify FiffEvoked from load_evoked ─────────────────────────
    qInfo() << "  [2/6] Verifying load_evoked::evoked (FiffEvoked)...";
    {
        const auto evoked = result<FiffEvoked>(ctx.results, "load_evoked::evoked");
        QVERIFY2(evoked != nullptr, "FiffEvoked is null");
        QVERIFY2(evoked->data.rows() > 0,
                 qPrintable(QString("evoked rows = %1").arg(evoked->data.rows())));
        QVERIFY2(evoked->data.cols() > 0,
                 qPrintable(QString("evoked cols = %1").arg(evoked->data.cols())));
        QVERIFY(evoked->times.size() > 0);
        QVERIFY(evoked->nave > 0);
        qInfo() << "        " << evoked->data.rows() << "channels x"
                << evoked->data.cols() << "time points, nave =" << evoked->nave;
    }

    // ── Stage 3: Verify FiffCov from load_cov ───────────────────────────────
    qInfo() << "  [3/6] Verifying load_cov::noise_cov (FiffCov)...";
    {
        const auto cov = result<FiffCov>(ctx.results, "load_cov::noise_cov");
        QVERIFY2(cov != nullptr, "FiffCov is null");
        QVERIFY2(!cov->empty(), "Noise covariance is empty");
        QVERIFY2(cov->data.rows() > 0, "Covariance matrix has 0 rows");
        QCOMPARE(cov->data.rows(), cov->data.cols()); // must be square
        QVERIFY(cov->names.size() > 0);
        QCOMPARE(cov->names.size(), (int)cov->data.rows());
        qInfo() << "        " << cov->data.rows() << "x" << cov->data.cols()
                << ", nfree =" << cov->nfree;
    }

    // ── Stage 4: Verify MNEForwardSolution from load_fwd ────────────────────
    qInfo() << "  [4/6] Verifying load_fwd::forward (MNEForwardSolution)...";
    {
        const auto fwd = result<MNEForwardSolution>(ctx.results, "load_fwd::forward");
        QVERIFY2(fwd != nullptr, "MNEForwardSolution is null");
        QVERIFY2(!fwd->empty(), "Forward solution is empty");
        QVERIFY2(fwd->nsource > 0,
                 qPrintable(QString("nsource = %1, expected > 0").arg(fwd->nsource)));
        QVERIFY(fwd->sol.data.rows() > 0);
        QVERIFY(fwd->sol.data.cols() > 0);
        QVERIFY(fwd->src.size() > 0);
        qInfo() << "        " << fwd->sol.data.rows() << "channels,"
                << fwd->nsource << "sources," << fwd->src.size() << "hemispheres";
    }

    // ── Stage 5: Verify MNEInverseOperator from make_inv ────────────────────
    qInfo() << "  [5/6] Verifying make_inv::inverse_operator (MNEInverseOperator)...";
    int expectedNsource = 0;
    {
        const auto invOp = result<MNEInverseOperator>(ctx.results, "make_inv::inverse_operator");
        QVERIFY2(invOp != nullptr, "MNEInverseOperator is null");
        QVERIFY2(invOp->nchan > 0,
                 qPrintable(QString("nchan = %1, expected > 0").arg(invOp->nchan)));
        QVERIFY2(invOp->nsource > 0,
                 qPrintable(QString("nsource = %1, expected > 0").arg(invOp->nsource)));
        QVERIFY(invOp->sing.size() > 0);
        QVERIFY(invOp->src.size() > 0);
        expectedNsource = invOp->nsource;
        qInfo() << "        nchan =" << invOp->nchan << ", nsource =" << invOp->nsource
                << ", singular values =" << invOp->sing.size();
    }

    // ── Stage 6: Verify InvSourceEstimate (dSPM STC) from apply_dspm ────────
    qInfo() << "  [6/6] Verifying apply_dspm::source_estimate (InvSourceEstimate)...";
    {
        const auto stc = result<InvSourceEstimate>(ctx.results, "apply_dspm::source_estimate");
        QVERIFY2(stc != nullptr, "InvSourceEstimate is null");
        QVERIFY2(!stc->empty(), "Source estimate is empty");

        // Dimensions: nsources x ntimes
        QCOMPARE((int)stc->data.rows(), expectedNsource);
        QVERIFY2(stc->data.cols() > 0,
                 qPrintable(QString("STC cols = %1").arg(stc->data.cols())));

        // dSPM values must be finite and non-negative (F-statistic)
        QVERIFY2(stc->data.allFinite(), "Source estimate contains non-finite values");
        QVERIFY2(stc->data.minCoeff() >= 0.0,
                 qPrintable(QString("dSPM min = %1, expected >= 0").arg(stc->data.minCoeff())));

        // Temporal parameters
        QVERIFY(stc->tstep > 0.0f);

        qInfo() << "        " << stc->data.rows() << "sources x"
                << stc->data.cols() << "time points";
        qInfo() << "        tmin =" << stc->tmin << ", tstep =" << stc->tstep;
        qInfo() << "        dSPM range: [" << stc->data.minCoeff()
                << "," << stc->data.maxCoeff() << "]";
    }

    qInfo() << "All 6 pipeline stages verified successfully with real data.";
}

//=============================================================================================================
// MNA file round-trip (write → read → compare)
//=============================================================================================================

void TestMnaInversePipeline::testMnaFileRoundTrip()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const std::filesystem::path tmpPath = path_from_utf8(tmpDir.filePath(QStringLiteral("roundtrip.mna")).toStdString());

    QVERIFY2(MnaIO::write(m_project, tmpPath), "Failed to write MNA project file");

    const MnaProject loaded = MnaIO::read(tmpPath).value();
    QCOMPARE(loaded.name, m_project.name);
    QCOMPARE(loaded.description, m_project.description);
    QCOMPARE(loaded.pipeline.size(), m_project.pipeline.size());

    for (std::size_t i = 0; i < m_project.pipeline.size(); ++i) {
        QCOMPARE(loaded.pipeline[i].id, m_project.pipeline[i].id);
        QCOMPARE(loaded.pipeline[i].op_type, m_project.pipeline[i].op_type);
        QCOMPARE(loaded.pipeline[i].inputs.size(), m_project.pipeline[i].inputs.size());
        QCOMPARE(loaded.pipeline[i].outputs.size(), m_project.pipeline[i].outputs.size());
        QCOMPARE(loaded.pipeline[i].attributes, m_project.pipeline[i].attributes);
    }
}

//=============================================================================================================

void TestMnaInversePipeline::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMnaInversePipeline)
#include "test_mna_inverse_pipeline.moc"
