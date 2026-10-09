//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fwd_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Checks field gradients and label / fixed-orientation forward solutions against independent references.
 *
 * Two oracles:
 *
 * 1. Every analytic dipole-position gradient must be the derivative of the
 *    matching field function, checked by central finite differences.
 *
 * 2. A forward solution restricted to lh.V1 must reproduce the V1 columns of
 *    the MNE-C reference ref-sample_audvis-meg-eeg-oct-6-fwd.fif, also when
 *    computed in MRI coordinates, from an ASCII transform, or for one sensor
 *    type only. Its fixed orientation version must match mne-python:
 *
 *      fx = mne.convert_forward_solution(ref, surf_ori=True, force_fixed=True,
 *                                        use_cps=False)
 *      np.abs(fx['sol']['data'][:, sel]).sum()  ->  165133.28497793473
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fwd/fwd_bem_model.h>
#include <fwd/fwd_coil_set.h>
#include <fwd/fwd_eeg_sphere_model.h>
#include <fwd/fwd_eeg_sphere_model_set.h>
#include <fwd/compute_fwd/compute_fwd.h>
#include <fwd/compute_fwd/compute_fwd_settings.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_coord_trans.h>
#include <mne/mne_forward_solution.h>
#include <fs/fs_label.h>

#include <cmath>
#include <functional>
#include <memory>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FWDLIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace FSLIB;
using namespace Eigen;

namespace
{

using FieldFunc = std::function<int(const Vector3f&, const Vector3f&, VectorXf&)>;
using GradFunc = std::function<int(const Vector3f&, const Vector3f&, VectorXf&, VectorXf&, VectorXf&, VectorXf&)>;

} // namespace

//=============================================================================================================
/**
 * DECLARE CLASS TestFwdPython
 *
 * @brief Checks forward-model gradients and restricted forward solutions.
 */
class TestFwdPython : public QObject
{
    Q_OBJECT

private:
    static QString data(const QString& file);
    std::shared_ptr<ComputeFwdSettings> settings(bool fixedOri, bool grad);
    void checkGradient(const FieldFunc& field, const GradFunc& grad, int n);

    QTemporaryDir m_dir;
    QString m_labelPath;
    FiffRawData m_raw;
    QList<FiffChInfo> m_meg;
    QList<FiffChInfo> m_eeg;
    MNEForwardSolution m_ref;
    VectorXi m_v1Sel;

private slots:
    void initTestCase();

    void gradient_sphereMeg();
    void gradient_bemMeg_data();
    void gradient_bemMeg();
    void gradient_bemEeg_data();
    void gradient_bemEeg();
    void gradient_sphereEeg();

    void labelForward_free();
    void labelForward_fixed();
    void labelForward_variants_data();
    void labelForward_variants();
    void sphereForward_data();
    void sphereForward();
    void sphereForward_modelFile();
    void ctfCompensatedForward_data();
    void ctfCompensatedForward();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestFwdPython::data(const QString& file)
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/" + file;
}

//=============================================================================================================

void TestFwdPython::initTestCase()
{
    const QString refPath = data("Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif");
    if (!QFile::exists(refPath)) {
        QSKIP("Reference forward solution not found");
    }
    QVERIFY(m_dir.isValid());

    QFile rawFile(data("MEG/sample/sample_audvis_trunc_raw.fif"));
    m_raw = FiffRawData(rawFile);
    for (const FiffChInfo& ch : m_raw.info.chs) {
        if (ch.kind == FIFFV_MEG_CH) {
            m_meg << ch;
        } else if (ch.kind == FIFFV_EEG_CH) {
            m_eeg << ch;
        }
    }

    QFile refFile(refPath);
    m_ref = MNEForwardSolution(refFile);
    QVERIFY(!m_ref.isEmpty());

    // MNE-C assigns labels to a hemisphere by the "-lh.label" suffix.
    m_labelPath = m_dir.filePath("V1-lh.label");
    QVERIFY(QFile::copy(data("subjects/sample/label/lh.V1.label"), m_labelPath));

    FsLabel v1;
    QVERIFY(FsLabel::read(m_labelPath, v1));
    m_ref.src.label_src_vertno_sel(v1, m_v1Sel);
    QCOMPARE(static_cast<int>(m_v1Sel.size()), 70);
}

//=============================================================================================================

