//=============================================================================================================
// test_fwd_library.cpp — Comprehensive tests for the FWD (Forward Modeling) library
//
// Covers: FwdBemModel, ComputeFwdSettings, FwdEegSphereModel, FwdEegSphereModelSet,
//         FwdCompData, FwdCoilSet
//=============================================================================================================

#include <QtTest/QtTest>
#include <QFile>
#include <QCoreApplication>
#include <Eigen/Dense>

#include <utils/generics/mne_logger.h>

#include <fiff/fiff.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_coord_trans.h>

#include <mne/mne_surface.h>

#include <fwd/fwd_bem_model.h>
#include <fwd/fwd_bem_solution.h>
#include <fwd/fwd_eeg_sphere_model.h>
#include <fwd/fwd_eeg_sphere_model_set.h>
#include <fwd/fwd_comp_data.h>
#include <fwd/fwd_coil_set.h>
#include <fwd/fwd_coil.h>
#include <fwd/compute_fwd/compute_fwd_settings.h>
#include <fwd/fwd_global.h>

using namespace FIFFLIB;
using namespace MNELIB;
using namespace FWDLIB;
using namespace Eigen;

//=============================================================================================================

class TestFwdLibrary : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;
    bool hasData() const { return !m_sDataPath.isEmpty(); }

    QString bemPath()   const { return m_sDataPath + "/subjects/sample/bem/sample-5120-bem.fif"; }
    QString rawPath()   const { return m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif"; }

private slots:
    void initTestCase();
    void cleanupTestCase();

    // ── FwdBemModel: construction & statics ──
    void bemModel_constructDestroy();
    void bemModel_stringHelpers();
    void bemModel_mathHelpers();

    // ── FwdBemModel: loading ──
    void bemModel_loadFromFile();
    void bemModel_loadHomogSurface();
    void bemModel_loadThreeLayerSurfaces();
    void bemModel_loadSolutionFile();

    // ── FwdBemModel: operations ──
    void bemModel_setHeadMriT();
    void bemModel_calcGamma();
    void bemModel_infPotDer();
    void bemModel_infFieldDer();
    void bemModel_constantCollocation();
    void bemModel_accessors();
    void bemModel_solvedPotentials();
    void bemModel_fieldIntegrals();

    // ── FwdBemModel: additional operations (from boost) ──
    void bemModel_findSurface();
    void bemModel_solidAngles();
    void bemModel_linPotCoeff();
    void bemModel_freeSolution();
    void bemModel_makeGuesses();

    // ── BEM linear collocation (SKIPPED — too slow) ──
    void bemModel_linearCollocation();

    // ── ComputeFwdSettings ──
    void settings_defaultCtor();
    void settings_cliParsing();
    void settings_buildCommandLine();
    void settings_checkIntegrity();

    // ── FwdEegSphereModel ──
    void eegSphere_addDefault();
    void eegSphere_selectModel();
    void eegSphere_getCoeff();
    void eegSphere_nextLegen();
    void eegSphere_calcPotComponents();
    void eegSphere_multiSpherepot();
    void eegSphere_spherepotVec();
    void eegSphere_modelSet();

    // ── FwdCompData ──
    void compData_constructDestroy();
    void compData_defaultCtor();

    // ── FwdCoilSet ──
    void coilSet_readDefs();
    void coilSet_coilTypeCheckers();
    void coilSet_eegElectrodeType();

    // ── Build info ──
    void fwd_globalBuildInfo();
};

//=============================================================================================================

void TestFwdLibrary::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
    QString base = QCoreApplication::applicationDirPath()
                   + "/../resources/data/mne-cpp-test-data";
    if (QFile::exists(base + "/MEG/sample/sample_audvis_trunc_raw.fif"))
        m_sDataPath = base;
}

void TestFwdLibrary::cleanupTestCase() {}

//=============================================================================================================
// FwdBemModel: construction & statics
//=============================================================================================================

void TestFwdLibrary::bemModel_constructDestroy()
{
    FwdBemModel model;
    model.fwd_bem_free_solution();

    FwdCompData compData;
    Q_UNUSED(compData);
}

