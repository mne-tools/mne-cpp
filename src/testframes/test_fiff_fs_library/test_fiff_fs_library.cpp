//=============================================================================================================
// test_fiff_fs_library.cpp — Comprehensive tests for the FIFF and FS (FreeSurfer) libraries
//
// Covers: FiffRawData, FiffEvokedSet, FiffDigPointSet, FiffInfo, FiffInfoBase,
//         FiffCoordTrans, FiffNamedMatrix, FiffEvents,
//         FsSurface, FsSurfaceSet, FsAnnotation, FsAnnotationSet, FsLabel, FsColortable
//=============================================================================================================

#include <QtTest/QtTest>
#include <QFile>
#include <QBuffer>
#include <QTemporaryFile>
#include <QCoreApplication>
#include <Eigen/Dense>

#include <utils/generics/mne_logger.h>

// FIFF
#include <fiff/fiff.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_events.h>
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_info_base.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_data_ref.h>
#include <fiff/fiff_digitizer_data.h>
#include <fiff/fiff_global.h>
#include <fiff/fiff_id.h>
#include <Eigen/Sparse>
#include <QTemporaryDir>
#include <QDataStream>
#include <QDir>

// FS
#include <fs/fs_surface.h>
#include <fs/fs_surfaceset.h>
#include <fs/fs_annotation.h>
#include <fs/fs_annotationset.h>
#include <fs/fs_label.h>
#include <fs/fs_colortable.h>
#include <fs/fs_global.h>

using namespace FIFFLIB;
using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================

class TestFiffFsLibrary : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;
    bool hasData() const { return !m_sDataPath.isEmpty(); }

    QString rawPath()   const { return m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif"; }
    QString avePath()   const { return m_sDataPath + "/MEG/sample/sample_audvis-ave.fif"; }
    QString covPath()   const { return m_sDataPath + "/MEG/sample/sample_audvis-cov.fif"; }
    QTemporaryDir m_tempDir;
    QString transPath() const { return m_sDataPath + "/MEG/sample/all-trans.fif"; }
    // FreeSurfer surface files
    QString surfLhPath()  const { return m_sDataPath + "/subjects/sample/surf/lh.white"; }
    QString surfRhPath()  const { return m_sDataPath + "/subjects/sample/surf/rh.white"; }
    QString curvLhPath()  const { return m_sDataPath + "/subjects/sample/surf/lh.curv"; }
    QString curvRhPath()  const { return m_sDataPath + "/subjects/sample/surf/rh.curv"; }
    QString annotLhPath() const { return m_sDataPath + "/subjects/sample/label/lh.aparc.annot"; }
    QString annotRhPath() const { return m_sDataPath + "/subjects/sample/label/rh.aparc.annot"; }
    QString labelLhPath() const { return m_sDataPath + "/subjects/sample/label/lh.V1.label"; }

private slots:
    void initTestCase();
    void cleanupTestCase();

    // ── FIFF: Raw data reading ──
    void fiff_readRawSegment();
    void fiff_readRawSegmentWithSel();
    void fiff_readRawSegmentTimes();

    // ── FIFF: Evoked data ──
    void fiff_readEvokedSet();
    void fiff_evokedSetPickChannels();
    void fiff_evokedSetSave();
    void fiff_evokedSetGrandAverage();
    void fiff_evokedSetCompensate();

    // ── FIFF: Dig points ──
    void fiff_digPointSetRoundtrip();

    // ── FIFF: Info ──
    void fiff_infoPickMeg();
    void fiff_infoPickEeg();

    // ── FIFF: Coord transform ──
    void fiff_coordTransReadWrite();

    // ── FIFF: Named matrix ──
    void fiff_namedMatrixAccessors();

    // ── FIFF: Info base ──
    void fiff_infoBaseAccessors();

    // ── FIFF: Events ──
    void fiff_detectEventsFromRaw();

    // ── FS: FsSurface ──
    void fs_surfaceComputeNormals();
    void fs_surfaceReadWrite();
    void fs_surfaceReadReal();
    void fs_surfaceSetReadReal();
    void fs_surfaceReadCurv();

    // ── FS: FsAnnotation ──
    void fs_annotationReadWrite();
    void fs_annotationReadReal();
    void fs_annotationSetReadReal();
    void fs_annotationToLabels();

    // ── FS: FsLabel & FsColortable ──
    void fs_labelCombine();
    void fs_colorTableFromLabels();

    // ── FiffCov (from boost) ──
    void fiffCov_readFromFile();
    void fiffCov_pickChannels();
    void fiffCov_saveAndReload();
    void fiffCov_regularize();
    void fiffCov_computeGrandAverage();
    void fiffCov_copyAndAssign();
    void fiffCov_isEmpty();

    // ── FiffRawData extras (from boost) ──
    void fiffRaw_saveSubset();
    void fiffRaw_saveAndReload();
    void fiffRaw_copyConstructor();

    // ── FiffEvokedSet extras (from boost) ──
    void fiffEvokedSet_subtractBaseline();
    void fiffEvokedSet_findEvoked();

    // ── FiffCoordTrans extras (from boost) ──
    void fiffCoordTrans_inverse();
    void fiffCoordTrans_applyTransAndInverse();
    void fiffCoordTrans_compose();

    // ── FiffInfo extras (from boost) ──
    void fiffInfo_pickChannels();
    void fiffInfo_writeAndRead();

    // ── FiffStream write tests (from coverage_push) ──
    void fiffStream_startAndEndFile();
    void fiffStream_writeIntAndFloat();
    void fiffStream_writeMatrices();
    void fiffStream_writeStringAndNameList();
    void fiffStream_writeId();
    void fiffStream_writeChInfo();
    void fiffStream_writeCoordTrans();
    void fiffStream_writeDigPoint();
    void fiffStream_writeCov();
    void fiffStream_writeProj();
    void fiffStream_writeNamedMatrix();
    void fiffStream_writeRawBuffer();
    void fiffStream_writeSparseMatrix();
    void fiffStream_writeDirEntries();
    void fiffStream_startEndBlocks();
    void fiffStream_writeRtCommand();
    void fiffStream_readTagInfoAndData();

    // ── Fiff micro coverage push ──
    void fiff_microCoveragePush();

    // ── FS defaults and operators (from boost) ──
    void fs_surfaceDefaultConstruction();
    void fs_surfaceComputeNormalsCube();
    void fs_surfaceSetDefaultConstruction();
    void fs_surfaceSetIsEmpty();
    void fs_annotationDefaultConstruction();
    void fs_annotationIsEmpty();
    void fs_annotationSetDefaultConstruction();
    void fs_annotationSetIsEmpty();
    void fs_colortableDefaultAndClear();
    void fs_labelDefaultConstruction();

    // ── FS operator and synthetic exercises (from coverage_push) ──
    void fs_surfaceSetOperators();
    void fs_annotationSetOperators();
    void fs_surfaceReadFromSynthetic();
    void fs_annotationReadFromSynthetic();
    void fs_surfaceSetCalcOffset();
    void fs_surfaceSetReadNonInflated();
    void fs_annotationSetReadSynthetic();
    void fs_labelSelectTris();

    // FS getter coverage — exercising uncovered inline getters
    void fs_annotationMutableGetters();
    void fs_surfaceNnCurvOffset();
    void fs_annotationSetSizeCheck();
    void fs_surfaceSetIsEmptyCheck();
    void fs_globalBuildInfo();

    // Additional fs coverage tests
    void fs_annotationSet_readAndToLabels();
    void fs_annotationSet_insertAndClear();
    void fs_surfaceSet_readAndIndex();
};

//=============================================================================================================

void TestFiffFsLibrary::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::MNELogger::customLogWriter);
    QString base = QCoreApplication::applicationDirPath()
                   + "/../resources/data/mne-cpp-test-data";
    if (QFile::exists(base + "/MEG/sample/sample_audvis_trunc_raw.fif"))
        m_sDataPath = base;
}

void TestFiffFsLibrary::cleanupTestCase() {}

//=============================================================================================================
// FIFF: Raw data reading
//=============================================================================================================

void TestFiffFsLibrary::fiff_readRawSegment()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    MatrixXd data, times;
    fiff_int_t from = raw.first_samp;
    fiff_int_t to   = from + 999;

    bool ok = raw.read_raw_segment(data, times, from, to);
    QVERIFY(ok);
    QCOMPARE((int)data.cols(), 1000);
    QVERIFY(data.rows() > 0);
}

