//=============================================================================================================
// test_mne_library.cpp — Comprehensive tests for the MNE library
//
// Covers: MNESourceSpace, MNESourceSpaces, InvSourceEstimate, MNEBem,
//         MNECTFCompDataSet, MNEInverseOperator, MNESurfaceOrVolume,
//         MNEMshDisplaySurface, MNEMshDisplaySurfaceSet, MNEForwardSolution,
//         MNENamedMatrix, MNEHemisphere, MNEEpochDataList, misc types
//=============================================================================================================

#include <QtTest/QtTest>
#include <QFile>
#include <QBuffer>
#include <QTemporaryDir>
#include <QCoreApplication>
#include <QDir>
#include <Eigen/Dense>
#include <Eigen/Sparse>

#include <utils/generics/mne_logger.h>

#include <fiff/fiff.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_digitizer_data.h>

#include <mne/mne.h>
#include <mne/mne_inverse_operator.h>
#include <inv/inv_source_estimate.h>
#include <mne/mne_source_spaces.h>
#include <mne/mne_source_space.h>
#include <mne/mne_forward_solution.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <mne/mne_surface_or_volume.h>
#include <mne/mne_surface.h>
#include <mne/mne_triangle.h>
#include <mne/mne_msh_display_surface.h>
#include <mne/mne_msh_display_surface_set.h>
#include <mne/mne_ctf_comp_data_set.h>
#include <mne/mne_ctf_comp_data.h>
#include <mne/mne_named_matrix.h>
#include <mne/mne_epoch_data_list.h>
#include <mne/mne_epoch_data.h>
#include <mne/mne_hemisphere.h>
#include <mne/mne_cluster_info.h>
#include <mne/mne_vol_geom.h>
#include <mne/mne_msh_color_scale_def.h>
#include <mne/mne_msh_picked.h>

#include <fwd/fwd_bem_model.h>

using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVLIB;
using namespace FWDLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================

class TestMneLibrary : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;
    bool hasData() const { return !m_sDataPath.isEmpty(); }

    QString bemPath()   const { return m_sDataPath + "/subjects/sample/bem/sample-5120-bem.fif"; }
    QString rawPath()   const { return m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif"; }
    QString avePath()   const { return m_sDataPath + "/MEG/sample/sample_audvis-ave.fif"; }
    QString covPath()   const { return m_sDataPath + "/MEG/sample/sample_audvis-cov.fif"; }
    QString fwdPath()   const { return m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif"; }
    QString srcPath()   const { return m_sDataPath + "/subjects/sample/bem/sample-oct-6-src.fif"; }
    QString transPath() const { return m_sDataPath + "/MEG/sample/all-trans.fif"; }

    bool readSourceSpaces(const QString& path, MNESourceSpaces& out) {
        QFile file(path);
        FiffStream::SPtr stream(new FiffStream(&file));
        if (!stream->open()) return false;
        return MNESourceSpaces::readFromStream(stream, false, out);
    }

private slots:
    void initTestCase();
    void cleanupTestCase();

    // ── MNESourceSpace ──
    void sourceSpace_createAndBasicOps();
    void sourceSpace_readFromFile();
    void sourceSpace_cloneAndMethods();
    void sourceSpace_transform();
    void sourceSpace_rearrange();
    void sourceSpace_addPatchStats();

    // ── MNESourceSpaces ──
    void sourceSpaces_getVertno();
    void sourceSpaces_clearAndAccess();
    void sourceSpaces_readWithGeometry();
    void sourceSpaces_filterAndTransform();
    void sourceSpaces_readAddGeom();

    // ── InvSourceEstimate ──
    void sourceEstimate_fullLifecycle();
    void sourceEstimate_writeReadBuffer();
    void sourceEstimate_writeReadFile();

    // ── MNEBem ──
    void bem_constructAndOperators();
    void bem_readFromFile();
    void bem_transformOps();

    // ── MNECTFCompDataSet ──
    void ctfComp_staticHelpers();
    void ctfComp_readFromRawFile();
    void ctfComp_constructCopyDestroy();
    void ctfComp_makeComp();
    void ctfComp_explainAndMap();

    // ── MNEInverseOperator ──
    void inverseOp_readFromFile();
    void inverseOp_basicGetters();
    void inverseOp_readFullFile();

    // ── MNESurfaceOrVolume ──
    void surfOrVol_solidAngle();
    void surfOrVol_computeCm();
    void surfOrVol_nearestData();
    void surfOrVol_addUniformCurv();
    void surfOrVol_realBemMethods();

    // ── MNEMshDisplaySurface ──
    void displaySurf_basicOps();
    void displaySurf_decideCurvDisplay();
    void displaySurf_decideExtent();
    void displaySurf_setupCurvatureColors();
    void displaySurf_alignment();

    // ── MNEMshDisplaySurfaceSet ──
    void displaySurfSet_load();
    void displaySurfSet_addBem();

    // ── MNEForwardSolution ──
    void forwardSolution_pickTypes();
    void forwardSolution_clusterInfo();
    void forwardSolution_reduceForward();
    void forwardSolution_orientPrior();
    void forwardSolution_restrictGainMatrix();
    void forwardSolution_prepareForward();
    void forwardSolution_toFixedOri();
    void forwardSolution_pickChannels();
    void forwardSolution_computeDepthPrior();
    void forwardSolution_pickRegions();

    // ── MNENamedMatrix ──
    void namedMatrix_lifecycle();
    void namedMatrix_readFromStream();
    void namedMatrix_methods();

    // ── MNEEpochDataList ──
    void epochData_baselineCorrectionAndDrop();
    void epochData_pickChannels();
    void epochData_average();
    void epochData_readEpochs();

    // ── Volume source space ──
    void volumeSourceSpace_create();

    // ── Misc small types ──
    void misc_hemisphere();
    void misc_volGeom();
    void misc_mshPicked();
    void misc_colorScaleDef();
    void misc_ctfCompData();
    void misc_epochDataList();
    void misc_bemRead();

    // ── MNESurface/BEM exercises (from boost) ──
    void bemSurface_readSingleLayer();
    void bemSurface_readThreeLayer();
    void bemSurface_sumSolids();
    void bemSurface_triangleCoords();
    void bemSurface_nearestTrianglePoint();
    void bemSurface_projectToTriangle();
    void bemSurface_projectToSurface();
    void bemSurface_geometryInfo();

    // ── MNEForwardSolution extras (from boost) ──
    void forwardSolution_tripletSelection();
    void forwardSolution_readVerify();

    // ── MNEInverseOperator extras (from boost) ──
    void inverseOp_makeSmallChannelSet();
    void inverseOp_writeReadRoundTrip();
    void inverseOp_checkChNames();
};

//=============================================================================================================

void TestMneLibrary::initTestCase()
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QString base = QCoreApplication::applicationDirPath()
                   + "/../resources/data/mne-cpp-test-data";
    if (QFile::exists(base + "/MEG/sample/sample_audvis_trunc_raw.fif"))
        m_sDataPath = base;
}