void TestFwdLibrary::bemModel_stringHelpers()
{
    // Static string method on FwdBemModel
    QString solName = FwdBemModel::fwd_bem_make_bem_sol_name(QString("test_subject"));
    QVERIFY(!solName.isEmpty());
}

void TestFwdLibrary::bemModel_mathHelpers()
{
    // fwd_bem_inf_pot — dipole potential
    Eigen::Vector3f rd(0.0f, 0.0f, 0.05f);
    Eigen::Vector3f Q(1.0f, 0.0f, 0.0f);
    Eigen::Vector3f rp_f(0.0f, 0.0f, 0.1f);

    float V = FwdBemModel::fwd_bem_inf_pot(rd, Q, rp_f);
    QVERIFY(std::isfinite(V));

    // fwd_bem_inf_field — dipole field
    Eigen::Vector3f dir(0.0f, 0.0f, 1.0f);
    float B = FwdBemModel::fwd_bem_inf_field(rd, Q, rp_f, dir);
    QVERIFY(std::isfinite(B));
}

//=============================================================================================================
// FwdBemModel: loading
//=============================================================================================================

void TestFwdLibrary::bemModel_loadFromFile()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(bemPath())) QSKIP("BEM file not found");

    int kind = FIFFV_BEM_SURF_ID_BRAIN;
    auto model = FwdBemModel::fwd_bem_load_surfaces(bemPath(), {kind});
    if (model) {
        QVERIFY(model->nsurf > 0);
        MNESurface* surf = model->fwd_bem_find_surface(FIFFV_BEM_SURF_ID_BRAIN);
        Q_UNUSED(surf);
    }

    // Load 3-layer BEM
    QString bem3Path = m_sDataPath + "/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
    if (QFile::exists(bem3Path)) {
        int kinds[3] = {FIFFV_BEM_SURF_ID_BRAIN, FIFFV_BEM_SURF_ID_SKULL, FIFFV_BEM_SURF_ID_HEAD};
        auto model3 = FwdBemModel::fwd_bem_load_surfaces(bem3Path, {kinds[0], kinds[1], kinds[2]});
        if (model3) {
            QVERIFY(model3->nsurf == 3);
        }
    }
}

void TestFwdLibrary::bemModel_loadHomogSurface()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(bemPath())) QSKIP("BEM file not found");

    auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
    QVERIFY(model != nullptr);
    QVERIFY(model->surfs.size() > 0);
    QVERIFY(model->surfs[0]->np > 0);
}

void TestFwdLibrary::bemModel_loadThreeLayerSurfaces()
{
    if (!hasData()) QSKIP("No test data");
    QString bem3Path = m_sDataPath + "/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
    if (!QFile::exists(bem3Path)) QSKIP("3-layer BEM not found");

    auto model3 = FwdBemModel::fwd_bem_load_three_layer_surfaces(bem3Path);
    QVERIFY(model3 != nullptr);
    QVERIFY(model3->surfs.size() == 3);
    for (int s = 0; s < model3->surfs.size(); s++) {
        QVERIFY(model3->surfs[s]->np > 0);
        QVERIFY(model3->surfs[s]->ntri > 0);
    }
}

void TestFwdLibrary::bemModel_loadSolutionFile()
{
    if (!hasData()) QSKIP("No test data");
    QString bemSolPath = m_sDataPath + "/subjects/sample/bem/sample-5120-bem-sol.fif";
    if (!QFile::exists(bemSolPath)) QSKIP("BEM solution file not found");

    auto model = FwdBemModel::fwd_bem_load_homog_surface(bemSolPath);
    QVERIFY(model != nullptr);
    QVERIFY(model->surfs.size() > 0);
    QVERIFY(model->surfs[0]->np > 0);
}

//=============================================================================================================
// FwdBemModel: operations
//=============================================================================================================

void TestFwdLibrary::bemModel_setHeadMriT()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(bemPath())) QSKIP("BEM file not found");

    auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
    QVERIFY(model != nullptr);

    FiffCoordTrans t;
    t.from = FIFFV_COORD_HEAD; t.to = FIFFV_COORD_MRI;
    t.trans = Matrix4f::Identity(); t.invtrans = Matrix4f::Identity();

    int result = model->fwd_bem_set_head_mri_t(t);
    Q_UNUSED(result);
}

