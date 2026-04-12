//=============================================================================================================
/**
 * @file     test_mna_inverse_pipeline.cpp
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

#include <fiff/fiff.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>

#include <mne/mne_inverse_operator.h>
#include <mne/mne_forward_solution.h>

#include <inv/minimum_norm/inv_minimum_norm.h>
#include <inv/inv_source_estimate.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QVariant>
#include <QVariantMap>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QCoreApplication>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
// QVariant metatype declarations for types not already registered in library headers
//=============================================================================================================

// Note: QSharedPointer<FiffCov> and QSharedPointer<MNEInverseOperator> are already
// declared in fiff_cov.h and mne_inverse_operator.h respectively.
// Only declare types that are missing from upstream headers.
#ifndef Q_MOC_RUN
// FiffInfo, FiffEvoked, MNEForwardSolution, InvSourceEstimate shared pointers
// are registered at runtime below if not already known to QMetaType.
#endif

//=============================================================================================================
// HELPERS — build MnaGraph from MnaProject pipeline steps
//=============================================================================================================

namespace {

struct InputRef {
    QString nodeId;
    QString portName;
};

InputRef parseInputRef(const QString& ref)
{
    InputRef r;
    int sep = ref.indexOf(QLatin1String("::"));
    if (sep >= 0) {
        r.nodeId   = ref.left(sep);
        r.portName = ref.mid(sep + 2);
    }
    return r;
}

MnaGraph graphFromPipeline(const QList<MnaStep>& steps)
{
    MnaGraph graph;

    for (const auto& step : steps) {
        MnaNode node;
        node.id         = step.id;
        node.opType     = step.tool;
        node.attributes = step.parameters;
        node.dirty      = true;

        for (const auto& inputRef : step.inputs) {
            InputRef ref = parseInputRef(inputRef);
            MnaPort in;
            in.name           = ref.portName;
            in.dataKind       = MnaDataKind::Matrix;
            in.direction      = MnaPortDir::Input;
            in.sourceNodeId   = ref.nodeId;
            in.sourcePortName = ref.portName;
            node.inputs.append(in);
        }

        for (const auto& outName : step.outputs) {
            MnaPort out;
            out.name      = outName;
            out.dataKind  = MnaDataKind::Matrix;
            out.direction = MnaPortDir::Output;
            node.outputs.append(out);
        }

        graph.addNode(node);
    }

    return graph;
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
    MnaGraph   m_graph;
    QString    m_mnaFilePath;
    QString    m_testDataPath;
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
    QString base = QCoreApplication::applicationDirPath()
                   + QStringLiteral("/../resources/data/mne-cpp-test-data");
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
    const QString dataPath = m_testDataPath;

    //
    // load_fiff_raw — reads raw FIFF and outputs FiffInfo
    //
    reg.registerOpFunc(QStringLiteral("load_fiff_raw"),
        [dataPath](const QVariantMap& /*inputs*/, const QVariantMap& attrs) -> QVariantMap {
            QString rawPath = attrs.value(QStringLiteral("raw_path")).toString();
            rawPath.replace(QLatin1String("${TEST_DATA}"), dataPath);

            QFile rawFile(rawPath);
            FiffRawData raw(rawFile);
            auto info = QSharedPointer<FiffInfo>(new FiffInfo(raw.info));

            return {
                {QStringLiteral("raw_data"), QVariant()},
                {QStringLiteral("info"),     QVariant::fromValue(info)}
            };
        });

    //
    // load_fiff_evoked — reads evoked response
    //
    reg.registerOpFunc(QStringLiteral("load_fiff_evoked"),
        [dataPath](const QVariantMap& /*inputs*/, const QVariantMap& attrs) -> QVariantMap {
            QString evkPath = attrs.value(QStringLiteral("evoked_path")).toString();
            evkPath.replace(QLatin1String("${TEST_DATA}"), dataPath);
            int setno = attrs.value(QStringLiteral("setno"), 0).toInt();

            QFile evkFile(evkPath);
            QPair<float, float> noBaseline(-1.0f, -1.0f);
            auto evoked = QSharedPointer<FiffEvoked>(new FiffEvoked(evkFile, setno, noBaseline));

            return {{QStringLiteral("evoked"), QVariant::fromValue(evoked)}};
        });

    //
    // load_fiff_cov — reads noise covariance matrix
    //
    reg.registerOpFunc(QStringLiteral("load_fiff_cov"),
        [dataPath](const QVariantMap& /*inputs*/, const QVariantMap& attrs) -> QVariantMap {
            QString covPath = attrs.value(QStringLiteral("cov_path")).toString();
            covPath.replace(QLatin1String("${TEST_DATA}"), dataPath);

            QFile covFile(covPath);
            auto cov = QSharedPointer<FiffCov>(new FiffCov(covFile));

            return {{QStringLiteral("noise_cov"), QVariant::fromValue(cov)}};
        });

    //
    // load_forward — reads pre-computed forward solution
    //
    reg.registerOpFunc(QStringLiteral("load_forward"),
        [dataPath](const QVariantMap& /*inputs*/, const QVariantMap& attrs) -> QVariantMap {
            QString fwdPath = attrs.value(QStringLiteral("fwd_path")).toString();
            fwdPath.replace(QLatin1String("${TEST_DATA}"), dataPath);

            QFile fwdFile(fwdPath);
            auto fwd = QSharedPointer<MNEForwardSolution>(new MNEForwardSolution(fwdFile));

            return {{QStringLiteral("forward"), QVariant::fromValue(fwd)}};
        });

    //
    // make_inverse_operator — builds MNEInverseOperator from FiffInfo + forward + covariance
    //
    reg.registerOpFunc(QStringLiteral("make_inverse_operator"),
        [](const QVariantMap& inputs, const QVariantMap& attrs) -> QVariantMap {
            auto info = inputs.value(QStringLiteral("info")).value<QSharedPointer<FiffInfo>>();
            auto fwd  = inputs.value(QStringLiteral("forward")).value<QSharedPointer<MNEForwardSolution>>();
            auto cov  = inputs.value(QStringLiteral("noise_cov")).value<QSharedPointer<FiffCov>>();

            float loose = attrs.value(QStringLiteral("loose"), 0.2).toFloat();
            float depth = attrs.value(QStringLiteral("depth"), 0.8).toFloat();

            MNEInverseOperator invOpLocal = MNEInverseOperator::make_inverse_operator(
                *info, *fwd, *cov, loose, depth, false, true);

            auto invOp = QSharedPointer<MNEInverseOperator>(new MNEInverseOperator(invOpLocal));

            return {{QStringLiteral("inverse_operator"), QVariant::fromValue(invOp)}};
        });

    //
    // apply_inverse — applies dSPM/MNE/sLORETA to evoked data, produces source estimate
    //
    reg.registerOpFunc(QStringLiteral("apply_inverse"),
        [](const QVariantMap& inputs, const QVariantMap& attrs) -> QVariantMap {
            auto invOp  = inputs.value(QStringLiteral("inverse_operator")).value<QSharedPointer<MNEInverseOperator>>();
            auto evoked = inputs.value(QStringLiteral("evoked")).value<QSharedPointer<FiffEvoked>>();

            QString method  = attrs.value(QStringLiteral("method"), QStringLiteral("dSPM")).toString();
            float snr       = attrs.value(QStringLiteral("snr"), 3.0).toFloat();
            float lambda2   = 1.0f / (snr * snr);
            bool pickNormal = attrs.value(QStringLiteral("pick_normal"), false).toBool();

            // Pick channels that match the inverse operator's noise covariance
            FiffEvoked picked = evoked->pick_channels(invOp->noise_cov->names);

            float tmin  = picked.times(0);
            float tstep = 1.0f / picked.info.sfreq;

            InvMinimumNorm mn(*invOp, lambda2, method);
            mn.doInverseSetup(evoked->nave, pickNormal);

            InvSourceEstimate stcLocal = mn.calculateInverse(picked.data, tmin, tstep, pickNormal);
            auto stc = QSharedPointer<InvSourceEstimate>(new InvSourceEstimate(stcLocal));

            return {{QStringLiteral("source_estimate"), QVariant::fromValue(stc)}};
        });
}