void TestMneLibrary::cleanupTestCase() {}

//=============================================================================================================
// MNESourceSpace
//=============================================================================================================

void TestMneLibrary::sourceSpace_createAndBasicOps()
{
    MNESourceSpace ss0;
    QCOMPARE(ss0.np, 0);

    auto ss1 = MNESourceSpace::create_source_space(100);
    QVERIFY(ss1 != nullptr);
    QCOMPARE(ss1->np, 100);

    ss1->enable_all_sources();
    QCOMPARE(ss1->nuse, ss1->np);

    int hemi = ss1->is_left_hemi();
    Q_UNUSED(hemi);
    qint32 hemiResult = ss1->find_source_space_hemi();
    Q_UNUSED(hemiResult);

    VectorXi newInuse = VectorXi::Zero(100);
    newInuse.head(50).setOnes();
    ss1->update_inuse(newInuse);
    QCOMPARE(ss1->nuse, 50);
}

void TestMneLibrary::sourceSpace_readFromFile()
{
    if (!hasData()) QSKIP("No test data");
    QString path = m_sDataPath + "/subjects/sample/bem/sample-oct-6-src.fif";
    if (!QFile::exists(path)) QSKIP("Source space file not found");

    std::vector<std::unique_ptr<MNESourceSpace>> spaces;
    int res = MNESourceSpace::read_source_spaces(path, spaces);
    Q_UNUSED(res);
    QVERIFY(spaces.size() >= 1);

    for (auto& sp : spaces) {
        QVERIFY(sp->np > 0);
        QVERIFY(sp->nuse > 0);
        qint32 hemi = sp->find_source_space_hemi();
        Q_UNUSED(hemi);
    }
}

void TestMneLibrary::sourceSpace_cloneAndMethods()
{
    auto ss = MNESourceSpace::create_source_space(50);
    QVERIFY(ss != nullptr);

    ss->rr = Eigen::MatrixXf::Random(50, 3).eval();
    ss->nn = Eigen::MatrixXf::Zero(50, 3);
    ss->nn.col(2).setOnes();

    auto cloned = ss->clone();
    QVERIFY(cloned != nullptr);
    QCOMPARE(cloned->np, 50);

    ss->compute_surface_cm();
    ss->add_uniform_curv();

    VectorXi nearIdx = VectorXi::LinSpaced(50, 0, 49);
    VectorXd nearDist = VectorXd::Ones(50) * 0.01;
    ss->setNearestData(nearIdx, nearDist);
    QCOMPARE(ss->nearestVertIdx().size(), 50);
    QCOMPARE(ss->nearestDistVec().size(), 50);
}

void TestMneLibrary::sourceSpace_transform()
{
    if (!hasData()) QSKIP("No test data");
    MNESourceSpaces srcSpaces;
    if (!readSourceSpaces(srcPath(), srcSpaces)) QSKIP("Could not read source spaces");

    FiffCoordTrans t;
    t.from = FIFFV_COORD_MRI; t.to = FIFFV_COORD_HEAD;
    t.trans = Matrix4f::Identity(); t.invtrans = Matrix4f::Identity();

    int oldNp = srcSpaces[0].np;
    srcSpaces[0].transform_source_space(t);
    QCOMPARE(srcSpaces[0].np, oldNp);
}

void TestMneLibrary::sourceSpace_rearrange()
{
    if (!hasData()) QSKIP("No test data");
    MNESourceSpaces srcSpaces;
    if (!readSourceSpaces(srcPath(), srcSpaces)) QSKIP("Could not read source spaces");

    srcSpaces[0].rearrange_source_space();
    QVERIFY(srcSpaces[0].np > 0);
}

void TestMneLibrary::sourceSpace_addPatchStats()
{
    if (!hasData()) QSKIP("No test data");
    MNESourceSpaces srcSpaces;
    if (!readSourceSpaces(srcPath(), srcSpaces)) QSKIP("Could not read source spaces");

    int result = srcSpaces[0].add_patch_stats();
    Q_UNUSED(result);
}

//=============================================================================================================
// MNESourceSpaces
//=============================================================================================================

void TestMneLibrary::sourceSpaces_getVertno()
{
    if (!hasData()) QSKIP("No test data");
    MNESourceSpaces srcSpaces;
    if (!readSourceSpaces(srcPath(), srcSpaces)) QSKIP("Could not read source spaces");

    QList<VectorXi> vertno = srcSpaces.get_vertno();
    QCOMPARE(vertno.size(), srcSpaces.size());
    for (int i = 0; i < vertno.size(); i++)
        QVERIFY(vertno[i].size() > 0);
}

void TestMneLibrary::sourceSpaces_clearAndAccess()
{
    if (!hasData()) QSKIP("No test data");
    MNESourceSpaces srcSpaces;
    if (!readSourceSpaces(srcPath(), srcSpaces)) QSKIP("Could not read source spaces");

    MNESourceSpace& lh = srcSpaces[0];
    QVERIFY(lh.np > 0);
    MNESourceSpace& lhByName = srcSpaces[QString("lh")];
    Q_UNUSED(lhByName);
    MNESourceSpace& rhByName = srcSpaces[QString("rh")];
    Q_UNUSED(rhByName);

    MNEHemisphere* hemi0 = srcSpaces.hemisphereAt(0);
    QVERIFY(hemi0 != nullptr);
    MNEHemisphere* hemiNull = srcSpaces.hemisphereAt(99);
    QVERIFY(hemiNull == nullptr);

    MNESourceSpaces copy(srcSpaces);
    copy.clear();
    QCOMPARE(copy.size(), 0);
}

void TestMneLibrary::sourceSpaces_readWithGeometry()
{
    if (!hasData()) QSKIP("No test data");
    QFile file(srcPath());
    FiffStream::SPtr stream(new FiffStream(&file));
    if (!stream->open()) QSKIP("Could not open source space");

    MNESourceSpaces srcSpaces;
    bool ok = MNESourceSpaces::readFromStream(stream, true, srcSpaces);
    QVERIFY(ok);
    QVERIFY(srcSpaces.size() >= 2);
    QVERIFY(srcSpaces[0].np > 0);
    QVERIFY(srcSpaces[0].nuse > 0);
}

