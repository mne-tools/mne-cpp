//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_label_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Cross validates label restriction and clustering of forward / inverse operators against mne-python.
 *
 * Restricting an operator to a label is pure bookkeeping, which is exactly why
 * it fails quietly: an offset into the wrong hemisphere or one normal per
 * source instead of three still yields a matrix of the right size.
 *
 * Reference values (ref-sample_audvis-meg-eeg-oct-6-fwd.fif, free orientation,
 * 3956 lh + 3972 rh sources):
 *
 *   from mne.source_space._source_space import label_src_vertno_sel
 *   v1 = mne.read_label('lh.V1.label')
 *   st = [l for l in mne.read_labels_from_annot('sample', 'aparc')
 *         if l.name == 'superiortemporal-rh'][0]
 *   vno, sel = label_src_vertno_sel(label, fwd['src'])
 *   f2 = mne.forward.restrict_forward_to_label(fwd, labels)
 *   np.abs(f2['sol']['data'].astype(float)).sum(), f2['source_rr'][0], ...
 *
 * Clustering (k-means over gain columns) has no mne-python counterpart; it is
 * checked through invariants that hold for any clustering result.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>
#include <mne/mne_source_spaces.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked.h>
#include <fs/fs_label.h>
#include <fs/fs_annotationset.h>
#include <fs/fs_surfaceset.h>

#include <cmath>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;
using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestMneLabelPython
 *
 * @brief Checks label restriction and clustering of forward and inverse operators.
 */
class TestMneLabelPython : public QObject
{
    Q_OBJECT

public:
    TestMneLabelPython() = default;

private:
    static QString dataPath(const QString& file);
    FsLabel label(const QString& name) const;

    MNEForwardSolution m_fwd;
    FsAnnotationSet m_annot;
    QList<FsLabel> m_aparc;
    FsLabel m_v1;

private slots:
    void initTestCase();

    void labelSourceSelection_data();
    void labelSourceSelection();
    void pickRegions_matchesPython_data();
    void pickRegions_matchesPython();
    void sourcePositionsByLabel();

    void clusterForward_invariants();
    void assembleKernel_label_data();
    void assembleKernel_label();
    void clusterKernel_invariants();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestMneLabelPython::dataPath(const QString& file)
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/" + file;
}

//=============================================================================================================

FsLabel TestMneLabelPython::label(const QString& name) const
{
    if (name == "lh.V1") {
        return m_v1;
    }
    for (const FsLabel& l : m_aparc) {
        if (l.name == name) {
            return l;
        }
    }
    return FsLabel();
}

//=============================================================================================================

void TestMneLabelPython::initTestCase()
{
    const QString fwdPath = dataPath("Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif");
    if (!QFile::exists(fwdPath)) {
        QSKIP("Forward solution not found");
    }
    QFile fwdFile(fwdPath);
    m_fwd = MNEForwardSolution(fwdFile);
    QVERIFY(!m_fwd.isEmpty());
    QVERIFY(!m_fwd.isFixedOrient());
    QCOMPARE(m_fwd.src[0].nuse, 3956);
    QCOMPARE(m_fwd.src[1].nuse, 3972);

    QVERIFY(FsLabel::read(dataPath("subjects/sample/label/lh.V1.label"), m_v1));

    m_annot = FsAnnotationSet(dataPath("subjects/sample/label/lh.aparc.annot"), dataPath("subjects/sample/label/rh.aparc.annot"));
    const FsSurfaceSet white(dataPath("subjects/sample/surf/lh.white"), dataPath("subjects/sample/surf/rh.white"));
    QList<RowVector4i> rgbas;
    QVERIFY(m_annot.toLabels(white, m_aparc, rgbas));
}

//=============================================================================================================

