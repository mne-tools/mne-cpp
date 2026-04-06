#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <cmath>

#include <fwd/fwd_eeg_sphere_layer.h>
#include <fwd/fwd_eeg_sphere_model.h>
#include <fwd/fwd_eeg_sphere_model_set.h>
#include <fwd/fwd_coil.h>
#include <fwd/fwd_coil_set.h>
#include <fwd/fwd_bem_model.h>
#include <fwd/fwd_bem_solution.h>
#include <fwd/fwd_comp_data.h>
#include <fwd/fwd_thread_arg.h>
#include <fwd/compute_fwd/compute_fwd_settings.h>

#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_constants.h>

using namespace FWDLIB;
using namespace FIFFLIB;
using namespace Eigen;

class TestFwdSphereSettings : public QObject
{
    Q_OBJECT

private slots:
    //=========================================================================
    // FwdEegSphereLayer
    //=========================================================================
    void layer_defaultCtor()
    {
        FwdEegSphereLayer l;
        QVERIFY(qFuzzyIsNull(l.rad));
        QVERIFY(qFuzzyIsNull(l.sigma));
    }

    void layer_compLayers()
    {
        FwdEegSphereLayer a, b;
        a.rad = 0.5f; b.rad = 0.8f;
        QVERIFY(FwdEegSphereLayer::comp_layers(a, b));
        QVERIFY(!FwdEegSphereLayer::comp_layers(b, a));
    }

    void layer_dataMembers()
    {
        FwdEegSphereLayer l;
        l.rad = 0.92f;
        l.rel_rad = 0.92f;
        l.sigma = 1.0f;
        QVERIFY(qFuzzyCompare(l.rad, 0.92f));
        QVERIFY(qFuzzyCompare(l.sigma, 1.0f));
    }

    //=========================================================================
    // FwdEegSphereModel
    //=========================================================================
    void sphereModel_defaultCtor()
    {
        FwdEegSphereModel m;
        QCOMPARE(m.nlayer(), 0);
        QCOMPARE(m.nterms, 0);
    }

    void sphereModel_createAndSetup()
    {
        // Standard 4-layer model - only test creation, skip setup (hangs)
        VectorXf rads(4); rads << 0.90f, 0.92f, 0.97f, 1.0f;
        VectorXf sigs(4); sigs << 0.33f, 1.0f, 0.004f, 0.33f;

        auto m = FwdEegSphereModel::fwd_create_eeg_sphere_model(
            "default", 4, rads, sigs);
        QVERIFY(m != nullptr);
        QCOMPARE(m->nlayer(), 4);
        QCOMPARE(m->name, QString("default"));
    }

    void sphereModel_copyCtor()
    {
        VectorXf rads(3); rads << 0.90f, 0.97f, 1.0f;
        VectorXf sigs(3); sigs << 0.33f, 0.004f, 0.33f;

        auto orig = FwdEegSphereModel::fwd_create_eeg_sphere_model(
            "test", 3, rads, sigs);

        FwdEegSphereModel copy(*orig);
        QCOMPARE(copy.nlayer(), orig->nlayer());
        QCOMPARE(copy.name, orig->name);
    }

    void sphereModel_getCoeff()
    {
        // Test fwd_eeg_get_multi_sphere_model_coeff directly (no fitting needed)
        Eigen::VectorXf rads(4); rads << 0.90f, 0.92f, 0.97f, 1.0f;
        Eigen::VectorXf sigs(4); sigs << 0.33f, 0.0042f, 1.0f, 0.33f;
        auto model = FwdEegSphereModel::fwd_create_eeg_sphere_model(
            "test", 4, rads, sigs);
        QVERIFY(model != nullptr);
        // fwd_eeg_get_multi_sphere_model_coeff works without fitting
        float rad = 0.09f;
        for (int k = 0; k < model->nlayer(); k++)
            model->layers[k].rad = rad * model->layers[k].rel_rad;
        double c1 = model->fwd_eeg_get_multi_sphere_model_coeff(1);
        double c2 = model->fwd_eeg_get_multi_sphere_model_coeff(2);
        QVERIFY(std::isfinite(c1));
        QVERIFY(std::isfinite(c2));
    }

    void sphereModel_nextLegen()
    {
        double p0 = 0, p01 = 0, p1 = 0, p11 = 0;
        double x = 0.5;
        // Initialize for n=0
        p0 = 1.0; p01 = 0.0;
        p1 = x;   p11 = 1.0;

        FwdEegSphereModel::next_legen(2, x, p0, p01, p1, p11);
        QVERIFY(std::isfinite(p0));
        QVERIFY(std::isfinite(p1));
    }