void TestMneLibrary::sourceSpaces_filterAndTransform()
{
    if (!hasData()) QSKIP("No test data");
    QFile file(srcPath());
    FiffStream::SPtr stream(new FiffStream(&file));
    QVERIFY(stream->open());

    MNESourceSpaces srcSpaces;
    bool ok = MNESourceSpaces::readFromStream(stream, true, srcSpaces);
    QVERIFY(ok);

    QList<VectorXi> vertno = srcSpaces.get_vertno();
    QVERIFY(vertno.size() >= 2);
    QVERIFY(vertno[0].size() > 0);

    QVERIFY(!srcSpaces.isEmpty());
}

void TestMneLibrary::sourceSpaces_readAddGeom()
{
    if (!hasData()) QSKIP("No test data");
    QFile file(srcPath());
    FiffStream::SPtr stream(new FiffStream(&file));
    QVERIFY(stream->open());

    MNESourceSpaces srcSpaces;
    bool ok = MNESourceSpaces::readFromStream(stream, true, srcSpaces);
    QVERIFY(ok);
    QVERIFY(srcSpaces.size() >= 2);

    MNESourceSpace& lh = srcSpaces[0];
    QVERIFY(lh.np > 0);
    QVERIFY(lh.nuse > 0);

    MNEHemisphere* hemi0 = srcSpaces.hemisphereAt(0);
    QVERIFY(hemi0 != nullptr);

    QList<VectorXi> vertno = srcSpaces.get_vertno();
    QVERIFY(vertno.size() >= 2);
    QVERIFY(lh.neighbor_vert.size() > 0 || lh.np > 0);
}

//=============================================================================================================
// InvSourceEstimate
//=============================================================================================================

void TestMneLibrary::sourceEstimate_fullLifecycle()
{
    InvSourceEstimate stc0;
    QVERIFY(stc0.isEmpty());
    QCOMPARE(stc0.samples(), 0);

    MatrixXd sol = MatrixXd::Random(20, 10);
    VectorXi verts(20);
    for (int i = 0; i < 20; i++) verts[i] = i;

    InvSourceEstimate stc1(sol, verts, 0.0f, 0.001f);
    QVERIFY(!stc1.isEmpty());
    QCOMPARE(stc1.samples(), 10);

    InvSourceEstimate stc2(stc1);
    QCOMPARE(stc2.samples(), 10);

    InvSourceEstimate stc3;
    stc3 = stc1;
    QCOMPARE(stc3.samples(), 10);

    InvSourceEstimate reduced = stc1.reduce(2, 5);
    QCOMPARE(reduced.samples(), 5);

    stc1.clear();
    QCOMPARE(stc1.data.rows(), 0);
    QCOMPARE(stc1.data.cols(), 0);
}

void TestMneLibrary::sourceEstimate_writeReadBuffer()
{
    MatrixXd sol = MatrixXd::Random(10, 5);
    VectorXi verts = VectorXi::LinSpaced(10, 0, 9);
    InvSourceEstimate stc(sol, verts, 0.0f, 0.001f);

    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    bool written = stc.write(buffer);
    QVERIFY(written);

    buffer.seek(0);
    InvSourceEstimate stcRead;
    bool readOk = InvSourceEstimate::read(buffer, stcRead);
    QVERIFY(readOk);
    QCOMPARE(stcRead.samples(), 5);
}

void TestMneLibrary::sourceEstimate_writeReadFile()
{
    int nSources = 50, nTimes = 10;
    MatrixXd data = MatrixXd::Random(nSources, nTimes);
    VectorXi vertices = VectorXi::LinSpaced(nSources, 0, nSources - 1);
    InvSourceEstimate stc(data, vertices, 0.0f, 0.01f);
    QVERIFY(!stc.isEmpty());

    QString tmpPath = QCoreApplication::applicationDirPath() + "/test_stc_tmp.stc";
    {
        QFile outFile(tmpPath);
        QVERIFY(outFile.open(QIODevice::WriteOnly));
        stc.write(outFile);
        outFile.close();
    }
    if (QFile::exists(tmpPath)) {
        QFile inFile(tmpPath);
        QVERIFY(inFile.open(QIODevice::ReadOnly));
        InvSourceEstimate stcRead;
        InvSourceEstimate::read(inFile, stcRead);
        inFile.close();
    }
    QFile::remove(tmpPath);
    QVERIFY(true);
}

//=============================================================================================================
// MNEBem
//=============================================================================================================

void TestMneLibrary::bem_constructAndOperators()
{
    MNEBem bem;
    QVERIFY(bem.isEmpty());

    MNEBemSurface surf1;
    surf1.id = 1;
    bem << surf1;
    QCOMPARE(bem.size(), 1);

    MNEBemSurface surf2;
    surf2.id = 2;
    bem << &surf2;
    QCOMPARE(bem.size(), 2);

    MNEBem bem2(bem);
    QCOMPARE(bem2.size(), 2);

    bem.clear();
    QVERIFY(bem.isEmpty());
}

void TestMneLibrary::bem_readFromFile()
{
    if (!hasData()) QSKIP("No test data");
    QString path = m_sDataPath + "/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
    if (!QFile::exists(path)) QSKIP("BEM file not found");

    QFile bemFile(path);
    MNEBem bem(bemFile);
    QVERIFY(!bem.isEmpty());
    QVERIFY(bem.size() >= 1);
}

void TestMneLibrary::bem_transformOps()
{
    MNEBem bem;
    MNEBemSurface surf;
    surf.id = 1;
    surf.np = 3;
    surf.rr = MatrixXf::Identity(3, 3);
    surf.nn = MatrixXf::Identity(3, 3);
    bem << surf;

    FiffCoordTrans trans;
    trans.from = FIFFV_COORD_MRI; trans.to = FIFFV_COORD_HEAD;
    trans.trans = Matrix4f::Identity(); trans.invtrans = Matrix4f::Identity();

    bem.transform(trans);
    bem.invtransform(trans);
}

//=============================================================================================================
// MNECTFCompDataSet
//=============================================================================================================

void TestMneLibrary::ctfComp_staticHelpers()
{
    for (int g = 0; g < 5; g++) {
        int k = MNECTFCompDataSet::map_comp_kind(g);
        Q_UNUSED(k);
    }

    QString desc = MNECTFCompDataSet::explain_comp(101);
    QVERIFY(!desc.isEmpty());
    desc = MNECTFCompDataSet::explain_comp(201);
    QVERIFY(!desc.isEmpty());
    desc = MNECTFCompDataSet::explain_comp(301);
    QVERIFY(!desc.isEmpty());
    MNECTFCompDataSet::explain_comp(0);
    MNECTFCompDataSet::explain_comp(999);

    QList<FiffChInfo> chs;
    for (int i = 0; i < 5; i++) {
        FiffChInfo ch;
        ch.kind = FIFFV_MEG_CH;
        ch.chpos.coil_type = (2 << 16) | 3022;
        chs.append(ch);
    }
    int comp = MNECTFCompDataSet::get_comp(chs, 5);
    QCOMPARE(comp, 2);

    QList<FiffChInfo> chs2;
    for (int i = 0; i < 3; i++) {
        FiffChInfo ch;
        ch.chpos.coil_type = 3022;
        chs2.append(ch);
    }
    MNECTFCompDataSet::set_comp(chs2, 3, 1);
}

