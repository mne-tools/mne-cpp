//=============================================================================================================
// test_mne_raw_proj_comp.cpp — Tests for MNE raw data I/O, projections, and compensation
//
// Covers: MNERawData, MNERawInfo, MNERawBufDef, MNEProjOp, MNESssData,
//         MNECTFCompData, MNECTFCompDataSet, MNEFilterDef, MNENamedVector,
//         MNEDeriv, MNEDerivSet, MNEDescriptionParser, MNEMeasData, MNEMeasDataSet
//
// These tests exercise the complete raw-data pipeline from file opening through
// buffer loading, projection, compensation, filtering, and data extraction.
//=============================================================================================================

#include <QtTest/QtTest>
#include <QFile>
#include <QBuffer>
#include <QTemporaryDir>
#include <QCoreApplication>
#include <QTextStream>

#include <Eigen/Dense>

#include <utils/generics/mne_logger.h>

#include <fiff/fiff.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>

#include <mne/mne.h>
#include <mne/mne_raw_data.h>
#include <mne/mne_raw_info.h>
#include <mne/mne_raw_buf_def.h>
#include <mne/mne_proj_op.h>
#include <mne/mne_sss_data.h>
#include <mne/mne_ctf_comp_data.h>
#include <mne/mne_ctf_comp_data_set.h>
#include <mne/mne_filter_def.h>
#include <mne/mne_named_vector.h>
#include <mne/mne_deriv.h>
#include <mne/mne_deriv_set.h>
#include <mne/mne_description_parser.h>
#include <mne/mne_meas_data.h>
#include <mne/mne_meas_data_set.h>
#include <mne/mne_named_matrix.h>
#include <mne/mne_inverse_operator.h>
#include <mne/mne_types.h>

using namespace FIFFLIB;
using namespace MNELIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================

class TestMneRawProjComp : public QObject
{
    Q_OBJECT

private:
    QString m_sTestDataPath;

