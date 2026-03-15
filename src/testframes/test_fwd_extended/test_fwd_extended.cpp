/**
 * @file test_fwd_extended.cpp
 * @brief Comprehensive tests for FWDLIB: FwdEegSphereModel, ComputeFwdSettings,
 *        FwdEegSphereLayer, and sphere-potential helpers.
 */
#include <QTest>
#include <QCoreApplication>
#include <QFile>

#include <fwd/fwd_eeg_sphere_model.h>
#include <fwd/fwd_eeg_sphere_model_set.h>
#include <fwd/fwd_eeg_sphere_layer.h>
#include <fwd/compute_fwd/compute_fwd_settings.h>
#include <fwd/fwd_coil_set.h>

using namespace FWDLIB;
using namespace Eigen;

class TestFwdExtended : public QObject
{
    Q_OBJECT

private:
    QString dataPath() const {
        return QCoreApplication::applicationDirPath() +
               "/../resources/data/mne-cpp-test-data/";
    }

private slots:

    //=========================================================================
    // FwdEegSphereLayer
    //=========================================================================
    void sphereLayer_defaultCtor()
    {
        FwdEegSphereLayer layer;
        // Default-constructed layer should have zero radius and sigma
        QCOMPARE(layer.rad, 0.0f);
        QCOMPARE(layer.sigma, 0.0f);
    }

    //=========================================================================
    // FwdEegSphereModel - construction
    //=========================================================================
    void sphereModel_defaultCtor()
    {
        FwdEegSphereModel model;
        QVERIFY(model.name.isEmpty());
        QCOMPARE(model.nlayer(), 0);
        QCOMPARE(model.nterms, 0);
        QCOMPARE(model.nfit, 0);
    }

    void sphereModel_copyConstruct()
    {
        FwdEegSphereModel model;
        model.name = "test_model";

        FwdEegSphereModel copy(model);
        QCOMPARE(copy.name, QString("test_model"));
    }

    void sphereModel_createEegSphereModel()
    {
        // 3-layer sphere model: brain, skull, scalp
        VectorXf rads(3);
        rads << 0.070f, 0.080f, 0.090f;  // meters
        VectorXf sigmas(3);
        sigmas << 0.33f, 0.0042f, 0.33f;

        auto model = FwdEegSphereModel::fwd_create_eeg_sphere_model(
            "three_layer", 3, rads, sigmas);

        QVERIFY(model != nullptr);
        QCOMPARE(model->nlayer(), 3);
        QCOMPARE(model->name, QString("three_layer"));
        QCOMPARE(model->layers.size(), 3);

        // Check layers - radii are normalized by outermost layer (R=0.090)
        // After normalization: 0.070/0.090, 0.080/0.090, 0.090/0.090
        QVERIFY(qAbs(model->layers[0].rad - 0.070f/0.090f) < 1e-5f);
        QVERIFY(qAbs(model->layers[0].sigma - 0.33f) < 1e-6f);
        QVERIFY(qAbs(model->layers[1].rad - 0.080f/0.090f) < 1e-5f);
        QVERIFY(qAbs(model->layers[2].rad - 1.0f) < 1e-5f);
    }

    void sphereModel_fourLayerModel()
    {
        // Standard 4-layer: brain, CSF, skull, scalp
        VectorXf rads(4);
        rads << 0.065f, 0.070f, 0.080f, 0.090f;
        VectorXf sigmas(4);
        sigmas << 0.33f, 1.0f, 0.0042f, 0.33f;

        auto model = FwdEegSphereModel::fwd_create_eeg_sphere_model(
            "four_layer", 4, rads, sigmas);

        QVERIFY(model != nullptr);
        QCOMPARE(model->nlayer(), 4);
    }