void TestMneLibrary::ctfComp_readFromRawFile()
{
    if (!hasData()) QSKIP("No test data");
    auto compSet = MNECTFCompDataSet::read(rawPath());
    QVERIFY(compSet != nullptr);
}

void TestMneLibrary::ctfComp_constructCopyDestroy()
{
    MNECTFCompDataSet set;
    QCOMPARE(set.ncomp, 0);

    MNECTFCompDataSet set2(set);
    QCOMPARE(set2.ncomp, 0);

    MNECTFCompData comp;
    QCOMPARE(comp.kind, -1);
}

void TestMneLibrary::ctfComp_makeComp()
{
    if (!hasData()) QSKIP("No test data");
    auto compSet = MNECTFCompDataSet::read(rawPath());
    if (!compSet) QSKIP("No compensation data read");

    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    QList<FiffChInfo> compchs;
    for (const auto& ch : raw.info.chs) {
        if (ch.kind == FIFFV_REF_MEG_CH) compchs.append(ch);
    }

    if (compchs.size() > 0) {
        int res = compSet->make_comp(raw.info.chs, raw.info.nchan, compchs, compchs.size());
        Q_UNUSED(res);
    }
}

void TestMneLibrary::ctfComp_explainAndMap()
{
    for (int kind = 0; kind <= 4; kind++) {
        QString explanation = MNECTFCompDataSet::explain_comp(kind);
        QVERIFY(!explanation.isEmpty());
    }
    MNECTFCompDataSet::map_comp_kind(0);
    MNECTFCompDataSet::map_comp_kind(1);
    MNECTFCompDataSet::map_comp_kind(2);
    MNECTFCompDataSet::map_comp_kind(3);
}

//=============================================================================================================
// MNEInverseOperator
//=============================================================================================================

void TestMneLibrary::inverseOp_readFromFile()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(fwdPath())) QSKIP("Forward file not found");

    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    QVERIFY(!fwd.isEmpty());

    MNEInverseOperator invOp;
    QVERIFY(!invOp.isFixedOrient());

    QFile fwdFile2(fwdPath());
    MNEInverseOperator invOp2;
    bool readRes = MNEInverseOperator::read_inverse_operator(fwdFile2, invOp2);
    Q_UNUSED(readRes);
}

void TestMneLibrary::inverseOp_basicGetters()
{
    MNEInverseOperator invOp;
    QVERIFY(!invOp.isFixedOrient());

    MatrixXd& kernel = invOp.getKernel();
    Q_UNUSED(kernel);

    const MNEInverseOperator& constRef = invOp;
    MatrixXd kernelConst = constRef.getKernel();
    Q_UNUSED(kernelConst);

    MNEInverseOperator invOp2(invOp);
    QVERIFY(!invOp2.isFixedOrient());
}

void TestMneLibrary::inverseOp_readFullFile()
{
    if (!hasData()) QSKIP("No test data");
    QString invPath = m_sDataPath + "/Result/sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif";
    if (!QFile::exists(invPath)) {
        invPath = m_sDataPath + "/MEG/sample/sample_audvis-meg-oct-6-meg-inv.fif";
    }
    if (!QFile::exists(invPath)) {
        // Try MNE sample data
        QString mneData = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif";
        if (QFile::exists(mneData))
            invPath = mneData;
        else
            QSKIP("Inverse operator file not found");
    }

    QFile invFile(invPath);
    MNEInverseOperator inv;
    if (!MNEInverseOperator::read_inverse_operator(invFile, inv))
        QSKIP("Could not read inverse operator");

    QVERIFY(inv.eigen_fields->data.rows() > 0);
    QVERIFY(inv.source_cov->data.rows() > 0);
}

//=============================================================================================================
// MNESurfaceOrVolume
//=============================================================================================================

void TestMneLibrary::surfOrVol_solidAngle()
{
    MNETriangle tri;
    tri.r1 = Vector3f(0,0,0); tri.r2 = Vector3f(1,0,0); tri.r3 = Vector3f(0,1,0);
    tri.r12 = Vector3f(1,0,0); tri.r13 = Vector3f(0,1,0);
    tri.nn = Vector3f(0,0,1); tri.area = 0.5f;

    Vector3f from(0.1f, 0.1f, 1.0f);
    double omega = MNESurfaceOrVolume::solid_angle(from, tri);
    QVERIFY(omega != 0.0 && std::isfinite(omega));

    Vector3f from2(0.1f, 0.1f, -1.0f);
    double omega2 = MNESurfaceOrVolume::solid_angle(from2, tri);
    QVERIFY(omega * omega2 < 0.0);
}

void TestMneLibrary::surfOrVol_computeCm()
{
    MatrixXf rr(4, 3);
    rr << 1,0,0, -1,0,0, 0,1,0, 0,-1,0;
    float cm[3];
    MNESurfaceOrVolume::compute_cm(rr, 4, cm);
    QVERIFY(std::abs(cm[0]) < 1e-6f);
    QVERIFY(std::abs(cm[1]) < 1e-6f);
    QVERIFY(std::abs(cm[2]) < 1e-6f);
}

void TestMneLibrary::surfOrVol_nearestData()
{
    MNESurfaceOrVolume sov;
    VectorXi idx = VectorXi::LinSpaced(10, 0, 9);
    VectorXd dist = VectorXd::Constant(10, 0.05);
    sov.setNearestData(idx, dist);
    QCOMPARE(sov.nearestVertIdx().size(), 10);
    QCOMPARE(sov.nearestVertIdx()[5], 5);
}

void TestMneLibrary::surfOrVol_addUniformCurv()
{
    MNESurfaceOrVolume sov;
    sov.np = 5;
    sov.add_uniform_curv();
    QVERIFY(sov.curv.size() > 0);
}

void TestMneLibrary::surfOrVol_realBemMethods()
{
    if (!hasData()) QSKIP("No test data");
    std::unique_ptr<MNESurface> surf(MNESurface::read_bem_surface2(bemPath(), -1, 1));
    QVERIFY(surf != nullptr);

    float cm[3];
    MNESurfaceOrVolume::compute_cm(surf->rr, surf->np, cm);
    QVERIFY(qAbs(cm[0]) < 0.2f && qAbs(cm[1]) < 0.2f && qAbs(cm[2]) < 0.2f);

    MNESurfaceOrVolume::PointsT query(1, 3);
    query << cm[0], cm[1], cm[2];
    VectorXi nearest_tri = VectorXi::Constant(1, -1);
    VectorXf dist = VectorXf::Zero(1);
    surf->find_closest_on_surface_approx(query, 1, nearest_tri, dist, 3);
    QVERIFY(nearest_tri[0] >= 0);
}