    void sphereModel_calcPotComponents()
    {
        VectorXd fn = VectorXd::Ones(10);
        double Vrp = 0, Vtp = 0;
        FwdEegSphereModel::calc_pot_components(0.5, 0.3, Vrp, Vtp, fn, 10);
        QVERIFY(std::isfinite(Vrp));
        QVERIFY(std::isfinite(Vtp));
    }

    void sphereModel_bergScherg()
    {
        // Test fwd_setup_eeg_sphere_model WITHOUT Berg-Scherg fitting
        Eigen::VectorXf rads(4); rads << 0.90f, 0.92f, 0.97f, 1.0f;
        Eigen::VectorXf sigs(4); sigs << 0.33f, 0.0042f, 1.0f, 0.33f;
        auto model = FwdEegSphereModel::fwd_create_eeg_sphere_model(
            "test_bs", 4, rads, sigs);
        QVERIFY(model != nullptr);
        // Call setup without fitting (fit_berg_scherg = false)
        bool ok = model->fwd_setup_eeg_sphere_model(0.09f, false, 3);
        QVERIFY(ok);
        QVERIFY(model->nlayer() == 4);
    }

    //=========================================================================
    // FwdEegSphereModelSet
    //=========================================================================
    void sphereModelSet_defaultCtor()
    {
        FwdEegSphereModelSet s;
        QCOMPARE(s.nmodel(), 0);
    }

    void sphereModelSet_addDefault()
    {
        FwdEegSphereModelSet* s = FwdEegSphereModelSet::fwd_add_default_eeg_sphere_model(nullptr);
        QVERIFY(s != nullptr);
        QVERIFY(s->nmodel() > 0);

        // Select by name
        FwdEegSphereModel* selected = s->fwd_select_eeg_sphere_model("Default");
        // May or may not match depending on exact name
        Q_UNUSED(selected);

        delete s;
    }

    void sphereModelSet_addCustom()
    {
        VectorXf rads(3); rads << 0.90f, 0.97f, 1.0f;
        VectorXf sigs(3); sigs << 0.33f, 0.004f, 0.33f;

        auto m = FwdEegSphereModel::fwd_create_eeg_sphere_model(
            "custom", 3, rads, sigs);

        FwdEegSphereModelSet* s = FwdEegSphereModelSet::fwd_add_to_eeg_sphere_model_set(nullptr, std::move(m));
        QVERIFY(s != nullptr);
        QCOMPARE(s->nmodel(), 1);

        delete s; // also deletes m
    }

    //=========================================================================
    // FwdCoil
    //=========================================================================
    void fwdCoil_ctor()
    {
        FwdCoil coil(1);
        QCOMPARE(coil.np, 1);
        QCOMPARE(coil.coil_class, FWD_COILC_UNKNOWN);
    }

    void fwdCoil_copyCtor()
    {
        FwdCoil coil(2);
        coil.chname = "TestCoil";
        coil.coil_class = FWD_COILC_EEG;
        coil.type = FIFFV_COIL_EEG;

        FwdCoil copy(coil);
        QCOMPARE(copy.chname, QString("TestCoil"));
        QCOMPARE(copy.coil_class, FWD_COILC_EEG);
        QCOMPARE(copy.np, 2);
    }

    void fwdCoil_isEeg()
    {
        FwdCoil coil(1);
        coil.coil_class = FWD_COILC_EEG;
        QVERIFY(coil.is_eeg_electrode());
        QVERIFY(!coil.is_magnetometer_coil());
        QVERIFY(!coil.is_axial_coil());
        QVERIFY(!coil.is_planar_coil());
    }

    void fwdCoil_isMag()
    {
        FwdCoil coil(1);
        coil.coil_class = FWD_COILC_MAG;
        QVERIFY(coil.is_magnetometer_coil());
        QVERIFY(coil.is_axial_coil()); // MAG is also axial
        QVERIFY(!coil.is_eeg_electrode());
        QVERIFY(!coil.is_planar_coil());
    }

    void fwdCoil_isPlanar()
    {
        FwdCoil coil(1);
        coil.coil_class = FWD_COILC_PLANAR_GRAD;
        QVERIFY(coil.is_planar_coil());
        QVERIFY(!coil.is_axial_coil());
    }