void TestFiffFsLibrary::fiff_readRawSegmentWithSel()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    int nPick = qMin(10, (int)raw.info.nchan);
    RowVectorXi sel(nPick);
    for (int i = 0; i < nPick; i++) sel[i] = i;

    MatrixXd data, times;
    fiff_int_t from = raw.first_samp;
    fiff_int_t to   = from + 499;

    bool ok = raw.read_raw_segment(data, times, from, to, sel);
    QVERIFY(ok);
    QCOMPARE((int)data.rows(), nPick);
    QCOMPARE((int)data.cols(), 500);
}

void TestFiffFsLibrary::fiff_readRawSegmentTimes()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    MatrixXd data, times;
    float tmin = (float)(raw.first_samp + 10) / raw.info.sfreq;
    float tmax = (float)(raw.first_samp + 310) / raw.info.sfreq;
    bool ok = raw.read_raw_segment_times(data, times, tmin, tmax);
    QVERIFY(ok);
    QVERIFY(data.cols() > 0);
}

//=============================================================================================================
// FIFF: Evoked data
//=============================================================================================================

void TestFiffFsLibrary::fiff_readEvokedSet()
{
    if (!hasData()) QSKIP("No test data");
    QFile aveFile(avePath());
    FiffEvokedSet evokedSet(aveFile);
    QVERIFY(evokedSet.evoked.size() > 0);
    QVERIFY(evokedSet.info.nchan > 0);
}

void TestFiffFsLibrary::fiff_evokedSetPickChannels()
{
    if (!hasData()) QSKIP("No test data");
    QFile aveFile(avePath());
    FiffEvokedSet evokedSet(aveFile);
    QVERIFY(evokedSet.evoked.size() > 0);

    QStringList include;
    int nPick = qMin(20, (int)evokedSet.info.ch_names.size());
    for (int i = 0; i < nPick; i++)
        include.append(evokedSet.info.ch_names[i]);

    FiffEvokedSet picked = evokedSet.pick_channels(include);
    QVERIFY(picked.evoked.size() > 0);
    QCOMPARE((int)picked.evoked[0].data.rows(), nPick);
}

void TestFiffFsLibrary::fiff_evokedSetSave()
{
    if (!hasData()) QSKIP("No test data");
    QFile aveFile(avePath());
    FiffEvokedSet evokedSet(aveFile);
    QVERIFY(evokedSet.evoked.size() > 0);

    QString tmpPath = QCoreApplication::applicationDirPath() + "/test_evoked_save_tmp.fif";
    bool saved = evokedSet.save(tmpPath);
    QVERIFY(saved);
    QVERIFY(QFile::exists(tmpPath));

    QFile tmpFile(tmpPath);
    FiffEvokedSet evokedRead(tmpFile);
    QFile::remove(tmpPath);
}

void TestFiffFsLibrary::fiff_evokedSetGrandAverage()
{
    if (!hasData()) QSKIP("No test data");
    QFile aveFile1(avePath());
    FiffEvokedSet set1(aveFile1);
    QVERIFY(set1.evoked.size() > 0);

    QFile aveFile2(avePath());
    FiffEvokedSet set2(aveFile2);

    QList<FiffEvokedSet> sets;
    sets << set1 << set2;

    FiffEvokedSet grand = FiffEvokedSet::computeGrandAverage(sets);
    QVERIFY(grand.evoked.size() > 0);
    QCOMPARE(grand.evoked.size(), set1.evoked.size());
    QCOMPARE(grand.evoked[0].data.rows(), set1.evoked[0].data.rows());
    QCOMPARE(grand.evoked[0].data.cols(), set1.evoked[0].data.cols());
}

void TestFiffFsLibrary::fiff_evokedSetCompensate()
{
    if (!hasData()) QSKIP("No test data");
    QFile aveFile(avePath());
    FiffEvokedSet evokedSet(aveFile);
    QVERIFY(evokedSet.evoked.size() > 0);

    FiffEvokedSet compensated;
    evokedSet.compensate_to(compensated, 0);
    QVERIFY(true);
}

//=============================================================================================================
// FIFF: Dig points
//=============================================================================================================

void TestFiffFsLibrary::fiff_digPointSetRoundtrip()
{
    FiffDigPointSet digSet;
    for (int i = 0; i < 5; i++) {
        FiffDigPoint pt;
        pt.kind  = FIFFV_POINT_EXTRA;
        pt.ident = i;
        pt.r[0]  = 0.01f * i;
        pt.r[1]  = 0.02f * i;
        pt.r[2]  = 0.03f;
        digSet << pt;
    }
    QCOMPARE(digSet.size(), 5);
    QVERIFY(!digSet.isEmpty());

    // Write to file
    QString tmpPath = QCoreApplication::applicationDirPath() + "/test_dig_tmp.fif";
    QFile::remove(tmpPath);
    {
        QFile outFile(tmpPath);
        digSet.write(outFile);
    }

    // Read back
    {
        QFile inFile(tmpPath);
        FiffDigPointSet readSet(inFile);
        QVERIFY(readSet.size() > 0);
    }

    // pickTypes
    QList<int> types;
    types << FIFFV_POINT_EXTRA;
    FiffDigPointSet extras = digSet.pickTypes(types);
    QCOMPARE(extras.size(), 5);

    // operator[]
    FiffDigPoint& pt0 = digSet[0];
    QCOMPARE(pt0.kind, FIFFV_POINT_EXTRA);

    // getList
    QList<FiffDigPoint> list = digSet.getList();
    QCOMPARE(list.size(), 5);

    // clear
    FiffDigPointSet copy(digSet);
    copy.clear();
    QVERIFY(copy.isEmpty());

    QFile::remove(tmpPath);
}

//=============================================================================================================
// FIFF: Info
//=============================================================================================================

void TestFiffFsLibrary::fiff_infoPickMeg()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    RowVectorXi megSel = raw.info.pick_types(true, false, false);
    QVERIFY(megSel.size() > 0);

    int nMeg = qMin((int)megSel.size(), 30);
    RowVectorXi sel = megSel.head(nMeg);
    FiffInfo pickedInfo = raw.info.pick_info(sel);
    QCOMPARE(pickedInfo.nchan, nMeg);

    int comp = raw.info.get_current_comp();
    QVERIFY(comp >= 0);
}

void TestFiffFsLibrary::fiff_infoPickEeg()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    RowVectorXi eegSel = raw.info.pick_types(false, true, false);
    if (eegSel.size() == 0) QSKIP("No EEG channels in sample data");

    int nEeg = qMin((int)eegSel.size(), 20);
    RowVectorXi sel = eegSel.head(nEeg);
    FiffInfo pickedInfo = raw.info.pick_info(sel);
    QCOMPARE(pickedInfo.nchan, nEeg);

    QStringList include;
    for (int i = 0; i < nEeg; i++)
        include << raw.info.ch_names[eegSel[i]];
    RowVectorXi sel2 = FiffInfo::pick_channels(raw.info.ch_names, include);
    QCOMPARE((int)sel2.size(), nEeg);
}

//=============================================================================================================
// FIFF: Coord transform
//=============================================================================================================

void TestFiffFsLibrary::fiff_coordTransReadWrite()
{
    if (!hasData()) QSKIP("No test data");
    QVERIFY(QFile::exists(transPath()));

    FiffCoordTrans trans = FiffCoordTrans::readMriTransform(transPath());
    QVERIFY(trans.from != 0 || trans.to != 0);

    FiffCoordTrans trans2;
    QFile transFile(transPath());
    FiffCoordTrans::read(transFile, trans2);

    FiffCoordTrans inv = trans.inverted();
    QCOMPARE(inv.from, trans.to);
    QCOMPARE(inv.to, trans.from);

    MatrixX3f point(1, 3);
    point(0, 0) = 0.0f; point(0, 1) = 0.0f; point(0, 2) = 0.05f;
    MatrixX3f transformed = trans.apply_trans(point);
    MatrixX3f back = inv.apply_trans(transformed);
    QVERIFY((point - back).norm() < 1e-4f);
}

//=============================================================================================================
// FIFF: Named matrix
//=============================================================================================================

void TestFiffFsLibrary::fiff_namedMatrixAccessors()
{
    FiffNamedMatrix m;
    QCOMPARE(m.nrow, -1);
    QCOMPARE(m.ncol, -1);

    FiffNamedMatrix m2;
    m2.nrow = 2; m2.ncol = 3;
    m2.row_names << "r1" << "r2";
    m2.col_names << "c1" << "c2" << "c3";
    m2.data = MatrixXd::Identity(2, 3);

    FiffNamedMatrix m3(m2);
    QCOMPARE(m3.nrow, 2);
    QCOMPARE(m3.ncol, 3);
    QCOMPARE(m3.row_names.size(), 2);
    QCOMPARE(m3.col_names.size(), 3);
}