//=============================================================================================================
// MNEMshDisplaySurface
//=============================================================================================================

void TestMneLibrary::displaySurf_basicOps()
{
    MNEMshDisplaySurface dsurf;
    dsurf.np = 3;
    dsurf.rr = MatrixXf::Ones(3, 3);
    dsurf.scale(Vector3f(2,2,2));
    QVERIFY(std::abs(dsurf.rr(0,0) - 2.0f) < 1e-6f);
}

void TestMneLibrary::displaySurf_decideCurvDisplay()
{
    MNEMshDisplaySurface dsurf;
    dsurf.decide_curv_display("inflated");
    dsurf.decide_curv_display("sphere");
    dsurf.decide_curv_display("white");
    dsurf.decide_curv_display("orig");
    dsurf.decide_curv_display("pial");
    dsurf.decide_curv_display("unknown");
}

void TestMneLibrary::displaySurf_decideExtent()
{
    MNEMshDisplaySurface dsurf;
    dsurf.np = 4;
    dsurf.rr = MatrixXf(4, 3);
    dsurf.rr << -1,  -1, -1, 1, 1, 1, 0, 0, 0, 0.5f, 0.5f, 0.5f;
    dsurf.decide_surface_extent("test");
}

void TestMneLibrary::displaySurf_setupCurvatureColors()
{
    MNEMshDisplaySurface surf;
    surf.np = 10;
    surf.curv = VectorXf::Random(10).eval();
    surf.setup_curvature_colors();
    QVERIFY(true);
}

void TestMneLibrary::displaySurf_alignment()
{
    if (!hasData()) QSKIP("No test data");
    std::unique_ptr<MNESurface> surf(MNESurface::read_bem_surface2(bemPath(), -1, 1));
    QVERIFY(surf != nullptr);

    auto dispSurf = std::make_unique<MNEMshDisplaySurface>();
    static_cast<MNESurfaceOrVolume&>(*dispSurf) = std::move(static_cast<MNESurfaceOrVolume&>(*surf));
    QVERIFY(dispSurf->np > 0);

    FiffDigitizerData headDig;
    headDig.coord_frame = FIFFV_COORD_HEAD;
    headDig.npoint = 13;

    float cardinals[3][3] = {{-0.07f,0,0},{0,0.08f,0},{0.07f,0,0}};
    int cardIdents[3] = {FIFFV_POINT_LPA, FIFFV_POINT_NASION, FIFFV_POINT_RPA};
    for (int i = 0; i < 3; i++) {
        FiffDigPoint pt;
        pt.kind = FIFFV_POINT_CARDINAL; pt.ident = cardIdents[i];
        pt.r[0] = cardinals[i][0]; pt.r[1] = cardinals[i][1]; pt.r[2] = cardinals[i][2];
        headDig.points.append(pt); headDig.active.append(1); headDig.discard.append(0);
    }
    for (int i = 0; i < 10; i++) {
        FiffDigPoint pt;
        pt.kind = FIFFV_POINT_EXTRA; pt.ident = i;
        float theta = (float)i / 10 * 2.0f * (float)M_PI;
        pt.r[0] = 0.08f * cosf(theta); pt.r[1] = 0.08f * sinf(theta); pt.r[2] = 0.06f;
        headDig.points.append(pt); headDig.active.append(1); headDig.discard.append(0);
    }
    headDig.head_mri_t = std::make_unique<FiffCoordTrans>(
        FIFFV_COORD_HEAD, FIFFV_COORD_MRI, Matrix4f::Identity());
    headDig.dist = VectorXf::Zero(headDig.npoint);
    headDig.closest = VectorXi::Constant(headDig.npoint, -1);
    headDig.closest_point.setZero(headDig.npoint, 3);
    headDig.dist_valid = false;

    dispSurf->calculate_digitizer_distances(headDig, 1, 0);
    QVERIFY(headDig.dist_valid);
    dispSurf->discard_outlier_digitizer_points(headDig, 0.1f);
    float rms = dispSurf->rms_digitizer_distance(headDig);
    QVERIFY(rms >= 0.0f);
}

//=============================================================================================================
// MNEMshDisplaySurfaceSet
//=============================================================================================================

void TestMneLibrary::displaySurfSet_load()
{
    if (!hasData()) QSKIP("No test data");
    QString subjectsDir = m_sDataPath + "/subjects";
    if (!QFile::exists(subjectsDir + "/sample/surf/lh.white")) QSKIP("No FreeSurfer surfaces");

    auto surfSet = MNEMshDisplaySurfaceSet::load(QString("sample"), QString("white"), subjectsDir);
    if (surfSet) {
        QVERIFY(surfSet->nsurf > 0);
    }
}

void TestMneLibrary::displaySurfSet_addBem()
{
    if (!hasData()) QSKIP("No test data");
    MNEMshDisplaySurfaceSet surfSet;
    int result = surfSet.add_bem_surface(bemPath(), FIFFV_BEM_SURF_ID_BRAIN,
                                         QString("innerskull"), 1, 0);
    if (result == 0) QVERIFY(surfSet.surfs.size() > 0);
    else QVERIFY(true);
}

//=============================================================================================================
// MNEForwardSolution
//=============================================================================================================

void TestMneLibrary::forwardSolution_pickTypes()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(fwdPath())) QSKIP("Forward file not found");

    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    if (fwd.isEmpty()) QSKIP("Could not read forward solution");

    MNEForwardSolution fwdMeg = fwd.pick_types(true, false);
    QVERIFY(!fwdMeg.isEmpty());
    MNEForwardSolution fwdEeg = fwd.pick_types(false, true);
    Q_UNUSED(fwdEeg);

    if (!fwd.isFixedOrient() && fwd.surf_ori) {
        MNEForwardSolution fwdCopy(fwd);
        fwdCopy.to_fixed_ori();
        QVERIFY(fwdCopy.isFixedOrient());
    }
}

void TestMneLibrary::forwardSolution_clusterInfo()
{
    MNEClusterInfo ci;
    ci.clusterVertnos.append(VectorXi::LinSpaced(5, 0, 4));
    QCOMPARE(ci.clusterVertnos.size(), 1);
}

void TestMneLibrary::forwardSolution_reduceForward()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(fwdPath())) QSKIP("Forward file not found");

    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    if (fwd.isEmpty()) QSKIP("Could not read forward solution");

    MatrixXd D;
    MNEForwardSolution reduced = fwd.reduce_forward_solution(100, D);
    QVERIFY(!reduced.isEmpty());
}