void TestFwdPython::checkGradient(const FieldFunc& field, const GradFunc& grad, int n)
{
    // Dipoles at a few depths and orientations inside the inner skull.
    const QList<QPair<Vector3f, Vector3f>> dipoles{
        {Vector3f(0.0f, 0.0f, 0.06f), Vector3f(1.0f, 0.0f, 0.0f)},
        {Vector3f(0.02f, -0.01f, 0.05f), Vector3f(0.0f, 1.0f, 0.0f)},
        {Vector3f(-0.03f, 0.02f, 0.04f), Vector3f(0.3f, -0.4f, 0.866f)},
    };

    // Central differences with h = 0.1 mm: truncation error is ~(h/d)^2 with
    // d ~ 3 cm, float round-off ~1e-7 / (h/d) ~ 3e-5. 1% of the gradient norm
    // is far above both and far below a wrong sign or a swapped component.
    const float h = 1e-4f;
    for (const auto& [rd, Q] : dipoles) {
        VectorXf val(n), gx(n), gy(n), gz(n);
        QCOMPARE(grad(rd, Q, val, gx, gy, gz), 0);

        VectorXf direct(n);
        QCOMPARE(field(rd, Q, direct), 0);
        QVERIFY2((val - direct).norm() <= 1e-5f * direct.norm(), "gradient call returned a different field value");

        const VectorXf* analytic[3] = {&gx, &gy, &gz};
        for (int c = 0; c < 3; ++c) {
            Vector3f step = Vector3f::Zero();
            step(c) = h;
            VectorXf plus(n), minus(n);
            QCOMPARE(field(rd + step, Q, plus), 0);
            QCOMPARE(field(rd - step, Q, minus), 0);
            const VectorXf numeric = (plus - minus) / (2.0f * h);
            const float rel = (*analytic[c] - numeric).norm() / numeric.norm();
            QVERIFY2(rel < 1e-2f, qPrintable(QString("d/d%1 relative error %2 at (%3, %4, %5)").arg(QChar('x' + c)).arg(rel).arg(rd.x()).arg(rd.y()).arg(rd.z())));
        }
    }
}

//=============================================================================================================

void TestFwdPython::gradient_sphereMeg()
{
    auto defs = FwdCoilSet::read_coil_defs(QCoreApplication::applicationDirPath() + "/../resources/general/coilDefinitions/coil_def.dat");
    QVERIFY(defs != nullptr);
    auto coils = defs->create_meg_coils(m_meg, m_meg.size(), FWD_COIL_ACCURACY_ACCURATE, m_raw.info.dev_head_t);
    QVERIFY(coils != nullptr);

    float r0[3] = {0.0f, 0.0f, 0.04f};
    const int n = coils->ncoil();
    checkGradient(
        [&](const Vector3f& rd, const Vector3f& Q, VectorXf& B) { return FwdBemModel::fwd_sphere_field(rd, Q, *coils, B, r0); },
        [&](const Vector3f& rd, const Vector3f& Q, VectorXf& B, VectorXf& x, VectorXf& y, VectorXf& z) { return FwdBemModel::fwd_sphere_field_grad(rd, Q, *coils, B, x, y, z, r0); },
        n);
}

//=============================================================================================================

void TestFwdPython::gradient_bemMeg_data()
{
    QTest::addColumn<int>("method");
    QTest::newRow("constant collocation") << static_cast<int>(FWD_BEM_CONSTANT_COLL);
    QTest::newRow("linear collocation") << static_cast<int>(FWD_BEM_LINEAR_COLL);
}

//=============================================================================================================

void TestFwdPython::gradient_bemMeg()
{
    QFETCH(int, method);

    auto model = FwdBemModel::fwd_bem_load_homog_surface(data("subjects/sample/bem/sample-5120-bem.fif"));
    QVERIFY(model != nullptr);
    QCOMPARE(model->fwd_bem_load_recompute_solution(data("subjects/sample/bem/sample-5120-bem-sol.fif"), method, 0), 0);
    QCOMPARE(model->bem_method, method);
    model->fwd_bem_set_head_mri_t(FiffCoordTrans::readMriTransform(data("MEG/sample/all-trans.fif")));

    auto defs = FwdCoilSet::read_coil_defs(QCoreApplication::applicationDirPath() + "/../resources/general/coilDefinitions/coil_def.dat");
    auto coils = defs->create_meg_coils(m_meg, m_meg.size(), FWD_COIL_ACCURACY_NORMAL, m_raw.info.dev_head_t);
    QCOMPARE(model->fwd_bem_specify_coils(coils.get()), 0);

    FwdBemModel* m = model.get();
    checkGradient(
        [&](const Vector3f& rd, const Vector3f& Q, VectorXf& B) { return FwdBemModel::fwd_bem_field(rd, Q, *coils, B, m); },
        [&](const Vector3f& rd, const Vector3f& Q, VectorXf& B, VectorXf& x, VectorXf& y, VectorXf& z) { return FwdBemModel::fwd_bem_field_grad(rd, Q, *coils, B, x, y, z, m); },
        coils->ncoil());
}