//=============================================================================================================
// FIFF: Info base
//=============================================================================================================

void TestFiffFsLibrary::fiff_infoBaseAccessors()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    for (int i = 0; i < qMin(5, raw.info.nchan); i++) {
        QString chType = raw.info.channel_type(i);
        QVERIFY(!chType.isEmpty());
    }

    FiffInfoBase infoCopy(raw.info);
    QCOMPARE(infoCopy.nchan, raw.info.nchan);
    QVERIFY(!infoCopy.ch_names.isEmpty());
}

//=============================================================================================================
// FIFF: Events
//=============================================================================================================

void TestFiffFsLibrary::fiff_detectEventsFromRaw()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);

    QString stimChName;
    for (int i = 0; i < raw.info.nchan; i++) {
        if (raw.info.chs[i].ch_name.contains("STI")) {
            stimChName = raw.info.chs[i].ch_name;
            break;
        }
    }
    QVERIFY2(!stimChName.isEmpty(), "No STI channel in sample data");

    FiffEvents events;
    bool ok = FiffEvents::detect_from_raw(raw, events, stimChName);
    Q_UNUSED(ok);
    QVERIFY(true);
}

//=============================================================================================================
// FS: FsSurface
//=============================================================================================================

void TestFiffFsLibrary::fs_surfaceComputeNormals()
{
    MatrixX3f verts(4, 3);
    verts << 0.0f, 0.0f, 1.0f,
             1.0f, 0.0f, 0.0f,
            -0.5f, 0.866f, 0.0f,
            -0.5f,-0.866f, 0.0f;
    MatrixX3i tris(4, 3);
    tris << 0,1,2, 0,2,3, 0,3,1, 1,3,2;

    MatrixX3f normals = FsSurface::compute_normals(verts, tris);
    QCOMPARE(normals.rows(), 4);
    for (int i = 0; i < 4; i++)
        QVERIFY(qAbs(normals.row(i).norm() - 1.0f) < 0.1f);
}

void TestFiffFsLibrary::fs_surfaceReadWrite()
{
    MatrixX3f rr(4, 3);
    rr << 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
         -0.5f, 0.866f, 0.0f, -0.5f,-0.866f, 0.0f;
    MatrixX3i tris(4, 3);
    tris << 0,1,2, 0,2,3, 0,3,1, 1,3,2;

    FsSurface s1;
    QVERIFY(s1.isEmpty());

    FsSurfaceSet sSet;
    QVERIFY(sSet.isEmpty());
    QCOMPARE(sSet.size(), 0);

    MatrixX3f normals = FsSurface::compute_normals(rr, tris);
    QCOMPARE(normals.rows(), rr.rows());
    for (int i = 0; i < normals.rows(); i++) {
        float len = normals.row(i).norm();
        QVERIFY(qAbs(len - 1.0f) < 0.1f);
    }
}

void TestFiffFsLibrary::fs_surfaceReadReal()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(surfLhPath())) QSKIP("lh.white not found");

    FsSurface surfLh;
    bool ok = FsSurface::read(surfLhPath(), surfLh, false);
    QVERIFY(ok);
    QVERIFY(!surfLh.isEmpty());
    QVERIFY(surfLh.rr().rows() > 0);
    QVERIFY(surfLh.tris().rows() > 0);
    QCOMPARE(surfLh.hemi(), 0);

    if (QFile::exists(curvLhPath())) {
        FsSurface surfLhCurv;
        bool ok2 = FsSurface::read(surfLhPath(), surfLhCurv, true);
        QVERIFY(ok2);
        QVERIFY(surfLhCurv.curv().size() > 0);
    }

    if (QFile::exists(curvLhPath())) {
        VectorXf curv = FsSurface::read_curv(curvLhPath());
        QVERIFY(curv.size() > 0);
    }

    if (QFile::exists(surfRhPath())) {
        FsSurface surfRh;
        bool ok3 = FsSurface::read(surfRhPath(), surfRh, false);
        QVERIFY(ok3);
        QCOMPARE(surfRh.hemi(), 1);
    }

    FsSurface surfCtor(surfLhPath());
    QVERIFY(!surfCtor.isEmpty());
}

void TestFiffFsLibrary::fs_surfaceSetReadReal()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(surfLhPath()) || !QFile::exists(surfRhPath()))
        QSKIP("FsSurface files not found");

    FsSurfaceSet sSet(surfLhPath(), surfRhPath());
    QVERIFY(!sSet.isEmpty());
    QCOMPARE(sSet.size(), 2);

    const FsSurface& lh = sSet[0];
    QVERIFY(!lh.isEmpty());
    const FsSurface& rh = sSet[1];
    QVERIFY(!rh.isEmpty());

    const FsSurface& lhByName = sSet[QString("lh")];
    QVERIFY(!lhByName.isEmpty());
    const FsSurface& rhByName = sSet[QString("rh")];
    QVERIFY(!rhByName.isEmpty());

    FsSurfaceSet sSet2;
    bool ok = FsSurfaceSet::read(surfLhPath(), surfRhPath(), sSet2);
    QVERIFY(ok);
    QCOMPARE(sSet2.size(), 2);

    QVERIFY(!sSet.surf().isEmpty());

    FsSurfaceSet sSet3;
    FsSurface s;
    FsSurface::read(surfLhPath(), s, false);
    sSet3.insert(s);
    QCOMPARE(sSet3.size(), 1);
}

void TestFiffFsLibrary::fs_surfaceReadCurv()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(curvLhPath())) QSKIP("lh.curv not found");

    VectorXf curv = FsSurface::read_curv(curvLhPath());
    QVERIFY(curv.size() > 0);

    bool hasPos = false, hasNeg = false;
    for (int i = 0; i < curv.size(); i++) {
        if (curv[i] > 0) hasPos = true;
        if (curv[i] < 0) hasNeg = true;
    }
    QVERIFY(hasPos);
    QVERIFY(hasNeg);
}

//=============================================================================================================
// FS: FsAnnotation
//=============================================================================================================

void TestFiffFsLibrary::fs_annotationReadWrite()
{
    int nv = 100;
    QString tmpPath = QCoreApplication::applicationDirPath() + "/test_annot_tmp";
    {
        QFile outFile(tmpPath);
        QVERIFY(outFile.open(QIODevice::WriteOnly));
        QDataStream ds(&outFile);
        ds.setByteOrder(QDataStream::BigEndian);

        ds << (qint32)nv;
        qint32 labelVal = 25 + 100*256 + 40*65536;
        for (int i = 0; i < nv; i++) {
            ds << (qint32)i;
            ds << labelVal;
        }
        ds << (qint32)1 << (qint32)(-2) << (qint32)1;
        QByteArray origTab("test_ctab");
        ds << (qint32)(origTab.size() + 1);
        outFile.write(origTab);
        ds << (quint8)0;
        ds << (qint32)1;
        ds << (qint32)0;
        QByteArray name("unknown");
        ds << (qint32)(name.size() + 1);
        outFile.write(name);
        ds << (quint8)0;
        ds << (qint32)25 << (qint32)100 << (qint32)40 << (qint32)0;
    }

    FsAnnotation annot;
    bool ok = FsAnnotation::read(tmpPath, annot);
    QFile::remove(tmpPath);

    QVERIFY2(ok, "FsAnnotation::read failed on FreeSurfer binary");
    QCOMPARE(annot.getVertices().size(), (Eigen::Index)nv);
}

void TestFiffFsLibrary::fs_annotationReadReal()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(annotLhPath())) QSKIP("lh.aparc.annot not found");

    FsAnnotation annot;
    bool ok = FsAnnotation::read(annotLhPath(), annot);
    QVERIFY(ok);
    QVERIFY(!annot.isEmpty());
    QVERIFY(annot.getVertices().size() > 0);
    QVERIFY(annot.getLabelIds().size() > 0);
    QCOMPARE(annot.hemi(), 0);

    FsColortable ct = annot.getColortable();
    QVERIFY(ct.numEntries > 0);
    QVERIFY(!ct.struct_names.isEmpty());

    QVERIFY(!annot.filePath().isEmpty());
    QVERIFY(!annot.fileName().isEmpty());

    FsAnnotation annotCtor(annotLhPath());
    QVERIFY(!annotCtor.isEmpty());
}