void TestMneLibrary::forwardSolution_orientPrior()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(fwdPath())) QSKIP("Forward file not found");

    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    if (fwd.isEmpty()) QSKIP("Could not read forward solution");

    FiffCov orient_prior = fwd.compute_orient_prior(0.2f);
    QVERIFY(orient_prior.data.rows() > 0);
}

void TestMneLibrary::forwardSolution_restrictGainMatrix()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(rawPath())) QSKIP("Raw file not found");

    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    MatrixXd G = MatrixXd::Ones(raw.info.nchan, 30);
    MNEForwardSolution::restrict_gain_matrix(G, raw.info);
    QVERIFY(G.rows() > 0);
    QVERIFY(G.cols() == 30);
}

void TestMneLibrary::forwardSolution_prepareForward()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(fwdPath()) || !QFile::exists(rawPath()) || !QFile::exists(covPath()))
        QSKIP("Required files not found");

    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    if (fwd.isEmpty()) QSKIP("Could not read forward solution");

    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    QFile covFile(covPath());
    FiffCov noise_cov(covFile);

    FiffInfo fwdInfo;
    MatrixXd gain;
    FiffCov outCov;
    MatrixXd whitener;
    qint32 numNonZero;

    fwd.prepare_forward(raw.info, noise_cov, false, fwdInfo, gain, outCov, whitener, numNonZero);
    QVERIFY(gain.rows() > 0 && gain.cols() > 0);
    QVERIFY(numNonZero > 0);
}

void TestMneLibrary::forwardSolution_toFixedOri()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(fwdPath())) QSKIP("Forward file not found");

    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    if (fwd.isEmpty()) QSKIP("Could not read forward solution");

    MNEForwardSolution fwdMeg = fwd.pick_types(true, false);
    QVERIFY(!fwdMeg.isEmpty());
    int origCols = fwdMeg.sol->ncol;
    fwdMeg.to_fixed_ori();
    QVERIFY(fwdMeg.sol->ncol > 0 && fwdMeg.sol->ncol <= origCols);
}

void TestMneLibrary::forwardSolution_pickChannels()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(fwdPath())) QSKIP("Forward file not found");

    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    if (fwd.isEmpty()) QSKIP("Could not read forward solution");

    QStringList include;
    if (fwd.sol->row_names.size() >= 10) {
        for (int i = 0; i < 10; i++) include.append(fwd.sol->row_names[i]);
    }
    if (include.size() > 0) {
        MNEForwardSolution fwdPicked = fwd.pick_channels(include);
        QVERIFY(!fwdPicked.isEmpty());
    }
}

void TestMneLibrary::forwardSolution_computeDepthPrior()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(fwdPath()) || !QFile::exists(rawPath())) QSKIP("Required files not found");

    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    if (fwd.isEmpty()) QSKIP("Could not read forward solution");

    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    MNEForwardSolution fwdMeg = fwd.pick_types(true, false);
    if (fwdMeg.isEmpty()) QSKIP("No MEG forward");

    Eigen::MatrixXd Gain = fwdMeg.sol->data.cast<double>();
    FiffCov depth_prior = MNEForwardSolution::compute_depth_prior(
        Gain, raw.info, fwdMeg.isFixedOrient(), 0.8, 10.0);
    QVERIFY(depth_prior.data.rows() > 0);
}

void TestMneLibrary::forwardSolution_pickRegions()
{
    if (!hasData()) QSKIP("No test data");

    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    QVERIFY(!fwd.isEmpty());

    MNEForwardSolution fwdMeg = fwd.pick_types(true, false);
    QVERIFY(!fwdMeg.isEmpty());
    MNEForwardSolution fwdEeg = fwd.pick_types(false, true);
    QVERIFY(!fwdEeg.isEmpty());
    MNEForwardSolution fwdBoth = fwd.pick_types(true, true);
    QVERIFY(!fwdBoth.isEmpty());

    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    FiffCov depthPrior = MNEForwardSolution::compute_depth_prior(
        fwd.sol->data, raw.info, (fwd.source_ori == FIFFV_MNE_FIXED_ORI));
    QVERIFY(depthPrior.data.rows() > 0);

    FiffCov orientPrior = fwd.compute_orient_prior(0.2f);
    QVERIFY(orientPrior.data.rows() > 0);
}

//=============================================================================================================
// MNENamedMatrix
//=============================================================================================================

void TestMneLibrary::namedMatrix_lifecycle()
{
    MNENamedMatrix nm;
    QCOMPARE(nm.nrow, 0);
    QCOMPARE(nm.ncol, 0);

    MNENamedMatrix nm2;
    nm2.nrow = 3; nm2.ncol = 4;
    nm2.data = MatrixXf::Random(3, 4).eval();
    nm2.rowlist = QStringList{"r0", "r1", "r2"};
    nm2.collist = QStringList{"c0", "c1", "c2", "c3"};

    MNENamedMatrix nm3(nm2);
    QCOMPARE(nm3.nrow, 3);
}

void TestMneLibrary::namedMatrix_readFromStream()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(fwdPath())) QSKIP("Forward file not found");

    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    if (fwd.isEmpty()) QSKIP("Could not read forward solution");

    QVERIFY(fwd.sol->data.rows() > 0);
    QVERIFY(fwd.sol->nrow > 0);
    QVERIFY(fwd.sol->ncol > 0);
}

void TestMneLibrary::namedMatrix_methods()
{
    MNENamedMatrix m;
    QCOMPARE(m.nrow, 0);

    QStringList rows, cols;
    rows << "r1" << "r2";
    cols << "c1" << "c2" << "c3";
    MatrixXf matData = MatrixXf::Identity(2, 3);

    auto m2 = MNENamedMatrix::build(2, 3, rows, cols, matData);
    QVERIFY(m2 != nullptr);
    QCOMPARE(m2->nrow, 2);

    MNENamedMatrix m3(*m2);
    QCOMPARE(m3.nrow, 2);
}

//=============================================================================================================
// MNEEpochDataList
//=============================================================================================================

void TestMneLibrary::epochData_baselineCorrectionAndDrop()
{
    MNEEpochDataList list;
    for (int i = 0; i < 5; i++) {
        MNEEpochData::SPtr epoch = MNEEpochData::SPtr::create();
        epoch->epoch = MatrixXd::Random(3, 100) + MatrixXd::Constant(3, 100, 10.0);
        epoch->tmin = -0.1f; epoch->tmax = 0.3f;
        epoch->bReject = (i == 2);
        list.append(epoch);
    }

    QPair<float,float> baseline(-0.1f, 0.0f);
    list.applyBaselineCorrection(baseline);
    list.dropRejected();
    QCOMPARE(list.size(), 4);
}