//=============================================================================================================

void TestFwdPython::gradient_bemEeg_data()
{
    gradient_bemMeg_data();
}

//=============================================================================================================

void TestFwdPython::gradient_bemEeg()
{
    QFETCH(int, method);

    auto model = FwdBemModel::fwd_bem_load_three_layer_surfaces(data("subjects/sample/bem/sample-1280-1280-1280-bem.fif"));
    QVERIFY(model != nullptr);
    QCOMPARE(model->fwd_bem_load_recompute_solution(data("subjects/sample/bem/sample-1280-1280-1280-bem-sol.fif"), method, 0), 0);
    QCOMPARE(model->bem_method, method);
    model->fwd_bem_set_head_mri_t(FiffCoordTrans::readMriTransform(data("MEG/sample/all-trans.fif")));

    auto els = FwdCoilSet::create_eeg_els(m_eeg, m_eeg.size());
    QVERIFY(els != nullptr);
    QCOMPARE(model->fwd_bem_specify_els(els.get()), 0);

    FwdBemModel* m = model.get();
    checkGradient(
        [&](const Vector3f& rd, const Vector3f& Q, VectorXf& V) { return FwdBemModel::fwd_bem_pot_els(rd, Q, *els, V, m); },
        [&](const Vector3f& rd, const Vector3f& Q, VectorXf& V, VectorXf& x, VectorXf& y, VectorXf& z) { return FwdBemModel::fwd_bem_pot_grad_els(rd, Q, *els, V, x, y, z, m); },
        els->ncoil());
}

//=============================================================================================================

void TestFwdPython::gradient_sphereEeg()
{
    std::unique_ptr<FwdEegSphereModelSet> set(FwdEegSphereModelSet::fwd_add_default_eeg_sphere_model(nullptr));
    QVERIFY(set != nullptr);
    std::unique_ptr<FwdEegSphereModel> model(set->fwd_select_eeg_sphere_model("Default"));
    QVERIFY(model != nullptr);
    QVERIFY(model->fwd_setup_eeg_sphere_model(0.09f, true, 3));
    model->r0 = Vector3f(0.0f, 0.0f, 0.04f);

    auto els = FwdCoilSet::create_eeg_els(m_eeg, m_eeg.size());
    QVERIFY(els != nullptr);

    FwdEegSphereModel* m = model.get();
    checkGradient(
        [&](const Vector3f& rd, const Vector3f& Q, VectorXf& V) { return FwdEegSphereModel::fwd_eeg_spherepot_coil(rd, Q, *els, V, m); },
        [&](const Vector3f& rd, const Vector3f& Q, VectorXf& V, VectorXf& x, VectorXf& y, VectorXf& z) { return FwdEegSphereModel::fwd_eeg_spherepot_grad_coil(rd, Q, *els, V, x, y, z, m); },
        els->ncoil());
}

//=============================================================================================================

std::shared_ptr<ComputeFwdSettings> TestFwdPython::settings(bool fixedOri, bool grad)
{
    // mne_forward_solution --meg --eeg --accurate --mindist 5 --label V1-lh.label
    auto s = std::make_shared<ComputeFwdSettings>();
    s->include_meg = true;
    s->include_eeg = true;
    s->accurate = true;
    s->fixed_ori = fixedOri;
    s->compute_grad = grad;
    s->srcname = data("subjects/sample/bem/sample-oct-6-src.fif");
    s->measname = data("MEG/sample/sample_audvis_trunc_raw.fif");
    s->mriname = data("MEG/sample/all-trans.fif");
    s->transname.clear();
    s->bemname = data("subjects/sample/bem/sample-1280-1280-1280-bem.fif");
    s->mindist = 5.0f / 1000.0f;
    s->solname = m_dir.filePath(fixedOri ? "fixed-fwd.fif" : "free-fwd.fif");
    // The second name has no hemisphere tag and is skipped with a warning, as in MNE-C.
    s->labels = {m_labelPath, m_dir.filePath("unassigned.label")};
    s->nlabel = s->labels.size();
    s->pFiffInfo = QSharedPointer<FiffInfo>::create(m_raw.info);
    s->checkIntegrity();
    return s;
}

//=============================================================================================================