void TestMneLabelPython::labelSourceSelection_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("count");
    QTest::addColumn<int>("first");
    QTest::addColumn<int>("last");
    QTest::addColumn<int>("firstVertno");

    // label_src_vertno_sel: indices into the concatenated lh + rh source list.
    // For a right-hemisphere label they start after the 3956 left sources; the
    // old code offset them by the label's own vertex count (5404) instead.
    QTest::newRow("lh.V1") << "lh.V1" << 70 << 2 << 1084 << 582;
    QTest::newRow("superiortemporal-lh") << "superiortemporal-lh" << 138 << 1254 << 3446 << 54493;
    QTest::newRow("superiortemporal-rh") << "superiortemporal-rh" << 149 << 5341 << 7491 << 59944;
}

//=============================================================================================================

void TestMneLabelPython::labelSourceSelection()
{
    QFETCH(QString, name);
    QFETCH(int, count);
    QFETCH(int, first);
    QFETCH(int, last);
    QFETCH(int, firstVertno);

    const FsLabel l = label(name);
    QVERIFY2(!l.isEmpty(), qPrintable(name));

    VectorXi sel;
    const QList<VectorXi> vertno = m_fwd.src.label_src_vertno_sel(l, sel);

    QCOMPARE(static_cast<int>(sel.size()), count);
    QCOMPARE(sel(0), first);
    QCOMPARE(sel(sel.size() - 1), last);
    QCOMPARE(static_cast<int>(vertno[l.hemi].size()), count);
    QCOMPARE(vertno[l.hemi](0), firstVertno);
    QCOMPARE(static_cast<int>(vertno[1 - l.hemi].size()), 0);
}

//=============================================================================================================

void TestMneLabelPython::pickRegions_matchesPython_data()
{
    QTest::addColumn<QStringList>("labels");
    QTest::addColumn<int>("nuseLh");
    QTest::addColumn<int>("nuseRh");
    QTest::addColumn<double>("gainAbsSum");
    QTest::addColumn<QList<double>>("firstRr");
    QTest::addColumn<QList<double>>("lastRr");

    // mne.forward.restrict_forward_to_label(fwd, labels)
    QTest::newRow("lh only") << QStringList{"lh.V1"} << 70 << 0 << 495998.3601547581 << QList<double>{-0.01345682260649108, -0.056182629128693914, 0.08644477516463758} << QList<double>{-0.020675333420574378, -0.016903093316528395, 0.06243761194864744};
    QTest::newRow("rh only") << QStringList{"superiortemporal-rh"} << 0 << 149 << 1250462.4633440091 << QList<double>{0.059981437077805566, -0.0063803256266439234, 0.06841191879069775} << QList<double>{0.03484627047157732, 0.04595065929692144, 0.02361936359956074};
    QTest::newRow("both") << QStringList{"lh.V1", "superiortemporal-rh"} << 70 << 149 << 1746460.8234987673 << QList<double>{-0.01345682260649108, -0.056182629128693914, 0.08644477516463758} << QList<double>{0.03484627047157732, 0.04595065929692144, 0.02361936359956074};
}

//=============================================================================================================