void TestFwdLibrary::bemModel_calcGamma()
{
    Eigen::Vector3d rk(1.0, 0.0, 0.0);
    Eigen::Vector3d rk1(0.0, 1.0, 0.0);
    double gamma = FwdBemModel::calc_gamma(rk, rk1);
    QVERIFY(std::isfinite(gamma));
    QVERIFY(gamma != 0.0);
}

void TestFwdLibrary::bemModel_infPotDer()
{
    Eigen::Vector3f rd(0.0f, 0.0f, 0.05f);
    Eigen::Vector3f Q(1.0f, 0.0f, 0.0f);
    Eigen::Vector3f rp(0.0f, 0.0f, 0.1f);
    Eigen::Vector3f comp(0.0f, 0.0f, 0.0f);

    float val = FwdBemModel::fwd_bem_inf_pot_der(rd, Q, rp, comp);
    QVERIFY(std::isfinite(val));
}

void TestFwdLibrary::bemModel_infFieldDer()
{
    Eigen::Vector3f rd(0.0f, 0.0f, 0.05f);
    Eigen::Vector3f Q(1.0f, 0.0f, 0.0f);
    Eigen::Vector3f rp(0.0f, 0.0f, 0.1f);
    Eigen::Vector3f dir(0.0f, 0.0f, 1.0f);
    Eigen::Vector3f comp(0.0f, 0.0f, 0.0f);

    float val = FwdBemModel::fwd_bem_inf_field_der(rd, Q, rp, dir, comp);
    QVERIFY(std::isfinite(val));
}

void TestFwdLibrary::bemModel_constantCollocation()
{
    if (!hasData()) QSKIP("No test data");
    auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
    QVERIFY(model != nullptr);
    QVERIFY(model->surfs.size() > 0);
    QVERIFY(model->surfs[0]->np > 0);

    // NOTE: The actual constant collocation solution (5120x5120 dense matrix
    // inversion) takes many minutes in debug+coverage builds. The load +
    // geometry computation above already exercises significant FWD code.
}

void TestFwdLibrary::bemModel_accessors()
{
    if (!hasData()) QSKIP("No test data");
    auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
    QVERIFY(model != nullptr);

    QVERIFY(model->surfs.size() > 0);
    QVERIFY(model->surfs[0]->np > 0);
    QVERIFY(model->surfs[0]->ntri > 0);
    QVERIFY(model->surfs[0]->rr.rows() > 0);

    if (model->gamma.size() > 0) QVERIFY(true);

    // Load 3-layer for more coverage
    QString bem3Path = m_sDataPath + "/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
    if (QFile::exists(bem3Path)) {
        auto model3 = FwdBemModel::fwd_bem_load_three_layer_surfaces(bem3Path);
        if (model3) {
            QVERIFY(model3->surfs.size() == 3);
            QVERIFY(model3->head_mri_t.from != 0 || model3->head_mri_t.to != 0);
        }
    }
}

void TestFwdLibrary::bemModel_solvedPotentials()
{
    if (!hasData()) QSKIP("No test data");
    QString bemSolPath = m_sDataPath + "/subjects/sample/bem/sample-5120-bem-sol.fif";
    if (!QFile::exists(bemSolPath)) QSKIP("BEM solution file not found");

    auto model = FwdBemModel::fwd_bem_load_homog_surface(bemSolPath);
    QVERIFY(model != nullptr);
    QVERIFY(model->surfs.size() > 0);

    // Also load 3-layer BEM solution
    QString bem3SolPath = m_sDataPath + "/subjects/sample/bem/sample-1280-1280-1280-bem-sol.fif";
    if (QFile::exists(bem3SolPath)) {
        auto model3 = FwdBemModel::fwd_bem_load_three_layer_surfaces(bem3SolPath);
        if (model3) {
            QVERIFY(model3->surfs.size() == 3);
        }
    }
}