void TestFwdPython::labelForward_free()
{
    auto fwd = std::make_shared<ComputeFwd>(settings(false, true))->calculateFwd();
    QVERIFY(fwd != nullptr);

    QCOMPARE(fwd->nsource, 70);
    QCOMPARE(fwd->src[0].nuse, 70);
    QCOMPARE(fwd->src[1].nuse, 0);
    QCOMPARE(fwd->nchan, m_ref.nchan);
    QCOMPARE(fwd->sol->row_names, m_ref.sol->row_names);
    QCOMPARE(static_cast<int>(fwd->sol->data.cols()), 3 * 70);
    QCOMPARE(static_cast<int>(fwd->source_rr.rows()), 70);
    QCOMPARE(static_cast<int>(fwd->source_nn.rows()), 3 * 70);

    // Each source column must equal the matching reference column. The
    // reference was computed by MNE-C with the same model; 1e-4 relative per
    // column is the tolerance test_mne_forward_solution uses for the full
    // solution, and a column from a neighbouring source differs by >10%.
    for (int i = 0; i < 70; ++i) {
        for (int c = 0; c < 3; ++c) {
            const VectorXd ref = m_ref.sol->data.col(3 * m_v1Sel(i) + c);
            const VectorXd got = fwd->sol->data.col(3 * i + c);
            QVERIFY2((got - ref).norm() <= 1e-4 * ref.norm(), qPrintable(QString("source %1 component %2 differs by %3").arg(i).arg(c).arg((got - ref).norm() / ref.norm())));
        }
        QVERIFY((fwd->source_rr.row(i) - m_ref.source_rr.row(m_v1Sel(i))).norm() < 1e-6f);
    }

    // Position derivatives: three per dipole component, nine per source.
    QCOMPARE(static_cast<int>(fwd->sol_grad->data.rows()), fwd->nchan);
    QCOMPARE(static_cast<int>(fwd->sol_grad->data.cols()), 9 * 70);
    for (int k = 0; k < fwd->sol_grad->data.cols(); ++k) {
        QVERIFY2(fwd->sol_grad->data.col(k).norm() > 0.0, qPrintable(QString("gradient column %1 is empty").arg(k)));
    }

    // Threads split the work by source space and dipole component; without threads it is done in one pass
    {
        auto single = settings(false, true);
        single->use_threads = false;
        single->solname = m_dir.filePath("free-single-fwd.fif");
        auto unthreaded = std::make_shared<ComputeFwd>(single)->calculateFwd();
        QVERIFY(unthreaded != nullptr);
        QCOMPARE(unthreaded->sol->data, fwd->sol->data);
        QCOMPARE(unthreaded->sol_grad->data, fwd->sol_grad->data);
    }

    // The file stores MEG and EEG as separate blocks; reading merges them again.
    const QString path = m_dir.filePath("grad-fwd.fif");
    {
        QFile file(path);
        QVERIFY(fwd->write(file));
    }
    const MatrixXd G = fwd->sol->data;
    const MatrixXd dG = fwd->sol_grad->data;
    {
        QFile file(path);
        MNEForwardSolution back;
        QVERIFY(MNEForwardSolution::read(file, back, false, false, {}, {}, false));
        QCOMPARE(back.sol->row_names, fwd->sol->row_names);
        QCOMPARE(back.sol_grad->row_names, fwd->sol_grad->row_names);
        QVERIFY((back.sol->data - G).norm() <= 1e-6 * G.norm());
        QCOMPARE(static_cast<int>(back.sol_grad->data.rows()), static_cast<int>(dG.rows()));
        QVERIFY((back.sol_grad->data - dG).norm() <= 1e-6 * dG.norm());
    }

    // Fixed orientation: G n and, per position derivative d, dG_d n (mne-python's kron(fix_rot, eye(3))).
    {
        QFile file(path);
        MNEForwardSolution fixed;
        QVERIFY(MNEForwardSolution::read(file, fixed, true, false, {}, {}, false));
        QCOMPARE(static_cast<int>(fixed.sol_grad->data.cols()), 3 * 70);
        for (int i = 0; i < 70; ++i) {
            const Vector3d n = fixed.source_nn.row(i).cast<double>().transpose();
            QVERIFY((fixed.sol->data.col(i) - G.middleCols(3 * i, 3) * n).norm() <= 1e-6 * G.middleCols(3 * i, 3).norm());
            for (int d = 0; d < 3; ++d) {
                const VectorXd want = dG.col(9 * i + d) * n(0) + dG.col(9 * i + 3 + d) * n(1) + dG.col(9 * i + 6 + d) * n(2);
                QVERIFY((fixed.sol_grad->data.col(3 * i + d) - want).norm() <= 1e-6 * want.norm());
            }
        }
    }
}

//=============================================================================================================