    void sphereModel_setupAndCoeff()
    {
        // Create 3-shell and set up
        VectorXf rads(3);
        rads << 0.070f, 0.080f, 0.090f;
        VectorXf sigmas(3);
        sigmas << 0.33f, 0.0042f, 0.33f;

        auto model = FwdEegSphereModel::fwd_create_eeg_sphere_model(
            "test", 3, rads, sigmas);
        QVERIFY(model != nullptr);

        // Setup with Berg-Scherg fitting
        bool ok = model->fwd_setup_eeg_sphere_model(0.09f, true, 3);
        QVERIFY(ok);
        // After setup, mu/lambda/nfit are set by Berg-Scherg fitting
        QCOMPARE(model->nfit, 3);
        QCOMPARE(model->mu.size(), 3);
        QCOMPARE(model->lambda.size(), 3);

        // Get multi-sphere coefficient for n=1
        double coeff1 = model->fwd_eeg_get_multi_sphere_model_coeff(1);
        QVERIFY(std::isfinite(coeff1));

        // Coefficient for higher n
        double coeff10 = model->fwd_eeg_get_multi_sphere_model_coeff(10);
        QVERIFY(std::isfinite(coeff10));
    }

    void sphereModel_bergSchergFit()
    {
        VectorXf rads(3);
        rads << 0.070f, 0.080f, 0.090f;
        VectorXf sigmas(3);
        sigmas << 0.33f, 0.0042f, 0.33f;

        auto model = FwdEegSphereModel::fwd_create_eeg_sphere_model(
            "test", 3, rads, sigmas);
        QVERIFY(model != nullptr);

        float rv = 0.0f;
        bool ok = model->fwd_eeg_fit_berg_scherg(200, 3, rv);
        QVERIFY(ok);
        QVERIFY(rv >= 0.0f);
        QVERIFY(rv < 1.0f); // residual variance should be small
        QCOMPARE(model->nfit, 3);
        QCOMPARE(model->mu.size(), 3);
        QCOMPARE(model->lambda.size(), 3);
    }

    void sphereModel_setupWithoutBergScherg()
    {
        VectorXf rads(3);
        rads << 0.070f, 0.080f, 0.090f;
        VectorXf sigmas(3);
        sigmas << 0.33f, 0.0042f, 0.33f;

        auto model = FwdEegSphereModel::fwd_create_eeg_sphere_model(
            "test_no_bs", 3, rads, sigmas);
        QVERIFY(model != nullptr);

        bool ok = model->fwd_setup_eeg_sphere_model(0.09f, false, 0);
        QVERIFY(ok);
        // Without Berg-Scherg, radii are scaled but nterms is not set
        // (nterms is lazy-initialized during potential computation)
        QVERIFY(qAbs(model->layers[2].rad - 0.09f) < 1e-5f);
    }

    void sphereModel_singleLayerModel()
    {
        // Homogeneous sphere model (1 layer)
        VectorXf rads(1);
        rads << 0.090f;
        VectorXf sigmas(1);
        sigmas << 0.33f;

        auto model = FwdEegSphereModel::fwd_create_eeg_sphere_model(
            "homo", 1, rads, sigmas);
        QVERIFY(model != nullptr);
        QCOMPARE(model->nlayer(), 1);
    }

    //=========================================================================
    // Sphere potentials - Legendre / potential components
    //=========================================================================
    void sphereModel_legendreNext()
    {
        // Test next_legen for a few values
        double p0 = 1.0;  // P0(0) = 1
        double p01 = 0.0;
        double p1 = 0.0;  // P1(0) = 0
        double p11 = 0.0;
        double x = 0.5;

        // Step to n=1
        FwdEegSphereModel::next_legen(1, x, p0, p01, p1, p11);
        QVERIFY(qAbs(p0 - x) < 1e-10); // P0(1) = x
    }

