//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fwd_bem_driver_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Checks the BEM forward drivers against an mne-python gain matrix.
 *
 * data/ (make_bem_driver_fixtures.py) holds a three-layer concentric-sphere BEM
 * solved by mne-python, the sensor and source geometry, and mne-python's
 * free-orientation gain. FwdBemModel::compute_forward_meg/_eeg are run on it
 * threaded and unthreaded, with free and fixed orientations and with position
 * gradients, which are checked by central differences of the same drivers.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fwd/fwd_bem_model.h>
#include <fwd/fwd_coil_set.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_named_matrix.h>
#include <mne/mne_source_space.h>
#include <mne/mne_surface.h>

#include <memory>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FWDLIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace Eigen;

namespace
{

constexpr int kNMeg = 4;
constexpr int kNEeg = 4;
constexpr int kNSrc = 5;

MatrixXd readMatrix(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    std::vector<std::vector<double>> rows;
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QStringList parts = in.readLine().split(' ', Qt::SkipEmptyParts);
        if (parts.isEmpty())
            continue;
        rows.emplace_back();
        for (const QString& p : parts)
            rows.back().push_back(p.toDouble());
    }
    MatrixXd m(static_cast<Index>(rows.size()), static_cast<Index>(rows.front().size()));
    for (Index i = 0; i < m.rows(); ++i)
        for (Index j = 0; j < m.cols(); ++j)
            m(i, j) = rows[static_cast<size_t>(i)][static_cast<size_t>(j)];
    return m;
}

} // namespace

//=============================================================================================================
/**
 * Runs the BEM forward drivers on a sphere BEM solved by mne-python.
 */
class TestFwdBemDriverPython : public QObject
{
    Q_OBJECT

private:
    // Two source spaces (3 + 2 sources) so the drivers split work and offsets.
    std::vector<std::unique_ptr<MNESourceSpace>> spaces(const Vector3f& shift = Vector3f::Zero()) const;
    MatrixXd forward(bool meg, bool fixedOri, bool threads, bool grad, MatrixXd* gradOut, const Vector3f& shift = Vector3f::Zero());