void TestFwdPython::labelForward_fixed()
{
    auto fwd = std::make_shared<ComputeFwd>(settings(true, false))->calculateFwd();
    QVERIFY(fwd != nullptr);
    QVERIFY(fwd->isFixedOrient());
    QCOMPARE(fwd->nsource, 70);
    QCOMPARE(static_cast<int>(fwd->sol->data.cols()), 70);
    QCOMPARE(static_cast<int>(fwd->source_nn.rows()), 70);
    // Fixed orientation: each normal must be the source-space normal of that vertex.
    for (int i = 0; i < 70; ++i) {
        QVERIFY((fwd->source_nn.row(i) - m_ref.src[0].nn.row(m_ref.src[0].vertno(m_v1Sel(i)))).norm() < 1e-6f);
    }

    // Reading with force_fixed must place right-hemisphere sources after the left ones:
    // mne.convert_forward_solution(ref, surf_ori=True, force_fixed=True), i.e. average patch normals
    {
        QFile refFile(data("Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif"));
        const MNEForwardSolution fixedRef(refFile, true);
        QCOMPARE(static_cast<int>(fixedRef.source_rr.rows()), 7928);
        const Vector3f rr(0.009998258482913594f, -0.05067880820682111f, 0.09408170987849368f);
        const Vector3f nn(0.1567601637522126f, -0.29064558856484646f, 0.9439022157555165f);
        QVERIFY((fixedRef.source_rr.row(3956).transpose() - rr).norm() < 1e-6f);
        QVERIFY((fixedRef.source_nn.row(3956).transpose() - nn).norm() < 1e-6f);
        QVERIFY(std::fabs(fixedRef.source_rr.cwiseAbs().cast<double>().sum() - 1069.459561085619) < 1e-4);
    }

    // mne-python's fixed conversion of the MNE-C reference: G_free * normal.
    const double absSum = fwd->sol->data.cwiseAbs().sum();
    QVERIFY2(std::fabs(absSum - 165133.28497793473) < 1e-4 * 165133.28497793473, qPrintable(QString("fixed gain abs sum %1, mne-python 165133.28497793473").arg(absSum, 0, 'g', 17)));

    for (int i = 0; i < 70; ++i) {
        const Vector3d nn = m_ref.src[0].nn.row(m_ref.src[0].vertno(m_v1Sel(i))).cast<double>().transpose();
        const VectorXd ref = m_ref.sol->data.middleCols(3 * m_v1Sel(i), 3) * nn;
        QVERIFY((fwd->sol->data.col(i) - ref).norm() <= 1e-4 * ref.norm());
    }
}

//=============================================================================================================

void TestFwdPython::labelForward_variants_data()
{
    QTest::addColumn<bool>("mriFrame");
    QTest::addColumn<bool>("asciiTrans");
    QTest::addColumn<bool>("meg");
    QTest::addColumn<bool>("eeg");
    QTest::addColumn<bool>("grad");

    QTest::newRow("MRI frame") << true << false << true << true << false;
    QTest::newRow("ASCII trans, MEG only, measurement file") << false << true << true << false << true;
    QTest::newRow("EEG only") << false << false << false << true << true;
}