void TestMneLabelPython::pickRegions_matchesPython()
{
    QFETCH(QStringList, labels);
    QFETCH(int, nuseLh);
    QFETCH(int, nuseRh);
    QFETCH(double, gainAbsSum);
    QFETCH(QList<double>, firstRr);
    QFETCH(QList<double>, lastRr);

    QList<FsLabel> picked;
    for (const QString& name : labels) {
        picked << label(name);
    }

    const MNEForwardSolution f = m_fwd.pick_regions(picked);
    const int nsrc = nuseLh + nuseRh;

    QCOMPARE(f.nsource, nsrc);
    QCOMPARE(static_cast<int>(f.sol->data.cols()), 3 * nsrc);
    QCOMPARE(f.sol->ncol, 3 * nsrc);
    QCOMPARE(static_cast<int>(f.source_rr.rows()), nsrc);
    // Free orientation: three normals per source, as in mne-python.
    QCOMPARE(static_cast<int>(f.source_nn.rows()), 3 * nsrc);

    QCOMPARE(f.src[0].nuse, nuseLh);
    QCOMPARE(f.src[1].nuse, nuseRh);
    QCOMPARE(f.src[0].inuse.sum(), nuseLh);
    QCOMPARE(f.src[1].inuse.sum(), nuseRh);
    for (int h = 0; h < 2; ++h) {
        for (int k = 0; k < f.src[h].vertno.size(); ++k) {
            QCOMPARE(f.src[h].inuse(f.src[h].vertno(k)), 1);
        }
    }

    // The gain matrix is float32 on disk, so the sums of the same elements
    // agree to summation order (~1e-12 relative); 1e-9 catches one column
    // taken from a neighbouring source.
    const double sum = f.sol->data.cwiseAbs().sum();
    QVERIFY2(std::fabs(sum - gainAbsSum) < 1e-9 * gainAbsSum, qPrintable(QString("gain abs sum %1, mne-python %2").arg(sum, 0, 'g', 17).arg(gainAbsSum, 0, 'g', 17)));

    // Positions are float32, so compare at float precision.
    for (int c = 0; c < 3; ++c) {
        QVERIFY(std::fabs(f.source_rr(0, c) - firstRr.at(c)) < 1e-7);
        QVERIFY(std::fabs(f.source_rr(nsrc - 1, c) - lastRr.at(c)) < 1e-7);
    }
}

//=============================================================================================================

void TestMneLabelPython::sourcePositionsByLabel()
{
    const FsSurfaceSet white(dataPath("subjects/sample/surf/lh.white"), dataPath("subjects/sample/surf/rh.white"));

    QVERIFY(m_fwd.getSourcePositionsByLabel({}, white).rows() == 0);
    QVERIFY(m_fwd.getSourcePositionsByLabel({m_v1}, FsSurfaceSet()).rows() == 0);

    // One row per source of the label, taken from the given surface.
    const MatrixX3f pos = m_fwd.getSourcePositionsByLabel({m_v1, label("superiortemporal-rh")}, white);
    QCOMPARE(static_cast<int>(pos.rows()), 70 + 149);
    const Vector3f expected = white[0].rr().row(582).transpose() - white[0].offset();
    QVERIFY((pos.row(0).transpose() - expected).norm() < 1e-6f);
}

//=============================================================================================================