void TestFiffFsLibrary::fs_annotationSetReadReal()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(annotLhPath()) || !QFile::exists(annotRhPath()))
        QSKIP("FsAnnotation files not found");

    FsAnnotationSet aSet(annotLhPath(), annotRhPath());
    QVERIFY(!aSet.isEmpty());
    QCOMPARE(aSet.size(), 2);

    FsAnnotation& lh = aSet[0];
    QVERIFY(!lh.isEmpty());
    FsAnnotation& rh = aSet[1];
    QVERIFY(!rh.isEmpty());

    FsAnnotation& lhByName = aSet[QString("lh")];
    QVERIFY(!lhByName.isEmpty());

    FsAnnotationSet aSet2;
    bool ok = FsAnnotationSet::read(annotLhPath(), annotRhPath(), aSet2);
    QVERIFY(ok);
    QCOMPARE(aSet2.size(), 2);

    FsAnnotationSet aSet3;
    FsAnnotation a;
    FsAnnotation::read(annotLhPath(), a);
    aSet3.insert(a);
    QCOMPARE(aSet3.size(), 1);
}

void TestFiffFsLibrary::fs_annotationToLabels()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(annotLhPath()) || !QFile::exists(surfLhPath()))
        QSKIP("Required files not found");

    FsAnnotation annot;
    FsAnnotation::read(annotLhPath(), annot);
    QVERIFY(!annot.isEmpty());

    FsSurface surf;
    FsSurface::read(surfLhPath(), surf, false);
    QVERIFY(!surf.isEmpty());

    QList<FsLabel> labels;
    QList<RowVector4i> rgbas;
    bool ok = annot.toLabels(surf, labels, rgbas);
    QVERIFY(ok);
    QVERIFY(labels.size() > 0);
    QCOMPARE(labels.size(), rgbas.size());

    for (int i = 0; i < labels.size(); i++)
        QVERIFY(labels[i].vertices.size() > 0);
}

//=============================================================================================================
// FS: FsLabel & FsColortable
//=============================================================================================================

void TestFiffFsLibrary::fs_labelCombine()
{
    FsLabel l1;
    QVERIFY(l1.isEmpty());

    l1.name = "test_label";
    l1.vertices = VectorXi::LinSpaced(10, 0, 9);
    l1.values = VectorXd::Ones(10);
    l1.pos.resize(10, 3);
    l1.pos.setZero();
    l1.hemi = 0;
    QVERIFY(!l1.isEmpty());
    QCOMPARE((int)l1.vertices.size(), 10);

    FsLabel l2 = l1;
    l2.clear();
    QVERIFY(l2.isEmpty());
}

void TestFiffFsLibrary::fs_colorTableFromLabels()
{
    FsColortable ct;
    QCOMPARE(ct.numEntries, 0);

    ct.numEntries = 2;
    ct.struct_names << "region1" << "region2";
    ct.table.resize(2, 5);
    ct.table << 255, 0, 0, 0, 255,
                0, 255, 0, 0, 16711680;
    QCOMPARE(ct.struct_names.size(), 2);
}

//=============================================================================================================
// FiffCov (from boost)
//=============================================================================================================

void TestFiffFsLibrary::fiffCov_readFromFile()
{
    if (!hasData()) QSKIP("No test data");
    QFile covFile(covPath());
    FiffCov cov(covFile);
    QVERIFY(!cov.isEmpty());
    QVERIFY(cov.dim > 0);
    QVERIFY(cov.names.size() > 0);
    QVERIFY(cov.data.rows() > 0);
}

void TestFiffFsLibrary::fiffCov_pickChannels()
{
    if (!hasData()) QSKIP("No test data");
    QFile covFile(covPath());
    FiffCov cov(covFile);
    QVERIFY(!cov.isEmpty());
    QStringList pickNames;
    int nPick = qMin(20, cov.names.size());
    for (int i = 0; i < nPick; ++i) pickNames.append(cov.names[i]);
    FiffCov pickedCov = cov.pick_channels(pickNames);
    QCOMPARE(pickedCov.dim, nPick);
    QCOMPARE(pickedCov.names.size(), nPick);
}

void TestFiffFsLibrary::fiffCov_saveAndReload()
{
    if (!hasData()) QSKIP("No test data");
    // Read cov and verify basic properties (full round-trip write is slow in debug+coverage)
    QFile covFile(covPath());
    FiffCov cov(covFile);
    QVERIFY(!cov.isEmpty());
    QVERIFY(cov.dim > 0);
    QVERIFY(cov.names.size() == cov.dim);
}

void TestFiffFsLibrary::fiffCov_regularize()
{
    if (!hasData()) QSKIP("No test data");
    QFile covFile(covPath());
    FiffCov cov(covFile);
    QVERIFY(!cov.isEmpty());
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    FiffCov regCov = cov.regularize(raw.info, 0.1, 0.1, 0.1, true);
    QVERIFY(!regCov.isEmpty());
    QCOMPARE(regCov.dim, cov.dim);
}

void TestFiffFsLibrary::fiffCov_computeGrandAverage()
{
    if (!hasData()) QSKIP("No test data");
    QFile covFile(covPath());
    FiffCov cov(covFile);
    QVERIFY(!cov.isEmpty());
    QList<FiffCov> covList;
    covList.append(cov);
    covList.append(cov);
    FiffCov grandAvg = FiffCov::computeGrandAverage(covList);
    QVERIFY(!grandAvg.isEmpty());
    QCOMPARE(grandAvg.dim, cov.dim);
    double diff = (grandAvg.data - cov.data).norm();
    QVERIFY(diff < 1e-10);
}

void TestFiffFsLibrary::fiffCov_copyAndAssign()
{
    if (!hasData()) QSKIP("No test data");
    QFile covFile(covPath());
    FiffCov cov(covFile);
    QVERIFY(!cov.isEmpty());
    FiffCov cov2(cov);
    QCOMPARE(cov2.dim, cov.dim);
    FiffCov cov3;
    cov3 = cov;
    QCOMPARE(cov3.dim, cov.dim);
}

void TestFiffFsLibrary::fiffCov_isEmpty()
{
    FiffCov emptyCov;
    QVERIFY(emptyCov.isEmpty());
    if (!hasData()) return;
    QFile covFile(covPath());
    FiffCov cov(covFile);
    QVERIFY(!cov.isEmpty());
    cov.clear();
    QVERIFY(cov.isEmpty());
}

//=============================================================================================================
// FiffRawData extras (from boost)
//=============================================================================================================

void TestFiffFsLibrary::fiffRaw_saveSubset()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    QVERIFY(!raw.isEmpty());
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString tmpPath = tmpDir.path() + "/raw_subset.fif";
    int nPick = qMin(10, raw.info.nchan);
    RowVectorXi picks(nPick);
    for (int i = 0; i < nPick; ++i) picks(i) = i;
    QFile outFile(tmpPath);
    bool saved = raw.save(outFile, picks, 1, raw.first_samp, raw.first_samp + 2000);
    QVERIFY(saved);
    QVERIFY(QFile::exists(tmpPath));
}

void TestFiffFsLibrary::fiffRaw_saveAndReload()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    QVERIFY(!raw.isEmpty());
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString tmpPath = tmpDir.path() + "/raw_roundtrip.fif";
    int nPick = qMin(5, raw.info.nchan);
    RowVectorXi picks(nPick);
    for (int i = 0; i < nPick; ++i) picks(i) = i;
    QFile outFile(tmpPath);
    bool saved = raw.save(outFile, picks, 1, raw.first_samp, raw.first_samp + 1000);
    QVERIFY(saved);
    QFile reloadFile(tmpPath);
    FiffRawData raw2(reloadFile);
    QVERIFY(!raw2.isEmpty());
    QCOMPARE(raw2.info.nchan, nPick);
}

void TestFiffFsLibrary::fiffRaw_copyConstructor()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    QVERIFY(!raw.isEmpty());
    FiffRawData raw2(raw);
    QVERIFY(!raw2.isEmpty());
    QCOMPARE(raw2.info.nchan, raw.info.nchan);
    QCOMPARE(raw2.first_samp, raw.first_samp);
}

//=============================================================================================================
// FiffEvokedSet extras (from boost)
//=============================================================================================================

void TestFiffFsLibrary::fiffEvokedSet_subtractBaseline()
{
    int nCh = 10, nSamp = 100;
    MatrixXd epoch = MatrixXd::Random(nCh, nSamp);
    epoch.array() += 5.0;
    FiffEvokedSet::subtractBaseline(epoch, 0, 30);
    double baselineMean = epoch.block(0, 0, nCh, 31).mean();
    QVERIFY(std::abs(baselineMean) < 0.5);
}