void TestFwdPython::labelForward_variants()
{
    QFETCH(bool, mriFrame);
    QFETCH(bool, asciiTrans);
    QFETCH(bool, meg);
    QFETCH(bool, eeg);
    QFETCH(bool, grad);

    auto s = settings(false, grad);
    s->include_meg = meg;
    s->include_eeg = eeg;
    if (mriFrame) {
        s->coord_frame = FIFFV_COORD_MRI;
    }
    if (asciiTrans) {
        // FreeSurfer-style head -> MRI text transform, translation in mm.
        const FiffCoordTrans headMri = FiffCoordTrans::readMriTransform(data("MEG/sample/all-trans.fif")).inverted();
        QFile file(m_dir.filePath("head-mri.txt"));
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out.setRealNumberPrecision(10);
        for (int r = 0; r < 4; ++r) {
            out << headMri.trans(r, 0) << ' ' << headMri.trans(r, 1) << ' ' << headMri.trans(r, 2) << ' '
                << (r < 3 ? 1000.0f * headMri.trans(r, 3) : 1.0f) << '\n';
        }
        file.close();
        s->mriname.clear();
        s->transname = file.fileName();
        s->pFiffInfo.reset();
        s->mindistoutname = m_dir.filePath("omitted.txt");
    }

    ComputeFwd computer(s);
    auto fwd = computer.calculateFwd();
    QVERIFY(fwd != nullptr);
    QCOMPARE(fwd->nsource, 70);
    QCOMPARE(fwd->coord_frame, mriFrame ? FIFFV_COORD_MRI : FIFFV_COORD_HEAD);
    if (asciiTrans) {
        QVERIFY(QFile::exists(s->mindistoutname));
    }

    // The reference is in head coordinates: q_head = R q_mri, so G_mri = G_head R.
    const int first = meg ? 0 : 306;
    const int nChan = (meg ? 306 : 0) + (eeg ? 60 : 0);
    const Matrix3d R = mriFrame ? Matrix3d(m_ref.mri_head_t.trans.topLeftCorner<3, 3>().cast<double>()) : Matrix3d::Identity();
    QCOMPARE(static_cast<int>(fwd->sol->data.rows()), nChan);
    QCOMPARE(fwd->sol->row_names, m_ref.sol->row_names.mid(first, nChan));
    auto check = [&]() {
        for (int i = 0; i < 70; ++i) {
            const MatrixXd ref = m_ref.sol->data.block(first, 3 * m_v1Sel(i), nChan, 3) * R;
            const MatrixXd got = fwd->sol->data.middleCols(3 * i, 3);
            QVERIFY2((got - ref).norm() <= 1e-4 * ref.norm(), qPrintable(QString("source %1 differs by %2").arg(i).arg((got - ref).norm() / ref.norm())));
        }
    };
    check();
    if (grad) {
        QCOMPARE(static_cast<int>(fwd->sol_grad->data.rows()), nChan);
        QCOMPARE(static_cast<int>(fwd->sol_grad->data.cols()), 9 * 70);
    }

    // Recomputing at the same head position must leave the MEG rows unchanged.
    if (meg) {
        QVERIFY(computer.updateHeadPos(m_raw.info.dev_head_t, *fwd));
        check();
    }
}

//=============================================================================================================

void TestFwdPython::sphereForward_data()
{
    // mne-python _compute_forwards_meeg on the 78 lh.V1 sources in head coordinates with
    // make_sphere_model(r0=(0, 0, 0.04), head_radius=0.09), accurate MEG coils and EEG:
    // np.linalg.norm(G[:, 3k + c]) per channel type.
    QTest::addColumn<int>("source");
    QTest::addColumn<Vector3d>("megNorms");
    QTest::addColumn<Vector3d>("eegNorms");

    QTest::newRow("source 0") << int{0} << Vector3d(0.0012455023232288232, 0.0008553711634315896, 0.0010237827397100762)
                              << Vector3d(551.6501123312413, 522.8970930124603, 458.6240696616932);
    QTest::newRow("source 40") << 40 << Vector3d(0.0007011757119319035, 0.0005468106389226044, 0.0006286912013834711)
                               << Vector3d(502.557459453998, 461.3687151657523, 438.55365038029845);
    QTest::newRow("source 77") << 77 << Vector3d(0.00020488718164145291, 0.00023728405477539279, 0.00020604610935806558)
                               << Vector3d(411.14962687679156, 379.70623960737703, 405.4180262013036);
}

void TestFwdPython::sphereForward()
{
    QFETCH(int, source);
    QFETCH(Vector3d, megNorms);
    QFETCH(Vector3d, eegNorms);

    // No BEM: MEG uses the sphere at r0, EEG the default four-layer sphere of radius 90 mm.
    auto s = settings(false, false);
    s->bemname.clear();
    s->mindist = 0.0f;
    s->filter_spaces = false;
    s->r0 = Vector3f(0.0f, 0.0f, 0.04f);
    s->eeg_sphere_rad = 0.09f;
    s->solname = m_dir.filePath("sphere-fwd.fif");
    auto fwd = std::make_shared<ComputeFwd>(s)->calculateFwd();
    QVERIFY(fwd != nullptr);
    QCOMPARE(fwd->nsource, 78);

    Vector3d meg;
    Vector3d eeg;
    for (int c = 0; c < 3; ++c) {
        const VectorXd col = fwd->sol->data.col(3 * source + c);
        meg[c] = col.head(306).norm();
        eeg[c] = col.tail(60).norm();
    }
    QVERIFY2((meg - megNorms).cwiseAbs().maxCoeff() < 1e-4 * megNorms.maxCoeff(),
             qPrintable(QStringLiteral("MEG norms %1 %2 %3").arg(meg[0], 0, 'g', 10).arg(meg[1], 0, 'g', 10).arg(meg[2], 0, 'g', 10)));
    QVERIFY2((eeg - eegNorms).cwiseAbs().maxCoeff() < 1e-3 * eegNorms.maxCoeff(),
             qPrintable(QStringLiteral("EEG norms %1 %2 %3").arg(eeg[0], 0, 'g', 10).arg(eeg[1], 0, 'g', 10).arg(eeg[2], 0, 'g', 10)));
}