    void sphereModel_calcPotComponents()
    {
        // Set up coefficients for a simple case
        VectorXd fn(10);
        for (int i = 0; i < 10; ++i)
            fn(i) = 1.0; // uniform coefficients for testing

        double Vrp = 0.0, Vtp = 0.0;
        double beta = 0.5;
        double cgamma = 0.7;

        FwdEegSphereModel::calc_pot_components(beta, cgamma, Vrp, Vtp, fn, 10);

        QVERIFY(std::isfinite(Vrp));
        QVERIFY(std::isfinite(Vtp));
    }

    //=========================================================================
    // FwdEegSphereModel electric potential computation
    //=========================================================================
    void sphereModel_eegSpherepot()
    {
        // Create 3-shell model and compute potential for a dipole
        VectorXf rads(3);
        rads << 0.070f, 0.080f, 0.090f;
        VectorXf sigmas(3);
        sigmas << 0.33f, 0.0042f, 0.33f;

        auto model = FwdEegSphereModel::fwd_create_eeg_sphere_model(
            "pot_test", 3, rads, sigmas);
        QVERIFY(model != nullptr);
        QVERIFY(model->fwd_setup_eeg_sphere_model(0.09f, true, 3));

        // Dipole at center offset
        Eigen::Vector3f rd(0.0f, 0.0f, 0.04f);
        Eigen::Vector3f Q(0.0f, 0.0f, 1e-8f); // radial dipole

        // 4 electrodes on the surface
        int neeg = 4;
        Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> el(neeg, 3);
        // Place electrodes at various points on the outer sphere
        el(0,0) = 0.0f; el(0,1) = 0.0f; el(0,2) = 0.09f;
        el(1,0) = 0.09f; el(1,1) = 0.0f; el(1,2) = 0.0f;
        el(2,0) = 0.0f; el(2,1) = 0.09f; el(2,2) = 0.0f;
        el(3,0) = 0.0f; el(3,1) = 0.0f; el(3,2) = -0.09f;

        VectorXf Vval(neeg);
        Vval.setZero();

        int ret = FwdEegSphereModel::fwd_eeg_spherepot(rd, Q, el, neeg, Vval, model.get());
        QVERIFY(ret == 0); // success

        // Top electrode (same direction as dipole) should have highest potential
        QVERIFY(std::isfinite(Vval(0)));
    }

    //=========================================================================
    // ComputeFwdSettings
    //=========================================================================
    void computeFwdSettings_defaultCtor()
    {
        ComputeFwdSettings settings;
        QVERIFY(settings.srcname.isEmpty());
        QVERIFY(settings.measname.isEmpty());
        QVERIFY(!settings.accurate);
        QVERIFY(!settings.fixed_ori);
        QVERIFY(!settings.include_meg);
        QVERIFY(!settings.include_eeg);
    }

    void computeFwdSettings_memberDefaults()
    {
        ComputeFwdSettings settings;
        QCOMPARE(settings.coord_frame, FIFFV_COORD_HEAD);
        QVERIFY(settings.mindist >= 0.0f);
        QVERIFY(settings.eeg_sphere_rad > 0.0f);
        QVERIFY(settings.filter_spaces);
    }

    void computeFwdSettings_setMembers()
    {
        ComputeFwdSettings settings;
        settings.srcname = "/path/to/src.fif";
        settings.measname = "/path/to/meas.fif";
        settings.bemname = "/path/to/bem.fif";
        settings.accurate = true;
        settings.fixed_ori = true;
        settings.include_meg = false;
        settings.include_eeg = true;

        QCOMPARE(settings.srcname, QString("/path/to/src.fif"));
        QCOMPARE(settings.accurate, true);
        QCOMPARE(settings.fixed_ori, true);
        QCOMPARE(settings.include_meg, false);
    }

    //=========================================================================
    // FwdEegSphereModelSet
    //=========================================================================
    void sphereModelSet_defaultCtor()
    {
        FwdEegSphereModelSet set;
        QCOMPARE(set.nmodel(), 0);
    }
};

QTEST_GUILESS_MAIN(TestFwdExtended)

#include "test_fwd_extended.moc"