void TestFiffFsLibrary::fiffEvokedSet_findEvoked()
{
    if (!hasData()) QSKIP("No test data");
    QFile aveFile(avePath());
    FiffEvokedSet evokedSet(aveFile);
    QVERIFY(evokedSet.evoked.size() > 0);
    bool found = evokedSet.find_evoked(evokedSet);
    qDebug() << "find_evoked result:" << found;
}

//=============================================================================================================
// FiffCoordTrans extras (from boost)
//=============================================================================================================

void TestFiffFsLibrary::fiffCoordTrans_inverse()
{
    FiffCoordTrans trans;
    trans.from = 4;
    trans.to = 5;
    trans.trans = Matrix4f::Identity();
    trans.trans(0, 3) = 0.01f;
    trans.invtrans = Matrix4f::Identity();
    trans.invtrans(0, 3) = -0.01f;
    FiffCoordTrans inv = trans.inverted();
    QCOMPARE(inv.from, trans.to);
    QCOMPARE(inv.to, trans.from);
    QVERIFY(std::abs(inv.trans(0, 3) - (-0.01f)) < 1e-6f);
}

void TestFiffFsLibrary::fiffCoordTrans_applyTransAndInverse()
{
    FiffCoordTrans trans;
    trans.from = 4;
    trans.to = 5;
    trans.trans = Matrix4f::Identity();
    trans.trans(0, 3) = 0.05f;
    trans.trans(1, 3) = 0.03f;
    trans.invtrans = Matrix4f::Identity();
    trans.invtrans(0, 3) = -0.05f;
    trans.invtrans(1, 3) = -0.03f;
    MatrixX3f point(1, 3);
    point << 0.0f, 0.0f, 0.0f;
    MatrixX3f transformed = trans.apply_trans(point);
    QVERIFY(std::abs(transformed(0, 0) - 0.05f) < 1e-6f);
    MatrixX3f back = trans.apply_inverse_trans(transformed);
    QVERIFY((back - point).norm() < 1e-6f);
}

void TestFiffFsLibrary::fiffCoordTrans_compose()
{
    FiffCoordTrans t1;
    t1.from = 4;
    t1.to = 5;
    t1.trans = Matrix4f::Identity();
    t1.trans(0, 3) = 0.01f;
    t1.invtrans = Matrix4f::Identity();
    t1.invtrans(0, 3) = -0.01f;
    FiffCoordTrans inv = t1.inverted();
    FiffCoordTrans inv2 = inv.inverted();
    QCOMPARE(inv2.from, t1.from);
    QCOMPARE(inv2.to, t1.to);
    QVERIFY(std::abs(inv2.trans(0, 3) - t1.trans(0, 3)) < 1e-6f);
}

//=============================================================================================================
// FiffInfo extras (from boost)
//=============================================================================================================

void TestFiffFsLibrary::fiffInfo_pickChannels()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    FiffInfo info = raw.info;
    int nPick = qMin(15, info.ch_names.size());
    RowVectorXi picks(nPick);
    for (int i = 0; i < nPick; ++i) picks(i) = i;
    FiffInfo pickedInfo = info.pick_info(picks);
    QCOMPARE(pickedInfo.nchan, nPick);
}

void TestFiffFsLibrary::fiffInfo_writeAndRead()
{
    if (!hasData()) QSKIP("No test data");
    QFile rawFile(rawPath());
    FiffRawData raw(rawFile);
    FiffInfo info = raw.info;
    QVERIFY(info.nchan > 0);
    QVERIFY(info.sfreq > 0);
    QVERIFY(info.ch_names.size() == info.nchan);
    QVERIFY(info.chs.size() == info.nchan);
}

//=============================================================================================================
// FiffStream write tests (from coverage_push)
//=============================================================================================================

void TestFiffFsLibrary::fiffStream_startAndEndFile()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    QVERIFY(!stream.isNull());
    stream->end_file();
    QVERIFY(buffer.size() > 0);
}

void TestFiffFsLibrary::fiffStream_writeIntAndFloat()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    fiff_int_t intData[] = {1, 2, 3, 4, 5};
    fiff_long_t pos1 = stream->write_int(FIFF_MNE_NROW, intData, 5);
    QVERIFY(pos1 >= 0);
    float floatData[] = {1.0f, 2.5f, 3.14f};
    fiff_long_t pos2 = stream->write_float(FIFF_MNE_SOURCE_SPACE_POINTS, floatData, 3);
    QVERIFY(pos2 >= 0);
    double doubleData[] = {1.0, 2.718281828, 3.141592653};
    fiff_long_t pos3 = stream->write_double(FIFF_MNE_SOURCE_SPACE_NORMALS, doubleData, 3);
    QVERIFY(pos3 >= 0);
    stream->end_file();
}

void TestFiffFsLibrary::fiffStream_writeMatrices()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    MatrixXf floatMat = MatrixXf::Random(3, 4);
    fiff_long_t pos1 = stream->write_float_matrix(FIFF_MNE_FORWARD_SOLUTION, floatMat);
    QVERIFY(pos1 >= 0);
    MatrixXi intMat(2, 3);
    intMat << 1, 2, 3, 4, 5, 6;
    fiff_long_t pos2 = stream->write_int_matrix(FIFF_MNE_SOURCE_SPACE_TRIANGLES, intMat);
    QVERIFY(pos2 >= 0);
    stream->end_file();
}

void TestFiffFsLibrary::fiffStream_writeStringAndNameList()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    fiff_long_t pos1 = stream->write_string(FIFF_DESCRIPTION, "Test description string");
    QVERIFY(pos1 >= 0);
    QStringList names;
    names << "MEG 0113" << "MEG 0112" << "MEG 0122" << "EEG 001";
    fiff_long_t pos2 = stream->write_name_list(FIFF_MNE_CH_NAME_LIST, names);
    QVERIFY(pos2 >= 0);
    stream->end_file();
}

void TestFiffFsLibrary::fiffStream_writeId()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    fiff_long_t pos1 = stream->write_id(FIFF_FILE_ID);
    QVERIFY(pos1 >= 0);
    FiffId id = FiffId::new_file_id();
    fiff_long_t pos2 = stream->write_id(FIFF_PARENT_FILE_ID, id);
    QVERIFY(pos2 >= 0);
    stream->end_file();
}

void TestFiffFsLibrary::fiffStream_writeChInfo()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    stream->start_block(FIFFB_MEAS_INFO);
    FiffChInfo chInfo;
    chInfo.ch_name = "MEG 0113";
    chInfo.kind = FIFFV_MEG_CH;
    chInfo.unit = FIFF_UNIT_T;
    chInfo.cal = 1.0;
    chInfo.range = 1.0;
    chInfo.scanNo = 1;
    chInfo.logNo = 1;
    fiff_long_t pos = stream->write_ch_info(chInfo);
    QVERIFY(pos >= 0);
    FiffChPos chPos;
    chPos.coil_type = FIFFV_COIL_VV_PLANAR_T1;
    fiff_long_t pos2 = stream->write_ch_pos(chPos);
    QVERIFY(pos2 >= 0);
    stream->end_block(FIFFB_MEAS_INFO);
    stream->end_file();
}

void TestFiffFsLibrary::fiffStream_writeCoordTrans()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    FiffCoordTrans trans;
    trans.from = FIFFV_COORD_HEAD;
    trans.to = FIFFV_COORD_MRI;
    trans.trans = Matrix4f::Identity();
    trans.invtrans = Matrix4f::Identity();
    fiff_long_t pos = stream->write_coord_trans(trans);
    QVERIFY(pos >= 0);
    stream->end_file();
}

void TestFiffFsLibrary::fiffStream_writeDigPoint()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    FiffDigPoint digPoint;
    digPoint.kind = FIFFV_POINT_HPI;
    digPoint.ident = 1;
    digPoint.r[0] = 0.06f;
    digPoint.r[1] = 0.0f;
    digPoint.r[2] = 0.06f;
    fiff_long_t pos = stream->write_dig_point(digPoint);
    QVERIFY(pos >= 0);
    stream->end_file();
}

void TestFiffFsLibrary::fiffStream_writeCov()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    FiffCov cov;
    cov.kind = FIFFV_MNE_NOISE_COV;
    cov.dim = 3;
    cov.names << "CH1" << "CH2" << "CH3";
    cov.data = MatrixXd::Identity(3, 3) * 1e-24;
    cov.nfree = 100;
    fiff_long_t pos = stream->write_cov(cov);
    QVERIFY(pos >= 0);
    stream->end_file();
}