//=============================================================================================================

void TestFwdPython::sphereForward_modelFile()
{
    // MNE-C format: name:rad:sigma:... with unsorted layers; a model with an unparsable number is dropped whole.
    const QString path = m_dir.filePath("eeg_models.dat");
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("# name : rad : sigma ...\n"
                   "\n"
                   "Three:1.0:0.33:0.87:0.33:0.92:0.0042\n"
                   "Broken:0.9:0.33:x:1.0\n"
                   "NameOnly\n");
    }

    std::unique_ptr<FwdEegSphereModelSet> set(FwdEegSphereModelSet::fwd_load_eeg_sphere_models(path, nullptr));
    QVERIFY(set != nullptr);
    QCOMPARE(set->nmodel(), 2);
    set->fwd_list_eeg_sphere_models();
    std::unique_ptr<FwdEegSphereModel> three(set->fwd_select_eeg_sphere_model("Three"));
    QVERIFY(three != nullptr);
    QCOMPARE(three->nlayer(), 3);
    QCOMPARE(three->layers[0].rel_rad, 0.87f);
    QCOMPARE(three->layers[1].sigma, 0.0042f);
    QVERIFY(set->fwd_select_eeg_sphere_model("Broken") == nullptr);
    QVERIFY(set->fwd_select_eeg_sphere_model("Missing") == nullptr);
    std::unique_ptr<FwdEegSphereModel> fallback(set->fwd_select_eeg_sphere_model(QString()));
    QVERIFY(fallback != nullptr);
    QCOMPARE(fallback->name, QString("Default"));

    std::unique_ptr<FwdEegSphereModelSet> onlyDefault(FwdEegSphereModelSet::fwd_load_eeg_sphere_models(m_dir.filePath("none.dat"), nullptr));
    QCOMPARE(onlyDefault->nmodel(), 1);
    QVERIFY(FwdEegSphereModelSet().fwd_select_eeg_sphere_model("Default") == nullptr);

    // mne-python make_sphere_model(r0=(0, 0, 0.04), head_radius=0.09, relative_radii=(0.87, 0.92, 1),
    // sigmas=(0.33, 0.0042, 0.33)) on the lh.V1 sources: np.linalg.norm(G[:, 3k + c]) over EEG.
    auto s = settings(false, false);
    s->include_meg = false;
    s->bemname.clear();
    s->mindist = 0.0f;
    s->filter_spaces = false;
    s->r0 = Vector3f(0.0f, 0.0f, 0.04f);
    s->eeg_sphere_rad = 0.09f;
    s->eeg_model_file = path;
    s->eeg_model_name = "Three";
    auto fwd = std::make_shared<ComputeFwd>(s)->calculateFwd();
    QVERIFY(fwd != nullptr);
    QCOMPARE(static_cast<int>(fwd->sol->data.rows()), 60);

    const QList<QPair<int, Vector3d>> refs{
        {28, Vector3d(370.27074684018515, 345.1938826601203, 349.37989001470487)},
        {61, Vector3d(352.97698173839217, 327.7262594697324, 338.9604653611034)},
        {77, Vector3d(325.90676712903104, 304.57117355185295, 330.43617483953057)},
    };
    for (const auto& [source, norms] : refs) {
        Vector3d got;
        for (int c = 0; c < 3; ++c)
            got[c] = fwd->sol->data.col(3 * source + c).norm();
        QVERIFY2((got - norms).cwiseAbs().maxCoeff() < 1e-3 * norms.maxCoeff(),
                 qPrintable(QStringLiteral("source %1: %2 %3 %4").arg(source).arg(got[0], 0, 'g', 10).arg(got[1], 0, 'g', 10).arg(got[2], 0, 'g', 10)));
    }
}

//=============================================================================================================

void TestFwdPython::ctfCompensatedForward_data()
{
    QTest::addColumn<int>("grade");
    QTest::newRow("third-order gradiometer") << 3;
    QTest::newRow("uncompensated") << int{0};
}