//=============================================================================================================
// Load and parse the .mna project file
//=============================================================================================================

void TestMnaInversePipeline::testLoadMnaFile()
{
    qInfo() << "Loading .mna file:" << m_mnaFilePath;
    m_project = MnaIO::read(m_mnaFilePath);

    QCOMPARE(m_project.name, QStringLiteral("dSPM Inverse Pipeline"));
    QVERIFY(!m_project.description.isEmpty());
    QCOMPARE(m_project.pipeline.size(), 6);

    QStringList stepIds;
    for (const auto& step : m_project.pipeline)
        stepIds.append(step.id);

    QVERIFY(stepIds.contains(QStringLiteral("load_raw")));
    QVERIFY(stepIds.contains(QStringLiteral("load_evoked")));
    QVERIFY(stepIds.contains(QStringLiteral("load_cov")));
    QVERIFY(stepIds.contains(QStringLiteral("load_fwd")));
    QVERIFY(stepIds.contains(QStringLiteral("make_inv")));
    QVERIFY(stepIds.contains(QStringLiteral("apply_dspm")));

    // Verify inverse parameters from the file
    MnaStep applyStep;
    for (const auto& step : m_project.pipeline) {
        if (step.id == QLatin1String("apply_dspm"))
            applyStep = step;
    }
    QCOMPARE(applyStep.tool, QStringLiteral("apply_inverse"));
    QCOMPARE(applyStep.parameters.value(QStringLiteral("method")).toString(), QStringLiteral("dSPM"));
    QCOMPARE(applyStep.parameters.value(QStringLiteral("snr")).toDouble(), 3.0);

    // Build the execution graph
    m_graph = graphFromPipeline(m_project.pipeline);
    QCOMPARE(m_graph.nodes().size(), 6);
}

