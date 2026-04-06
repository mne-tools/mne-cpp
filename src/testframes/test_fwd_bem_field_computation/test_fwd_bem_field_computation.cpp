//=============================================================================================================
/**
 * @file     test_fwd_bem_field_computation.cpp
 * @author   MNE-CPP Tests
 * @brief    Tests for BEM field/potential computation, sphere model fields,
 *           guess-point generation, and forward computation pipeline.
 */
//=============================================================================================================

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fwd/fwd_bem_model.h>
#include <fwd/fwd_comp_data.h>
#include <fwd/fwd_coil_set.h>
#include <fwd/fwd_coil.h>
#include <fwd/fwd_thread_arg.h>
#include <fwd/fwd_eeg_sphere_model.h>
#include <fwd/compute_fwd/compute_fwd.h>
#include <fwd/compute_fwd/compute_fwd_settings.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_coord_trans.h>
#include <mne/mne_source_space.h>
#include <mne/mne_surface.h>
#include <mne/mne_forward_solution.h>

#include <QCoreApplication>
#include <QFile>
#include <QTest>
#include <QDebug>
#include <QDir>

#include <Eigen/Core>

#include <memory>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FWDLIB;
using namespace FIFFLIB;
using namespace MNELIB;

//=============================================================================================================
// TEST CLASS
//=============================================================================================================

class TestFwdBemFieldComputation : public QObject
{
    Q_OBJECT

public:
    TestFwdBemFieldComputation() {}

private:
    QString testDataPath() const {
        return QCoreApplication::applicationDirPath()
               + "/../resources/data/mne-cpp-test-data";
    }
    QString coilDefPath() const {
        return QCoreApplication::applicationDirPath()
               + "/../resources/general/coilDefinitions/coil_def.dat";
    }
    QString bemPath() const {
        return testDataPath() + "/subjects/sample/bem/sample-5120-bem.fif";
    }
    QString bemSolPath() const {
        return testDataPath() + "/subjects/sample/bem/sample-5120-bem-sol.fif";
    }
    QString bem3Path() const {
        return testDataPath() + "/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
    }
    QString bem3SolPath() const {
        return testDataPath() + "/subjects/sample/bem/sample-1280-1280-1280-bem-sol.fif";
    }
    QString rawPath() const {
        return testDataPath() + "/MEG/sample/sample_audvis_trunc_raw.fif";
    }
    QString srcPath() const {
        return testDataPath() + "/subjects/sample/bem/sample-oct-6-src.fif";
    }
    QString transPath() const {
        return testDataPath() + "/MEG/sample/all-trans.fif";
    }

private slots:

    void initTestCase()
    {
        QVERIFY2(QFile::exists(rawPath()),
                 qPrintable(QString("Test data not found: %1").arg(rawPath())));
        QVERIFY2(QFile::exists(bemPath()),
                 qPrintable(QString("BEM data not found: %1").arg(bemPath())));
        QVERIFY2(QFile::exists(srcPath()),
                 qPrintable(QString("Source space not found: %1").arg(srcPath())));
        QVERIFY2(QFile::exists(coilDefPath()),
                 qPrintable(QString("Coil definitions not found: %1").arg(coilDefPath())));
    }

    // ---- FwdBemModel: BEM Solution Computation ----

    void testBemComputeSolution_linear()
    {
        // Test computing a linear collocation BEM solution from loaded surfaces

        auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
        QVERIFY(model != nullptr);
        QVERIFY(model->nsurf == 1);

        // Compute linear collocation solution
        int result = model->fwd_bem_compute_solution(FWD_BEM_LINEAR_COLL);
        QVERIFY2(result == 0, "fwd_bem_compute_solution(LINEAR_COLL) should succeed");
        QVERIFY(model->nsol > 0);
        QVERIFY(model->solution.rows() > 0);
        QVERIFY(model->solution.cols() > 0);
    }

    void testBemComputeSolution_constant()
    {
        // Test computing a constant collocation BEM solution

        auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
        QVERIFY(model != nullptr);

        int result = model->fwd_bem_compute_solution(FWD_BEM_CONSTANT_COLL);
        QVERIFY2(result == 0, "fwd_bem_compute_solution(CONSTANT_COLL) should succeed");
        QVERIFY(model->nsol > 0);
    }

    // ---- FwdBemModel: Coil/Electrode Specification ----