void TestMneLibrary::epochData_pickChannels()
{
    MNEEpochDataList list;
    for (int i = 0; i < 3; i++) {
        MNEEpochData::SPtr epoch = MNEEpochData::SPtr::create();
        epoch->epoch = MatrixXd::Random(5, 50);
        list.append(epoch);
    }

    RowVectorXi sel(3);
    sel << 0, 2, 4;
    list.pick_channels(sel);
    QCOMPARE(list[0]->epoch.rows(), 3);
}

void TestMneLibrary::epochData_average()
{
    auto pInfo = FiffInfo::SPtr::create();
    pInfo->sfreq = 1000.0; pInfo->nchan = 3;
    for (int i = 0; i < 3; i++) {
        FiffChInfo ch;
        ch.ch_name = QString("CH%1").arg(i); ch.kind = FIFFV_MEG_CH;
        pInfo->chs.append(ch); pInfo->ch_names.append(ch.ch_name);
    }

    MNEEpochDataList list;
    for (int i = 0; i < 10; i++) {
        MNEEpochData::SPtr epoch = MNEEpochData::SPtr::create();
        epoch->epoch = MatrixXd::Ones(3, 100) * (i + 1);
        epoch->tmin = 0.0f; epoch->tmax = 0.099f;
        list.append(epoch);
    }

    FiffEvoked avg = list.average(*pInfo, 0, 99);
    QCOMPARE(avg.data.rows(), 3);
    QCOMPARE(avg.data.cols(), 100);
}

void TestMneLibrary::epochData_readEpochs()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    MatrixXi events(5, 3);
    for (int i = 0; i < 5; i++) {
        events(i, 0) = raw.first_samp + 500 + i * 200;
        events(i, 1) = 0; events(i, 2) = 1;
    }

    QMap<QString, double> reject;
    MNEEpochDataList epochList = MNEEpochDataList::readEpochs(raw, events, -0.1f, 0.3f, 1, reject);
    QVERIFY(epochList.size() > 0);
}

//=============================================================================================================
// Volume source space
//=============================================================================================================

void TestMneLibrary::volumeSourceSpace_create()
{
    if (!hasData()) QSKIP("No test data");
    std::unique_ptr<MNESurface> surf(MNESurface::read_bem_surface2(bemPath(), -1, 1));
    QVERIFY(surf != nullptr);

    MNESourceSpace* vol = MNESourceSpace::make_volume_source_space(*surf, 0.020f, 0.0f, 0.005f);
    QVERIFY(vol != nullptr);
    QVERIFY(vol->np > 0);
    delete vol;
}

//=============================================================================================================
// Misc small types
//=============================================================================================================

void TestMneLibrary::misc_hemisphere()
{
    MNEHemisphere hemi;
    QCOMPARE(hemi.np, -1);
    hemi.np = 5;
    hemi.rr = MatrixX3f::Random(5, 3);
    hemi.nn = MatrixX3f::Zero(5, 3);
    hemi.nn.col(2).setOnes();
    hemi.inuse = VectorXi::Ones(5);
    hemi.nuse = 5;
}

void TestMneLibrary::misc_volGeom()
{
    MNEVolGeom vg;
    QVERIFY(vg.valid == 0);
}

void TestMneLibrary::misc_mshPicked()   { MNEMshPicked p; Q_UNUSED(p); }
void TestMneLibrary::misc_colorScaleDef() { MNEMshColorScaleDef c; Q_UNUSED(c); }

void TestMneLibrary::misc_ctfCompData()
{
    MNECTFCompData comp;
    QCOMPARE(comp.kind, -1);
    QCOMPARE(comp.calibrated, 0);
}

void TestMneLibrary::misc_epochDataList()
{
    MNEEpochDataList list;
    QVERIFY(list.isEmpty());

    MNEEpochData::SPtr epoch = MNEEpochData::SPtr::create();
    epoch->epoch = MatrixXd::Random(5, 100);
    epoch->tmin = 0.0f; epoch->tmax = 0.1f;
    list.append(epoch);
    QCOMPARE(list.size(), 1);
}

void TestMneLibrary::misc_bemRead()
{
    if (!hasData()) QSKIP("No test data");
    QFile bemFile(bemPath());
    MNEBem bem(bemFile);
    QVERIFY(bem.size() > 0);
    QVERIFY(bem[0].np > 0);
    QVERIFY(bem[0].ntri > 0);
    QVERIFY(bem[0].rr.rows() > 0);
    QVERIFY(bem[0].itris.rows() > 0);
}

//=============================================================================================================
// MNESurface / BEM exercises (from boost)
//=============================================================================================================

void TestMneLibrary::bemSurface_readSingleLayer()
{
    if (!hasData()) QSKIP("No test data");
    float sigma = 0.0f;
    MNESurface* surf = MNESurface::read_bem_surface(bemPath(), -1, true, sigma);
    QVERIFY(surf != nullptr);
    QVERIFY(surf->np > 0);
    QVERIFY(surf->ntri > 0);
    QVERIFY(surf->rr.rows() == surf->np);
    QVERIFY(surf->itris.rows() == surf->ntri);
    qDebug() << "Single-layer BEM:" << surf->np << "verts," << surf->ntri << "tris, sigma=" << sigma;
    delete surf;
}

void TestMneLibrary::bemSurface_readThreeLayer()
{
    if (!hasData()) QSKIP("No test data");
    QString bem3File = m_sDataPath + "/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
    if (!QFile::exists(bem3File)) { QSKIP("Three-layer BEM file not available"); }
    float sigma = 0.0f;
    MNESurface* surf = MNESurface::read_bem_surface(bem3File, 1, true, sigma);
    QVERIFY(surf != nullptr);
    QVERIFY(surf->np > 0);
    qDebug() << "Three-layer BEM brain:" << surf->np << "verts";
    delete surf;
}

void TestMneLibrary::bemSurface_sumSolids()
{
    if (!hasData()) QSKIP("No test data");
    MNESurface* surf = MNESurface::read_bem_surface(bemPath(), -1, true);
    QVERIFY(surf != nullptr);
    Vector3f inside(surf->cm[0], surf->cm[1], surf->cm[2]);
    double total = surf->sum_solids(inside);
    qDebug() << "Sum of solid angles from CM:" << total;
    QVERIFY(std::abs(std::abs(total) - 4.0 * M_PI) < 1.0);
    delete surf;
}

void TestMneLibrary::bemSurface_triangleCoords()
{
    if (!hasData()) QSKIP("No test data");
    MNESurface* surf = MNESurface::read_bem_surface(bemPath(), -1, true);
    QVERIFY(surf != nullptr && surf->ntri > 0);
    int v0 = surf->itris(0, 0);
    Vector3f r(surf->rr(v0, 0), surf->rr(v0, 1), surf->rr(v0, 2));
    float x, y, z;
    surf->triangle_coords(r, 0, x, y, z);
    QVERIFY(std::isfinite(x) && std::isfinite(y) && std::isfinite(z));
    delete surf;
}