//=============================================================================================================
// Verify topological ordering of the DAG
//=============================================================================================================

void TestMnaInversePipeline::testGraphTopology()
{
    if (m_graph.nodes().isEmpty())
        QSKIP("Graph not built — testLoadMnaFile must run first");

    QStringList order = m_graph.topologicalSort();
    QCOMPARE(order.size(), 6);

    int idxLoadRaw    = order.indexOf(QStringLiteral("load_raw"));
    int idxLoadEvoked = order.indexOf(QStringLiteral("load_evoked"));
    int idxLoadCov    = order.indexOf(QStringLiteral("load_cov"));
    int idxLoadFwd    = order.indexOf(QStringLiteral("load_fwd"));
    int idxMakeInv    = order.indexOf(QStringLiteral("make_inv"));
    int idxApplyDspm  = order.indexOf(QStringLiteral("apply_dspm"));

    // All nodes present
    QVERIFY(idxLoadRaw >= 0);
    QVERIFY(idxLoadEvoked >= 0);
    QVERIFY(idxLoadCov >= 0);
    QVERIFY(idxLoadFwd >= 0);
    QVERIFY(idxMakeInv >= 0);
    QVERIFY(idxApplyDspm >= 0);

    // Dependency ordering
    QVERIFY(idxLoadRaw < idxMakeInv);
    QVERIFY(idxLoadFwd < idxMakeInv);
    QVERIFY(idxLoadCov < idxMakeInv);
    QVERIFY(idxMakeInv < idxApplyDspm);
    QVERIFY(idxLoadEvoked < idxApplyDspm);
}

//=============================================================================================================
// Execute the full pipeline with real FIFF data and verify every intermediate result
//=============================================================================================================