    void testBemSpecifyCoils()
    {
        // Test specifying MEG coils for BEM forward computation

        auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
        QVERIFY(model != nullptr);
        model->fwd_bem_load_recompute_solution(bemSolPath(), FWD_BEM_LINEAR_COLL, 0);

        // Load coil definitions and create MEG coils from raw data
        auto coilDefs = FwdCoilSet::read_coil_defs(coilDefPath());
        QVERIFY(coilDefs != nullptr);

        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);

        // Collect MEG channels
        QList<FiffChInfo> megChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_MEG_CH) {
                megChs.append(raw.info.chs[i]);
            }
        }

        // Create coils using dev->head transform so coord_frame is set
        auto coils = coilDefs->create_meg_coils(megChs, megChs.size(),
                                                 FWD_COIL_ACCURACY_NORMAL,
                                                 raw.info.dev_head_t);
        QVERIFY(coils != nullptr);
        QVERIFY(coils->ncoil() > 0);

        // Load head->MRI transform and pass it to the model
        FiffCoordTrans head_mri_t = FiffCoordTrans::readMriTransform(transPath());
        QVERIFY(!head_mri_t.isEmpty());
        model->fwd_bem_set_head_mri_t(head_mri_t);

        int result = model->fwd_bem_specify_coils(coils.get());
        QVERIFY2(result == 0, "fwd_bem_specify_coils should succeed");
    }

    void testBemSpecifyEls()
    {
        // Test specifying EEG electrodes for BEM forward computation

        auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
        QVERIFY(model != nullptr);
        model->fwd_bem_load_recompute_solution(bemSolPath(), FWD_BEM_LINEAR_COLL, 0);

        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);

        // Collect only EEG channels
        QList<FiffChInfo> eegChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_EEG_CH) {
                eegChs.append(raw.info.chs[i]);
            }
        }
        QVERIFY(eegChs.size() > 0);

        auto eegEls = FwdCoilSet::create_eeg_els(eegChs, eegChs.size());
        QVERIFY(eegEls != nullptr);
        QVERIFY(eegEls->ncoil() > 0);

        // Load head->MRI transform
        FiffCoordTrans head_mri_t = FiffCoordTrans::readMriTransform(transPath());
        QVERIFY(!head_mri_t.isEmpty());
        model->fwd_bem_set_head_mri_t(head_mri_t);

        int result = model->fwd_bem_specify_els(eegEls.get());
        QVERIFY2(result == 0, "fwd_bem_specify_els should succeed");
    }

    // ---- FwdBemModel: Field/Potential Calculation ----

    void testBemFieldCalc()
    {
        // Test constant-collocation BEM field calculation for a single dipole

        auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
        QVERIFY(model != nullptr);
        model->fwd_bem_load_recompute_solution(bemSolPath(), FWD_BEM_CONSTANT_COLL, 0);

        auto coilDefs = FwdCoilSet::read_coil_defs(coilDefPath());
        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);
        QList<FiffChInfo> megChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_MEG_CH)
                megChs.append(raw.info.chs[i]);
        }
        auto coils = coilDefs->create_meg_coils(megChs, megChs.size(), FWD_COIL_ACCURACY_NORMAL, raw.info.dev_head_t);
        FiffCoordTrans head_mri_t = FiffCoordTrans::readMriTransform(transPath());
        model->fwd_bem_set_head_mri_t(head_mri_t);
        model->fwd_bem_specify_coils(coils.get());

        // Place a dipole inside the BEM surface
        Vector3f rd(0.0f, 0.0f, 0.06f);  // ~6cm deep
        Vector3f Q(0.0f, 0.0f, 1e-8f);   // z-directed, 10 nAm

        VectorXf B = VectorXf::Zero(coils->ncoil());
        model->fwd_bem_field_calc(rd, Q, *coils, B);

        // Verify non-zero field at MEG sensors
        QVERIFY2(B.norm() > 0, "BEM field should be non-zero for dipole inside head");
    }

    void testBemLinFieldCalc()
    {
        // Test linear-collocation BEM field calculation

        auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
        QVERIFY(model != nullptr);
        model->fwd_bem_load_recompute_solution(bemSolPath(), FWD_BEM_LINEAR_COLL, 0);

        auto coilDefs = FwdCoilSet::read_coil_defs(coilDefPath());
        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);
        QList<FiffChInfo> megChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_MEG_CH)
                megChs.append(raw.info.chs[i]);
        }
        auto coils = coilDefs->create_meg_coils(megChs, megChs.size(), FWD_COIL_ACCURACY_NORMAL, raw.info.dev_head_t);
        FiffCoordTrans head_mri_t = FiffCoordTrans::readMriTransform(transPath());
        model->fwd_bem_set_head_mri_t(head_mri_t);
        model->fwd_bem_specify_coils(coils.get());

        Vector3f rd(0.0f, 0.0f, 0.06f);
        Vector3f Q(0.0f, 0.0f, 1e-8f);

        VectorXf B = VectorXf::Zero(coils->ncoil());
        model->fwd_bem_lin_field_calc(rd, Q, *coils, B);

        QVERIFY2(B.norm() > 0, "Linear BEM field should be non-zero");
    }

    void testBemPotCalc()
    {
        // Test constant-collocation BEM potential calculation for EEG electrodes

        auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
        QVERIFY(model != nullptr);
        model->fwd_bem_load_recompute_solution(bemSolPath(), FWD_BEM_CONSTANT_COLL, 0);

        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);
        QList<FiffChInfo> eegChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_EEG_CH)
                eegChs.append(raw.info.chs[i]);
        }
        auto eegEls = FwdCoilSet::create_eeg_els(eegChs, eegChs.size());
        FiffCoordTrans head_mri_t = FiffCoordTrans::readMriTransform(transPath());
        model->fwd_bem_set_head_mri_t(head_mri_t);
        model->fwd_bem_specify_els(eegEls.get());

        Vector3f rd(0.0f, 0.0f, 0.06f);
        Vector3f Q(0.0f, 0.0f, 1e-8f);

        VectorXf pot = VectorXf::Zero(eegEls->ncoil());
        model->fwd_bem_pot_calc(rd, Q, eegEls.get(), 0, pot);

        QVERIFY2(pot.norm() > 0, "BEM potential should be non-zero for dipole");
    }

    void testBemLinPotCalc()
    {
        // Test linear-collocation BEM potential calculation

        auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
        QVERIFY(model != nullptr);
        model->fwd_bem_load_recompute_solution(bemSolPath(), FWD_BEM_LINEAR_COLL, 0);

        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);
        QList<FiffChInfo> eegChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_EEG_CH)
                eegChs.append(raw.info.chs[i]);
        }
        auto eegEls = FwdCoilSet::create_eeg_els(eegChs, eegChs.size());
        FiffCoordTrans head_mri_t = FiffCoordTrans::readMriTransform(transPath());
        model->fwd_bem_set_head_mri_t(head_mri_t);
        model->fwd_bem_specify_els(eegEls.get());

        Vector3f rd(0.0f, 0.0f, 0.06f);
        Vector3f Q(0.0f, 0.0f, 1e-8f);

        VectorXf pot = VectorXf::Zero(eegEls->ncoil());
        model->fwd_bem_lin_pot_calc(rd, Q, eegEls.get(), 0, pot);

        QVERIFY2(pot.norm() > 0, "Linear BEM potential should be non-zero");
    }

    // ---- FwdBemModel: Gradient Calculations ----

    void testBemFieldGradCalc()
    {
        // Test gradient of BEM MEG field with respect to dipole position

        auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
        model->fwd_bem_load_recompute_solution(bemSolPath(), FWD_BEM_CONSTANT_COLL, 0);

        auto coilDefs = FwdCoilSet::read_coil_defs(coilDefPath());
        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);
        QList<FiffChInfo> megChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_MEG_CH)
                megChs.append(raw.info.chs[i]);
        }
        auto coils = coilDefs->create_meg_coils(megChs, megChs.size(), FWD_COIL_ACCURACY_NORMAL, raw.info.dev_head_t);
        FiffCoordTrans head_mri_t = FiffCoordTrans::readMriTransform(transPath());
        model->fwd_bem_set_head_mri_t(head_mri_t);
        model->fwd_bem_specify_coils(coils.get());

        Vector3f rd(0.0f, 0.0f, 0.06f);
        Vector3f Q(0.0f, 0.0f, 1e-8f);
        int nc = coils->ncoil();

        VectorXf xgrad = VectorXf::Zero(nc);
        VectorXf ygrad = VectorXf::Zero(nc);
        VectorXf zgrad = VectorXf::Zero(nc);

        model->fwd_bem_field_grad_calc(rd, Q, *coils, xgrad, ygrad, zgrad);

        // Gradients should be non-zero for a dipole inside the model
        float gradNorm = xgrad.norm() + ygrad.norm() + zgrad.norm();
        QVERIFY2(gradNorm > 0, "BEM field gradients should be non-zero");
    }

    void testBemPotGradCalc()
    {
        // Test gradient of BEM EEG potential

        auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
        model->fwd_bem_load_recompute_solution(bemSolPath(), FWD_BEM_CONSTANT_COLL, 0);

        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);
        QList<FiffChInfo> eegChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_EEG_CH)
                eegChs.append(raw.info.chs[i]);
        }
        auto eegEls = FwdCoilSet::create_eeg_els(eegChs, eegChs.size());
        FiffCoordTrans head_mri_t = FiffCoordTrans::readMriTransform(transPath());
        model->fwd_bem_set_head_mri_t(head_mri_t);
        model->fwd_bem_specify_els(eegEls.get());

        Vector3f rd(0.0f, 0.0f, 0.06f);
        Vector3f Q(0.0f, 0.0f, 1e-8f);
        int nc = eegEls->ncoil();

        VectorXf xgrad = VectorXf::Zero(nc);
        VectorXf ygrad = VectorXf::Zero(nc);
        VectorXf zgrad = VectorXf::Zero(nc);

        model->fwd_bem_pot_grad_calc(rd, Q, eegEls.get(), 0, xgrad, ygrad, zgrad);

        float gradNorm = xgrad.norm() + ygrad.norm() + zgrad.norm();
        QVERIFY2(gradNorm > 0, "BEM potential gradients should be non-zero");
    }

    // ---- FwdBemModel: Static Callbacks ----

    void testBemFieldCallback()
    {
        // Test the static callback interface used during forward computation

        auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
        model->fwd_bem_load_recompute_solution(bemSolPath(), FWD_BEM_LINEAR_COLL, 0);

        auto coilDefs = FwdCoilSet::read_coil_defs(coilDefPath());
        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);
        QList<FiffChInfo> megChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_MEG_CH)
                megChs.append(raw.info.chs[i]);
        }
        auto coils = coilDefs->create_meg_coils(megChs, megChs.size(), FWD_COIL_ACCURACY_NORMAL, raw.info.dev_head_t);
        FiffCoordTrans head_mri_t = FiffCoordTrans::readMriTransform(transPath());
        model->fwd_bem_set_head_mri_t(head_mri_t);
        model->fwd_bem_specify_coils(coils.get());

        Vector3f rd(0.0f, 0.0f, 0.06f);
        Vector3f Q(0.0f, 0.0f, 1e-8f);
        VectorXf B = VectorXf::Zero(coils->ncoil());

        int ret = FwdBemModel::fwd_bem_field(rd, Q, *coils, B, (void*)model.get());
        QVERIFY(ret == 0);
        QVERIFY(B.norm() > 0);
    }

    void testBemPotElsCallback()
    {
        // Test the static EEG potential callback

        auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
        model->fwd_bem_load_recompute_solution(bemSolPath(), FWD_BEM_LINEAR_COLL, 0);

        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);
        QList<FiffChInfo> eegChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_EEG_CH)
                eegChs.append(raw.info.chs[i]);
        }
        auto eegEls = FwdCoilSet::create_eeg_els(eegChs, eegChs.size());
        FiffCoordTrans head_mri_t = FiffCoordTrans::readMriTransform(transPath());
        model->fwd_bem_set_head_mri_t(head_mri_t);
        model->fwd_bem_specify_els(eegEls.get());

        Vector3f rd(0.0f, 0.0f, 0.06f);
        Vector3f Q(0.0f, 0.0f, 1e-8f);
        VectorXf pot = VectorXf::Zero(eegEls->ncoil());

        int ret = FwdBemModel::fwd_bem_pot_els(rd, Q, *eegEls, pot, (void*)model.get());
        QVERIFY(ret == 0);
        QVERIFY(pot.norm() > 0);
    }

    // ---- FwdBemModel: Sphere Field Functions ----

    void testSphereField()
    {
        // Test spherical head model MEG field computation

        auto coilDefs = FwdCoilSet::read_coil_defs(coilDefPath());
        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);
        QList<FiffChInfo> megChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_MEG_CH)
                megChs.append(raw.info.chs[i]);
        }
        auto coils = coilDefs->create_meg_coils(megChs, megChs.size(), FWD_COIL_ACCURACY_NORMAL);

        Vector3f rd(0.0f, 0.02f, 0.06f);
        Vector3f Q(1e-8f, 0.0f, 0.0f);
        VectorXf Bval = VectorXf::Zero(coils->ncoil());

        // Sphere model origin
        float r0[3] = {0.0f, 0.0f, 0.04f};
        int ret = FwdBemModel::fwd_sphere_field(rd, Q, *coils, Bval, r0);
        QVERIFY(ret == 0);
        QVERIFY2(Bval.norm() > 0, "Sphere model field should be non-zero");
    }

    void testSphereFieldVec()
    {
        // Test vectorized sphere field computation (3 dipole orientations)

        auto coilDefs = FwdCoilSet::read_coil_defs(coilDefPath());
        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);
        QList<FiffChInfo> megChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_MEG_CH)
                megChs.append(raw.info.chs[i]);
        }
        auto coils = coilDefs->create_meg_coils(megChs, megChs.size(), FWD_COIL_ACCURACY_NORMAL);

        Vector3f rd(0.0f, 0.02f, 0.06f);
        MatrixXf Bval = MatrixXf::Zero(3, coils->ncoil());

        // Sphere model origin
        float r0[3] = {0.0f, 0.0f, 0.04f};
        int ret = FwdBemModel::fwd_sphere_field_vec(rd, *coils, Bval, r0);
        QVERIFY(ret == 0);
        QVERIFY2(Bval.norm() > 0, "Vectorized sphere field should be non-zero");
    }

    void testSphereFieldGrad()
    {
        // Test sphere field with spatial gradients

        auto coilDefs = FwdCoilSet::read_coil_defs(coilDefPath());
        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);
        QList<FiffChInfo> megChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_MEG_CH)
                megChs.append(raw.info.chs[i]);
        }
        auto coils = coilDefs->create_meg_coils(megChs, megChs.size(), FWD_COIL_ACCURACY_NORMAL);

        Vector3f rd(0.0f, 0.02f, 0.06f);
        Vector3f Q(1e-8f, 0.0f, 0.0f);
        int nc = coils->ncoil();
        VectorXf Bval = VectorXf::Zero(nc);
        VectorXf xgrad = VectorXf::Zero(nc);
        VectorXf ygrad = VectorXf::Zero(nc);
        VectorXf zgrad = VectorXf::Zero(nc);

        // Sphere model origin
        float r0[3] = {0.0f, 0.0f, 0.04f};
        int ret = FwdBemModel::fwd_sphere_field_grad(rd, Q, *coils, Bval,
                                                      xgrad, ygrad, zgrad, r0);
        QVERIFY(ret == 0);
        QVERIFY(Bval.norm() > 0);
        float gradNorm = xgrad.norm() + ygrad.norm() + zgrad.norm();
        QVERIFY2(gradNorm > 0, "Sphere field gradients should be non-zero");
    }

    // ---- FwdBemModel: Magnetic Dipole Field ----

    void testMagDipoleField()
    {
        // Test magnetic dipole field computation (used for MEG sensor modeling)

        auto coilDefs = FwdCoilSet::read_coil_defs(coilDefPath());
        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);
        QList<FiffChInfo> megChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_MEG_CH)
                megChs.append(raw.info.chs[i]);
        }
        auto coils = coilDefs->create_meg_coils(megChs, megChs.size(), FWD_COIL_ACCURACY_NORMAL);

        // Magnetic dipole at origin with z-directed moment
        Vector3f rm(0.0f, 0.0f, 0.0f);
        Vector3f M(0.0f, 0.0f, 1.0f);
        VectorXf Bval = VectorXf::Zero(coils->ncoil());

        int ret = FwdBemModel::fwd_mag_dipole_field(rm, M, *coils, Bval, nullptr);
        QVERIFY(ret == 0);
        QVERIFY2(Bval.norm() > 0, "Magnetic dipole field should be non-zero");
    }

    void testMagDipoleFieldVec()
    {
        // Test vectorized magnetic dipole field (3 orientations simultaneously)

        auto coilDefs = FwdCoilSet::read_coil_defs(coilDefPath());
        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);
        QList<FiffChInfo> megChs;
        for (int i = 0; i < raw.info.nchan; ++i) {
            if (raw.info.chs[i].kind == FIFFV_MEG_CH)
                megChs.append(raw.info.chs[i]);
        }
        auto coils = coilDefs->create_meg_coils(megChs, megChs.size(), FWD_COIL_ACCURACY_NORMAL);

        Vector3f rm(0.0f, 0.0f, 0.0f);
        MatrixXf Bval = MatrixXf::Zero(3, coils->ncoil());

        int ret = FwdBemModel::fwd_mag_dipole_field_vec(rm, *coils, Bval, nullptr);
        QVERIFY(ret == 0);
        QVERIFY(Bval.norm() > 0);
    }

    // ---- FwdBemModel: make_guesses ----

    void testMakeGuesses()
    {
        // Test generating a guess point grid for dipole fitting.
        // Creates a regular grid of points inside a sphere and
        // excludes points too close to surfaces.
        float guessrad = 0.080f;  // 80mm sphere radius
        Vector3f r0(0.0f, 0.0f, 0.04f);  // center of sphere
        float grid = 0.010f;  // 10mm grid spacing
        float exclude = 0.020f;  // exclude 20mm from surface
        float mindist = 0.005f;

        auto guess = FwdBemModel::make_guesses(nullptr, guessrad, r0, grid, exclude, mindist);
        QVERIFY(guess != nullptr);
        QVERIFY2(guess->np > 0, "Should generate guess points");
        QVERIFY2(guess->nuse > 0, "Should have in-use guess points");
        QVERIFY(guess->rr.rows() == guess->np);
        QVERIFY(guess->rr.cols() == 3);

        // All in-use guess points should be inside the sphere (within guessrad)
        // and at least 'exclude' away from the surface, and at least 'mindist' inside
        for (int i = 0; i < guess->np; ++i) {
            if (!guess->inuse[i])
                continue;
            float dist = (guess->rr.row(i).transpose().cast<float>() - r0).norm();
            QVERIFY2(dist <= guessrad,
                      qPrintable(QString("In-use guess point %1 at distance %2 exceeds sphere radius %3")
                                 .arg(i).arg(dist).arg(guessrad)));
            QVERIFY2(dist >= exclude,
                      qPrintable(QString("In-use guess point %1 at distance %2 is closer than exclude %3")
                                 .arg(i).arg(dist).arg(exclude)));
        }
    }

    // ---- FwdCompData ----

    void testFwdCompDataMembers()
    {
        // Test FwdCompData default construction and member initialization
        FwdCompData compData;
        QVERIFY(compData.set == nullptr);
        QVERIFY(compData.comp_coils == nullptr);
        QVERIFY(compData.client == nullptr);
    }

    // ---- FwdThreadArg ----

    void testFwdThreadArgConstruct()
    {
        // Test FwdThreadArg default construction
        FwdThreadArg arg;
        QVERIFY(arg.res == nullptr);
        QVERIFY(arg.res_grad == nullptr);
        QVERIFY(arg.coils_els == nullptr);
        QVERIFY(arg.s == nullptr);
        QVERIFY(arg.fixed_ori == false);
        QVERIFY(arg.comp == -1);
    }

    // ---- Full Forward Computation Pipeline ----

    void testComputeFwdPipeline()
    {
        // Test the complete forward computation pipeline using ComputeFwd.
        // This exercises compute_fwd.cpp (initFwd, calculateFwd, populateMetadata)
        // which showed 0% coverage in existing reports.

        auto pSettings = std::make_shared<ComputeFwdSettings>();
        pSettings->include_meg = true;
        pSettings->include_eeg = true;
        pSettings->accurate = true;
        pSettings->srcname = srcPath();
        pSettings->measname = rawPath();
        pSettings->mriname = transPath();
        pSettings->transname.clear();
        pSettings->bemname = bem3Path();
        pSettings->mindist = 5.0f / 1000.0f;

        // Use a temporary output file
        QString tmpDir = QDir::tempPath();
        pSettings->solname = tmpDir + "/test_fwd_coverage_output.fif";

        QFile rawFile(rawPath());
        FiffRawData raw(rawFile);
        pSettings->pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo(raw.info));
        pSettings->checkIntegrity();

        auto pFwdComputer = std::make_shared<ComputeFwd>(pSettings);
        auto pFwd = pFwdComputer->calculateFwd();

        QVERIFY(pFwd != nullptr);
        QVERIFY(pFwd->nsource > 0);
        QVERIFY(pFwd->nchan > 0);
        QVERIFY(pFwd->sol->data.rows() > 0);
        QVERIFY(pFwd->sol->data.cols() > 0);

        // Test updateHeadPos
        bool updated = pFwdComputer->updateHeadPos(raw.info.dev_head_t, *pFwd);
        QVERIFY(updated);

        // Clean up temp file
        QFile::remove(pSettings->solname);
    }
};

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFwdBemFieldComputation)
#include "test_fwd_bem_field_computation.moc"