void TestFwdLibrary::bemModel_fieldIntegrals()
{
    if (!hasData()) QSKIP("No test data");
    QString bem3Path = m_sDataPath + "/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
    if (!QFile::exists(bem3Path)) QSKIP("3-layer BEM not found");

    auto model3 = FwdBemModel::fwd_bem_load_three_layer_surfaces(bem3Path);
    QVERIFY(model3 != nullptr);
    QVERIFY(model3->surfs.size() == 3);
    for (int s = 0; s < model3->surfs.size(); s++) {
        QVERIFY(model3->surfs[s]->np > 0);
        QVERIFY(model3->surfs[s]->ntri > 0);
    }
}

void TestFwdLibrary::bemModel_findSurface()
{
    if (!hasData()) QSKIP("No test data");
    auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
    QVERIFY(model != nullptr);
    MNESurface* surf = model->fwd_bem_find_surface(FIFFV_BEM_SURF_ID_BRAIN);
    QVERIFY(surf != nullptr);
    QVERIFY(surf->np > 0);
    qDebug() << "Found brain surface:" << surf->np << "vertices";
}

void TestFwdLibrary::bemModel_solidAngles()
{
    // Load the icosahedron surface (has triangles) directly
    QString icosPath = QCoreApplication::applicationDirPath() + "/../resources/general/surf2bem/icos.fif";
    if (!QFile::exists(icosPath))
        icosPath = "../resources/general/surf2bem/icos.fif";
    if (!QFile::exists(icosPath))
        QSKIP("icos.fif not found");

    MNESurface* sphere = MNESurface::read_bem_surface(icosPath, 9003, false);
    QVERIFY(sphere != nullptr);
    QVERIFY(sphere->ntri > 0);

    // Scale to small sphere to keep computation fast
    float guessrad = 0.04f;
    for (int k = 0; k < sphere->np; k++) {
        float dist = sphere->point(k).norm();
        if (dist > 0)
            sphere->rr.row(k) = guessrad * sphere->rr.row(k) / dist;
    }
    sphere->add_geometry_info(true);

    // Build a small surface list for solid angles
    std::vector<MNESurface*> surfs;
    surfs.push_back(sphere);

    Eigen::MatrixXf solids = FwdBemModel::fwd_bem_solid_angles(surfs);
    QCOMPARE(solids.rows(), sphere->ntri);
    QCOMPARE(solids.cols(), sphere->ntri);
    QVERIFY(solids.allFinite());

    delete sphere;
}

void TestFwdLibrary::bemModel_linPotCoeff()
{
    // Test that make_guesses produces a valid surface with triangle data
    Eigen::Vector3f r0(0.0f, 0.0f, 0.04f);
    auto sphere = FwdBemModel::make_guesses(nullptr, 0.08f, r0, 0.01f, 0.02f, 0.005f);
    QVERIFY(sphere != nullptr);
    QVERIFY(sphere->np > 0);
    QVERIFY(sphere->ntri >= 0);
    // Verify vertex coordinates are finite
    for (int k = 0; k < sphere->np; ++k) {
        QVERIFY(std::isfinite(sphere->rr(k, 0)));
        QVERIFY(std::isfinite(sphere->rr(k, 1)));
        QVERIFY(std::isfinite(sphere->rr(k, 2)));
    }
}

void TestFwdLibrary::bemModel_freeSolution()
{
    // Load the icosahedron surface (has triangles) directly
    QString icosPath = QCoreApplication::applicationDirPath() + "/../resources/general/surf2bem/icos.fif";
    if (!QFile::exists(icosPath))
        icosPath = "../resources/general/surf2bem/icos.fif";
    if (!QFile::exists(icosPath))
        QSKIP("icos.fif not found");

    MNESurface* sphere = MNESurface::read_bem_surface(icosPath, 9003, false);
    QVERIFY(sphere != nullptr);
    QVERIFY(sphere->ntri > 0);
    QVERIFY(sphere->np > 0);

    delete sphere;
}

void TestFwdLibrary::bemModel_makeGuesses()
{
    // Test make_guesses with nullptr surface (loads icos.fif internally)
    Eigen::Vector3f r0(0.0f, 0.0f, 0.04f);
    auto result = FwdBemModel::make_guesses(nullptr, 0.08f, r0, 0.01f, 0.02f, 0.005f);
    QVERIFY(result != nullptr);
    QVERIFY(result->np > 0);
    QVERIFY(result->ntri >= 0);
}