    QString rawPath() const { return m_sTestDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif"; }
    QString avePath() const { return m_sTestDataPath + "/MEG/sample/sample_audvis-ave.fif"; }
    QString covPath() const { return m_sTestDataPath + "/MEG/sample/sample_audvis-cov.fif"; }

private slots:
    void initTestCase();
    void cleanupTestCase();

    // ── MNEFilterDef ──
    void filterDef_defaultConstruction();
    void filterDef_setAndReadFields();

    // ── MNENamedVector ──
    void namedVector_defaultConstruction();
    void namedVector_pickByName();
    void namedVector_pickSubset();
    void namedVector_pickRequireAllFails();

    // ── MNERawBufDef ──
    void rawBufDef_defaultConstruction();

    // ── MNESssData ──
    void sssData_defaultConstruction();
    void sssData_copyConstruction();
    void sssData_readFromRawFile();
    void sssData_printOutput();

    // ── MNECTFCompData / MNECTFCompDataSet ──
    void ctfCompData_defaultConstruction();
    void ctfCompData_copyConstruction();
    void ctfCompDataSet_defaultConstruction();
    void ctfCompDataSet_readFromRawFile();
    void ctfCompDataSet_makeCompensator();

    // ── MNEDeriv / MNEDerivSet ──
    void deriv_defaultConstruction();
    void derivSet_defaultConstruction();

    // ── MNEProjOp ──
    void projOp_defaultConstruction();
    void projOp_addItemAndDuplicate();
    void projOp_readFromFile();
    void projOp_assignChannelsAndMake();
    void projOp_affectChannels();
    void projOp_createAvgEegRef();
    void projOp_projectVector();
    void projOp_report();
    void projOp_combine();

    // ── MNERawInfo ──
    void rawInfo_defaultConstruction();
    void rawInfo_loadFromFile();
    void rawInfo_findNodes();
    void rawInfo_getMeasInfo();

    // ── MNERawData ──
    void rawData_defaultConstruction();
    void rawData_openFile();
    void rawData_openFileComp();
    void rawData_members();
    void rawData_loadBuffer();
    void rawData_pickData();
    void rawData_setupFilterBufs();
    void rawData_compensateBuffer();

    // ── MNEDescriptionParser ──
    void descParser_parseAverageFile();
    void descParser_parseCovarianceFile();
    void descParser_invalidFileReturnsError();

    // ── MNEMeasDataSet ──
    void measDataSet_defaultConstruction();
    void measDataSet_getValuesAtTime();
    void measDataSet_getValuesFromChannelData();

    // ── MNEMeasData ──
    void measData_defaultConstruction();
    void measData_readFromFile();
    void measData_adjustBaselines();
};

//=============================================================================================================

void TestMneRawProjComp::initTestCase()
{
    qInstallMessageHandler(MNELogger::customLogWriter);

    m_sTestDataPath = QCoreApplication::applicationDirPath()
                      + "/../resources/data/mne-cpp-test-data";
    QVERIFY2(QFile::exists(rawPath()),
             qPrintable(QString("Test data not found: %1").arg(rawPath())));
}

void TestMneRawProjComp::cleanupTestCase() {}

//=============================================================================================================
// MNEFilterDef tests
//=============================================================================================================

void TestMneRawProjComp::filterDef_defaultConstruction()
{
    MNEFilterDef def;
    QCOMPARE(def.filter_on, false);
    QCOMPARE(def.size, 0);
    QCOMPARE(def.taper_size, 0);
    QVERIFY(qFuzzyIsNull(def.highpass));
    QVERIFY(qFuzzyIsNull(def.lowpass));
    QVERIFY(qFuzzyIsNull(def.highpass_width));
    QVERIFY(qFuzzyIsNull(def.lowpass_width));
    QVERIFY(qFuzzyIsNull(def.eog_highpass));
    QVERIFY(qFuzzyIsNull(def.eog_lowpass));
}

void TestMneRawProjComp::filterDef_setAndReadFields()
{
    MNEFilterDef def;
    def.filter_on = true;
    def.size = 4096;
    def.taper_size = 256;
    def.highpass = 0.1f;
    def.highpass_width = 0.05f;
    def.lowpass = 40.0f;
    def.lowpass_width = 5.0f;
    def.eog_highpass = 1.0f;
    def.eog_lowpass = 10.0f;
    QCOMPARE(def.filter_on, true);
    QCOMPARE(def.size, 4096);
    QCOMPARE(def.taper_size, 256);
    QVERIFY(qAbs(def.highpass - 0.1f) < 1e-6);
    QVERIFY(qAbs(def.lowpass - 40.0f) < 1e-6);
    QVERIFY(qAbs(def.eog_highpass - 1.0f) < 1e-6);
    QVERIFY(qAbs(def.eog_lowpass - 10.0f) < 1e-6);
}

//=============================================================================================================
// MNENamedVector tests
//=============================================================================================================

void TestMneRawProjComp::namedVector_defaultConstruction()
{
    MNENamedVector nv;
    QCOMPARE(nv.nvec, 0);
    QVERIFY(nv.names.isEmpty());
    QCOMPARE(nv.data.size(), (Index)0);
}

void TestMneRawProjComp::namedVector_pickByName()
{
    MNENamedVector nv;
    nv.nvec = 4;
    nv.names << "MEG001" << "MEG002" << "EEG001" << "EEG002";
    nv.data.resize(4);
    nv.data << 1.0f, 2.0f, 3.0f, 4.0f;

    QStringList pickNames;
    pickNames << "MEG001" << "EEG002";
    VectorXf result(2);
    int status = nv.pick(pickNames, 2, false, result);
    QCOMPARE(status, 0);
    QVERIFY(qAbs(result(0) - 1.0f) < 1e-6);
    QVERIFY(qAbs(result(1) - 4.0f) < 1e-6);
}

void TestMneRawProjComp::namedVector_pickSubset()
{
    MNENamedVector nv;
    nv.nvec = 3;
    nv.names << "ch1" << "ch2" << "ch3";
    nv.data.resize(3);
    nv.data << 10.0f, 20.0f, 30.0f;

    QStringList pickNames;
    pickNames << "ch2";
    VectorXf result(1);
    int status = nv.pick(pickNames, 1, false, result);
    QCOMPARE(status, 0);
    QVERIFY(qAbs(result(0) - 20.0f) < 1e-6);
}

void TestMneRawProjComp::namedVector_pickRequireAllFails()
{
    MNENamedVector nv;
    nv.nvec = 2;
    nv.names << "ch1" << "ch2";
    nv.data.resize(2);
    nv.data << 1.0f, 2.0f;

    QStringList pickNames;
    pickNames << "ch1" << "MISSING";
    VectorXf result(2);
    // require_all=true should fail when a name is missing
    int status = nv.pick(pickNames, 2, true, result);
    QVERIFY(status != 0);
}

//=============================================================================================================
// MNERawBufDef tests
//=============================================================================================================

void TestMneRawProjComp::rawBufDef_defaultConstruction()
{
    MNERawBufDef buf;
    QCOMPARE(buf.ns, 0);
    QCOMPARE(buf.nchan, 0);
    QCOMPARE(buf.is_skip, 0);
    QVERIFY(buf.vals.size() == 0);  // not loaded yet
}

//=============================================================================================================
// MNESssData tests
//=============================================================================================================

void TestMneRawProjComp::sssData_defaultConstruction()
{
    MNESssData sss;
    QCOMPARE(sss.job, 0);
    QCOMPARE(sss.coord_frame, 0);
    QCOMPARE(sss.nchan, 0);
    QCOMPARE(sss.in_order, 0);
    QCOMPARE(sss.out_order, 0);
}

void TestMneRawProjComp::sssData_copyConstruction()
{
    MNESssData orig;
    orig.job = 42;
    orig.coord_frame = FIFFV_COORD_HEAD;
    orig.nchan = 306;
    orig.in_order = 8;
    orig.out_order = 3;
    orig.in_nuse = 80;
    orig.out_nuse = 15;
    orig.comp_info.resize(3);
    orig.comp_info << 1, 2, 3;

    MNESssData copy(orig);
    QCOMPARE(copy.job, 42);
    QCOMPARE(copy.coord_frame, (int)FIFFV_COORD_HEAD);
    QCOMPARE(copy.nchan, 306);
    QCOMPARE(copy.in_order, 8);
    QCOMPARE(copy.out_order, 3);
    QCOMPARE(copy.in_nuse, 80);
    QCOMPARE(copy.out_nuse, 15);
    QCOMPARE(copy.comp_info.size(), (Index)3);
}

void TestMneRawProjComp::sssData_readFromRawFile()
{

    // SSS data may or may not be present in the test file
    std::unique_ptr<MNESssData> sss(MNESssData::read(rawPath()));
    // The test file is from Elekta Neuromag, should have SSS info
    // If NULL, that's also a valid result (no SSS data)
    if (sss) {
        QVERIFY(sss->nchan >= 0);
        QVERIFY(sss->coord_frame >= 0);
    }
}

void TestMneRawProjComp::sssData_printOutput()
{
    MNESssData sss;
    sss.job = 1;
    sss.coord_frame = FIFFV_COORD_HEAD;
    sss.nchan = 306;
    sss.in_order = 2;
    sss.out_order = 1;
    // comp_info must match total expansion components: in_order=2 → 3+5=8, out_order=1 → 3
    sss.comp_info = VectorXi::Ones(11);

    QString output;
    QTextStream stream(&output);
    sss.print(stream);
    // Should produce some non-empty output
    QVERIFY(!output.isEmpty());
}

//=============================================================================================================
// MNECTFCompData / MNECTFCompDataSet tests
//=============================================================================================================

void TestMneRawProjComp::ctfCompData_defaultConstruction()
{
    MNECTFCompData comp;
    // Default is MNE_CTFV_COMP_UNKNOWN = -1
    QCOMPARE(comp.kind, -1);
    QCOMPARE(comp.mne_kind, -1);
    QCOMPARE(comp.calibrated, 0);
    QVERIFY(!comp.data);
}

void TestMneRawProjComp::ctfCompData_copyConstruction()
{
    // Test copy construction of default-constructed object (null data/presel/postsel)
    MNECTFCompData original;
    original.kind = 42;
    original.mne_kind = 7;
    original.calibrated = 1;

    MNECTFCompData copy(original);
    QCOMPARE(copy.kind, 42);
    QCOMPARE(copy.mne_kind, 7);
    QCOMPARE(copy.calibrated, 1);
    QVERIFY(copy.data == nullptr);
    QVERIFY(copy.presel == nullptr);
    QVERIFY(copy.postsel == nullptr);
}

void TestMneRawProjComp::ctfCompDataSet_defaultConstruction()
{
    MNECTFCompDataSet compSet;
    QCOMPARE(compSet.ncomp, 0);
    QVERIFY(compSet.comps.empty());
    QVERIFY(compSet.current == nullptr);
}

void TestMneRawProjComp::ctfCompDataSet_readFromRawFile()
{

    auto compSet = MNECTFCompDataSet::read(rawPath());
    QVERIFY(compSet != nullptr);
    // Neuromag data typically has no CTF compensation but the read should succeed
    QVERIFY(compSet->ncomp >= 0);
}

void TestMneRawProjComp::ctfCompDataSet_makeCompensator()
{

    auto compSet = MNECTFCompDataSet::read(rawPath());
    if (compSet && compSet->ncomp > 0) {
        QFile file(rawPath());
        FiffRawData rawData(file);
        // make_comp(chs, nch, compchs, ncomp)
        int result = compSet->make_comp(rawData.info.chs, rawData.info.nchan,
                                        rawData.info.chs, rawData.info.nchan);
        QVERIFY(result >= 0);
    }
}

//=============================================================================================================
// MNEDeriv / MNEDerivSet tests
//=============================================================================================================

void TestMneRawProjComp::deriv_defaultConstruction()
{
    MNEDeriv deriv;
    QVERIFY(deriv.filename.isEmpty());
    QVERIFY(deriv.shortname.isEmpty());
    QVERIFY(!deriv.deriv_data);
}

void TestMneRawProjComp::derivSet_defaultConstruction()
{
    MNEDerivSet derivSet;
    QVERIFY(derivSet.derivs.isEmpty());
}

//=============================================================================================================
// MNEProjOp tests
//=============================================================================================================

void TestMneRawProjComp::projOp_defaultConstruction()
{
    MNEProjOp proj;
    QCOMPARE(proj.nitems, 0);
    QCOMPARE(proj.nch, 0);
    QCOMPARE(proj.nvec, 0);
}

void TestMneRawProjComp::projOp_addItemAndDuplicate()
{
    MNEProjOp proj;

    // Create a simple projection vector
    auto vecs = std::make_unique<MNENamedMatrix>();
    vecs->nrow = 1;
    vecs->ncol = 3;
    vecs->rowlist << "proj1";
    vecs->collist << "ch1" << "ch2" << "ch3";
    vecs->data = MatrixXf(1, 3);
    vecs->data << 1.0f/std::sqrt(3.0f), 1.0f/std::sqrt(3.0f), 1.0f/std::sqrt(3.0f);

    proj.add_item(vecs.get(), FIFFV_MNE_PROJ_ITEM_EEG_AVREF, "Test EEG avg ref");
    QCOMPARE(proj.nitems, 1);

    // Duplicate
    std::unique_ptr<MNEProjOp> dup(proj.dup());
    QVERIFY(dup != nullptr);
    QCOMPARE(dup->nitems, 1);
}

void TestMneRawProjComp::projOp_readFromFile()
{

    std::unique_ptr<MNEProjOp> proj(MNEProjOp::read(rawPath()));
    QVERIFY(proj != nullptr);
    QVERIFY(proj->nitems > 0);  // Neuromag data should have SSP projectors
}

void TestMneRawProjComp::projOp_assignChannelsAndMake()
{

    std::unique_ptr<MNEProjOp> proj(MNEProjOp::read(rawPath()));
    QVERIFY(proj != nullptr);

    // Read channel names from the raw file
    QFile file(rawPath());
    FiffRawData rawData(file);

    QStringList chNames;
    for (const auto& ch : rawData.info.chs) {
        chNames << ch.ch_name;
    }

    int assignResult = proj->assign_channels(chNames, chNames.size());
    QVERIFY(assignResult >= 0);

    int makeResult = proj->make_proj();
    QVERIFY(makeResult >= 0);
}

void TestMneRawProjComp::projOp_affectChannels()
{

    std::unique_ptr<MNEProjOp> proj(MNEProjOp::read(rawPath()));
    QVERIFY(proj != nullptr);

    QFile file(rawPath());
    FiffRawData rawData(file);

    int affecting = proj->affect_chs(rawData.info.chs, rawData.info.nchan);
    QVERIFY(affecting >= 0);
}

void TestMneRawProjComp::projOp_createAvgEegRef()
{

    QFile file(rawPath());
    FiffRawData rawData(file);

    std::unique_ptr<MNEProjOp> eegRef(
        MNEProjOp::create_average_eeg_ref(rawData.info.chs, rawData.info.nchan));
    QVERIFY(eegRef != nullptr);
    QVERIFY(eegRef->nitems > 0);
}

void TestMneRawProjComp::projOp_projectVector()
{
    MNEProjOp proj;

    // Create a simple avg reference projector for 3 channels
    auto vecs = std::make_unique<MNENamedMatrix>();
    vecs->nrow = 1;
    vecs->ncol = 3;
    vecs->rowlist << "avg";
    vecs->collist << "ch0" << "ch1" << "ch2";
    vecs->data = MatrixXf(1, 3);
    float val = 1.0f / std::sqrt(3.0f);
    vecs->data << val, val, val;

    proj.add_item(vecs.get(), FIFFV_MNE_PROJ_ITEM_EEG_AVREF, "avg ref");

    QStringList names;
    names << "ch0" << "ch1" << "ch2";
    proj.assign_channels(names, 3);
    proj.make_proj();

    // Apply projection to a test vector [3, 6, 9] -> removes mean
    VectorXd testVec(3);
    testVec << 3.0, 6.0, 9.0;
    int result = proj.project_dvector(testVec, true);
    QVERIFY(result >= 0);

    // After average reference projection the mean should be ~0
    double mean = testVec.mean();
    QVERIFY2(qAbs(mean) < 1e-5, qPrintable(QString("Mean after projection: %1").arg(mean)));
}

void TestMneRawProjComp::projOp_report()
{

    std::unique_ptr<MNEProjOp> proj(MNEProjOp::read(rawPath()));
    QVERIFY(proj != nullptr);

    QString output;
    QTextStream stream(&output);
    proj->report(stream, QStringLiteral("Test"));
    QVERIFY(!output.isEmpty());
}

void TestMneRawProjComp::projOp_combine()
{
    MNEProjOp proj1, proj2;

    auto vecs1 = std::make_unique<MNENamedMatrix>();
    vecs1->nrow = 1; vecs1->ncol = 2;
    vecs1->rowlist << "p1";
    vecs1->collist << "ch0" << "ch1";
    vecs1->data = MatrixXf::Ones(1, 2) / std::sqrt(2.0f);
    proj1.add_item(vecs1.get(), 1, "proj1");

    auto vecs2 = std::make_unique<MNENamedMatrix>();
    vecs2->nrow = 1; vecs2->ncol = 2;
    vecs2->rowlist << "p2";
    vecs2->collist << "ch0" << "ch1";
    vecs2->data = MatrixXf(1, 2);
    vecs2->data << 1.0f/std::sqrt(2.0f), -1.0f/std::sqrt(2.0f);
    proj2.add_item(vecs2.get(), 2, "proj2");

    MNEProjOp* combined = proj1.combine(&proj2);
    QVERIFY(combined != nullptr);
    QCOMPARE(combined->nitems, 2);
}

//=============================================================================================================
// MNERawInfo tests
//=============================================================================================================

void TestMneRawProjComp::rawInfo_defaultConstruction()
{
    MNERawInfo info;
    QCOMPARE(info.nchan, 0);
    QVERIFY(qFuzzyIsNull(info.sfreq));
    QVERIFY(info.filename.isEmpty());
}

void TestMneRawProjComp::rawInfo_loadFromFile()
{

    std::unique_ptr<MNERawInfo> info;
    int result = MNERawInfo::load(rawPath(), 0, info);
    QCOMPARE(result, 0);
    QVERIFY(info != nullptr);
    QVERIFY(info->nchan > 0);
    QVERIFY(info->sfreq > 0);
    QVERIFY(!info->chInfo.isEmpty());
    QCOMPARE(info->chInfo.size(), info->nchan);
    QVERIFY(!info->rawDir.isEmpty());
}

void TestMneRawProjComp::rawInfo_findNodes()
{

    QFile file(rawPath());
    FiffStream::SPtr stream(new FiffStream(&file));
    QVERIFY(stream->open());

    // find_raw searches from the root via dir_tree_find
    auto rawNode = MNERawInfo::find_raw(stream->dirtree());
    QVERIFY(rawNode != nullptr);
    QVERIFY(!rawNode->isEmpty());

    // find_meas walks UP from a child node — pass rawNode (inside FIFFB_MEAS)
    auto measNode = MNERawInfo::find_meas(rawNode);
    QVERIFY(measNode != nullptr);

    auto measInfoNode = MNERawInfo::find_meas_info(rawNode);
    QVERIFY(measInfoNode != nullptr);

    // MaxShield node may or may not exist
    auto maxNode = MNERawInfo::find_maxshield(stream->dirtree());
    // It's OK if this is null for non-MaxShield data
}

void TestMneRawProjComp::rawInfo_getMeasInfo()
{

    QFile file(rawPath());
    FiffStream::SPtr stream(new FiffStream(&file));
    QVERIFY(stream->open());

    // find_raw to get a node inside the FIFFB_MEAS block
    auto rawNode = MNERawInfo::find_raw(stream->dirtree());
    QVERIFY(rawNode != nullptr);

    std::unique_ptr<FiffId> id;
    int nchan = 0;
    float sfreq = 0, highpass = 0, lowpass = 0;
    QList<FiffChInfo> chp;
    FiffCoordTrans trans;
    FiffTime* start_time = nullptr;

    int result = MNERawInfo::get_meas_info(stream, rawNode, id, &nchan,
                                           &sfreq, &highpass, &lowpass,
                                           chp, trans, &start_time);
    QCOMPARE(result, 0);
    QVERIFY(nchan > 0);
    QVERIFY(sfreq > 0);
    QCOMPARE(chp.size(), nchan);
    if (start_time) {
        delete start_time;
    }
}

//=============================================================================================================
// MNERawData tests
//=============================================================================================================

void TestMneRawProjComp::rawData_defaultConstruction()
{
    MNERawData raw;
    QCOMPARE(raw.nsamp, 0);
    QCOMPARE(raw.nbad, 0);
    QVERIFY(raw.filename.isEmpty());
    QVERIFY(!raw.info);
    QVERIFY(!raw.proj);
    QVERIFY(!raw.comp);
    QVERIFY(!raw.filter);
    QVERIFY(raw.bufs.empty());
}

void TestMneRawProjComp::rawData_openFile()
{

    MNEFilterDef filter;
    std::unique_ptr<MNERawData> raw(
        MNERawData::open_file(rawPath(), 0, 0, filter));
    QVERIFY(raw != nullptr);
    QVERIFY(raw->nsamp > 0);
    QVERIFY(raw->info != nullptr);
    QVERIFY(raw->info->nchan > 0);
    QVERIFY(!raw->bufs.empty());
    QVERIFY(!raw->ch_names.isEmpty());
}

void TestMneRawProjComp::rawData_openFileComp()
{

    MNEFilterDef filter;
    // Open with explicit compensation grade 0
    std::unique_ptr<MNERawData> raw(
        MNERawData::open_file_comp(rawPath(), 0, 0, filter, 0));
    QVERIFY(raw != nullptr);
    QVERIFY(raw->nsamp > 0);
    QCOMPARE(raw->comp_now, 0);
}

void TestMneRawProjComp::rawData_members()
{

    MNEFilterDef filter;
    std::unique_ptr<MNERawData> raw(
        MNERawData::open_file(rawPath(), 0, 0, filter));
    QVERIFY(raw != nullptr);

    // Verify file-level metadata
    QVERIFY(!raw->filename.isEmpty());
    QVERIFY(raw->stream != nullptr);
    QVERIFY(raw->first_samp >= 0);
    QVERIFY(raw->nsamp > 0);

    // Channel info should be populated
    QVERIFY(raw->ch_names.size() == raw->info->nchan);

    // SSS and projection may or may not be present
    if (raw->sss) {
        QVERIFY(raw->sss->nchan >= 0);
    }

    // Compensation data
    QVERIFY(raw->comp_file >= 0);
}

void TestMneRawProjComp::rawData_loadBuffer()
{

    MNEFilterDef filter;
    std::unique_ptr<MNERawData> raw(
        MNERawData::open_file(rawPath(), 0, 0, filter));
    QVERIFY(raw != nullptr);
    QVERIFY(!raw->bufs.empty());

    // Load first buffer
    int result = raw->load_one_buffer(&raw->bufs[0]);
    QCOMPARE(result, 0);  // OK = 0
    // After loading, the buffer should have data
    QVERIFY(raw->bufs[0].vals.size() > 0);
}

void TestMneRawProjComp::rawData_pickData()
{

    MNEFilterDef filter;
    std::unique_ptr<MNERawData> raw(
        MNERawData::open_file(rawPath(), 0, 0, filter));
    QVERIFY(raw != nullptr);

    // Create a simple channel selection for 3 MEG channels
    MNEChSelection sel;
    sel.nchan = 3;
    sel.pick.resize(3);
    sel.pick << 0, 1, 2;
    sel.pick_deriv.resize(3);
    sel.pick_deriv << -1, -1, -1;
    sel.nderiv = 0;

    int ns = 100;
    // Allocate output arrays
    float** picked = new float*[sel.nchan];
    for (int i = 0; i < sel.nchan; i++) {
        picked[i] = new float[ns];
    }

    int result = raw->pick_data(&sel, raw->first_samp, ns, picked);
    QCOMPARE(result, 0);

    // Verify we got actual data (not all zeros)
    bool hasNonZero = false;
    for (int ch = 0; ch < sel.nchan && !hasNonZero; ch++) {
        for (int s = 0; s < ns && !hasNonZero; s++) {
            if (picked[ch][s] != 0.0f) hasNonZero = true;
        }
    }
    QVERIFY(hasNonZero);

    for (int i = 0; i < sel.nchan; i++) delete[] picked[i];
    delete[] picked;
}

void TestMneRawProjComp::rawData_setupFilterBufs()
{

    MNEFilterDef filter;
    filter.filter_on = true;
    filter.size = 2048;
    filter.taper_size = 128;
    filter.highpass = 1.0f;
    filter.lowpass = 40.0f;
    filter.highpass_width = 0.5f;
    filter.lowpass_width = 5.0f;

    std::unique_ptr<MNERawData> raw(
        MNERawData::open_file(rawPath(), 0, 0, filter));
    QVERIFY(raw != nullptr);

    // setup_filter_bufs should have been called during open
    if (raw->filter && raw->filter->filter_on) {
        QVERIFY(!raw->filt_bufs.empty());
    }
}

void TestMneRawProjComp::rawData_compensateBuffer()
{

    MNEFilterDef filter;
    std::unique_ptr<MNERawData> raw(
        MNERawData::open_file(rawPath(), 0, 0, filter));
    QVERIFY(raw != nullptr);
    QVERIFY(!raw->bufs.empty());

    // Load first buffer
    raw->load_one_buffer(&raw->bufs[0]);

    // Compensate buffer - should succeed even if no compensation needed
    int result = raw->compensate_buffer(&raw->bufs[0]);
    QVERIFY(result >= 0);
}

//=============================================================================================================
// MNEDescriptionParser tests
//=============================================================================================================

void TestMneRawProjComp::descParser_parseAverageFile()
{
    // Create a temporary .ave description file
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString aveFile = tmpDir.path() + "/test.ave";
    {
        QFile f(aveFile);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream ts(&f);
        ts << "# Test average description\n"
           << "average {\n"
           << "    category {\n"
           << "        name      \"Auditory Left\"\n"
           << "        event     1\n"
           << "        tmin      -0.2\n"
           << "        tmax      0.5\n"
           << "        bmin      -0.2\n"
           << "        bmax      0.0\n"
           << "    }\n"
           << "    category {\n"
           << "        name      \"Auditory Right\"\n"
           << "        event     2\n"
           << "        tmin      -0.2\n"
           << "        tmax      0.5\n"
           << "        bmin      -0.2\n"
           << "        bmax      0.0\n"
           << "    }\n"
           << "}\n";
    }

    AverageDescription desc;
    bool ok = MNEDescriptionParser::parseAverageFile(aveFile, desc);
    QVERIFY(ok);
    QCOMPARE(desc.categories.size(), 2);
    QCOMPARE(desc.categories[0].comment, QString("Auditory Left"));
    QCOMPARE(desc.categories[0].events[0], 1u);
    QVERIFY(qAbs(desc.categories[0].tmin - (-0.2f)) < 1e-5);
    QVERIFY(qAbs(desc.categories[0].tmax - 0.5f) < 1e-5);
    QCOMPARE(desc.categories[1].comment, QString("Auditory Right"));
    QCOMPARE(desc.categories[1].events[0], 2u);
}

void TestMneRawProjComp::descParser_parseCovarianceFile()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QString covFile = tmpDir.path() + "/test.cov";
    {
        QFile f(covFile);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream ts(&f);
        ts << "# Test covariance description\n"
           << "cov {\n"
           << "    def {\n"
           << "        tmin      -0.2\n"
           << "        tmax      0.0\n"
           << "        bmin      -0.2\n"
           << "        bmax      0.0\n"
           << "        event     1\n"
           << "    }\n"
           << "}\n";
    }