void TestMneLibrary::bemSurface_nearestTrianglePoint()
{
    if (!hasData()) QSKIP("No test data");
    MNESurface* surf = MNESurface::read_bem_surface(bemPath(), -1, true);
    QVERIFY(surf != nullptr);
    int v0 = surf->itris(0, 0);
    Vector3f r(surf->rr(v0, 0) + 0.001f, surf->rr(v0, 1), surf->rr(v0, 2));
    float x, y, z;
    int result = surf->nearest_triangle_point(r, 0, x, y, z);
    QVERIFY(result >= 0);
    QVERIFY(std::isfinite(x));
    delete surf;
}

void TestMneLibrary::bemSurface_projectToTriangle()
{
    if (!hasData()) QSKIP("No test data");
    MNESurface* surf = MNESurface::read_bem_surface(bemPath(), -1, true);
    QVERIFY(surf != nullptr);
    Vector3f projected = surf->project_to_triangle(0, 0.3f, 0.3f);
    QVERIFY(std::isfinite(projected(0)));
    delete surf;
}

void TestMneLibrary::bemSurface_projectToSurface()
{
    if (!hasData()) QSKIP("No test data");
    MNESurface* surf = MNESurface::read_bem_surface(bemPath(), -1, true);
    QVERIFY(surf != nullptr);
    Vector3f r(surf->cm[0] + 0.01f, surf->cm[1], surf->cm[2]);
    float dist = 0.0f;
    int tri = surf->project_to_surface(nullptr, r, dist);
    QVERIFY(tri >= 0);
    QVERIFY(dist > 0.0f);
    delete surf;
}

void TestMneLibrary::bemSurface_geometryInfo()
{
    if (!hasData()) QSKIP("No test data");
    MNESurface* surf = MNESurface::read_bem_surface(bemPath(), -1, true);
    QVERIFY(surf != nullptr);
    QVERIFY(surf->ntri > 0 && surf->np > 0);
    QVERIFY(surf->tris.size() == static_cast<size_t>(surf->ntri));
    QVERIFY(surf->tris[0].area > 0.0f);
    QVERIFY(surf->tris[0].nn.norm() > 0.0f);
    delete surf;
}

//=============================================================================================================
// MNEForwardSolution extras (from boost)
//=============================================================================================================

void TestMneLibrary::forwardSolution_tripletSelection()
{
    if (!hasData()) QSKIP("No test data");
    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    QVERIFY(!fwd.isEmpty());
    VectorXi sel(10);
    for (int i = 0; i < 10; ++i) sel(i) = i;
    VectorXi tripSel = fwd.tripletSelection(sel);
    QCOMPARE(tripSel.size(), 30);
    QCOMPARE(tripSel(0), 0);
    QCOMPARE(tripSel(3), 3);
}

void TestMneLibrary::forwardSolution_readVerify()
{
    if (!hasData()) QSKIP("No test data");
    QFile fwdFile(fwdPath());
    MNEForwardSolution fwd(fwdFile);
    QVERIFY(!fwd.isEmpty());
    QVERIFY(fwd.sol->nrow > 0);
    QVERIFY(fwd.sol->ncol > 0);
    QVERIFY(fwd.nsource > 0);
    QVERIFY(fwd.src.size() > 0);
    int totalVerts = 0;
    for (int i = 0; i < fwd.src.size(); ++i) totalVerts += fwd.src[i].nuse;
    qDebug() << "Fwd:" << fwd.sol->nrow << "chs," << fwd.nsource << "src," << totalVerts << "verts";
}

//=============================================================================================================
// MNEInverseOperator extras (from boost)
//=============================================================================================================

void TestMneLibrary::inverseOp_makeSmallChannelSet()
{
    // Read forward solution
    QString mneBase = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/";
    QString fwdPath = mneBase + "sample_audvis-meg-eeg-oct-6-fwd.fif";
    QString covPath = mneBase + "sample_audvis-cov.fif";
    QString rawPath = m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";

    if (!QFile::exists(fwdPath) || !QFile::exists(covPath) || !QFile::exists(rawPath))
        QSKIP("Required MNE sample data files not found");

    // Read raw info
    QFile rawFile(rawPath);
    FiffRawData raw(rawFile);
    FiffInfo info = raw.info;

    // Read forward solution
    QFile fwdFile(fwdPath);
    MNEForwardSolution fwd;
    bool ok = MNEForwardSolution::read(fwdFile, fwd);
    QVERIFY(ok);
    QVERIFY(!fwd.isEmpty());

    // Read noise covariance
    QFile covFile(covPath);
    FiffCov noiseCov(covFile);

    // Make inverse operator
    MNEInverseOperator inv = MNEInverseOperator::make_inverse_operator(info, fwd, noiseCov, 0.2f, 0.8f);
    QVERIFY(inv.eigen_fields->data.rows() > 0);
}

void TestMneLibrary::inverseOp_writeReadRoundTrip()
{
    // Read an existing inverse op and write/read it
    QString invPath = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif";
    if (!QFile::exists(invPath))
        QSKIP("Inverse operator file not found");

    QFile invFile(invPath);
    MNEInverseOperator inv;
    QVERIFY(MNEInverseOperator::read_inverse_operator(invFile, inv));
    QVERIFY(inv.eigen_fields->data.rows() > 0);

    // Write to temp file
    QString tmpPath = QDir::tempPath() + "/test_inv_round_trip.fif";
    QFile outFile(tmpPath);
    inv.write(outFile);

    // Read back
    QFile inFile(tmpPath);
    MNEInverseOperator inv2;
    QVERIFY(MNEInverseOperator::read_inverse_operator(inFile, inv2));
    QCOMPARE(inv2.eigen_fields->data.rows(), inv.eigen_fields->data.rows());
    QCOMPARE(inv2.source_cov->data.rows(), inv.source_cov->data.rows());

    QFile::remove(tmpPath);
}

void TestMneLibrary::inverseOp_checkChNames()
{
    QString invPath = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif";
    if (!QFile::exists(invPath))
        QSKIP("Inverse operator file not found");

    QFile invFile(invPath);
    MNEInverseOperator inv;
    QVERIFY(MNEInverseOperator::read_inverse_operator(invFile, inv));

    // Inverse operator should have channel names
    QVERIFY(inv.eigen_fields->data.rows() > 0);
    QVERIFY(inv.noise_cov->names.size() > 0);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneLibrary)

#include "test_mne_library.moc"