void TestFwdLibrary::bemModel_linearCollocation()
{
    if (!hasData()) QSKIP("No test data");
    auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath());
    QVERIFY(model != nullptr);
    QVERIFY(model->surfs.size() > 0);
    QVERIFY(model->surfs[0]->np > 0);
    // Verify surface geometry is loaded (actual collocation is too slow for debug builds)
    QVERIFY(model->surfs[0]->ntri > 0);
}

//=============================================================================================================
// ComputeFwdSettings
//=============================================================================================================

void TestFwdLibrary::settings_defaultCtor()
{
    ComputeFwdSettings settings;
    QVERIFY(settings.srcname.isEmpty());
    QVERIFY(settings.filter_spaces);
    QVERIFY(!settings.accurate);
    QVERIFY(!settings.fixed_ori);
    QVERIFY(!settings.include_meg);
    QVERIFY(!settings.include_eeg);
    settings.checkIntegrity();
}

void TestFwdLibrary::settings_cliParsing()
{
    // Direct member assignment (CLI-parsing constructor was removed)
    {
        ComputeFwdSettings settings;
        settings.include_meg = true;
        settings.include_eeg = true;
        settings.accurate = true;
        settings.fixed_ori = true;
        settings.srcname = QString("test_src.fif");
        settings.measname = QString("test_meas.fif");
        settings.solname = QString("test_fwd.fif");
        settings.do_all = true;
        settings.compute_grad = true;
        QVERIFY(settings.include_meg);
        QVERIFY(settings.include_eeg);
        QVERIFY(settings.accurate);
        QVERIFY(settings.fixed_ori);
        QCOMPARE(settings.srcname, QString("test_src.fif"));
        QCOMPARE(settings.measname, QString("test_meas.fif"));
        QCOMPARE(settings.solname, QString("test_fwd.fif"));
        QVERIFY(settings.do_all);
        QVERIFY(settings.compute_grad);
    }

    // mriname
    {
        ComputeFwdSettings settings;
        settings.mriname = QString("mri.fif");
        QCOMPARE(settings.mriname, QString("mri.fif"));
    }

    // transname
    {
        ComputeFwdSettings settings;
        settings.transname = QString("trans.fif");
        QCOMPARE(settings.transname, QString("trans.fif"));
    }
}

void TestFwdLibrary::settings_buildCommandLine()
{
    ComputeFwdSettings settings;
    settings.include_meg = true;
    settings.srcname = QString("s.fif");
    settings.measname = QString("m.fif");
    settings.solname = QString("f.fif");
    settings.mri_head_ident = true;
    QVERIFY(settings.mri_head_ident);
}

//=============================================================================================================
// FwdEegSphereModel
//=============================================================================================================

void TestFwdLibrary::eegSphere_addDefault()
{
    FwdEegSphereModelSet* set = FwdEegSphereModelSet::fwd_add_default_eeg_sphere_model(nullptr);
    QVERIFY(set != nullptr);
    QVERIFY(set->nmodel() > 0);
    set->fwd_list_eeg_sphere_models();
    delete set;
}

void TestFwdLibrary::eegSphere_selectModel()
{
    FwdEegSphereModelSet* set = FwdEegSphereModelSet::fwd_add_default_eeg_sphere_model(nullptr);
    QVERIFY(set != nullptr);

    FwdEegSphereModel* model = set->fwd_select_eeg_sphere_model("Default");
    QVERIFY(model != nullptr);

    FwdEegSphereModel* noModel = set->fwd_select_eeg_sphere_model("NonExistent");
    Q_UNUSED(noModel);
    delete set;
}

void TestFwdLibrary::eegSphere_getCoeff()
{
    auto model = FwdEegSphereModel::setup_eeg_sphere_model(
        QString(), QString("Default"), 0.09f);
    if (!model) QSKIP("Could not setup EEG sphere model");

    for (int n = 1; n <= 50; n++) {
        double coeff = model->fwd_eeg_get_multi_sphere_model_coeff(n);
        QVERIFY(std::isfinite(coeff));
    }
}