void TestFwdPython::ctfCompensatedForward()
{
    QFETCH(int, grade);

    // make_ctf_fwd_fixture.py: mne-python make_forward_solution, sphere at (0, 0, 40) mm, 7 MEG channels.
    MatrixXd ref(7, 6);
    {
        QFile file(QStringLiteral(MNE_FWD_DATA_DIR "/ctf_grade%1_gain.txt").arg(grade));
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QTextStream in(&file);
        for (int r = 0; r < 7; ++r)
            for (int c = 0; c < 6; ++c)
                in >> ref(r, c);
    }

    auto s = std::make_shared<ComputeFwdSettings>();
    s->include_meg = true;
    s->include_eeg = false;
    s->compute_grad = true;
    s->srcname = QStringLiteral(MNE_FWD_DATA_DIR "/two-dipole-src.fif");
    s->measname = QStringLiteral(MNE_CTF_COMP_DATA_DIR "/ctf_grade%1_raw.fif").arg(grade);
    s->mriname.clear();
    s->transname.clear();
    s->mri_head_ident = true;
    s->bemname.clear();
    s->r0 = Vector3f(0.0f, 0.0f, 0.04f);
    s->mindist = 0.0f;
    s->filter_spaces = false;
    s->do_all = true;
    s->solname = m_dir.filePath("ctf-fwd.fif");
    s->checkIntegrity();
    auto fwd = std::make_shared<ComputeFwd>(s)->calculateFwd();
    QVERIFY(fwd != nullptr);
    QCOMPARE(static_cast<int>(fwd->sol->data.rows()), 7);
    QCOMPARE(static_cast<int>(fwd->sol->data.cols()), 6);
    const double err = (fwd->sol->data - ref).norm() / ref.norm();
    QVERIFY2(err < 1e-4, qPrintable(QStringLiteral("gain differs from mne-python by %1").arg(err)));

    // Compensated position derivatives (fwd_comp_field_grad): column 9 s + 3 c + d is dG_c / dr_d of
    // source s, against mne-python's central differences with h = 0.1 mm in the fixture.
    MatrixXd refGrad(7, 18);
    {
        QFile file(QStringLiteral(MNE_FWD_DATA_DIR "/ctf_grade%1_grad.txt").arg(grade));
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QTextStream in(&file);
        for (int r = 0; r < 7; ++r)
            for (int c = 0; c < 18; ++c)
                in >> refGrad(r, c);
    }
    const MatrixXd& dG = fwd->sol_grad->data;
    QCOMPARE(static_cast<int>(dG.cols()), 18);
    const double gradErr = (dG - refGrad).norm() / refGrad.norm();
    QVERIFY2(gradErr < 1e-3, qPrintable(QStringLiteral("gradient differs by %1").arg(gradErr)));

    // Without gradients the sphere model takes the vector-field path (fwd_comp_field_vec).
    s->compute_grad = false;
    auto vecFwd = std::make_shared<ComputeFwd>(s)->calculateFwd();
    QVERIFY(vecFwd != nullptr);
    const double vecErr = (vecFwd->sol->data - ref).norm() / ref.norm();
    QVERIFY2(vecErr < 1e-4, qPrintable(QStringLiteral("vector-field gain differs by %1").arg(vecErr)));

    // The BEM takes the single-orientation path (fwd_comp_field); mne-python on the one-layer BEM of the fixture.
    MatrixXd bemRef(7, 6);
    {
        QFile file(QStringLiteral(MNE_FWD_DATA_DIR "/ctf_grade%1_bem_gain.txt").arg(grade));
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        QTextStream in(&file);
        for (int r = 0; r < 7; ++r)
            for (int c = 0; c < 6; ++c)
                in >> bemRef(r, c);
    }
    // Like MNE-C, the surfaces are read from the -sol name; a file without a solution has it recomputed.
    QTemporaryDir bemDir;
    s->bemname = bemDir.filePath("one-layer-bem.fif");
    QTest::ignoreMessage(QtCriticalMsg, QRegularExpression("Cannot open .*one-layer-bem-sol.fif"));
    QTest::ignoreMessage(QtCriticalMsg, QRegularExpression("Cannot open .*one-layer-bem-sol.fif"));
    QTest::ignoreMessage(QtCriticalMsg, "ComputeFwd::calculateFwd - the forward computation could not be set up.");
    QVERIFY(std::make_shared<ComputeFwd>(s)->calculateFwd() == nullptr);
    QVERIFY(QFile::copy(QStringLiteral(MNE_FWD_DATA_DIR "/one-layer-bem.fif"), bemDir.filePath("one-layer-bem-sol.fif")));
    auto bemFwd = std::make_shared<ComputeFwd>(s)->calculateFwd();
    QVERIFY(bemFwd != nullptr);
    const double bemErr = (bemFwd->sol->data - bemRef).norm() / bemRef.norm();
    QVERIFY2(bemErr < 1e-3, qPrintable(QStringLiteral("BEM gain differs from mne-python by %1").arg(bemErr)));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFwdPython)
#include "test_fwd_python.moc"