void TestMnaInversePipeline::testExecuteRealPipeline()
{
    if (m_graph.nodes().isEmpty())
        QSKIP("Graph not built — testLoadMnaFile must run first");

    qInfo() << "Executing real dSPM inverse pipeline with sample_audvis data...";
    MnaGraphExecutor::Context ctx = MnaGraphExecutor::execute(m_graph, {});

    // ── Stage 1: Verify FiffInfo from load_raw ──────────────────────────────
    qInfo() << "  [1/6] Verifying load_raw::info (FiffInfo)...";
    {
        QVariant v = ctx.results.value(QStringLiteral("load_raw::info"));
        QVERIFY2(v.isValid(), "load_raw::info not in results");
        auto info = v.value<QSharedPointer<FiffInfo>>();
        QVERIFY2(!info.isNull(), "FiffInfo is null");
        QVERIFY2(info->nchan > 0,
                 qPrintable(QString("nchan = %1, expected > 0").arg(info->nchan)));
        QVERIFY(info->sfreq > 0.0);
        qInfo() << "        nchan =" << info->nchan << ", sfreq =" << info->sfreq << "Hz";
    }

    // ── Stage 2: Verify FiffEvoked from load_evoked ─────────────────────────
    qInfo() << "  [2/6] Verifying load_evoked::evoked (FiffEvoked)...";
    {
        QVariant v = ctx.results.value(QStringLiteral("load_evoked::evoked"));
        QVERIFY2(v.isValid(), "load_evoked::evoked not in results");
        auto evoked = v.value<QSharedPointer<FiffEvoked>>();
        QVERIFY2(!evoked.isNull(), "FiffEvoked is null");
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
        QVariant v = ctx.results.value(QStringLiteral("load_cov::noise_cov"));
        QVERIFY2(v.isValid(), "load_cov::noise_cov not in results");
        auto cov = v.value<QSharedPointer<FiffCov>>();
        QVERIFY2(!cov.isNull(), "FiffCov is null");
        QVERIFY2(!cov->isEmpty(), "Noise covariance is empty");
        QVERIFY2(cov->data.rows() > 0, "Covariance matrix has 0 rows");
        QCOMPARE(cov->data.rows(), cov->data.cols());   // must be square
        QVERIFY(cov->names.size() > 0);
        QCOMPARE(cov->names.size(), (int)cov->data.rows());
        qInfo() << "        " << cov->data.rows() << "x" << cov->data.cols()
                << ", nfree =" << cov->nfree;
    }

    // ── Stage 4: Verify MNEForwardSolution from load_fwd ────────────────────
    qInfo() << "  [4/6] Verifying load_fwd::forward (MNEForwardSolution)...";
    {
        QVariant v = ctx.results.value(QStringLiteral("load_fwd::forward"));
        QVERIFY2(v.isValid(), "load_fwd::forward not in results");
        auto fwd = v.value<QSharedPointer<MNEForwardSolution>>();
        QVERIFY2(!fwd.isNull(), "MNEForwardSolution is null");
        QVERIFY2(!fwd->isEmpty(), "Forward solution is empty");
        QVERIFY2(fwd->nsource > 0,
                 qPrintable(QString("nsource = %1, expected > 0").arg(fwd->nsource)));
        QVERIFY(fwd->sol->data.rows() > 0);
        QVERIFY(fwd->sol->data.cols() > 0);
        QVERIFY(fwd->src.size() > 0);
        qInfo() << "        " << fwd->sol->data.rows() << "channels,"
                << fwd->nsource << "sources," << fwd->src.size() << "hemispheres";
    }

    // ── Stage 5: Verify MNEInverseOperator from make_inv ────────────────────
    qInfo() << "  [5/6] Verifying make_inv::inverse_operator (MNEInverseOperator)...";
    int expectedNsource = 0;
    {
        QVariant v = ctx.results.value(QStringLiteral("make_inv::inverse_operator"));
        QVERIFY2(v.isValid(), "make_inv::inverse_operator not in results");
        auto invOp = v.value<QSharedPointer<MNEInverseOperator>>();
        QVERIFY2(!invOp.isNull(), "MNEInverseOperator is null");
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
        QVariant v = ctx.results.value(QStringLiteral("apply_dspm::source_estimate"));
        QVERIFY2(v.isValid(), "apply_dspm::source_estimate not in results");
        auto stc = v.value<QSharedPointer<InvSourceEstimate>>();
        QVERIFY2(!stc.isNull(), "InvSourceEstimate is null");
        QVERIFY2(!stc->isEmpty(), "Source estimate is empty");

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
    QString tmpPath = tmpDir.path() + QStringLiteral("/roundtrip.mna");

    bool written = MnaIO::write(m_project, tmpPath);
    QVERIFY2(written, "Failed to write MNA project file");

    MnaProject loaded = MnaIO::read(tmpPath);
    QCOMPARE(loaded.name, m_project.name);
    QCOMPARE(loaded.description, m_project.description);
    QCOMPARE(loaded.pipeline.size(), m_project.pipeline.size());

    for (int i = 0; i < m_project.pipeline.size(); ++i) {
        QCOMPARE(loaded.pipeline[i].id,   m_project.pipeline[i].id);
        QCOMPARE(loaded.pipeline[i].tool, m_project.pipeline[i].tool);
        QCOMPARE(loaded.pipeline[i].inputs.size(),  m_project.pipeline[i].inputs.size());
        QCOMPARE(loaded.pipeline[i].outputs.size(), m_project.pipeline[i].outputs.size());
    }
}

//=============================================================================================================

void TestMnaInversePipeline::cleanupTestCase()
{
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMnaInversePipeline)
#include "test_mna_inverse_pipeline.moc"