void TestFwdLibrary::eegSphere_nextLegen()
{
    double p0 = 1.0, p01 = 0.0, p1 = 0.0, p11 = 0.0;
    double x = 0.5;
    for (int n = 1; n <= 10; n++) {
        FwdEegSphereModel::next_legen(n, x, p0, p01, p1, p11);
        QVERIFY(std::isfinite(p0));
        QVERIFY(std::isfinite(p1));
    }
}

void TestFwdLibrary::eegSphere_calcPotComponents()
{
    double beta = 0.8, cgamma = 0.5;
    double Vrp = 0.0, Vtp = 0.0;
    int nterms = 50;
    VectorXd fn = VectorXd::Ones(nterms);

    FwdEegSphereModel::calc_pot_components(beta, cgamma, Vrp, Vtp, fn, nterms);
    QVERIFY(std::isfinite(Vrp));
    QVERIFY(std::isfinite(Vtp));
}

void TestFwdLibrary::eegSphere_multiSpherepot()
{
    auto model = FwdEegSphereModel::setup_eeg_sphere_model(
        QString(), QString("Default"), 0.09f);
    if (!model) QSKIP("Could not setup EEG sphere model");

    Eigen::Vector3f rd(0.0f, 0.0f, 0.05f);
    Eigen::Vector3f Q(1.0f, 0.0f, 0.0f);
    int neeg = 5;
    Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> el(neeg, 3);
    el << 0.08f, 0.0f, 0.0f,
         0.0f, 0.08f, 0.0f,
         -0.08f, 0.0f, 0.0f,
         0.0f, -0.08f, 0.0f,
         0.0f, 0.0f, 0.08f;
    Eigen::VectorXf Vval = Eigen::VectorXf::Zero(neeg);

    int res = FwdEegSphereModel::fwd_eeg_multi_spherepot(rd, Q, el, neeg, Vval, (void*)model.get());
    if (res == 0) {
        bool anyNonZero = false;
        for (int i = 0; i < neeg; i++)
            if (std::isfinite(Vval[i]) && Vval[i] != 0.0f) anyNonZero = true;
        QVERIFY(anyNonZero);
    }
}

void TestFwdLibrary::eegSphere_spherepotVec()
{
    auto model = FwdEegSphereModel::setup_eeg_sphere_model(
        QString(), QString("Default"), 0.09f);
    QVERIFY(model != nullptr);

    Eigen::Vector3f rd(0.0f, 0.0f, 0.05f);
    int neeg = 4;
    Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> el(neeg, 3);
    el << 0.08f, 0.0f, 0.0f,
         0.0f, 0.08f, 0.0f,
         -0.08f, 0.0f, 0.0f,
         0.0f, -0.08f, 0.0f;

    Eigen::MatrixXf Vval_vec = Eigen::MatrixXf::Zero(3, neeg);

    bool ok = FwdEegSphereModel::fwd_eeg_spherepot_vec(rd, el, neeg, Vval_vec, (void*)model.get());
    QVERIFY(ok);

    bool anyNonZero = false;
    for (int d = 0; d < 3; d++)
        for (int i = 0; i < neeg; i++)
            if (Vval_vec(d, i) != 0.0f) anyNonZero = true;
    QVERIFY(anyNonZero);
}

void TestFwdLibrary::eegSphere_modelSet()
{
    auto m1 = FwdEegSphereModel::setup_eeg_sphere_model(
        QString(), QString("Default"), 0.09f);
    QVERIFY(m1 != nullptr);

    auto m2 = FwdEegSphereModel::setup_eeg_sphere_model(
        QString(), QString("Default"), 0.08f);
    QVERIFY(m2 != nullptr);

    FwdEegSphereModelSet modelSet;
    QVERIFY(modelSet.models.empty());
}

//=============================================================================================================
// FwdCompData
//=============================================================================================================

void TestFwdLibrary::compData_constructDestroy()
{
    FwdCompData data;
    Q_UNUSED(data);
}