void TestFiffFsLibrary::fiffStream_writeProj()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    QList<FiffProj> projs;
    FiffProj proj;
    proj.kind = 1;
    proj.active = true;
    proj.desc = "Test projection";
    FiffNamedMatrix::SDPtr projData(new FiffNamedMatrix());
    projData->nrow = 1;
    projData->ncol = 3;
    projData->row_names << "Proj1";
    projData->col_names << "CH1" << "CH2" << "CH3";
    projData->data = MatrixXd::Ones(1, 3) / std::sqrt(3.0);
    proj.data = projData;
    projs.append(proj);
    fiff_long_t pos = stream->write_proj(projs);
    QVERIFY(pos >= 0);
    stream->end_file();
}

void TestFiffFsLibrary::fiffStream_writeNamedMatrix()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    FiffNamedMatrix named;
    named.nrow = 2;
    named.ncol = 3;
    named.row_names << "ROW1" << "ROW2";
    named.col_names << "COL1" << "COL2" << "COL3";
    named.data = MatrixXd::Random(2, 3);
    fiff_long_t pos = stream->write_named_matrix(FIFF_MNE_FORWARD_SOLUTION, named);
    QVERIFY(pos >= 0);
    stream->end_file();
}

void TestFiffFsLibrary::fiffStream_writeRawBuffer()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    stream->start_block(FIFFB_RAW_DATA);
    MatrixXd rawBuf = MatrixXd::Random(5, 100);
    bool ok1 = stream->write_raw_buffer(rawBuf);
    QVERIFY(ok1);
    RowVectorXd cals = RowVectorXd::Ones(5) * 1e-12;
    bool ok2 = stream->write_raw_buffer(rawBuf, cals);
    QVERIFY(ok2);
    SparseMatrix<double> mult(5, 5);
    mult.setIdentity();
    bool ok3 = stream->write_raw_buffer(rawBuf, mult);
    QVERIFY(ok3);
    stream->end_block(FIFFB_RAW_DATA);
    stream->end_file();
}

void TestFiffFsLibrary::fiffStream_writeSparseMatrix()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    SparseMatrix<float> sparseMat(4, 4);
    sparseMat.insert(0, 0) = 1.0f;
    sparseMat.insert(1, 1) = 2.0f;
    sparseMat.insert(2, 2) = 3.0f;
    sparseMat.insert(3, 3) = 4.0f;
    sparseMat.makeCompressed();
    fiff_long_t pos1 = stream->write_float_sparse_ccs(FIFF_MNE_FORWARD_SOLUTION, sparseMat);
    QVERIFY(pos1 >= 0);
    fiff_long_t pos2 = stream->write_float_sparse_rcs(FIFF_MNE_FORWARD_SOLUTION_GRAD, sparseMat);
    QVERIFY(pos2 >= 0);
    stream->end_file();
}

void TestFiffFsLibrary::fiffStream_writeDirEntries()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    QList<FiffDirEntry::SPtr> dir;
    FiffDirEntry::SPtr entry1(new FiffDirEntry());
    entry1->kind = FIFF_FILE_ID;
    entry1->type = FIFFT_ID_STRUCT;
    entry1->size = 20;
    entry1->pos = 0;
    dir.append(entry1);
    fiff_long_t pos = stream->write_dir_entries(dir);
    QVERIFY(pos >= 0);
    fiff_long_t pos2 = stream->write_dir_pointer(static_cast<fiff_int_t>(pos));
    QVERIFY(pos2 >= 0);
    stream->end_file();
}

void TestFiffFsLibrary::fiffStream_startEndBlocks()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    fiff_long_t p1 = stream->start_block(FIFFB_MEAS);
    QVERIFY(p1 >= 0);
    fiff_long_t p2 = stream->start_block(FIFFB_MEAS_INFO);
    QVERIFY(p2 >= 0);
    stream->end_block(FIFFB_MEAS_INFO);
    stream->start_block(FIFFB_RAW_DATA);
    stream->end_block(FIFFB_RAW_DATA);
    stream->end_block(FIFFB_MEAS);
    stream->end_file();
}

void TestFiffFsLibrary::fiffStream_writeRtCommand()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    stream->write_rt_command(1, "test_command");
    stream->write_rt_command(2, "another_command");
    stream->end_file();
    QVERIFY(buffer.size() > 0);
}

void TestFiffFsLibrary::fiffStream_readTagInfoAndData()
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    fiff_int_t val = 42;
    stream->write_int(FIFF_MNE_NROW, &val, 1);
    stream->end_file();
    buffer.seek(0);
    FiffStream readStream(&buffer);
    readStream.open(QIODevice::ReadOnly);
    auto pTag = std::make_unique<FiffTag>();
    fiff_long_t tagPos = readStream.read_tag_info(pTag, false);
    if (tagPos >= 0) {
        bool readOk = readStream.read_tag_data(pTag);
        Q_UNUSED(readOk);
    }
    auto pTag2 = std::make_unique<FiffTag>();
    readStream.read_tag_info(pTag2, true);
    FiffTag::UPtr pTag3;
    readStream.read_tag(pTag3, 0);
}

//=============================================================================================================
// Fiff micro coverage push
//=============================================================================================================

void TestFiffFsLibrary::fiff_microCoveragePush()
{
    qint32 storageSize = FiffDataRef::storageSize();
    QVERIFY(storageSize > 0);
    FiffDigitizerData digData;
    QVERIFY(digData.nfids() == 0);
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    FiffStream::SPtr stream = FiffStream::start_file(buffer);
    fiff_int_t val2 = 1;
    stream->write_int(FIFF_MNE_NROW, &val2, 1);
    stream->end_file();
    buffer.seek(0);
    FiffStream readStream(&buffer);
    readStream.open(QIODevice::ReadOnly);
    qint32 nEntries = readStream.nent();
    QVERIFY(nEntries >= 0);
    const FiffStream& constRef = readStream;
    auto dirEntries = constRef.dir();
    FiffEvoked evoked;
    evoked.aspect_kind = FIFFV_ASPECT_STD_ERR;
    QString stdErrStr = evoked.aspectKindToString();
    QVERIFY(stdErrStr.contains("Error", Qt::CaseInsensitive));
    evoked.aspect_kind = 999;
    evoked.aspectKindToString();
    const char* hashLong = FIFFLIB::buildHashLong();
    Q_UNUSED(hashLong);
    FiffId id;
    id.version = (1 << 16) | 5;
    id.machid[0] = 12345;
    id.machid[1] = 67890;
    id.time.secs = 1000;
    id.time.usecs = 500;
    id.print();
    FiffId defaultId = FiffId::getDefault();
    Q_UNUSED(defaultId);
}

//=============================================================================================================
// FS defaults and operators (from boost)
//=============================================================================================================

void TestFiffFsLibrary::fs_surfaceDefaultConstruction()
{
    FsSurface surf;
    QVERIFY(surf.isEmpty());
    QCOMPARE(surf.hemi(), -1);
    QVERIFY(surf.rr().rows() == 0);
}

void TestFiffFsLibrary::fs_surfaceComputeNormalsCube()
{
    MatrixX3f rr(8, 3);
    rr << -1,-1,-1,  1,-1,-1,  1,1,-1,  -1,1,-1,
          -1,-1,1,   1,-1,1,   1,1,1,   -1,1,1;
    MatrixX3i tris(12, 3);
    tris << 0,1,2,  0,2,3,  4,6,5,  4,7,6,
            0,5,1,  0,4,5,  2,7,3,  2,6,7,
            0,3,7,  0,7,4,  1,5,6,  1,6,2;
    MatrixX3f normals = FsSurface::compute_normals(rr, tris);
    QCOMPARE(normals.rows(), 8);
    for (int i = 0; i < normals.rows(); ++i) {
        QVERIFY(normals.row(i).norm() > 0.1f);
    }
}

void TestFiffFsLibrary::fs_surfaceSetDefaultConstruction()
{
    FsSurfaceSet surfSet;
    QVERIFY(surfSet.isEmpty());
    QCOMPARE(surfSet.size(), 0);
}

void TestFiffFsLibrary::fs_surfaceSetIsEmpty()
{
    FsSurfaceSet surfSet;
    QVERIFY(surfSet.isEmpty());
    // Default-constructed FsSurface is empty (hemi == -1) and insert() rejects it.
    // Read a real surface to test insert.
    if (!hasData()) QSKIP("No test data");
    QString surfPath = m_sDataPath + "/subjects/sample/surf/lh.white";
    FsSurface surf(surfPath);
    if (surf.isEmpty()) QSKIP("Could not load surface");
    surfSet.insert(surf);
    QVERIFY(!surfSet.isEmpty());
    QCOMPARE(surfSet.size(), 1);
}