void TestMneLabelPython::clusterForward_invariants()
{
    // Clustering every aparc region is slow, so restrict to two regions first.
    const MNEForwardSolution small = m_fwd.pick_regions({m_v1, label("superiortemporal-rh")});

    // sqeuclidean centroids are means (cityblock would give medians).
    MatrixXd D;
    const MNEForwardSolution c = small.cluster_forward_solution(m_annot, 40, D, FiffCov(), FiffInfo(), "sqeuclidean");
    QVERIFY(c.isClustered());

    int nClusters = 0;
    int nAssigned = 0;
    for (int h = 0; h < 2; ++h) {
        const MNEClusterInfo& info = c.src.hemisphereAt(h)->cluster_info;
        nClusters += info.clusterVertnos.size();
        QCOMPARE(info.centroidVertno.size(), info.clusterVertnos.size());
        for (int k = 0; k < info.clusterVertnos.size(); ++k) {
            nAssigned += static_cast<int>(info.clusterVertnos[k].size());
            // Every centroid is one of the region's own sources.
            QVERIFY((small.src[h].vertno.array() == info.centroidVertno[k]).any());
        }
    }

    // Each source lands in exactly one cluster, and no region holds more than
    // the requested cluster size on average.
    QCOMPARE(nAssigned, small.nsource);
    QCOMPARE(c.nsource, nClusters);
    QCOMPARE(static_cast<int>(c.sol->data.cols()), 3 * nClusters);
    QVERIFY(nClusters >= (small.nsource + 39) / 40);

    // D averages the sources of each cluster: columns sum to one per
    // orientation, each source contributes to exactly one cluster.
    QCOMPARE(static_cast<int>(D.rows()), 3 * small.nsource);
    QCOMPARE(static_cast<int>(D.cols()), 3 * nClusters);
    QVERIFY((D.colwise().sum().array() - 1.0).abs().maxCoeff() < 1e-12);
    QVERIFY(((D.array() > 0).rowwise().count() == 1).all());

    // The clustered gain is the cluster average of the original gain columns.
    QVERIFY((c.sol->data - small.sol->data * D).cwiseAbs().maxCoeff() < 1e-9 * small.sol->data.cwiseAbs().maxCoeff());

    // With a noise covariance the clusters are found on the whitened gain (MEG and EEG in
    // comparable units) but the centroids remain averages of the original gain.
    QFile covFile(dataPath("MEG/sample/sample_audvis-cov.fif"));
    const FiffCov cov(covFile);
    QFile aveFile(dataPath("MEG/sample/sample_audvis-ave.fif"));
    const FiffEvoked evoked(aveFile, 0);
    MatrixXd Dw;
    const MNEForwardSolution cw = small.cluster_forward_solution(m_annot, 40, Dw, cov, evoked.info, "cityblock");
    QVERIFY(cw.isClustered());
    QCOMPARE(static_cast<int>(Dw.rows()), 3 * small.nsource);
    QVERIFY((Dw.colwise().sum().array() - 1.0).abs().maxCoeff() < 1e-12);
    QVERIFY(((Dw.array() > 0).rowwise().count() == 1).all());
    QVERIFY((cw.sol->data - small.sol->data * Dw).cwiseAbs().maxCoeff() < 1e-9 * small.sol->data.cwiseAbs().maxCoeff());

    // Fixed orientation is not supported and returns the input unchanged.
    MNEForwardSolution fixed = small;
    fixed.source_ori = FIFFV_MNE_FIXED_ORI;
    MatrixXd Df;
    QTest::ignoreMessage(QtWarningMsg, "Error: Fixed orientation not implemented yet!");
    QVERIFY(!fixed.cluster_forward_solution(m_annot, 40, Df, FiffCov(), FiffInfo()).isClustered());
}

//=============================================================================================================

void TestMneLabelPython::assembleKernel_label_data()
{
    QTest::addColumn<QString>("method");
    QTest::addColumn<QString>("name");

    QTest::newRow("MNE lh") << "MNE" << "lh.V1";
    QTest::newRow("dSPM lh") << "dSPM" << "lh.V1";
    QTest::newRow("dSPM rh") << "dSPM" << "superiortemporal-rh";
    QTest::newRow("sLORETA rh") << "sLORETA" << "superiortemporal-rh";
}

//=============================================================================================================

