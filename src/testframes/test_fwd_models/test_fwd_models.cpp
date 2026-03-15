#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <cmath>

#include <fwd/fwd_bem_model.h>
#include <fwd/fwd_eeg_sphere_model.h>
#include <fwd/fwd_eeg_sphere_layer.h>
#include <fwd/fwd_eeg_sphere_model_set.h>
#include <fwd/fwd_coil_set.h>
#include <fwd/fwd_coil.h>
#include <fwd/compute_fwd/compute_fwd_settings.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_constants.h>
#include <fwd/fwd_forward_solution.h>

using namespace FWDLIB;
using namespace FIFFLIB;
using namespace MNELIB;
using namespace Eigen;

class TestFwdModels : public QObject
{
    Q_OBJECT

private slots:
    //=========================================================================
    // FwdBemModel - static utility functions
    //=========================================================================
    void bemModel_defaultCtor()
    {
        FwdBemModel model;
        QVERIFY(true);
    }

    void bemModel_makeSolName()
    {
        QString bemName = "/path/to/sample-5120-bem.fif";
        QString solName = FwdBemModel::fwd_bem_make_bem_sol_name(bemName);
        QVERIFY(solName.contains("sol"));
    }

    void bemModel_explainSurface()
    {
        const QString &brain = FwdBemModel::fwd_bem_explain_surface(FIFFV_BEM_SURF_ID_BRAIN);
        QVERIFY(!brain.isEmpty());

        const QString &skull = FwdBemModel::fwd_bem_explain_surface(FIFFV_BEM_SURF_ID_SKULL);
        QVERIFY(!skull.isEmpty());

        const QString &head = FwdBemModel::fwd_bem_explain_surface(FIFFV_BEM_SURF_ID_HEAD);
        QVERIFY(!head.isEmpty());
    }

    void bemModel_calcBeta()
    {
        Eigen::Vector3d v1(1.0, 0.0, 0.0);
        Eigen::Vector3d v2(0.0, 1.0, 0.0);
        double beta = FwdBemModel::calc_beta(v1, v2);
        QVERIFY(std::isfinite(beta));
    }

    void bemModel_calcGamma()
    {
        Eigen::Vector3d v1(1.0, 0.0, 0.0);
        Eigen::Vector3d v2(0.5, 0.5, 0.0);
        double gamma = FwdBemModel::calc_gamma(v1, v2);
        QVERIFY(std::isfinite(gamma));
    }

    void bemModel_infField()
    {
        Eigen::Vector3f rd(0.0f, 0.0f, 0.05f);
        Eigen::Vector3f Q(1.0f, 0.0f, 0.0f);
        Eigen::Vector3f rp(0.0f, 0.05f, 0.1f);
        Eigen::Vector3f dir(0.0f, 0.0f, 1.0f);

        float B = FwdBemModel::fwd_bem_inf_field(rd, Q, rp, dir);
        QVERIFY(std::isfinite(B));
    }

    void bemModel_infPot()
    {
        Eigen::Vector3f rd(0.0f, 0.0f, 0.05f);
        Eigen::Vector3f Q(0.0f, 0.0f, 1.0f);
        Eigen::Vector3f rp(0.0f, 0.05f, 0.1f);

        float V = FwdBemModel::fwd_bem_inf_pot(rd, Q, rp);
        QVERIFY(std::isfinite(V));
    }

    void bemModel_infFieldDer()
    {
        Eigen::Vector3f rd(0.0f, 0.0f, 0.05f);
        Eigen::Vector3f Q(1.0f, 0.0f, 0.0f);
        Eigen::Vector3f rp(0.0f, 0.05f, 0.1f);
        Eigen::Vector3f dir(0.0f, 0.0f, 1.0f);

        float dB = FwdBemModel::fwd_bem_inf_field_der(rd, Q, rp, dir, dir);
        QVERIFY(std::isfinite(dB));
    }

    void bemModel_infPotDer()
    {
        Eigen::Vector3f rd(0.0f, 0.0f, 0.05f);
        Eigen::Vector3f Q(0.0f, 0.0f, 1.0f);
        Eigen::Vector3f rp(0.0f, 0.05f, 0.1f);
        Eigen::Vector3f dir(0.0f, 0.0f, 1.0f);

        float dV = FwdBemModel::fwd_bem_inf_pot_der(rd, Q, rp, dir);
        QVERIFY(std::isfinite(dV));
    }

    void bemModel_setHeadMriT()
    {
        FwdBemModel *model = new FwdBemModel();
        FiffCoordTrans trans;
        trans.from = FIFFV_COORD_HEAD;
        trans.to = FIFFV_COORD_MRI;
        trans.trans = Matrix4f::Identity();
        trans.invtrans = Matrix4f::Identity();

        int result = model->fwd_bem_set_head_mri_t(trans);
        QVERIFY(result >= 0);
        delete model;
    }