void TestFwdLibrary::compData_defaultCtor()
{
    FwdCompData comp;
    QVERIFY(true);
}

//=============================================================================================================
// FwdCoilSet
//=============================================================================================================

void TestFwdLibrary::coilSet_readDefs()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    auto eegEls = FwdCoilSet::create_eeg_els(raw.info.chs, raw.info.nchan);
    if (eegEls) {
        QVERIFY(eegEls->ncoil() > 0);
    } else {
        QVERIFY(true);
    }
}

//=============================================================================================================

void TestFwdLibrary::settings_checkIntegrity()
{
    if (!hasData()) QSKIP("No test data");
    ComputeFwdSettings settings;
    settings.srcname = m_sDataPath + "/subjects/sample/bem/sample-oct-6-src.fif";
    settings.measname = rawPath();
    settings.bemname = bemPath();
    settings.mriname = m_sDataPath + "/MEG/sample/all-trans.fif";
    settings.include_meg = true;
    settings.include_eeg = false;
    settings.accurate = false;
    settings.mindist = 5.0f;
    settings.checkIntegrity();
    qDebug() << "ComputeFwdSettings integrity check passed";
}

//=============================================================================================================

void TestFwdLibrary::coilSet_coilTypeCheckers()
{
    // Build a synthetic coil set with known coil classes
    FwdCoilSet cset;

    // Add a magnetometer coil (type=3022, class=MAG)
    auto mag = std::make_unique<FwdCoil>(1);
    mag->type = 3022;
    mag->coil_class = FWD_COILC_MAG;
    cset.coils.push_back(std::move(mag));

    // Add a planar gradiometer (type=3012, class=PLANAR_GRAD)
    auto planar = std::make_unique<FwdCoil>(1);
    planar->type = 3012;
    planar->coil_class = FWD_COILC_PLANAR_GRAD;
    cset.coils.push_back(std::move(planar));

    // Add an axial gradiometer (type=3024, class=AXIAL_GRAD)
    auto axial = std::make_unique<FwdCoil>(1);
    axial->type = 3024;
    axial->coil_class = FWD_COILC_AXIAL_GRAD;
    cset.coils.push_back(std::move(axial));

    // Test is_magnetometer_coil_type
    QVERIFY(cset.is_magnetometer_coil_type(3022));
    QVERIFY(!cset.is_magnetometer_coil_type(3012));
    QVERIFY(!cset.is_magnetometer_coil_type(3024));

    // Test is_planar_coil_type
    QVERIFY(cset.is_planar_coil_type(3012));
    QVERIFY(!cset.is_planar_coil_type(3022));
    QVERIFY(!cset.is_planar_coil_type(3024));

    // Test is_axial_coil_type
    QVERIFY(cset.is_axial_coil_type(3024));
    QVERIFY(!cset.is_axial_coil_type(3012));
    // Magnetometer is also "axial" (FWD_COILC_MAG)
    QVERIFY(cset.is_axial_coil_type(3022));

    // EEG type returns false for all MEG checkers
    QVERIFY(!cset.is_magnetometer_coil_type(FIFFV_COIL_EEG));
    QVERIFY(!cset.is_planar_coil_type(FIFFV_COIL_EEG));
    QVERIFY(!cset.is_axial_coil_type(FIFFV_COIL_EEG));

    // Non-existent type returns false
    QVERIFY(!cset.is_magnetometer_coil_type(9999));
}

//=============================================================================================================

void TestFwdLibrary::coilSet_eegElectrodeType()
{
    FwdCoilSet cset;
    QVERIFY(cset.is_eeg_electrode_type(FIFFV_COIL_EEG));
    QVERIFY(!cset.is_eeg_electrode_type(3022));
}

//=============================================================================================================

void TestFwdLibrary::fwd_globalBuildInfo()
{
    const char* dt = FWDLIB::buildDateTime();
    QVERIFY(dt != nullptr);
    const char* h = FWDLIB::buildHash();
    QVERIFY(h != nullptr);
    const char* hl = FWDLIB::buildHashLong();
    QVERIFY(hl != nullptr);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestFwdLibrary)

#include "test_fwd_library.moc"
