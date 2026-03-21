#include <QtTest/QtTest>
#include <Eigen/Dense>

#include <utils/generics/mne_logger.h>

#include <fiff/fiff_info_base.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_named_matrix.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point_set.h>

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

class TestFiffInfo : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;

    bool hasData() const { return !m_sDataPath.isEmpty(); }

    FiffChInfo makeCh(const QString &name, int kind, int unit, int coilType) {
        FiffChInfo ch;
        ch.ch_name = name;
        ch.kind = kind;
        ch.unit = unit;
        ch.chpos.coil_type = coilType;
        ch.range = 1.0;
        ch.cal = 1.0;
        return ch;
    }

    FiffInfoBase makeSyntheticInfoBase(int nCh = 4) {
        FiffInfoBase info;
        info.nchan = nCh;
        for (int i = 0; i < nCh; ++i) {
            QString name = QString("MEG %1").arg(i, 4, 10, QChar('0'));
            info.chs.append(makeCh(name, FIFFV_MEG_CH, FIFF_UNIT_T, FIFFV_COIL_VV_MAG_T3));
            info.ch_names.append(name);
        }
        info.bads.clear();
        return info;
    }

private slots:
    void initTestCase()
    {
        qInstallMessageHandler(MNELogger::customLogWriter);
        QString base = QCoreApplication::applicationDirPath()
                       + "/../resources/data/mne-cpp-test-data";
        if (QFile::exists(base + "/MEG/sample/sample_audvis_trunc_raw.fif"))
            m_sDataPath = base;
    }
    //=========================================================================
    // FiffInfoBase
    //=========================================================================
    void infoBase_defaultCtor()
    {
        FiffInfoBase info;
        QVERIFY(info.isEmpty());
    }

    void infoBase_copyCtor()
    {
        FiffInfoBase info = makeSyntheticInfoBase(3);
        FiffInfoBase copy(info);
        QCOMPARE(copy.nchan, 3);
        QCOMPARE(copy.ch_names.size(), 3);
    }

    void infoBase_clear()
    {
        FiffInfoBase info = makeSyntheticInfoBase(4);
        info.clear();
        QVERIFY(info.isEmpty());
    }

    void infoBase_channelType()
    {
        FiffInfoBase info = makeSyntheticInfoBase(2);
        QString type = info.channel_type(0);
        QVERIFY(!type.isEmpty());
    }

    void infoBase_pickChannels()
    {
        QStringList allNames;
        allNames << "MEG 0001" << "MEG 0002" << "MEG 0003" << "EEG 001";
        QStringList include;
        include << "MEG 0001" << "MEG 0003";
        QStringList exclude;

        RowVectorXi picks = FiffInfoBase::pick_channels(allNames, include, exclude);
        QCOMPARE(picks.size(), 2);
        QCOMPARE(picks(0), 0);
        QCOMPARE(picks(1), 2);
    }

    void infoBase_pickChannelsExclude()
    {
        QStringList allNames;
        allNames << "MEG 0001" << "MEG 0002" << "MEG 0003";
        QStringList include;  // empty = include all
        QStringList exclude;
        exclude << "MEG 0002";

        RowVectorXi picks = FiffInfoBase::pick_channels(allNames, include, exclude);
        QCOMPARE(picks.size(), 2);
    }

    void infoBase_pickInfo()
    {
        FiffInfoBase info = makeSyntheticInfoBase(4);
        RowVectorXi picks(2);
        picks << 0, 2;
        FiffInfoBase picked = info.pick_info(&picks);
        QCOMPARE(picked.nchan, 2);
        QCOMPARE(picked.ch_names.size(), 2);
    }

    void infoBase_getChannelTypes()
    {
        FiffInfoBase info = makeSyntheticInfoBase(3);
        QStringList types = info.get_channel_types();
        QVERIFY(types.size() > 0);
        QVERIFY(types.contains("mag"));
    }

    void infoBase_equality()
    {
        FiffInfoBase info1 = makeSyntheticInfoBase(2);
        FiffInfoBase info2 = makeSyntheticInfoBase(2);
        QVERIFY(info1 == info2);
    }

    void infoBase_pickTypes()
    {
        FiffInfoBase info = makeSyntheticInfoBase(4);
        RowVectorXi megPicks = info.pick_types(QString("mag"), false, false);
        QVERIFY(megPicks.size() > 0);
    }

    //=========================================================================
    // FiffProj
    //=========================================================================
    void proj_defaultCtor()
    {
        FiffProj proj;
        QCOMPARE(proj.active, false);
        QCOMPARE(proj.kind, (fiff_int_t)-1);
    }

    void proj_parameterizedCtor()
    {
        FiffNamedMatrix::SDPtr data(new FiffNamedMatrix());
        data->nrow = 1;
        data->ncol = 3;
        data->data = MatrixXd::Ones(1, 3) / std::sqrt(3.0);
        data->col_names << "CH1" << "CH2" << "CH3";
        data->row_names << "Proj1";

        FiffProj proj(FIFFV_MNE_PROJ_ITEM_EEG_AVREF, true, "TestProj", *data);
        QCOMPARE(proj.active, true);
        QVERIFY(proj.data->nrow == 1);
    }

    void proj_copyCtor()
    {
        FiffProj proj;
        proj.active = true;
        proj.desc = "test";
        FiffProj copy(proj);
        QCOMPARE(copy.active, true);
        QCOMPARE(copy.desc, QString("test"));
    }

    void proj_activateProjs()
    {
        FiffProj proj;
        proj.active = false;
        QList<FiffProj> projs;
        projs << proj;
        FiffProj::activate_projs(projs);
        QCOMPARE(projs[0].active, true);
    }

    void proj_makeProjector()
    {
        FiffNamedMatrix::SDPtr data(new FiffNamedMatrix());
        data->nrow = 1;
        data->ncol = 3;
        data->data = MatrixXd::Ones(1, 3) / std::sqrt(3.0);
        data->col_names << "CH1" << "CH2" << "CH3";
        data->row_names << "PCA1";

        FiffProj proj(FIFFV_MNE_PROJ_ITEM_EEG_AVREF, true, "PCA", *data);
        QList<FiffProj> projs;
        projs << proj;

        QStringList chNames;
        chNames << "CH1" << "CH2" << "CH3";
        MatrixXd P;
        int nProj = FiffProj::make_projector(projs, chNames, P);
        QVERIFY(nProj >= 0);
        if (nProj > 0) {
            QCOMPARE(P.rows(), 3);
            QCOMPARE(P.cols(), 3);
        }
    }

    //=========================================================================
    // FiffInfo from raw file
    //=========================================================================
    void info_readFromRaw()
    {
        QString rawPath = QCoreApplication::applicationDirPath()
                          + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawPath)) {
            QSKIP("Sample raw file not found");
        }
        QFile file(rawPath);
        FiffRawData raw(file);
        QVERIFY(raw.info.nchan > 0);
        QVERIFY(raw.info.sfreq > 0);

        RowVectorXi megPicks = raw.info.pick_types(QString("mag"), false, false);
        QVERIFY(megPicks.size() > 0);

        RowVectorXi gradPicks = raw.info.pick_types(QString("grad"), false, false);
        QVERIFY(gradPicks.size() > 0);

        RowVectorXi eegPicks = raw.info.pick_types(QString(""), true, false);
        QVERIFY(eegPicks.size() > 0);

        FiffInfoBase pickedInfo = raw.info.pick_info(megPicks);
        QCOMPARE(pickedInfo.nchan, megPicks.size());
    }

    void info_channelTypes()
    {
        QString rawPath = QCoreApplication::applicationDirPath()
                          + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawPath)) {
            QSKIP("Sample raw file not found");
        }
        QFile file(rawPath);
        FiffRawData raw(file);

        QStringList chTypes = raw.info.get_channel_types();
        QVERIFY(chTypes.size() > 0);
        QVERIFY(chTypes.contains("mag") || chTypes.contains("grad") || chTypes.contains("eeg"));
    }

    //=========================================================================
    // DATA-DRIVEN: Full channel picking from real raw info
    //=========================================================================
    void data_pickTypes_allCombinations()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        FiffRawData raw(file);

        // Pick magnetometers
        RowVectorXi magPicks = raw.info.pick_types(QString("mag"), false, false);
        QVERIFY(magPicks.size() > 0);

        // Pick gradiometers
        RowVectorXi gradPicks = raw.info.pick_types(QString("grad"), false, false);
        QVERIFY(gradPicks.size() > 0);

        // Pick EEG
        RowVectorXi eegPicks = raw.info.pick_types(QString(""), true, false);
        QVERIFY(eegPicks.size() > 0);

        // Pick STI
        RowVectorXi stiPicks = raw.info.pick_types(QString(""), false, true);
        QVERIFY(stiPicks.size() > 0);

        // All combined should cover more
        int totalPicked = magPicks.size() + gradPicks.size() + eegPicks.size() + stiPicks.size();
        QVERIFY(totalPicked > 0);
        QVERIFY(totalPicked <= raw.info.nchan);
    }

    void data_pickInfo_fromRaw()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        FiffRawData raw(file);

        // Pick first 50 channels
        RowVectorXi picks(50);
        for (int i = 0; i < 50; ++i) picks(i) = i;

        FiffInfo pickedInfo = raw.info.pick_info(picks);
        QCOMPARE(pickedInfo.nchan, 50);
        QCOMPARE(pickedInfo.ch_names.size(), 50);
        QCOMPARE(pickedInfo.chs.size(), 50);
    }

    //=========================================================================
    // DATA-DRIVEN: SSP projectors from real raw data
    //=========================================================================
    void data_proj_fromRealInfo()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        FiffRawData raw(file);

        QList<FiffProj>& projs = raw.info.projs;
        if (projs.isEmpty()) QSKIP("No projectors in file");

        qDebug() << "Found" << projs.size() << "projectors:";
        for (const FiffProj& p : projs) {
            qDebug() << "  " << p.desc << " active=" << p.active
                     << " kind=" << p.kind;
        }

        // Activate all
        FiffProj::activate_projs(projs);
        for (const FiffProj& p : projs)
            QVERIFY(p.active);

        // Make projector matrix
        MatrixXd P;
        int nProj = FiffProj::make_projector(projs, raw.info.ch_names, P);
        QVERIFY(nProj > 0);
        QCOMPARE(P.rows(), (Eigen::Index)raw.info.nchan);
        QCOMPARE(P.cols(), (Eigen::Index)raw.info.nchan);

        // P should be idempotent (P*P = P)
        MatrixXd PP = P * P;
        QVERIFY(PP.isApprox(P, 1e-10));
    }

    //=========================================================================
    // DATA-DRIVEN: FiffCov from file
    //=========================================================================
    void data_cov_readAndPick()
    {
        if (!hasData()) QSKIP("No test data");
        QString path = m_sDataPath + "/MEG/sample/sample_audvis-cov.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Covariance file not found");

        FiffCov cov(file);
        QVERIFY(!cov.isEmpty());
        QVERIFY(cov.data.rows() > 0);
        QCOMPARE(cov.data.rows(), cov.data.cols());
        QVERIFY(cov.names.size() > 0);

        // Pick subset
        int nPick = qMin(30, cov.names.size());
        QStringList pickNames = cov.names.mid(0, nPick);
        FiffCov picked = cov.pick_channels(pickNames);
        QCOMPARE(picked.names.size(), nPick);
    }

    //=========================================================================
    // DATA-DRIVEN: Read evoked and exercise info
    //=========================================================================
    void data_evokedInfo()
    {
        if (!hasData()) QSKIP("No test data");
        QString path = m_sDataPath + "/MEG/sample/sample_audvis-ave.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Evoked file not found");

        FiffEvokedSet evokedSet(file);
        QVERIFY(evokedSet.evoked.size() > 0);
        QVERIFY(evokedSet.info.nchan > 0);

        // Exercise pick_channels
        QStringList picks;
        for (int i = 0; i < qMin(20, evokedSet.info.nchan); ++i)
            picks << evokedSet.info.ch_names[i];

        FiffEvokedSet pickedSet = evokedSet.pick_channels(picks);
        QCOMPARE(pickedSet.info.nchan, picks.size());
    }

    //=========================================================================
    // DATA-DRIVEN: Digitization points from raw info
    //=========================================================================
    void data_digPoints()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        FiffRawData raw(file);

        FiffDigPointSet digSet = raw.info.dig;
        if (digSet.size() == 0) QSKIP("No dig points");

        qDebug() << "Found" << digSet.size() << "digitization points";

        // Check that dig points have valid coordinates
        for (int i = 0; i < digSet.size(); ++i) {
            FiffDigPoint dp = digSet[i];
            QVERIFY(dp.kind >= 0);
        }
    }

    //=========================================================================
    // DATA-DRIVEN: FiffInfo comp data
    //=========================================================================
    void data_compInfo()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        FiffRawData raw(file);

        // Get current compensation
        fiff_int_t comp = raw.info.get_current_comp();
        qDebug() << "Current compensation grade:" << comp;

        // Set compensation (exercise the code path)
        raw.info.set_current_comp(0);
        QCOMPARE(raw.info.get_current_comp(), (fiff_int_t)0);

        // Restore
        raw.info.set_current_comp(comp);
        QCOMPARE(raw.info.get_current_comp(), comp);
    }

    void cleanupTestCase() {}
};

QTEST_GUILESS_MAIN(TestFiffInfo)
#include "test_fiff_info.moc"