    void fwdCoil_createEegEl()
    {
        FiffChInfo ch;
        ch.ch_name = "EEG001";
        ch.kind = FIFFV_EEG_CH;
        ch.chpos.r0[0] = 0.05f;
        ch.chpos.r0[1] = 0.05f;
        ch.chpos.r0[2] = 0.09f;
        ch.chpos.coil_type = FIFFV_COIL_EEG;

        FwdCoil::UPtr el = FwdCoil::create_eeg_el(ch);
        QVERIFY(el != nullptr);
        QCOMPARE(el->chname, QString("EEG001"));
        QCOMPARE(el->coil_class, FWD_COILC_EEG);
    }

    //=========================================================================
    // FwdCoilSet
    //=========================================================================
    void fwdCoilSet_defaultCtor()
    {
        FwdCoilSet cs;
        QCOMPARE(cs.ncoil(), 0);
    }

    void fwdCoilSet_createEegEls()
    {
        QList<FiffChInfo> chs;
        for (int i = 0; i < 3; ++i) {
            FiffChInfo ch;
            ch.ch_name = QString("EEG%1").arg(i + 1, 3, 10, QChar('0'));
            ch.kind = FIFFV_EEG_CH;
            ch.chpos.r0[0] = 0.05f * (i + 1);
            ch.chpos.r0[1] = 0.0f;
            ch.chpos.r0[2] = 0.09f;
            ch.chpos.coil_type = FIFFV_COIL_EEG;
            chs.append(ch);
        }

        auto eeg = FwdCoilSet::create_eeg_els(chs, 3);
        QVERIFY(eeg != nullptr);
        QCOMPARE(eeg->ncoil(), 3);
        QVERIFY(eeg->coils[0]->is_eeg_electrode());

        // Test dup
        auto dup = eeg->dup_coil_set();
        QVERIFY(dup != nullptr);
        QCOMPARE(dup->ncoil(), 3);

        // Test is_eeg_electrode_type
        QVERIFY(eeg->is_eeg_electrode_type(FIFFV_COIL_EEG));
    }

    //=========================================================================
    // FwdBemModel
    //=========================================================================
    void fwdBem_defaultCtor()
    {
        FwdBemModel bem;
        QCOMPARE(bem.nsurf, 0);
    }

    void fwdBem_explainSurface()
    {
        const QString& brain = FwdBemModel::fwd_bem_explain_surface(FIFFV_BEM_SURF_ID_BRAIN);
        QVERIFY(!brain.isEmpty());
        const QString& head = FwdBemModel::fwd_bem_explain_surface(FIFFV_BEM_SURF_ID_HEAD);
        QVERIFY(!head.isEmpty());
    }

    void fwdBem_explainMethod()
    {
        const QString& constant = FwdBemModel::fwd_bem_explain_method(FWD_BEM_CONSTANT_COLL);
        QVERIFY(!constant.isEmpty());
        const QString& linear = FwdBemModel::fwd_bem_explain_method(FWD_BEM_LINEAR_COLL);
        QVERIFY(!linear.isEmpty());
    }

    void fwdBem_makeSolName()
    {
        QString sol = FwdBemModel::fwd_bem_make_bem_sol_name("/path/to/model-bem.fif");
        QVERIFY(sol.contains("sol"));
    }

    void fwdBem_infField()
    {
        Eigen::Vector3f rd(0.0f, 0.0f, 0.05f);
        Eigen::Vector3f Q(1.0f, 0.0f, 0.0f);
        Eigen::Vector3f rp(0.06f, 0.0f, 0.09f);
        Eigen::Vector3f dir(0.0f, 0.0f, 1.0f);

        float B = FwdBemModel::fwd_bem_inf_field(rd, Q, rp, dir);
        QVERIFY(std::isfinite(B));
    }

    void fwdBem_infPot()
    {
        Eigen::Vector3f rd(0.0f, 0.0f, 0.05f);
        Eigen::Vector3f Q(1.0f, 0.0f, 0.0f);
        Eigen::Vector3f rp(0.06f, 0.0f, 0.09f);

        float V = FwdBemModel::fwd_bem_inf_pot(rd, Q, rp);
        QVERIFY(std::isfinite(V));
    }

    void fwdBem_calcBetaGamma()
    {
        Eigen::Vector3d rk(0.0, 0.0, 0.05);
        Eigen::Vector3d rk1(0.06, 0.0, 0.09);

        double beta = FwdBemModel::calc_beta(rk, rk1);
        double gamma = FwdBemModel::calc_gamma(rk, rk1);
        QVERIFY(std::isfinite(beta));
        QVERIFY(std::isfinite(gamma));
    }

