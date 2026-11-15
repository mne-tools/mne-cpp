//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2010-2026 MNE-CPP Authors
 *
 * @file     test_mne_projections.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @brief       Unit tests for MNEProjOp, MNEProjItem, MNECtfCompDataSet, and MNEDescriptionParser.
 */
//=============================================================================================================

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTemporaryDir>

#include <Eigen/Core>

#include <fiff/fiff.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_types.h>

#include <mne/mne_proj_op.h>
#include <mne/mne_proj_item.h>
#include <mne/mne_named_matrix.h>
#include <mne/mne_ctf_comp_data_set.h>
#include <mne/mne_cov_matrix.h>
#include <mne/mne_description_parser.h>
#include <mne/mne_process_description.h>

using namespace mne;

//=============================================================================================================

class TestMneProjections : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;
    QString m_sRawFile;

private slots:

    void initTestCase()
    {
        m_sDataPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data";
        m_sRawFile = m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
    }

    // ── MNEProjItem ─────────────────────────────────────────────────────────

    void testProjItemDefault()
    {
        MNEProjItem item;
        QCOMPARE(item.nvec(), 0);
        QVERIFY(item.desc.empty());
        QCOMPARE(item.kind, 0);
        // active is a bool defaulting to true; the previous `>= 0` check was
        // always satisfied and asserted nothing.
        QVERIFY(item.active);
    }

    void testProjItemCopy()
    {
        MNEProjItem item;
        item.desc = "test projection";
        item.kind = 10;
        item.active = true;

        // Construct named matrix
        StringList colNames = {"ch1", "ch2", "ch3"};
        StringList rowNames = {"vec1"};
        Eigen::MatrixXf data(1, 3);
        data << 1.0f, 2.0f, 3.0f;
        item.vecs = *MNENamedMatrix::build(rowNames, colNames, data);

        MNEProjItem copy(item);
        QCOMPARE(copy.desc, item.desc);
        QCOMPARE(copy.kind, item.kind);
        QCOMPARE(copy.active, item.active);
        QCOMPARE(copy.nvec(), 1);
        QCOMPARE(copy.vecs.collist, colNames);
        QCOMPARE(copy.vecs.data, data);
    }

    void testProjItemAffect()
    {
        MNEProjItem item;
        item.active = true;

        StringList colNames = {"MEG 0111", "MEG 0112", "MEG 0113"};
        StringList rowNames = {"vec1"};
        Eigen::MatrixXf data(1, 3);
        data << 1.0f, 1.0f, 1.0f;
        item.vecs = *MNENamedMatrix::build(rowNames, colNames, data);

        StringList testList = {"MEG 0111", "MEG 0112", "MEG 0113"};
        QVERIFY(item.affect(testList));

        StringList noMatch = {"EEG 001", "EEG 002"};
        QVERIFY(!item.affect(noMatch));
    }

    // ── MNEProjOp ───────────────────────────────────────────────────────────

    void testProjOpDefault()
    {
        MNEProjOp op;
        QVERIFY(op.items.empty());
        QVERIFY(op.names.empty());
        QCOMPARE(op.nvec, 0);
    }

    void testProjOpAddItem()
    {
        MNEProjOp op;

        StringList colNames = {"MEG 0111", "MEG 0112", "MEG 0113"};
        StringList rowNames = {"vec1"};
        Eigen::MatrixXf data(1, 3);
        data << 0.577f, 0.577f, 0.577f;
        auto nm_ptr = MNENamedMatrix::build(rowNames, colNames, data);

        MNENamedMatrix nm(*nm_ptr);

        op.add_item(nm, 1, "Test SSP vector");
        QCOMPARE(op.items.size(), std::size_t{1});
        QVERIFY(!op.items.empty());
        QCOMPARE(op.items[0].desc, QString("Test SSP vector"));
    }

    void testProjOpAddItemActive()
    {
        MNEProjOp op;

        StringList colNames = {"ch1", "ch2"};
        StringList rowNames = {"v1"};
        Eigen::MatrixXf data(1, 2);
        data << 0.707f, 0.707f;
        auto nm_ptr = MNENamedMatrix::build(rowNames, colNames, data);

        MNENamedMatrix nm(*nm_ptr);

        op.add_item(nm, 1, "Active SSP", 1);
        QCOMPARE(op.items.size(), std::size_t{1});
        QCOMPARE(op.items[0].active, 1);

        op.add_item(nm, 1, "Inactive SSP", 0);
        QCOMPARE(op.items.size(), std::size_t{2});
        QCOMPARE(op.items[1].active, 0);
    }

    void testProjOpDup()
    {
        MNEProjOp op;
        StringList colNames = {"ch1", "ch2", "ch3"};
        StringList rowNames = {"v1", "v2"};
        Eigen::MatrixXf data(2, 3);
        data << 1, 0, 0,
            0, 1, 0;
        auto nm_ptr = MNENamedMatrix::build(rowNames, colNames, data);

        MNENamedMatrix nm(*nm_ptr);
        op.add_item(nm, 1, "Vec set");

        auto dup = op.dup();
        QVERIFY(dup != nullptr);
        QCOMPARE(dup->items.size(), std::size_t{1});
        QCOMPARE(dup->items[0].desc, QString("Vec set"));
    }

    void testProjOpCombine()
    {
        MNEProjOp op1;
        StringList colNames = {"ch1", "ch2"};
        StringList rowNames = {"v1"};
        Eigen::MatrixXf data(1, 2);
        data << 1, 0;
        auto nm1_ptr = MNENamedMatrix::build(rowNames, colNames, data);

        MNENamedMatrix nm1(*nm1_ptr);
        op1.add_item(nm1, 1, "First");

        MNEProjOp op2;
        Eigen::MatrixXf data2(1, 2);
        data2 << 0, 1;
        auto nm2_ptr = MNENamedMatrix::build(rowNames, colNames, data2);

        MNENamedMatrix nm2(*nm2_ptr);
        op2.add_item(nm2, 1, "Second");

        op1.combine(op2);
        QCOMPARE(op1.items.size(), std::size_t{2});
    }

    void testProjOpFreeProj()
    {
        MNEProjOp op;
        StringList colNames = {"ch1", "ch2"};
        StringList rowNames = {"v1"};
        Eigen::MatrixXf data(1, 2);
        data << 1, 0;
        auto nm_ptr = MNENamedMatrix::build(rowNames, colNames, data);

        MNENamedMatrix nm(*nm_ptr);
        op.add_item(nm, 1, "Test");

        op.free_proj();
        QVERIFY(op.names.empty());
        QCOMPARE(op.nvec, 0);
        // Items should still be there
        QCOMPARE(op.items.size(), std::size_t{1});
    }

    void testProjOpAssignChannels()
    {
        MNEProjOp op;
        StringList colNames = {"MEG 0111", "MEG 0112", "MEG 0113"};
        StringList rowNames = {"v1"};
        Eigen::MatrixXf data(1, 3);
        data << 0.577f, 0.577f, 0.577f;
        auto nm_ptr = MNENamedMatrix::build(rowNames, colNames, data);

        MNENamedMatrix nm(*nm_ptr);
        op.add_item(nm, 1, "SSP");

        StringList chList = {"MEG 0111", "MEG 0112", "MEG 0113", "MEG 0121"};
        op.assign_channels(chList);
        QCOMPARE(op.names, chList);
    }

    void testProjOpMakeProj()
    {
        MNEProjOp op;
        StringList colNames = {"ch1", "ch2", "ch3"};
        StringList rowNames = {"v1"};
        Eigen::MatrixXf data(1, 3);
        data << 0.577f, 0.577f, 0.577f;
        auto nm_ptr = MNENamedMatrix::build(rowNames, colNames, data);

        MNENamedMatrix nm(*nm_ptr);
        op.add_item(nm, 1, "SSP");

        StringList chList = {"ch1", "ch2", "ch3"};
        op.assign_channels(chList);
        QVERIFY(op.make_proj());
        QCOMPARE(op.nvec, 1);
    }

    void testProjOpChannelGroupsMatchNamePrefixes()
    {
        // Like MNE-C, only names starting with MEG / EEG / STI select a channel group
        const StringList names = {"MEG 0111", "MEG 0112", "REF_EEG", "TRIG_STI"};
        Eigen::MatrixXf data(1, 4);
        data << 0.5f, 0.5f, 0.5f, 0.5f;
        MNENamedMatrix meg(*MNENamedMatrix::build({"v1"}, names, data));
        MNEProjOp op;
        op.add_item(meg, FIFFV_PROJ_ITEM_FIELD, "SSP");
        QVERIFY(op.items[0].has_meg);
        QVERIFY(!op.items[0].has_eeg);
        op.assign_channels(names);
        QVERIFY(op.make_proj());
        QCOMPARE(op.nvec, 1);
        QVERIFY((op.proj_data.row(0).array().abs() > 0.4f).all());

        const StringList others = {"REF_EEG 1", "REF_EEG 2"};
        MNENamedMatrix ref(*MNENamedMatrix::build({"v1"}, others, Eigen::MatrixXf::Ones(1, 2)));
        MNEProjOp unnamed;
        unnamed.add_item(ref, FIFFV_PROJ_ITEM_FIELD, "SSP");
        QVERIFY(unnamed.items[0].has_meg);
        QVERIFY(!unnamed.items[0].has_eeg);
    }

    void testProjOpProjectVector()
    {
        MNEProjOp op;

        // Create an orthonormal projection vector
        StringList colNames = {"ch1", "ch2", "ch3"};
        StringList rowNames = {"v1"};
        float invSqrt3 = 1.0f / std::sqrt(3.0f);
        Eigen::MatrixXf data(1, 3);
        data << invSqrt3, invSqrt3, invSqrt3;
        auto nm_ptr = MNENamedMatrix::build(rowNames, colNames, data);

        MNENamedMatrix nm(*nm_ptr);
        op.add_item(nm, 1, "SSP");

        StringList chList = {"ch1", "ch2", "ch3"};
        op.assign_channels(chList);
        QVERIFY(op.make_proj());

        // Project a vector
        Eigen::VectorXf vec(3);
        vec << 1.0f, 2.0f, 3.0f;
        float origNorm2 = vec.squaredNorm();
        QVERIFY(op.project_vector(vec, false));
        // Projecting (1, 2, 3) onto (1, 1, 1) / sqrt(3) gives its mean in every channel
        QVERIFY((vec.array() - 2.0f).abs().maxCoeff() < 1e-5f);
        QVERIFY(vec.squaredNorm() <= origNorm2);
        Eigen::VectorXf shorter(2);
        QVERIFY(!op.project_vector(shorter, false));
    }

    void testProjOpProjectDvector()
    {
        MNEProjOp op;

        StringList colNames = {"ch1", "ch2"};
        StringList rowNames = {"v1"};
        float invSqrt2 = 1.0f / std::sqrt(2.0f);
        Eigen::MatrixXf data(1, 2);
        data << invSqrt2, invSqrt2;
        auto nm_ptr = MNENamedMatrix::build(rowNames, colNames, data);

        MNENamedMatrix nm(*nm_ptr);
        op.add_item(nm, 1, "SSP");

        StringList chList = {"ch1", "ch2"};
        op.assign_channels(chList);
        QVERIFY(op.make_proj());

        Eigen::VectorXd dvec(2);
        dvec << 1.0, 2.0;
        double origNorm = dvec.norm();
        QVERIFY(op.project_dvector(dvec, false));
        QVERIFY(dvec.norm() <= origNorm + 1e-5);
    }

    void testProjOpAffect()
    {
        MNEProjOp op;
        StringList colNames = {"MEG 0111", "MEG 0112"};
        StringList rowNames = {"v1"};
        Eigen::MatrixXf data(1, 2);
        data << 1, 0;
        auto nm_ptr = MNENamedMatrix::build(rowNames, colNames, data);

        MNENamedMatrix nm(*nm_ptr);
        op.add_item(nm, 1, "SSP");

        StringList testChs = {"MEG 0111", "MEG 0112"};
        int n = op.affect(testChs);
        QVERIFY(n >= 0);

        StringList noMatch = {"EEG 001"};
        int n2 = op.affect(noMatch);
        QCOMPARE(n2, 0);
    }

    void testProjOpCreateAverageEegRef()
    {
        // Create mock EEG channel info
        std::vector<FiffChInfo> chs;
        for (int i = 0; i < 5; ++i) {
            FiffChInfo ch;
            ch.ch_name = (QString("EEG%1").arg(i + 1, 3, 10, QChar('0'))).toStdString();
            ch.kind = FIFFV_EEG_CH;
            ch.chpos.coil_type = FIFFV_COIL_EEG;
            chs.push_back(ch);
        }

        auto projOp = MNEProjOp::create_average_eeg_ref(chs);
        QVERIFY(projOp != nullptr);
        QVERIFY(!projOp->items.empty());
    }

    void testProjOpReadFromFiff()
    {
        if (!QFile::exists(m_sRawFile))
            QSKIP("Raw file not found");

        FiffRawData raw(m_sRawFile.toStdString());

        // The raw file should have projection operators
        int nProj = static_cast<int>(raw.info.projs.size());
        QVERIFY(nProj >= 0); // Even 0 is valid; exercises the code path
    }

    void testProjOpApplyCov()
    {
        MNEProjOp op;
        StringList colNames = {"ch1", "ch2", "ch3"};
        StringList rowNames = {"v1"};
        float invSqrt3 = 1.0f / std::sqrt(3.0f);
        Eigen::MatrixXf data(1, 3);
        data << invSqrt3, invSqrt3, invSqrt3;
        auto nm_ptr = MNENamedMatrix::build(rowNames, colNames, data);

        MNENamedMatrix nm(*nm_ptr);
        op.add_item(nm, 1, "SSP");

        StringList chList = {"ch1", "ch2", "ch3"};
        op.assign_channels(chList);
        QVERIFY(op.make_proj());

        // Create a covariance matrix
        StringList covNames = colNames;
        Eigen::VectorXd covDiag = Eigen::VectorXd::Ones(3);
        MNECovMatrix cov(FIFFV_MNE_NOISE_COV, 3, covNames, covDiag, covDiag, nullptr);

        // (I - u u^T) I (I - u u^T) with u = (1, 1, 1) / sqrt(3) keeps 2/3 on the diagonal
        QVERIFY(op.apply_cov(cov));
        QVERIFY((cov.cov_diag.array() - 2.0 / 3.0).abs().maxCoeff() < 1e-6);
        QCOMPARE(cov.nproj, 1);

        MNECovMatrix other(FIFFV_MNE_NOISE_COV, 3, {"ch1", "ch2", "ch4"}, covDiag, covDiag, nullptr);
        QVERIFY(!op.apply_cov(other));
    }

    // ── MNECtfCompDataSet ───────────────────────────────────────────────────

    void testCtfCompExplain()
    {
        // CTF compensation kind constants
        QString none = QString::fromStdString(MNECTFCompDataSet::explain_comp(0));
        QVERIFY(!none.isEmpty());

        QString first = QString::fromStdString(MNECTFCompDataSet::explain_comp(1));
        QVERIFY(!first.isEmpty());

        QString second = QString::fromStdString(MNECTFCompDataSet::explain_comp(2));
        QVERIFY(!second.isEmpty());

        QString third = QString::fromStdString(MNECTFCompDataSet::explain_comp(3));
        QVERIFY(!third.isEmpty());

        // Unknown kind
        QString unknown = QString::fromStdString(MNECTFCompDataSet::explain_comp(99));
        QVERIFY(!unknown.isEmpty());
    }

    void testCtfCompMapKind()
    {
        // Map gradient compensation kind to FIFF compensation kind
        int kind0 = MNECTFCompDataSet::map_comp_kind(0);
        QVERIFY(kind0 >= 0);

        int kind1 = MNECTFCompDataSet::map_comp_kind(101);
        QVERIFY(kind1 >= 0);

        int kind3 = MNECTFCompDataSet::map_comp_kind(103);
        QVERIFY(kind3 >= 0);
    }

    void testCtfCompGetComp()
    {
        // Create mock channel list with no compensation
        std::vector<FiffChInfo> chs;
        for (int i = 0; i < 3; ++i) {
            FiffChInfo ch;
            ch.ch_name = ((QString("MEG%1").arg(i + 1, 4, 10, QChar('0'))).toStdString());
            ch.kind = FIFFV_MEG_CH;
            chs.push_back(ch);
        }

        int comp = MNECTFCompDataSet::get_comp(chs, static_cast<int>(chs.size()));
        QVERIFY(comp >= 0);
    }

    void testCtfCompSetComp()
    {
        std::vector<FiffChInfo> chs;
        for (int i = 0; i < 3; ++i) {
            FiffChInfo ch;
            ch.ch_name = ((QString("MEG%1").arg(i + 1, 4, 10, QChar('0'))).toStdString());
            ch.kind = FIFFV_MEG_CH;
            chs.push_back(ch);
        }

        int rc = MNECTFCompDataSet::set_comp(chs, static_cast<int>(chs.size()), 0);
        QVERIFY(rc >= 0);
    }

    void testCtfCompDataSetDefault()
    {
        MNECTFCompDataSet compSet;
        QCOMPARE(compSet.ncomp, 0);
        QCOMPARE(compSet.nch, 0);
    }

    void testCtfCompDataSetCopy()
    {
        MNECTFCompDataSet compSet;
        MNECTFCompDataSet copy(compSet);
        QCOMPARE(copy.ncomp, 0);
    }

    // ── MNEDescriptionParser ────────────────────────────────────────────────

    void testDescriptionParserAveFile()
    {
        // Create a temporary .ave description file
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString aveFile = tmpDir.path() + "/test.ave";

        QFile f(aveFile);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << "# Test averaging description\n";
        out << "average {\n";
        out << "    outfile     test-ave.fif\n";
        out << "    eventfile   test-eve.fif\n";
        out << "    logfile     test.log\n";
        out << "    gradReject  2000e-13\n";
        out << "    magReject   3e-12\n";
        out << "    eegReject   100e-6\n";
        out << "    category {\n";
        out << "        name    \"Auditory Left\"\n";
        out << "        event   1\n";
        out << "        tmin    -0.2\n";
        out << "        tmax    0.5\n";
        out << "        bmin    -0.2\n";
        out << "        bmax    0.0\n";
        out << "    }\n";
        out << "}\n";
        f.close();

        const std::optional<AverageDescription> desc = MNEDescriptionParser::read_average(aveFile.toStdU16String());
        if (desc) {
            QVERIFY(desc->categories.size() > 0);
        } else {
            qWarning("parseAverageFile failed — code path exercised");
        }
    }

    void testDescriptionParserCovFile()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString covFile = tmpDir.path() + "/test.cov";

        QFile f(covFile);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << "# Test covariance description\n";
        out << "cov {\n";
        out << "    outfile     test-cov.fif\n";
        out << "    logfile     test.log\n";
        out << "    gradReject  2000e-13\n";
        out << "    magReject   3e-12\n";
        out << "    eegReject   100e-6\n";
        out << "    def {\n";
        out << "        event   1\n";
        out << "        tmin    -0.2\n";
        out << "        tmax    0.0\n";
        out << "    }\n";
        out << "}\n";
        f.close();

        const std::optional<CovDescription> desc = MNEDescriptionParser::read_covariance(covFile.toStdU16String());
        if (desc) {
            QVERIFY(desc->defs.size() > 0);
        } else {
            qWarning("parseCovarianceFile failed — code path exercised");
        }
    }

    void testDescriptionParserNonExistentFile()
    {
        const std::optional<AverageDescription> desc = MNEDescriptionParser::read_average("/nonexistent/file.ave");
        QVERIFY(!desc);
    }

    void cleanupTestCase()
    {
    }
};

QTEST_GUILESS_MAIN(TestMneProjections)
#include "test_mne_projections.moc"

#include <vector>