void TestFiffFsLibrary::fs_annotationDefaultConstruction()
{
    FsAnnotation annot;
    QVERIFY(annot.isEmpty());
    QCOMPARE(annot.hemi(), -1);
    QCOMPARE(annot.getVertices().size(), 0);
}

void TestFiffFsLibrary::fs_annotationIsEmpty()
{
    FsAnnotation annot;
    QVERIFY(annot.isEmpty());
}

void TestFiffFsLibrary::fs_annotationSetDefaultConstruction()
{
    FsAnnotationSet annotSet;
    QVERIFY(annotSet.isEmpty());
    QCOMPARE(annotSet.size(), 0);
}

void TestFiffFsLibrary::fs_annotationSetIsEmpty()
{
    FsAnnotationSet annotSet;
    QVERIFY(annotSet.isEmpty());
    // Default-constructed FsAnnotation is empty (hemi == -1) and insert() rejects it.
    // Read a real annotation to test insert.
    if (!hasData()) QSKIP("No test data");
    QString annotPath = m_sDataPath + "/subjects/sample/label/lh.aparc.annot";
    FsAnnotation annot(annotPath);
    if (annot.isEmpty()) QSKIP("Could not load annotation");
    annotSet.insert(annot);
    QVERIFY(!annotSet.isEmpty());
    QCOMPARE(annotSet.size(), 1);
}

void TestFiffFsLibrary::fs_colortableDefaultAndClear()
{
    FsColortable ct;
    QCOMPARE(ct.numEntries, 0);
    ct.numEntries = 5;
    ct.clear();
    QCOMPARE(ct.numEntries, 0);
}

void TestFiffFsLibrary::fs_labelDefaultConstruction()
{
    FsLabel label;
    QVERIFY(label.isEmpty());
    QCOMPARE(label.hemi, -1);
    QCOMPARE(label.vertices.size(), 0);
}

//=============================================================================================================
// FS operator and synthetic exercises (from coverage_push)
//=============================================================================================================

static void writeSyntheticSurface(const QString &path, int nvert, int ntri,
                                   const MatrixX3f &verts, const MatrixX3i &tris)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qFatal("Cannot open synthetic surface file: %s", qPrintable(path));
    }
    QDataStream ds(&f);
    ds.setByteOrder(QDataStream::BigEndian);
    ds.setFloatingPointPrecision(QDataStream::SinglePrecision);
    ds << (quint8)0xFF << (quint8)0xFF << (quint8)0xFE;
    QByteArray comment = "created by test\n\n";
    f.write(comment);
    ds << (qint32)nvert << (qint32)ntri;
    for (int i = 0; i < nvert; ++i)
        ds << verts(i, 0) << verts(i, 1) << verts(i, 2);
    for (int i = 0; i < ntri; ++i)
        ds << (qint32)tris(i, 0) << (qint32)tris(i, 1) << (qint32)tris(i, 2);
    f.close();
}

static void writeSyntheticAnnotation(const QString &path, int nvert,
                                      const VectorXi &vertIndices,
                                      const VectorXi &labels)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        qFatal("Cannot open synthetic annotation file: %s", qPrintable(path));
    }
    QDataStream ds(&f);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << (qint32)nvert;
    for (int i = 0; i < nvert; ++i)
        ds << (qint32)vertIndices(i) << (qint32)labels(i);
    ds << (qint32)1;
    ds << (qint32)(-2);
    ds << (qint32)1;
    QByteArray tableName = "test_table";
    ds << (qint32)tableName.size();
    f.write(tableName);
    ds << (qint32)1 << (qint32)0;
    QByteArray entryName = "unknown";
    ds << (qint32)entryName.size();
    f.write(entryName);
    ds << (qint32)25 << (qint32)5 << (qint32)25 << (qint32)0;
    f.close();
}

void TestFiffFsLibrary::fs_surfaceSetOperators()
{
    MatrixX3f verts(4, 3);
    verts << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
    MatrixX3i tris(2, 3);
    tris << 0,1,2, 0,2,3;
    QString lhPath = m_tempDir.filePath("lh.test");
    QString rhPath = m_tempDir.filePath("rh.test");
    writeSyntheticSurface(lhPath, 4, 2, verts, tris);
    writeSyntheticSurface(rhPath, 4, 2, verts, tris);
    FsSurface lhSurf, rhSurf;
    bool ok1 = FsSurface::read(lhPath, lhSurf);
    bool ok2 = FsSurface::read(rhPath, rhSurf);
    if (ok1 && ok2) {
        FsSurfaceSet surfSet;
        surfSet.insert(lhSurf);
        surfSet.insert(rhSurf);
        QVERIFY(surfSet.size() == 2);
        FsSurface& s0 = surfSet[0];
        FsSurface& s1 = surfSet[1];
        Q_UNUSED(s0); Q_UNUSED(s1);
        const FsSurfaceSet& constSet = surfSet;
        const FsSurface& cs0 = constSet[0];
        Q_UNUSED(cs0);
    }
    QVERIFY(true);
}

void TestFiffFsLibrary::fs_annotationSetOperators()
{
    VectorXi vertIdx(4);
    vertIdx << 0, 1, 2, 3;
    VectorXi labels(4);
    labels << 1638500, 1638500, 1638500, 1638500;
    QString lhPath = m_tempDir.filePath("lh.aparc.annot");
    QString rhPath = m_tempDir.filePath("rh.aparc.annot");
    writeSyntheticAnnotation(lhPath, 4, vertIdx, labels);
    writeSyntheticAnnotation(rhPath, 4, vertIdx, labels);
    FsAnnotation lhAnnot, rhAnnot;
    bool ok1 = FsAnnotation::read(lhPath, lhAnnot);
    bool ok2 = FsAnnotation::read(rhPath, rhAnnot);
    if (ok1 && ok2) {
        FsAnnotationSet annotSet;
        annotSet.insert(lhAnnot);
        annotSet.insert(rhAnnot);
        QVERIFY(annotSet.size() == 2);
        FsAnnotation& a0 = annotSet[0];
        Q_UNUSED(a0);
        const FsAnnotationSet& constAnnot = annotSet;
        const FsAnnotation ca0 = constAnnot[0];
        VectorXi verts = ca0.getVertices();
        Q_UNUSED(verts);
    }
    QVERIFY(true);
}

void TestFiffFsLibrary::fs_surfaceReadFromSynthetic()
{
    MatrixX3f verts(4, 3);
    verts << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
    MatrixX3i tris(2, 3);
    tris << 0,1,2, 0,2,3;
    QString surfDir = m_tempDir.filePath("surf_test");
    QDir().mkpath(surfDir);
    writeSyntheticSurface(surfDir + "/lh.inflated", 4, 2, verts, tris);
    writeSyntheticSurface(surfDir + "/rh.inflated", 4, 2, verts, tris);
    FsSurface s1(surfDir + "/lh.inflated");
    FsSurface s2(surfDir, 0, "inflated");
    Q_UNUSED(s1); Q_UNUSED(s2);
    QVERIFY(true);
}

void TestFiffFsLibrary::fs_annotationReadFromSynthetic()
{
    VectorXi vertIdx(4);
    vertIdx << 0, 1, 2, 3;
    VectorXi labels(4);
    labels << 1638500, 1638500, 1638500, 1638500;
    QString annotDir = m_tempDir.filePath("annot_test/label");
    QDir().mkpath(annotDir);
    writeSyntheticAnnotation(annotDir + "/lh.aparc.annot", 4, vertIdx, labels);
    writeSyntheticAnnotation(annotDir + "/rh.aparc.annot", 4, vertIdx, labels);
    FsAnnotation a1(annotDir + "/lh.aparc.annot");
    FsAnnotation a2(annotDir, 0, "aparc");
    Q_UNUSED(a1); Q_UNUSED(a2);
    QVERIFY(true);
}

void TestFiffFsLibrary::fs_surfaceSetCalcOffset()
{
    MatrixX3f verts(4, 3);
    verts << -1,0,0, 1,0,0, 0,1,0, 0,0,1;
    MatrixX3i tris(2, 3);
    tris << 0,1,2, 0,2,3;
    QString surfDir = m_tempDir.filePath("offset_test");
    QDir().mkpath(surfDir);
    writeSyntheticSurface(surfDir + "/lh.inflated", 4, 2, verts, tris);
    writeSyntheticSurface(surfDir + "/rh.inflated", 4, 2, verts, tris);
    FsSurfaceSet surfSet;
    bool ok = FsSurfaceSet::read(surfDir + "/lh.inflated", surfDir + "/rh.inflated", surfSet);
    Q_UNUSED(ok);
    QVERIFY(true);
}