    std::unique_ptr<FwdBemModel> m_model;
    std::unique_ptr<FwdCoilSet> m_meg;
    std::unique_ptr<FwdCoilSet> m_eeg;
    MatrixXd m_gain;
    MatrixXf m_srcRr;

private slots:
    void initTestCase();
    void gain_data();
    void gain();
    void gradient_data();
    void gradient();
    void saveModel();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

void TestFwdBemDriverPython::initTestCase()
{
    const QString dir = QStringLiteral(MNE_FWD_BEM_DRIVER_DATA_DIR);
    m_gain = readMatrix(dir + "/gain.txt");
    const MatrixXd geo = readMatrix(dir + "/geometry.txt");
    QCOMPARE(m_gain.rows(), kNMeg + kNEeg);
    QCOMPARE(m_gain.cols(), 3 * kNSrc);
    QCOMPARE(geo.rows(), 2 * kNMeg + kNEeg + kNSrc);
    m_srcRr = geo.bottomRows(kNSrc).cast<float>();

    m_model = FwdBemModel::fwd_bem_load_three_layer_surfaces(dir + "/sphere3-bem-sol.fif");
    QVERIFY(m_model != nullptr);
    QCOMPARE(m_model->fwd_bem_load_recompute_solution(dir + "/sphere3-bem-sol.fif", FWD_BEM_UNKNOWN, 0), 0);
    QCOMPARE(m_model->bem_method, static_cast<int>(FWD_BEM_LINEAR_COLL));
    QCOMPARE(m_model->fwd_bem_set_head_mri_t(FiffCoordTrans::identity(FIFFV_COORD_HEAD, FIFFV_COORD_MRI)), 0);

    QList<FiffChInfo> meg, eeg;
    for (int k = 0; k < kNMeg; ++k) {
        FiffChInfo ch;
        ch.ch_name = QStringLiteral("MEG%1").arg(k);
        ch.kind = FIFFV_MEG_CH;
        ch.chpos.coil_type = FIFFV_COIL_POINT_MAGNETOMETER;
        ch.chpos.r0 = geo.row(k).cast<float>().transpose();
        const Vector3f ez = geo.row(kNMeg + k).cast<float>().transpose();
        const Vector3f ref = std::abs(ez.y()) < 0.9f ? Vector3f::UnitY() : Vector3f::UnitX();
        ch.chpos.ex = ref.cross(ez).normalized();
        ch.chpos.ey = ez.cross(ch.chpos.ex);
        ch.chpos.ez = ez;
        meg << ch;
    }
    for (int k = 0; k < kNEeg; ++k) {
        FiffChInfo ch;
        ch.ch_name = QStringLiteral("EEG%1").arg(k);
        ch.kind = FIFFV_EEG_CH;
        ch.chpos.coil_type = FIFFV_COIL_EEG;
        ch.chpos.r0 = geo.row(2 * kNMeg + k).cast<float>().transpose();
        eeg << ch;
    }
    auto defs = FwdCoilSet::read_coil_defs(QCoreApplication::applicationDirPath() + "/../resources/general/coilDefinitions/coil_def.dat");
    QVERIFY(defs != nullptr);
    m_meg = defs->create_meg_coils(meg, kNMeg, FWD_COIL_ACCURACY_ACCURATE, FiffCoordTrans::identity(FIFFV_COORD_DEVICE, FIFFV_COORD_HEAD));
    m_eeg = FwdCoilSet::create_eeg_els(eeg, kNEeg);
    QVERIFY(m_meg && m_eeg);
}

//=============================================================================================================

std::vector<std::unique_ptr<MNESourceSpace>> TestFwdBemDriverPython::spaces(const Vector3f& shift) const
{
    std::vector<std::unique_ptr<MNESourceSpace>> out;
    for (const auto& [first, n] : {std::pair{0, 3}, std::pair{3, 2}}) {
        auto s = MNESourceSpace::create_source_space(n + 1);
        s->coord_frame = FIFFV_COORD_HEAD;
        for (int j = 0; j < n; ++j) {
            s->point(j) = m_srcRr.row(first + j).transpose() + shift;
            s->nn.row(j) = RowVector3f(0.0f, 0.0f, 1.0f);
            s->inuse[j] = 1;
        }
        // An unused vertex the drivers have to skip.
        s->point(n) = Vector3f(0.0f, 0.0f, 0.0f);
        s->nuse = n;
        out.push_back(std::move(s));
    }
    return out;
}

//=============================================================================================================

MatrixXd TestFwdBemDriverPython::forward(bool meg, bool fixedOri, bool threads, bool grad, MatrixXd* gradOut, const Vector3f& shift)
{
    auto src = spaces(shift);
    FiffNamedMatrix resp, respGrad;
    const int stat = meg ? m_model->compute_forward_meg(src, m_meg.get(), nullptr, nullptr, fixedOri, Vector3f::Zero(), threads, resp, respGrad, grad)
                         : m_model->compute_forward_eeg(src, m_eeg.get(), fixedOri, nullptr, threads, resp, respGrad, grad);
    if (stat != 0)
        return {};
    if (gradOut)
        *gradOut = respGrad.data;
    return resp.data;
}

//=============================================================================================================

void TestFwdBemDriverPython::gain_data()
{
    QTest::addColumn<bool>("meg");
    QTest::addColumn<bool>("threads");
    QTest::newRow("MEG, threads") << true << true;
    QTest::newRow("MEG, no threads") << true << false;
    QTest::newRow("EEG, threads") << false << true;
    QTest::newRow("EEG, no threads") << false << false;
}

void TestFwdBemDriverPython::gain()
{
    QFETCH(bool, meg);
    QFETCH(bool, threads);

    const MatrixXd ref = meg ? m_gain.topRows(kNMeg) : m_gain.bottomRows(kNEeg);
    const MatrixXd free = forward(meg, false, threads, false, nullptr);
    QCOMPARE(free.rows(), ref.rows());
    QCOMPARE(free.cols(), ref.cols());
    // mne-python uses the same linear collocation solution and point sensors in float64.
    const double scale = ref.cwiseAbs().maxCoeff();
    QVERIFY2((free - ref).cwiseAbs().maxCoeff() < 1e-4 * scale, qPrintable(QString("max diff %1 of %2").arg((free - ref).cwiseAbs().maxCoeff()).arg(scale)));

    // Fixed orientation along the source normal (+z) is the z column of each source.
    const MatrixXd fixed = forward(meg, true, threads, false, nullptr);
    QCOMPARE(fixed.cols(), kNSrc);
    for (int s = 0; s < kNSrc; ++s)
        QVERIFY((fixed.col(s) - free.col(3 * s + 2)).cwiseAbs().maxCoeff() < 1e-6 * scale);

    // The gradient drivers must return the same field values.
    MatrixXd g;
    QVERIFY((forward(meg, false, threads, true, &g) - free).cwiseAbs().maxCoeff() < 1e-6 * scale);
    QCOMPARE(g.cols(), 9 * kNSrc);
    QVERIFY((forward(meg, true, threads, true, &g) - fixed).cwiseAbs().maxCoeff() < 1e-6 * scale);
    QCOMPARE(g.cols(), 3 * kNSrc);
}

//=============================================================================================================

void TestFwdBemDriverPython::gradient_data()
{
    gain_data();
}

void TestFwdBemDriverPython::gradient()
{
    QFETCH(bool, meg);
    QFETCH(bool, threads);

    // Columns per source: (d/dx, d/dy, d/dz) of Qx, then of Qy, then of Qz.
    MatrixXd g;
    const MatrixXd free = forward(meg, false, threads, true, &g);
    QCOMPARE(g.cols(), 9 * kNSrc);
    const float h = 1e-4f;
    for (int c = 0; c < 3; ++c) {
        Vector3f step = Vector3f::Zero();
        step(c) = h;
        const MatrixXd numeric = (forward(meg, false, threads, false, nullptr, step) - forward(meg, false, threads, false, nullptr, -step)) / (2.0 * h);
        for (int s = 0; s < kNSrc; ++s) {
            for (int q = 0; q < 3; ++q) {
                const VectorXd analytic = g.col(9 * s + 3 * q + c);
                const VectorXd num = numeric.col(3 * s + q);
                QVERIFY2((analytic - num).norm() < 2e-2 * num.norm() + 1e-3 * free.cwiseAbs().maxCoeff(),
                         qPrintable(QString("source %1 Q%2 d/d%3: %4 vs %5").arg(s).arg(QChar('x' + q)).arg(QChar('x' + c)).arg(analytic.norm()).arg(num.norm())));
            }
        }
    }
}

//=============================================================================================================
// MAIN
//=============================================================================================================

void TestFwdBemDriverPython::saveModel()
{
    // The model read from mne.write_bem_solution's file is saved and read back unchanged
    QTemporaryDir dir;
    const QString linear = dir.filePath(QStringLiteral("linear-bem-sol.fif"));
    QCOMPARE(m_model->fwd_bem_save_model(linear), 0);
    auto back = FwdBemModel::fwd_bem_load_three_layer_surfaces(linear);
    QVERIFY(back != nullptr);
    QCOMPARE(back->sigma, m_model->sigma);
    for (int k = 0; k < m_model->nsurf; ++k) {
        QCOMPARE(back->surfs[k]->id, m_model->surfs[k]->id);
        QCOMPARE(back->surfs[k]->rr, m_model->surfs[k]->rr);
        QCOMPARE(back->surfs[k]->itris, m_model->surfs[k]->itris);
    }
    QCOMPARE(back->fwd_bem_load_recompute_solution(linear, FWD_BEM_LINEAR_COLL, 0), 0);
    QCOMPARE(back->sol_name, linear);
    QCOMPARE(back->solution, m_model->solution);

    // A constant collocation solution comes back as one; asking for the linear one recomputes it
    QCOMPARE(back->fwd_bem_compute_solution(FWD_BEM_CONSTANT_COLL), 0);
    const MatrixXf constantSolution = back->solution;
    const QString constant = dir.filePath(QStringLiteral("constant-bem-sol.fif"));
    QCOMPARE(back->fwd_bem_save_model(constant), 0);
    auto reread = FwdBemModel::fwd_bem_load_three_layer_surfaces(constant);
    QVERIFY(reread != nullptr);
    QCOMPARE(reread->fwd_bem_load_recompute_solution(constant, FWD_BEM_UNKNOWN, 0), 0);
    QCOMPARE(reread->bem_method, static_cast<int>(FWD_BEM_CONSTANT_COLL));
    QCOMPARE(reread->sol_name, constant);
    QCOMPARE(reread->solution, constantSolution);
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Approximation method in file"));
    QCOMPARE(reread->fwd_bem_load_recompute_solution(constant, FWD_BEM_LINEAR_COLL, 0), 0);
    QVERIFY(reread->sol_name.isEmpty());
    // The recomputed linear solution matches mne.make_bem_solution's
    const float relDiff = (reread->solution - m_model->solution).norm() / m_model->solution.norm();
    QVERIFY2(relDiff < 1e-5f, qPrintable(QString::number(relDiff)));

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("No model to save"));
    QCOMPARE(FwdBemModel().fwd_bem_save_model(dir.filePath(QStringLiteral("empty.fif"))), -1);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestFwdBemDriverPython)
#include "test_fwd_bem_driver_python.moc"