void TestMneLabelPython::assembleKernel_label()
{
    QFETCH(QString, method);
    QFETCH(QString, name);

    const QString covPath = dataPath("MEG/sample/sample_audvis-cov.fif");
    const QString avePath = dataPath("MEG/sample/sample_audvis-ave.fif");
    QFile covFile(covPath);
    const FiffCov cov(covFile);
    QFile aveFile(avePath);
    const FiffEvoked evoked(aveFile, 0);

    // A restricted forward keeps the inverse small while leaving sources in
    // both hemispheres, so the rh offset is exercised.
    const MNEForwardSolution fwd = m_fwd.pick_regions({m_v1, label("superiortemporal-rh")});
    const MNEInverseOperator inv = MNEInverseOperator::make_inverse_operator(evoked.info, fwd, cov, 0.2f, 0.8f, false, true);
    const bool dSPM = method == "dSPM";
    const bool sLORETA = method == "sLORETA";
    MNEInverseOperator prepared = inv.prepare_inverse_operator(evoked.nave, 1.0f / 9.0f, dSPM, sLORETA);

    MatrixXd kFull;
    SparseMatrix<double> nnFull;
    QList<VectorXi> vFull;
    QVERIFY(prepared.assemble_kernel(FsLabel(), method, false, kFull, nnFull, vFull));

    const FsLabel l = label(name);
    MatrixXd kLabel;
    SparseMatrix<double> nnLabel;
    QList<VectorXi> vLabel;
    QVERIFY(prepared.assemble_kernel(l, method, false, kLabel, nnLabel, vLabel));

    // The label kernel is the label's rows of the full kernel, the same rows
    // mne-python's _assemble_kernel selects with src_sel. It is computed by a
    // separate matrix product, so compare to rounding (FMA/SIMD differ by
    // platform), not bitwise.
    VectorXi sel;
    prepared.src.label_src_vertno_sel(l, sel);
    QCOMPARE(static_cast<int>(kLabel.rows()), 3 * static_cast<int>(sel.size()));
    QCOMPARE(kLabel.cols(), kFull.cols());
    const int hemiOffset = l.hemi == 1 ? static_cast<int>(vFull[0].size()) : 0;
    QCOMPARE(static_cast<int>(vLabel[l.hemi].size()), static_cast<int>(sel.size()));
    for (int i = 0; i < sel.size(); ++i) {
        const MatrixXd rowsFull = kFull.block(3 * sel(i), 0, 3, kFull.cols());
        QVERIFY2((kLabel.block(3 * i, 0, 3, kLabel.cols()) - rowsFull).norm() <= 1e-10 * rowsFull.norm(),
                 qPrintable(QStringLiteral("label source %1").arg(i)));
        QCOMPARE(vLabel[l.hemi](i), vFull[l.hemi](sel(i) - hemiOffset));
    }

    if (method == "MNE") {
        QCOMPARE(static_cast<int>(nnLabel.rows()), 0);
    } else {
        QCOMPARE(static_cast<int>(nnLabel.rows()), static_cast<int>(sel.size()));
        QCOMPARE(static_cast<int>(nnLabel.cols()), static_cast<int>(sel.size()));
        for (int i = 0; i < sel.size(); ++i) {
            QCOMPARE(nnLabel.coeff(i, i), nnFull.coeff(sel(i), sel(i)));
            QVERIFY(nnLabel.coeff(i, i) > 0.0);
        }
    }
}

//=============================================================================================================

void TestMneLabelPython::clusterKernel_invariants()
{
    const QString covPath = dataPath("MEG/sample/sample_audvis-cov.fif");
    QFile covFile(covPath);
    const FiffCov cov(covFile);
    QFile aveFile(dataPath("MEG/sample/sample_audvis-ave.fif"));
    const FiffEvoked evoked(aveFile, 0);

    const MNEForwardSolution fwd = m_fwd.pick_regions({m_v1, label("superiortemporal-rh")});
    MNEInverseOperator inv = MNEInverseOperator::make_inverse_operator(evoked.info, fwd, cov, 0.2f, 0.8f, false, true)
                                 .prepare_inverse_operator(evoked.nave, 1.0f / 9.0f, false);
    MatrixXd K;
    SparseMatrix<double> nn;
    QList<VectorXi> v;
    QVERIFY(inv.assemble_kernel(FsLabel(), "MNE", false, K, nn, v));

    MatrixXd D;
    const MatrixXd clusteredMT = inv.cluster_kernel(m_annot, 40, D, "sqeuclidean");

    QCOMPARE(static_cast<int>(D.rows()), static_cast<int>(K.rows()));
    QCOMPARE(clusteredMT.cols(), D.cols());
    QCOMPARE(clusteredMT.rows(), K.cols());
    QVERIFY((D.colwise().sum().array() - 1.0).abs().maxCoeff() < 1e-12);
    QVERIFY(((D.array() > 0).rowwise().count() == 1).all());
    // Cluster centroids are the means of the clustered kernel rows.
    QVERIFY((clusteredMT - K.transpose() * D).cwiseAbs().maxCoeff() < 1e-9 * K.cwiseAbs().maxCoeff());
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneLabelPython)
#include "test_mne_label_python.moc"