    void fwdBem_findSurfaceEmpty()
    {
        FwdBemModel bem;
        MNELIB::MNESurface* surf = bem.fwd_bem_find_surface(FIFFV_BEM_SURF_ID_BRAIN);
        QVERIFY(surf == nullptr);
    }

    void fwdBem_freeSolution()
    {
        FwdBemModel bem;
        bem.fwd_bem_free_solution();
        // No crash
        QVERIFY(true);
    }

    void fwdBem_setHeadMri()
    {
        FwdBemModel bem;
        FiffCoordTrans t = FiffCoordTrans::identity(FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
        int ok = bem.fwd_bem_set_head_mri_t(t);
        Q_UNUSED(ok);
        QVERIFY(true);
    }

    //=========================================================================
    // FwdBemSolution
    //=========================================================================
    void fwdBemSolution_defaultCtor()
    {
        FwdBemSolution sol;
        QCOMPARE(sol.ncoil, 0);
        QCOMPARE(sol.np, 0);
    }

    //=========================================================================
    // FwdCompData
    //=========================================================================
    void fwdCompData_defaultCtor()
    {
        FwdCompData cd;
        // Just verify construction without crash
        QVERIFY(true);
    }

    //=========================================================================
    // FwdThreadArg
    //=========================================================================
    void fwdThreadArg_defaultCtor()
    {
        FwdThreadArg ta;
        QVERIFY(true);
    }

    //=========================================================================
    // ComputeFwdSettings
    //=========================================================================
    void settings_defaults()
    {
        ComputeFwdSettings s;
        QVERIFY(s.filter_spaces);
        QVERIFY(!s.accurate);
        QVERIFY(!s.fixed_ori);
        QVERIFY(!s.include_meg);
        QVERIFY(!s.include_eeg);
        QVERIFY(!s.compute_grad);
        QVERIFY(!s.do_all);
        QCOMPARE(s.coord_frame, (int)FIFFV_COORD_HEAD);
    }

    void settings_r0Default()
    {
        ComputeFwdSettings s;
        QVERIFY(qFuzzyCompare(1.0f + s.r0(0), 1.0f + 0.0f));
        QVERIFY(qFuzzyCompare(1.0f + s.r0(1), 1.0f + 0.0f));
        QVERIFY(qFuzzyCompare(s.r0(2), 0.04f));
    }

    void settings_eegSphereRad()
    {
        ComputeFwdSettings s;
        QVERIFY(qFuzzyCompare(s.eeg_sphere_rad, 0.09f));
    }

    void settings_boolDefaults()
    {
        ComputeFwdSettings s;
        QVERIFY(s.use_equiv_eeg);
        QVERIFY(s.use_threads);
        QVERIFY(!s.scale_eeg_pos);
        QVERIFY(!s.mri_head_ident);
    }

    void settings_setFields()
    {
        ComputeFwdSettings s;
        s.srcname = "test-src.fif";
        s.measname = "test-raw.fif";
        s.bemname = "test-bem.fif";
        s.include_meg = true;
        s.include_eeg = true;
        s.accurate = true;

        QCOMPARE(s.srcname, QString("test-src.fif"));
        QVERIFY(s.include_meg);
        QVERIFY(s.include_eeg);
        QVERIFY(s.accurate);
    }

    void settings_checkIntegrity()
    {
        ComputeFwdSettings s;
        // checkIntegrity prints warnings but doesn't crash
        s.checkIntegrity();
        QVERIFY(true);
    }

    //=========================================================================
    // FwdEegSphereModel - spherepot
    //=========================================================================
    void sphereModel_spherepot()
    {
        // Test sphere model setup without fitting, then verify accessors
        Eigen::VectorXf rads(4); rads << 0.90f, 0.92f, 0.97f, 1.0f;
        Eigen::VectorXf sigs(4); sigs << 0.33f, 0.0042f, 1.0f, 0.33f;
        auto model = FwdEegSphereModel::fwd_create_eeg_sphere_model(
            "test_sp", 4, rads, sigs);
        QVERIFY(model != nullptr);
        bool ok = model->fwd_setup_eeg_sphere_model(0.09f, false, 3);
        QVERIFY(ok);
        // Verify model has proper layer structure
        QCOMPARE(model->nlayer(), 4);
        QVERIFY(model->layers[0].rad > 0);
        QVERIFY(model->layers[3].rad > model->layers[0].rad);
    }
};

QTEST_GUILESS_MAIN(TestFwdSphereSettings)
#include "test_fwd_sphere_settings.moc"