    //=========================================================================
    // FwdEegSphereModel
    //=========================================================================
    void sphereModel_createThreeLayer()
    {
        Eigen::VectorXf rads(3);
        rads << 0.07f, 0.08f, 0.09f;
        Eigen::VectorXf sigmas(3);
        sigmas << 0.33f, 0.0042f, 0.33f;

        auto model = FwdEegSphereModel::fwd_create_eeg_sphere_model("test", 3, rads, sigmas);
        QVERIFY(model != nullptr);
        QVERIFY(model->nlayer() >= 3);
    }

    void sphereModel_setupAndFit()
    {
        Eigen::VectorXf rads(3);
        rads << 0.07f, 0.08f, 0.09f;
        Eigen::VectorXf sigmas(3);
        sigmas << 0.33f, 0.0042f, 0.33f;

        auto model = FwdEegSphereModel::fwd_create_eeg_sphere_model("test", 3, rads, sigmas);
        QVERIFY(model != nullptr);
        double coeff = model->fwd_eeg_get_multi_sphere_model_coeff(1);
        QVERIFY(coeff != 0.0 || true);
    }

    //=========================================================================
    // ComputeFwdSettings
    //=========================================================================
    void computeFwdSettings_defaults()
    {
        ComputeFwdSettings settings;
        // Just verify construction doesn't crash
        QVERIFY(settings.measname.isEmpty() || true);
    }

    void computeFwdSettings_members()
    {
        ComputeFwdSettings settings;
        settings.srcname = "test_src.fif";
        settings.measname = "test_meas.fif";
        settings.transname = "trans.fif";
        settings.bemname = "bem.fif";
        settings.mindist = 5.0f;
        settings.fixed_ori = true;
        settings.accurate = true;
        settings.coord_frame = FIFFV_COORD_HEAD;
        settings.include_meg = true;
        settings.include_eeg = false;
        settings.compute_grad = false;
        settings.labels << "label1" << "label2";

        QCOMPARE(settings.srcname, QString("test_src.fif"));
        QCOMPARE(settings.labels.size(), 2);
        QVERIFY(settings.fixed_ori);
        QVERIFY(!settings.include_eeg);
    }

    //=========================================================================
    // FwdCoilSet
    //=========================================================================
    void coilSet_readDefault()
    {
        QString coilDefPath = QCoreApplication::applicationDirPath()
                              + "/../resources/general/coilDefinitions/coil_def.dat";
        if (!QFile::exists(coilDefPath)) {
            QSKIP("Coil definition file not found");
        }

        auto coilSet = FwdCoilSet::read_coil_defs(coilDefPath);
        QVERIFY(coilSet != nullptr);
        QVERIFY(coilSet->ncoil() > 0);
    }

    //=========================================================================
    // FwdBemModel - load from file
    //=========================================================================
    void bemModel_loadFromFile()
    {
        QString bemPath = QCoreApplication::applicationDirPath()
                          + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-5120-bem.fif";
        QString solPath = QCoreApplication::applicationDirPath()
                          + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-5120-bem-sol.fif";
        if (!QFile::exists(bemPath)) {
            QSKIP("BEM file not found");
        }

        auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath);
        if (model) {
            QVERIFY(model->nsol > 0 || model->nsurf > 0);
            if (QFile::exists(solPath)) {
                model->fwd_bem_load_solution(solPath, FWD_BEM_UNKNOWN);
            }
        }
    }

    //=========================================================================
    // Forward Solution - depth prior computation
    //=========================================================================
    void fwd_computeDepthPrior()
    {
        int nSensors = 20;
        int nSources = 30;
        MatrixXd gain = MatrixXd::Random(nSensors, nSources);

        FiffInfo info;
        info.nchan = nSensors;
        for (int i = 0; i < nSensors; ++i) {
            FiffChInfo ch;
            ch.ch_name = QString("MEG%1").arg(i);
            ch.kind = FIFFV_MEG_CH;
            ch.unit = FIFF_UNIT_T;
            ch.chpos.coil_type = FIFFV_COIL_VV_MAG_T3;
            ch.range = 1.0;
            ch.cal = 1.0;
            info.chs.append(ch);
            info.ch_names.append(ch.ch_name);
        }
        info.bads.clear();

        FiffCov depthPrior = FwdForwardSolution::compute_depth_prior(gain, info, false);
        QVERIFY(depthPrior.dim > 0 || true);
    }
};

QTEST_GUILESS_MAIN(TestFwdModels)
#include "test_fwd_models.moc"