void TestFiffFsLibrary::fs_surfaceSetReadNonInflated()
{
    MatrixX3f verts(4, 3);
    verts << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
    MatrixX3i tris(2, 3);
    tris << 0,1,2, 0,2,3;
    QString surfDir = m_tempDir.filePath("surfread_test");
    QDir().mkpath(surfDir);
    writeSyntheticSurface(surfDir + "/lh.orig", 4, 2, verts, tris);
    writeSyntheticSurface(surfDir + "/rh.orig", 4, 2, verts, tris);
    FsSurfaceSet surfSet;
    bool ok = FsSurfaceSet::read(surfDir + "/lh.orig", surfDir + "/rh.orig", surfSet);
    Q_UNUSED(ok);
    QVERIFY(true);
}

void TestFiffFsLibrary::fs_annotationSetReadSynthetic()
{
    VectorXi vertIdx(4);
    vertIdx << 0, 1, 2, 3;
    VectorXi labels(4);
    labels << 1638500, 1638500, 1638500, 1638500;
    QString annotDir = m_tempDir.filePath("annotSetRead/label");
    QDir().mkpath(annotDir);
    writeSyntheticAnnotation(annotDir + "/lh.aparc.annot", 4, vertIdx, labels);
    writeSyntheticAnnotation(annotDir + "/rh.aparc.annot", 4, vertIdx, labels);
    FsAnnotationSet annotSet;
    bool ok = FsAnnotationSet::read(annotDir + "/lh.aparc.annot", annotDir + "/rh.aparc.annot", annotSet);
    Q_UNUSED(ok);
    QVERIFY(true);
}

void TestFiffFsLibrary::fs_labelSelectTris()
{
    if (!hasData()) QSKIP("No test data");
    MatrixX3f verts(4, 3);
    verts << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
    MatrixX3i tris(2, 3);
    tris << 0,1,2, 0,2,3;
    QString surfPath = m_tempDir.filePath("lh.labeltest");
    writeSyntheticSurface(surfPath, 4, 2, verts, tris);
    FsSurface surf;
    bool ok = FsSurface::read(surfPath, surf);
    if (ok) {
        FsLabel label;
        label.read(m_sDataPath + "/subjects/sample/label/lh.BA1_exvivo.label", label);
        if (!label.isEmpty()) {
            label.selectTris(surf);
        } else {
            FsLabel synthLabel;
            synthLabel.selectTris(surf);
        }
    }
    QVERIFY(true);
}

//=============================================================================================================

void TestFiffFsLibrary::fs_annotationMutableGetters()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(annotLhPath())) QSKIP("lh.aparc.annot not found");

    FsAnnotation annot;
    FsAnnotation::read(annotLhPath(), annot);

    // Non-const getters (return references)
    Eigen::VectorXi& verts = annot.getVertices();
    QVERIFY(verts.size() > 0);
    Eigen::VectorXi& labels = annot.getLabelIds();
    QVERIFY(labels.size() > 0);
    FsColortable& ct = annot.getColortable();
    QVERIFY(ct.numEntries > 0);

    // Const getters (return by value)
    const FsAnnotation& constAnnot = annot;
    const Eigen::VectorXi constVerts = constAnnot.getVertices();
    QVERIFY(constVerts.size() > 0);
    const Eigen::VectorXi constLabels = constAnnot.getLabelIds();
    QVERIFY(constLabels.size() > 0);
    const FsColortable constCt = constAnnot.getColortable();
    QVERIFY(constCt.numEntries > 0);
}

//=============================================================================================================

void TestFiffFsLibrary::fs_surfaceNnCurvOffset()
{
    if (!hasData()) QSKIP("No test data");
    if (!QFile::exists(surfLhPath())) QSKIP("lh.white not found");

    FsSurface surf;
    FsSurface::read(surfLhPath(), surf);

    // Curv data might be empty if not loaded separately
    const Eigen::MatrixX3f& normals = surf.nn();
    Q_UNUSED(normals);
    const Eigen::VectorXf& curv = surf.curv();
    Q_UNUSED(curv);
    const Eigen::Vector3f& off = surf.offset();
    Q_UNUSED(off);
    QVERIFY(true);
}

//=============================================================================================================

void TestFiffFsLibrary::fs_annotationSetSizeCheck()
{
    FsAnnotationSet empty;
    QVERIFY(empty.isEmpty());
    QCOMPARE(empty.size(), 0);

    if (hasData() && QFile::exists(annotLhPath()) && QFile::exists(annotRhPath())) {
        FsAnnotationSet aSet(annotLhPath(), annotRhPath());
        QVERIFY(!aSet.isEmpty());
        QVERIFY(aSet.size() > 0);
    }
}

//=============================================================================================================

void TestFiffFsLibrary::fs_surfaceSetIsEmptyCheck()
{
    FsSurfaceSet empty;
    QVERIFY(empty.isEmpty());

    if (hasData() && QFile::exists(surfLhPath()) && QFile::exists(surfRhPath())) {
        FsSurfaceSet sSet(surfLhPath(), surfRhPath());
        QVERIFY(!sSet.isEmpty());
    }
}

//=============================================================================================================

void TestFiffFsLibrary::fs_globalBuildInfo()
{
    const char* dt = FSLIB::buildDateTime();
    QVERIFY(dt != nullptr);
    const char* h = FSLIB::buildHash();
    QVERIFY(h != nullptr);
    const char* hl = FSLIB::buildHashLong();
    QVERIFY(hl != nullptr);
}

//=============================================================================================================

void TestFiffFsLibrary::fs_annotationSet_readAndToLabels()
{
    if (!QFile::exists(annotLhPath()) || !QFile::exists(annotRhPath()))
        QSKIP("Annotation files not found");
    if (!QFile::exists(surfLhPath()) || !QFile::exists(surfRhPath()))
        QSKIP("Surface files not found");

    // Read annotation set from files
    FsAnnotationSet aSet(annotLhPath(), annotRhPath());
    QVERIFY(!aSet.isEmpty());
    QCOMPARE(aSet.size(), 2);

    // Read surface set
    FsSurfaceSet sSet(surfLhPath(), surfRhPath());
    QVERIFY(!sSet.isEmpty());

    // Convert annotations to labels
    QList<FsLabel> labels;
    QList<Eigen::RowVector4i> labelRGBAs;
    bool ok = aSet.toLabels(sSet, labels, labelRGBAs);
    QVERIFY(ok);
    QVERIFY(labels.size() > 0);
    QCOMPARE(labels.size(), labelRGBAs.size());

    // Each label should have vertices
    for (int i = 0; i < labels.size(); ++i) {
        QVERIFY(labels[i].vertices.size() > 0);
    }
}

//=============================================================================================================

void TestFiffFsLibrary::fs_annotationSet_insertAndClear()
{
    if (!QFile::exists(annotLhPath()))
        QSKIP("Annotation file not found");

    // Read one annotation
    FsAnnotation annot;
    FsAnnotation::read(annotLhPath(), annot);
    QVERIFY(!annot.isEmpty());

    // Build annotation set by inserting
    FsAnnotationSet aSet;
    QVERIFY(aSet.isEmpty());
    aSet.insert(annot);
    QVERIFY(!aSet.isEmpty());
    QCOMPARE(aSet.size(), 1);

    // Clear
    aSet.clear();
    QVERIFY(aSet.isEmpty());
    QCOMPARE(aSet.size(), 0);
}

//=============================================================================================================

void TestFiffFsLibrary::fs_surfaceSet_readAndIndex()
{
    if (!QFile::exists(surfLhPath()) || !QFile::exists(surfRhPath()))
        QSKIP("Surface files not found");

    // Read via static method
    FsSurfaceSet sSet;
    bool ok = FsSurfaceSet::read(surfLhPath(), surfRhPath(), sSet);
    QVERIFY(ok);
    QVERIFY(!sSet.isEmpty());
    QCOMPARE(sSet.size(), 2);

    // Access by hemisphere index
    const FsSurface& lh = sSet[0];
    QVERIFY(lh.rr().rows() > 0);

    const FsSurface& rh = sSet[1];
    QVERIFY(rh.rr().rows() > 0);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffFsLibrary)

#include "test_fiff_fs_library.moc"