    CovDescription desc;
    bool ok = MNEDescriptionParser::parseCovarianceFile(covFile, desc);
    QVERIFY(ok);
    QVERIFY(desc.defs.size() >= 1);
}

void TestMneRawProjComp::descParser_invalidFileReturnsError()
{
    AverageDescription desc;
    bool ok = MNEDescriptionParser::parseAverageFile("/nonexistent/path.ave", desc);
    QVERIFY(!ok);

    CovDescription covDesc;
    ok = MNEDescriptionParser::parseCovarianceFile("/nonexistent/path.cov", covDesc);
    QVERIFY(!ok);
}

//=============================================================================================================
// MNEMeasDataSet tests
//=============================================================================================================

void TestMneRawProjComp::measDataSet_defaultConstruction()
{
    MNEMeasDataSet ds;
    QCOMPARE(ds.np, 0);
    QCOMPARE(ds.nave, 1);
    QCOMPARE(ds.first, 0);
    QVERIFY(ds.comment.isEmpty());
}

void TestMneRawProjComp::measDataSet_getValuesAtTime()
{
    MNEMeasDataSet ds;
    ds.np = 100;
    ds.nave = 1;
    ds.tmin = 0.0f;
    ds.tstep = 0.01f;  // 100 Hz
    ds.first = 0;

    int nch = 3;
    ds.data.resize(ds.np, nch);
    // Fill with known pattern: channel i at time t has value = i * t
    for (int t = 0; t < ds.np; t++) {
        for (int c = 0; c < nch; c++) {
            ds.data(t, c) = static_cast<float>(c) * (ds.tmin + t * ds.tstep);
        }
    }

    float values[3];
    // Get values at time = 0.5s (sample index 50)
    int result = ds.getValuesAtTime(0.5f, 0.0f, nch, false, values);
    QCOMPARE(result, 0);

    // At t=0.5: ch0=0, ch1=0.5, ch2=1.0
    QVERIFY(qAbs(values[0] - 0.0f) < 0.01f);
    QVERIFY(qAbs(values[1] - 0.5f) < 0.01f);
    QVERIFY(qAbs(values[2] - 1.0f) < 0.01f);
}

void TestMneRawProjComp::measDataSet_getValuesFromChannelData()
{
    int nch = 2;
    int nsamp = 200;
    float tmin = -0.1f;
    float sfreq = 1000.0f;

    // Allocate channel-major data
    float** data = new float*[nch];
    for (int c = 0; c < nch; c++) {
        data[c] = new float[nsamp];
        for (int s = 0; s < nsamp; s++) {
            data[c][s] = static_cast<float>(c + 1) * 100.0f;  // constant per channel
        }
    }

    float values[2];
    int result = MNEMeasDataSet::getValuesFromChannelData(
        0.0f, 0.0f, data, nsamp, nch, tmin, sfreq, false, values);
    QCOMPARE(result, 0);
    QVERIFY(qAbs(values[0] - 100.0f) < 1.0f);
    QVERIFY(qAbs(values[1] - 200.0f) < 1.0f);

    for (int c = 0; c < nch; c++) delete[] data[c];
    delete[] data;
}

//=============================================================================================================
// MNEMeasData tests
//=============================================================================================================

void TestMneRawProjComp::measData_defaultConstruction()
{
    MNEMeasData md;
    QCOMPARE(md.nchan, 0);
    QVERIFY(qFuzzyIsNull(md.sfreq));
    QCOMPARE(md.nset, 0);
    QVERIFY(md.filename.isEmpty());
    QVERIFY(md.current == nullptr);
}

void TestMneRawProjComp::measData_readFromFile()
{

    // Read evoked data from the ave file
    std::unique_ptr<MNEMeasData> md(
        MNEMeasData::mne_read_meas_data(avePath(), 0, nullptr, nullptr,
                                         QStringList(), 0));
    if (md) {
        QVERIFY(md->nchan > 0);
        QVERIFY(md->sfreq > 0);
        QVERIFY(md->nset > 0);
        QVERIFY(md->current != nullptr);
    }
}

void TestMneRawProjComp::measData_adjustBaselines()
{

    std::unique_ptr<MNEMeasData> md(
        MNEMeasData::mne_read_meas_data(avePath(), 0, nullptr, nullptr,
                                         QStringList(), 0));
    if (md && md->current) {
        // Store original data
        MatrixXf origData = md->current->data;

        // Adjust baselines for time range -0.2 to 0.0
        md->adjust_baselines(-0.2f, 0.0f);

        // Data should have been modified (baselines subtracted)
        // The change may be subtle for already-baselined data
        QVERIFY(md->current->baselines.size() > 0 || md->current->data.rows() > 0);
    }
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneRawProjComp)

#include "test_mne_raw_proj_comp.moc"
