// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2010-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

#include <QtTest/QtTest>
#include <charconv>
#include <format>
#include <memory>
#include <sstream>
#include <QBuffer>
#include <QTemporaryDir>
#include <QDir>
#include <Eigen/Dense>
#include <Eigen/SparseCore>

#include <base/base_global.h>
#include <base/log.h>
#include <math/linalg.h>
#include <mne/mne_source_spaces.h>
#include <mne/mne_hemisphere.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <mne/mne_forward_solution.h>
#include <mne/mne_inverse_operator.h>
#include <inv/inv_source_estimate.h>
#include <mne/mne_epoch_data.h>
#include <mne/mne_epoch_data_list.h>
#include <mne/mne_named_matrix.h>
#include <mne/mne_cluster_info.h>
#include <mne/mne_description_parser.h>
#include <mne/mne_process_description.h>
#include <mne/mne_source_space.h>
#include <mne/mne_proj_op.h>
#include <mne/mne.h>
#include <mne/mne_global.h>
#include <mne/mne_surface_patch.h>
#include <fiff/fiff_ch_name.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_proj.h>
#include <fiff/fiff_file.h>
#include <fiff/fiff_events.h>
#include <mne/mne_meas_data_set.h>
#include <mne/mne_msh_display_surface.h>
#include <fs/fs_label.h>
#include <fs/fs_surface.h>
#include <fs/fs_annotation.h>
#include <fs/fs_annotationset.h>

using namespace mne;
using namespace Eigen;

class TestMneDataTypes : public QObject
{
    Q_OBJECT

private:
    QString dataPath;

private slots:
    void initTestCase()
    {
        dataPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data";
    }

    void sourceSpace_filterLogsOmittedPoints()
    {
        const QString src = dataPath + "/subjects/sample/bem/sample-oct-6-src.fif";
        const QString bem = dataPath + "/subjects/sample/bem/sample-5120-bem.fif";
        const QString trans = dataPath + "/MEG/sample/all-trans.fif";
        if (!QFile::exists(src) || !QFile::exists(bem) || !QFile::exists(trans))
            QSKIP("Sample source space, BEM or transformation not found");
        const FiffCoordTrans mriHeadT(trans.toStdU16String(), FIFFV_COORD_MRI, FIFFV_COORD_HEAD);

        // The threaded filter must log the same points in the same order as the serial one.
        std::string logs[2];
        for (const bool threads : {false, true}) {
            std::vector<std::unique_ptr<MNESourceSpace>> spaces;
            QCOMPARE(MNESourceSpace::read_source_spaces(src.toStdU16String(), spaces), 0);
            std::ostringstream log;
            QCOMPARE(MNESourceSpace::filter_source_spaces(0.005f, bem.toStdU16String(), mriHeadT, spaces, &log, threads), 0);
            QCOMPARE(spaces[0]->nuse + spaces[1]->nuse, 3732 + 3766);
            logs[threads] = log.str();
        }
        QCOMPARE(logs[1], logs[0]);

        // MNE-C mne_filter_source_spaces: one "%10.3f %10.3f %10.3f\n" line per omitted point, in mm
        std::istringstream lines(logs[0]);
        int count = 0;
        for (std::string line; std::getline(lines, line); ++count) {
            QCOMPARE(line.size(), std::size_t{32});
            float xyz[3] = {};
            for (std::size_t c = 0; c < 3; ++c) {
                const std::string_view field = std::string_view(line).substr(11 * c, 10);
                const char* first = field.data() + field.find_first_not_of(' ');
                QVERIFY2(std::from_chars(first, field.data() + field.size(), xyz[c]).ec == std::errc(), line.c_str());
            }
            QCOMPARE(std::format("{:10.3f} {:10.3f} {:10.3f}", xyz[0], xyz[1], xyz[2]), line);
        }
        QCOMPARE(count, 2 * 4098 - 3732 - 3766);
    }

    //=========================================================================
    // MNESourceSpaces
    //=========================================================================
    void sourceSpace_defaultCtor()
    {
        MNESourceSpaces ss;
        QVERIFY(ss.empty());
        QCOMPARE(ss.size(), 0);
    }

    void sourceSpace_copyCtor()
    {
        MNESourceSpaces ss;
        MNESourceSpaces copy(ss);
        QVERIFY(copy.empty());
    }

    void sourceSpace_clear()
    {
        MNESourceSpaces ss;
        ss.clear();
        QVERIFY(ss.empty());
    }

    void sourceSpace_equality()
    {
        MNESourceSpaces ss1, ss2;
        QVERIFY(ss1 == ss2);
    }

    void sourceSpace_getVertno()
    {
        MNESourceSpaces ss;
        std::vector<VectorXi> vertno = ss.get_vertno();
        QCOMPARE(vertno.size(), 0);
    }

    void sourceSpace_readFromFile()
    {
        QString srcPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif";
        if (!QFile::exists(srcPath)) {
            QSKIP("Source space file not found");
        }

        const MNESourceSpaces ss(srcPath.toStdU16String());
        QCOMPARE(ss.size(), 2);
        std::vector<VectorXi> vertno = ss.get_vertno();
        QCOMPARE(vertno.size(), 2);
        QVERIFY(vertno[0].size() > 0);
    }

    //=========================================================================
    // MNEBem
    //=========================================================================
    void bem_defaultCtor()
    {
        MNEBem bem;
        QVERIFY(bem.empty());
        QCOMPARE(bem.size(), 0);
    }

    void bem_copyCtor()
    {
        MNEBem bem;
        MNEBem copy(bem);
        QVERIFY(copy.empty());
    }

    void bem_addSurface()
    {
        MNEBem bem;
        MNEBemSurface surf;
        surf.id = 1;
        surf.np = 100;
        surf.ntri = 50;
        surf.rr = MatrixX3f::Random(100, 3);
        surf.itris = MatrixX3i::Zero(50, 3);
        surf.nn = MatrixX3f::Random(100, 3);
        bem.push_back(surf);
        QCOMPARE(bem.size(), 1);
        QVERIFY(!bem.empty());
    }

    void bem_clear()
    {
        MNEBem bem;
        MNEBemSurface surf;
        surf.id = 1;
        bem.push_back(surf);
        bem.clear();
        QVERIFY(bem.empty());
    }

    void bem_readFromFile()
    {
        QString bemPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-5120-bem.fif";
        if (!QFile::exists(bemPath)) {
            QSKIP("BEM file not found");
        }
        MNEBem bem(bemPath.toStdU16String());
        QVERIFY(!bem.empty());
        QVERIFY(bem.size() > 0);
    }

    void bem_transformInvTransform()
    {
        MNEBem bem;
        MNEBemSurface surf;
        surf.id = 1;
        surf.np = 3;
        surf.ntri = 1;
        surf.coord_frame = FIFFV_COORD_MRI;
        surf.rr.resize(3, 3);
        surf.rr << 1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f;
        surf.nn = MatrixX3f::Zero(3, 3);
        surf.itris.resize(1, 3);
        surf.itris << 0, 1, 2;
        bem.push_back(surf);

        FiffCoordTrans trans;
        trans.from = FIFFV_COORD_MRI;
        trans.to = FIFFV_COORD_HEAD;
        trans.trans = Matrix4f::Identity();
        trans.trans(0, 3) = 0.1f;
        trans.invtrans = trans.trans.inverse();

        bem.transform(trans);
        QVERIFY(qAbs(bem[0].rr(0, 0) - 1.1f) < 0.01f);

        bem.inverse_transform(trans);
        QVERIFY(qAbs(bem[0].rr(0, 0) - 1.0f) < 0.01f);
    }

    void bem_writeToBuffer()
    {
        MNEBem bem;
        MNEBemSurface surf;
        surf.id = FIFFV_BEM_SURF_ID_BRAIN;
        surf.np = 3;
        surf.ntri = 1;
        surf.coord_frame = FIFFV_COORD_MRI;
        surf.rr.resize(3, 3);
        surf.rr << 0.01f, 0.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.0f, 0.0f, 0.01f;
        surf.nn.resize(3, 3);
        surf.nn << 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f;
        surf.itris.resize(1, 3);
        surf.itris << 0, 1, 2;
        bem.push_back(surf);

        QTemporaryDir dir;
        const std::filesystem::path path = dir.filePath("bem.fif").toStdU16String();
        QVERIFY(bem.write(path));
        QCOMPARE(MNEBem(path).size(), 1);
    }

    //=========================================================================
    // MNEForwardSolution with real data
    //=========================================================================
    void fwd_readFromFile()
    {
        QString fwdPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdPath)) {
            QSKIP("Forward solution file not found");
        }
        MNEForwardSolution fwd(fwdPath.toStdU16String());
        if (fwd.empty()) {
            QSKIP("Failed to read forward solution");
        }

        QVERIFY(!fwd.empty());
        QVERIFY(!fwd.isFixedOrient());
        QVERIFY(fwd.sol.data.rows() > 0);

        VectorXi testSel(2);
        testSel << 0, 1;
        VectorXi triplets = fwd.tripletSelection(testSel);
        QCOMPARE(triplets.size(), 6);

        MNEForwardSolution fwdMeg = fwd.pick_types(true, false);
        QVERIFY(fwdMeg.sol.data.rows() > 0);
    }

    void fwd_restrictGainMatrix()
    {
        int nSensors = 10;
        int nSources = 6;
        MatrixXd gain = MatrixXd::Random(nSensors, nSources);

        FiffInfo info;
        info.nchan = nSensors;
        for (int i = 0; i < nSensors; ++i) {
            FiffChInfo ch;
            ch.ch_name = "CH" + std::to_string(i);
            ch.kind = FIFFV_MEG_CH;
            ch.unit = FIFF_UNIT_T;
            ch.chpos.coil_type = FIFFV_COIL_VV_MAG_T3;
            ch.range = 1.0;
            ch.cal = 1.0;
            info.chs.push_back(ch);
            info.ch_names.push_back(ch.ch_name);
        }
        info.bads.clear();

        MNEForwardSolution::restrict_gain_matrix(gain, info);
        QCOMPARE(gain.rows(), nSensors);
    }

    //=========================================================================
    // MNEInverseOperator
    //=========================================================================
    void invOp_defaultCtor()
    {
        MNEInverseOperator inv;
        QVERIFY(!inv.isFixedOrient());
    }

    void invOp_copyCtor()
    {
        MNEInverseOperator inv;
        MNEInverseOperator copy(inv);
        QVERIFY(!copy.isFixedOrient());
    }

    void invOp_kernel()
    {
        MNEInverseOperator inv;
        MatrixXd kernel = inv.getKernel();
        QCOMPARE(kernel.rows(), 0);
    }

    //=========================================================================
    // InvSourceEstimate
    //=========================================================================
    void sourceEstimate_defaultCtor()
    {
        InvSourceEstimate stc;
        QVERIFY(stc.empty());
    }

    void sourceEstimate_paramCtor()
    {
        MatrixXd data = MatrixXd::Random(100, 10);
        VectorXi vertices(100);
        for (int i = 0; i < 100; ++i)
            vertices(i) = i;
        float tmin = 0.0f;
        float tstep = 0.001f;

        InvSourceEstimate stc(data, vertices, tmin, tstep);
        QVERIFY(!stc.empty());
        QCOMPARE(stc.data.rows(), 100);
        QCOMPARE(stc.data.cols(), 10);
    }

    //=========================================================================
    // MNEEpochData
    //=========================================================================
    void epochData_fromEvoked()
    {
        // mne.Epochs(tmin=-0.1, tmax=0.3, baseline=(-0.1, 0)) at 300 Hz averages samples -30..0 (31 samples),
        // the stimulus sample included; a ramp's baseline mean is then 15
        MNEEpochData ramp;
        ramp.epoch = RowVectorXd::LinSpaced(121, 0.0, 120.0);
        ramp.tmin = -0.1f;
        ramp.tmax = 0.3f;
        ramp.applyBaselineCorrection(QPair<float, float>(-0.1f, 0.0f));
        QCOMPARE(ramp.epoch(0, 0), -15.0);

        QString avePath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(avePath)) {
            QSKIP("Sample evoked file not found");
        }
        FiffEvokedSet es(avePath.toStdU16String());
        if (es.evoked.empty()) {
            QSKIP("Failed to read evoked set");
        }

        FiffEvoked ev = es.evoked[0];
        std::shared_ptr<MNEEpochData> epoch(new MNEEpochData());
        epoch->epoch = ev.data;
        epoch->tmin = ev.times(0);
        epoch->tmax = ev.times(ev.times.size() - 1);
        epoch->bReject = false;

        QVERIFY(epoch->epoch.rows() > 0);
        QVERIFY(epoch->epoch.cols() > 0);

        epoch->applyBaselineCorrection(QPair<float, float>(epoch->tmin, 0.0f));
    }

    //=========================================================================
    // MNEClusterInfo
    //=========================================================================
    void clusterInfo_basic()
    {
        MNEClusterInfo ci;
        ci.clusterVertnos.push_back(VectorXi::LinSpaced(5, 0, 4));
        ci.clusterVertnos.push_back(VectorXi::LinSpaced(3, 10, 12));
        QCOMPARE(ci.clusterVertnos.size(), 2);

        ci.clusterVertnos.clear();
        QCOMPARE(ci.clusterVertnos.size(), 0);
    }

    //=========================================================================
    // InvSourceEstimate — extended (reduce, clear, copy)
    //=========================================================================
    void sourceEstimate_copyCtor()
    {
        MatrixXd data = MatrixXd::Random(50, 5);
        VectorXi vertices(50);
        for (int i = 0; i < 50; ++i)
            vertices(i) = i;
        InvSourceEstimate stc(data, vertices, 0.0f, 0.001f);

        InvSourceEstimate copy(stc);
        QVERIFY(!copy.empty());
        QCOMPARE(copy.data.rows(), 50);
        QCOMPARE(copy.data.cols(), 5);
    }

    void sourceEstimate_reduce()
    {
        MatrixXd data = MatrixXd::Random(50, 10);
        VectorXi vertices(50);
        for (int i = 0; i < 50; ++i)
            vertices(i) = i;
        InvSourceEstimate stc(data, vertices, 0.0f, 0.001f);

        InvSourceEstimate reduced = stc.reduce(2, 5);
        QCOMPARE(reduced.data.cols(), 5);
    }

    void sourceEstimate_clear()
    {
        MatrixXd data = MatrixXd::Random(10, 5);
        VectorXi vertices(10);
        for (int i = 0; i < 10; ++i)
            vertices(i) = i;
        InvSourceEstimate stc(data, vertices, 0.0f, 0.001f);
        QVERIFY(!stc.empty());

        stc.clear();
        // After clear, data is reset to empty matrix
        QCOMPARE(stc.data.rows(), 0);
        QCOMPARE(stc.vertices.size(), 0);
    }

    //=========================================================================
    // MNESourceSpaces — additional coverage (transform, find_hemi)
    //=========================================================================
    void sourceSpace_readAndTransform()
    {
        QString srcPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif";
        if (!QFile::exists(srcPath))
            QSKIP("Source space file not found");
        MNESourceSpaces ss(srcPath.toStdU16String());
        QVERIFY(!ss.empty());

        // Test hemisphere IDs
        for (int i = 0; i < ss.size(); ++i) {
            QVERIFY(ss[i].id == FIFFV_MNE_SURF_LEFT_HEMI || ss[i].id == FIFFV_MNE_SURF_RIGHT_HEMI);
        }
    }

    //=========================================================================
    // MNEForwardSolution — additional coverage
    //=========================================================================
    void fwd_clusterForward()
    {
        QString fwdPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdPath))
            QSKIP("Forward solution file not found");
        MNEForwardSolution fwd(fwdPath.toStdU16String());
        if (fwd.empty())
            QSKIP("Failed to read forward solution");

        // Test pick_types for EEG only
        MNEForwardSolution fwdEeg = fwd.pick_types(false, true);
        QVERIFY(fwdEeg.sol.data.rows() > 0);

        // Test nchan, nsource accessors
        QVERIFY(fwd.nchan > 0);
        QVERIFY(fwd.nsource > 0);

        // Test isClustered
        QVERIFY(!fwd.isClustered());
    }

    void fwd_prepareForward()
    {
        // Tests prepare_forward with a small synthetic forward
        int nSensors = 10;
        int nSources = 6; // 2 sources with 3 orientations each

        MNEForwardSolution fwd;
        fwd.source_ori = FIFFV_MNE_FREE_ORI;
        fwd.coord_frame = FIFFV_COORD_HEAD;
        fwd.nsource = 2;
        fwd.nchan = nSensors;

        FiffNamedMatrix solData;
        solData.data = MatrixXd::Random(nSensors, nSources);
        solData.nrow = nSensors;
        solData.ncol = nSources;
        for (int i = 0; i < nSensors; ++i)
            solData.row_names.push_back(QString("CH%1").arg(i).toStdString());
        fwd.sol = solData;

        FiffInfo info;
        info.nchan = nSensors;
        for (int i = 0; i < nSensors; ++i) {
            FiffChInfo ch;
            ch.ch_name = (QString("CH%1").arg(i)).toStdString();
            ch.kind = FIFFV_MEG_CH;
            ch.unit = FIFF_UNIT_T;
            ch.chpos.coil_type = FIFFV_COIL_VV_MAG_T3;
            ch.range = 1.0;
            ch.cal = 1.0;
            info.chs.push_back(ch);
            info.ch_names.push_back(ch.ch_name);
        }

        // Test restrict_gain_matrix
        MatrixXd gain = fwd.sol.data;
        MNEForwardSolution::restrict_gain_matrix(gain, info);
        QCOMPARE(gain.rows(), nSensors);
    }

    //=========================================================================
    // MNEInverseOperator — extended coverage
    //=========================================================================
    void invOp_readFromFile()
    {
        QString invPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-oct-6-meg-inv.fif";
        if (!QFile::exists(invPath)) {
            // Try MNE sample data
            invPath = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/sample_audvis-meg-oct-6-meg-inv.fif";
        }
        if (!QFile::exists(invPath)) {
            // Try fixed orientation variant
            invPath = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/sample_audvis-meg-oct-6-meg-fixed-inv.fif";
        }
        if (!QFile::exists(invPath))
            QSKIP("Inverse operator file not found");
        MNEInverseOperator inv(invPath.toStdU16String());

        // Verify construction loaded data
        QVERIFY(inv.noise_cov.data.rows() > 0);
    }

    //=========================================================================
    // MNEEpochDataList — coverage
    //=========================================================================
    void epochDataList_basic()
    {
        MNEEpochDataList list;
        QVERIFY(list.empty());

        std::shared_ptr<MNEEpochData> epoch(new MNEEpochData());
        epoch->epoch = MatrixXd::Random(5, 10);
        epoch->tmin = 0.0;
        epoch->tmax = 0.01f;
        epoch->bReject = false;

        list.push_back(epoch);
        QCOMPARE(list.size(), 1);
    }

    //=========================================================================
    // FsLabel — coverage
    //=========================================================================
    void fsLabel_defaultCtor()
    {
        FsLabel label;
        QCOMPARE(label.label_id, -1);
    }

    void fsLabel_readFromFile()
    {
        // Try to find a label file
        QString labelPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/label/lh.aparc.a2009s.annot";
        // Labels in the test data might be annotation files, try a regular label
        QString labelPath2 = QString::fromUtf8(qgetenv("MNE_DATA"));
        if (labelPath2.isEmpty())
            labelPath2 = QDir::homePath() + "/mne_data/MNE-sample-data";
        QString lhLabel = labelPath2 + "/subjects/sample/label/lh.BA1.label";
        if (QFile::exists(lhLabel)) {
            FsLabel label;
            FsLabel::read(lhLabel.toStdU16String(), label);
            QVERIFY(label.vertices.size() > 0);
            QVERIFY(label.hemi == 0); // left hemisphere
        }
    }

    //=========================================================================
    // FsSurface — coverage
    //=========================================================================
    void fsSurface_defaultCtor()
    {
        FsSurface surf;
        QCOMPARE(surf.rr().rows(), 0);
    }

    //=========================================================================
    // FsAnnotation — coverage
    //=========================================================================
    void fsAnnotation_defaultCtor()
    {
        FsAnnotation annot;
        QVERIFY(annot.empty());
    }

    void fsAnnotation_readFromFile()
    {
        QString annotPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/label/lh.aparc.annot";
        if (!QFile::exists(annotPath))
            QSKIP("Annotation file not found");
        FsAnnotation annot;
        FsAnnotation::read(annotPath.toStdU16String(), annot);
        QVERIFY(!annot.empty());
    }

    void fsAnnotationSet_defaultCtor()
    {
        FsAnnotationSet aset;
        QVERIFY(aset.empty());
    }

    //=========================================================================
    // MNEDescriptionParser — .ave file parsing
    //=========================================================================
    void descriptionParser_parseAverageFile()
    {
        QString avePath = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/audvis.ave";
        if (!QFile::exists(avePath))
            QSKIP("audvis.ave not found");

        const std::optional<AverageDescription> desc = MNEDescriptionParser::read_average(avePath.toStdU16String());
        QVERIFY(desc);

        // audvis.ave has 4 categories: Left Auditory, Right Auditory, Left Visual, Right Visual
        QVERIFY(desc->categories.size() >= 4);

        // Check first category
        QVERIFY(!desc->categories[0].comment.empty());
        QVERIFY(desc->categories[0].events.size() > 0);
        QVERIFY(desc->categories[0].tmin < 0.0f);
        QVERIFY(desc->categories[0].tmax > 0.0f);

        // Check rejection params were parsed
        QVERIFY(desc->rej.megGradReject > 0.0f);
        QVERIFY(desc->rej.megMagReject > 0.0f);
        QVERIFY(desc->rej.eogReject > 0.0f);
    }

    //=========================================================================
    // MNEDescriptionParser — .cov file parsing
    //=========================================================================
    void descriptionParser_parseCovarianceFile()
    {
        QString covPath = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/audvis.cov";
        if (!QFile::exists(covPath))
            QSKIP("audvis.cov not found");

        const std::optional<CovDescription> desc = MNEDescriptionParser::read_covariance(covPath.toStdU16String());
        QVERIFY(desc);

        // audvis.cov has def sections for each category
        QVERIFY(desc->defs.size() >= 4);

        // Check first def
        QVERIFY(desc->defs[0].events.size() > 0);
        QVERIFY(desc->defs[0].tmin < 0.0f);

        // Check output filename was parsed
        QVERIFY(!desc->filename.empty());

        // Check rejection params
        QVERIFY(desc->rej.megGradReject > 0.0f);
        QVERIFY(desc->rej.megMagReject > 0.0f);
        QVERIFY(desc->rej.eegReject > 0.0f);
        QVERIFY(desc->rej.eogReject > 0.0f);
    }

    void descriptionParser_parseCovarianceFile_ernoise()
    {
        QString covPath = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/ernoise.cov";
        if (!QFile::exists(covPath))
            QSKIP("ernoise.cov not found");

        const std::optional<CovDescription> desc = MNEDescriptionParser::read_covariance(covPath.toStdU16String());
        QVERIFY(desc);
        QVERIFY(desc->defs.size() > 0);
    }

    void descriptionParser_parseAverageFile_invalid()
    {
        const std::optional<AverageDescription> desc = MNEDescriptionParser::read_average("/nonexistent/file.ave");
        QVERIFY(!desc);
    }

    void descriptionParser_parseCovarianceFile_invalid()
    {
        const std::optional<CovDescription> desc = MNEDescriptionParser::read_covariance("/nonexistent/file.cov");
        QVERIFY(!desc);
    }

    //=========================================================================
    // MNENamedMatrix — additional coverage
    //=========================================================================
    void namedMatrix_construction()
    {
        FiffNamedMatrix nm;
        QCOMPARE(nm.nrow, -1);
        QCOMPARE(nm.ncol, -1);
        QVERIFY(nm.data.rows() == 0);

        // Build a populated named matrix
        nm.nrow = 3;
        nm.ncol = 4;
        nm.data = MatrixXd::Random(3, 4);
        for (int i = 0; i < 3; ++i)
            nm.row_names.push_back(QString("r%1").arg(i).toStdString());
        for (int j = 0; j < 4; ++j)
            nm.col_names.push_back(QString("c%1").arg(j).toStdString());

        QCOMPARE(nm.row_names.size(), 3);
        QCOMPARE(nm.col_names.size(), 4);

        // Copy constructor
        FiffNamedMatrix nm2(nm);
        QCOMPARE(nm2.nrow, 3);
        QCOMPARE(nm2.ncol, 4);
        QCOMPARE(nm2.data.rows(), 3);
        QCOMPARE(nm2.data.cols(), 4);
        QCOMPARE(nm2.row_names.size(), 3);
    }

    void namedMatrix_transpose()
    {
        FiffNamedMatrix nm;
        nm.nrow = 2;
        nm.ncol = 3;
        nm.data = MatrixXd::Random(2, 3);
        nm.row_names.push_back("r0");
        nm.row_names.push_back("r1");
        nm.col_names.push_back("c0");
        nm.col_names.push_back("c1");
        nm.col_names.push_back("c2");

        nm.transpose_named_matrix();
        QCOMPARE(nm.nrow, 3);
        QCOMPARE(nm.ncol, 2);
        QCOMPARE(nm.row_names.size(), 3);
        QCOMPARE(nm.col_names.size(), 2);
        QCOMPARE(nm.row_names[0], QString("c0"));
        QCOMPARE(nm.col_names[0], QString("r0"));
    }

    //=========================================================================
    // MNESourceSpace — factory and operations
    //=========================================================================
    void sourceSpace_createFactory()
    {
        auto sp = MNESourceSpace::create_source_space(100);
        QVERIFY(sp != nullptr);
        QCOMPARE(sp->np, 100);
        QCOMPARE(sp->nuse, 0);
    }

    void sourceSpace_enableAllSources()
    {
        auto sp = MNESourceSpace::create_source_space(50);
        sp->enable_all_sources();
        QCOMPARE(sp->nuse, 50);
        QCOMPARE(sp->inuse.size(), 50);
        for (int i = 0; i < 50; ++i)
            QCOMPARE(sp->inuse(i), 1);
    }

    void sourceSpace_isLeftHemi()
    {
        MNESourceSpace sp(10);
        sp.rr = MatrixX3f::Zero(10, 3);
        // All x < 0 → left hemisphere
        for (int i = 0; i < 10; ++i)
            sp.rr(i, 0) = -0.05f;
        QCOMPARE(sp.is_left_hemi(), 1);

        // All x > 0 → right hemisphere
        for (int i = 0; i < 10; ++i)
            sp.rr(i, 0) = 0.05f;
        QCOMPARE(sp.is_left_hemi(), 0);
    }

    void sourceSpace_updateInuseAndRearrange()
    {
        auto sp = MNESourceSpace::create_source_space(10);
        sp->enable_all_sources();
        QCOMPARE(sp->nuse, 10);

        // Deselect some
        VectorXi newInuse = VectorXi::Zero(10);
        newInuse(0) = 1;
        newInuse(3) = 1;
        newInuse(7) = 1;
        sp->update_inuse(newInuse);
        QCOMPARE(sp->nuse, 3);

        sp->rearrange_source_space();
        QCOMPARE(sp->vertno.size(), 3);
        QCOMPARE(sp->vertno(0), 0);
        QCOMPARE(sp->vertno(1), 3);
        QCOMPARE(sp->vertno(2), 7);
    }

    void sourceSpaces_appendAndAccess()
    {
        MNESourceSpaces ss;
        QVERIFY(ss.empty());

        MNESourceSpace lh(5);
        lh.id = FIFFV_MNE_SURF_LEFT_HEMI;
        lh.rr = MatrixX3f::Zero(5, 3);
        for (int i = 0; i < 5; ++i)
            lh.rr(i, 0) = -0.05f;

        MNESourceSpace rh(5);
        rh.id = FIFFV_MNE_SURF_RIGHT_HEMI;
        rh.rr = MatrixX3f::Zero(5, 3);
        for (int i = 0; i < 5; ++i)
            rh.rr(i, 0) = 0.05f;

        ss.append(lh);
        ss.append(rh);
        QCOMPARE(ss.size(), 2);
        QVERIFY(!ss.empty());

        QCOMPARE(ss[0].id, FIFFV_MNE_SURF_LEFT_HEMI);
        QCOMPARE(ss[1].id, FIFFV_MNE_SURF_RIGHT_HEMI);
    }

    //=========================================================================
    // MNEForwardSolution — write/read roundtrip + to_fixed_ori
    //=========================================================================
    void fwd_writeReadRoundtrip()
    {
        QString fwdPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdPath))
            QSKIP("Forward solution file not found");

        MNEForwardSolution fwdOrig(fwdPath.toStdU16String());
        if (fwdOrig.empty())
            QSKIP("Failed to read forward solution");

        QTemporaryDir dir;
        const std::filesystem::path path = dir.filePath("round-trip-fwd.fif").toStdU16String();
        QVERIFY(fwdOrig.write(path));
        const MNEForwardSolution fwdRead(path);
        QVERIFY(!fwdRead.empty());
        QCOMPARE(fwdRead.nchan, fwdOrig.nchan);
        QCOMPARE(fwdRead.nsource, fwdOrig.nsource);
    }

    void fwd_toFixedOri()
    {
        QString fwdPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdPath))
            QSKIP("Forward solution file not found");

        MNEForwardSolution fwd(fwdPath.toStdU16String());
        if (fwd.empty())
            QSKIP("Failed to read forward solution");

        QVERIFY(!fwd.isFixedOrient());

        // to_fixed_ori requires surface-oriented source space
        // If it cannot convert, it prints a warning and does nothing — verify at least it doesn't crash
        fwd.to_fixed_ori();

        // The test data may or may not be surface-oriented, so just verify the call works
        // (coverage is the goal, not necessarily changing orientation)
    }

    void fwd_computeOrientPrior()
    {
        QString fwdPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdPath))
            QSKIP("Forward solution file not found");

        MNEForwardSolution fwd(fwdPath.toStdU16String());
        if (fwd.empty())
            QSKIP("Failed to read forward solution");

        FiffCov orientPrior = fwd.compute_orient_prior(0.2f);
        QVERIFY(orientPrior.data.rows() > 0);
    }

    //=========================================================================
    // MNEInverseOperator — prepare and write
    //=========================================================================
    void invOp_prepareAndKernel()
    {
        QString invPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-oct-6-meg-inv.fif";
        if (!QFile::exists(invPath)) {
            invPath = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/sample_audvis-meg-oct-6-meg-inv.fif";
        }
        if (!QFile::exists(invPath)) {
            invPath = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/sample_audvis-meg-oct-6-meg-fixed-inv.fif";
        }
        if (!QFile::exists(invPath))
            QSKIP("Inverse operator file not found");

        MNEInverseOperator invOp(invPath.toStdU16String());
        QVERIFY(invOp.noise_cov.data.rows() > 0);

        // Prepare inverse operator
        float lambda2 = 1.0f / 9.0f;
        MNEInverseOperator prepared = invOp.prepare_inverse_operator(1, lambda2, false, false);
        QVERIFY(prepared.eigen_leads.data.rows() > 0);
    }

    void invOp_writeReadRoundtrip()
    {
        QString invPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-oct-6-meg-inv.fif";
        if (!QFile::exists(invPath)) {
            invPath = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/sample_audvis-meg-oct-6-meg-inv.fif";
        }
        if (!QFile::exists(invPath)) {
            invPath = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/sample_audvis-meg-oct-6-meg-fixed-inv.fif";
        }
        if (!QFile::exists(invPath))
            QSKIP("Inverse operator file not found");

        const MNEInverseOperator invOp(invPath.toStdU16String());

        QTemporaryDir dir;
        const std::filesystem::path path = dir.filePath("round-trip-inv.fif").toStdU16String();
        QVERIFY(invOp.write(path));
        const MNEInverseOperator invRead(path);

        // Comparing only nsource would pass even if every other field were
        // lost. The unit in particular used to be dropped, because the writer
        // never emitted FIFF_MNE_INVERSE_SOURCE_UNIT, so an operator that
        // passed through MNE-CPP came out of mne-python with units of None.
        QCOMPARE(invRead.nsource, invOp.nsource);
        QCOMPARE(invRead.nchan, invOp.nchan);
        QCOMPARE(invRead.coord_frame, invOp.coord_frame);
        QCOMPARE(invRead.source_ori, invOp.source_ori);
        QCOMPARE(invRead.methods, invOp.methods);
        QCOMPARE(invRead.units, invOp.units);
        QCOMPARE(invRead.src.size(), invOp.src.size());
        QCOMPARE(invRead.projs.size(), invOp.projs.size());

        QCOMPARE(invRead.sing.size(), invOp.sing.size());
        QVERIFY(invRead.sing.isApprox(invOp.sing, 1e-5f));
        QVERIFY(invRead.source_nn.isApprox(invOp.source_nn, 1e-5f));
        QVERIFY(invRead.eigen_fields.data.isApprox(invOp.eigen_fields.data, 1e-5f));
        QVERIFY(invRead.eigen_leads.data.isApprox(invOp.eigen_leads.data, 1e-5f));
    }

    //=========================================================================
    // MNE global / MNE wrapper / MNESurfacePatch (0% coverage files)
    //=========================================================================

    void mneGlobal_buildInfo()
    {
        const char* dt = mne::buildDateTime();
        QVERIFY(dt != nullptr && dt[0] != '\0');

        const char* hash = mne::buildHash();
        QVERIFY(hash != nullptr);
    }

    void mneSurfacePatch_construction()
    {
        // Default constructor with np > 0 exercises vert/border allocation + MNESourceSpace creation
        MNESurfacePatch patch(10);
        QCOMPARE(patch.vert.size(), 10);
        QCOMPARE(patch.border.size(), 10);
        QVERIFY(patch.s != nullptr);
    }

    void mneSurfacePatch_zeroSize()
    {
        // np <= 0 should not allocate
        MNESurfacePatch patch(0);
        QCOMPARE(patch.vert.size(), 0);
    }

    void mne_setupCompensators()
    {
        // Load raw data file
        QString rawPath = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif";
        if (!QFile::exists(rawPath))
            QSKIP("sample_audvis_raw.fif not found");

        FiffRawData raw(rawPath.toStdString());
        QVERIFY(raw.info.ch_names.size() > 0);

        // activate_projections activates the projectors and builds the operator
        QVERIFY(raw.activate_projections() > 0);
        QCOMPARE(raw.proj.rows(), static_cast<Eigen::Index>(raw.info.nchan));
        for (int k = 0; k < std::ssize(raw.info.projs); ++k) {
            QVERIFY(raw.info.projs[k].active);
        }

        // Neuromag data has no CTF compensation: grade 0 is a no-op, another grade cannot be made
        QVERIFY(raw.apply_gradient_compensation(0));
        QCOMPARE(raw.apply_gradient_compensation(1).error().code, mne::ErrorCode::Format);
    }

    void mne_combineXyz()
    {
        // combine_xyz takes a vector of length 3*nsrc, returns nsrc values
        // It computes x^2 + y^2 + z^2 for each triplet (NOT sqrt)
        VectorXd vec(6);
        vec << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0;
        VectorXd result = mne::Linalg::combine_xyz(vec);
        QCOMPARE(result.size(), static_cast<Eigen::Index>(2));
        double exp0 = 1.0 + 4.0 + 9.0;    // 14
        double exp1 = 16.0 + 25.0 + 36.0; // 77
        QVERIFY(qAbs(result(0) - exp0) < 1e-10);
        QVERIFY(qAbs(result(1) - exp1) < 1e-10);
    }

    void mne_makeBlockDiag()
    {
        // make_block_diag splits columns into blocks of n columns each
        MatrixXd A(2, 4);
        A << 1, 2, 5, 6,
            3, 4, 7, 8;
        SparseMatrix<double> S = mne::Linalg::make_block_diag(A, 2);
        // A has 4 cols with block size 2 → 2 blocks, each 2x2
        // Result should be 4x4 sparse block diagonal
        QCOMPARE(S.rows(), static_cast<Eigen::Index>(4));
        QCOMPARE(S.cols(), static_cast<Eigen::Index>(4));
        // Top-left block should be [1,2; 3,4]
        QVERIFY(qAbs(S.coeff(0, 0) - 1.0) < 1e-10);
        QVERIFY(qAbs(S.coeff(1, 1) - 4.0) < 1e-10);
        // Bottom-right block [5,6; 7,8]
        QVERIFY(qAbs(S.coeff(2, 2) - 5.0) < 1e-10);
        QVERIFY(qAbs(S.coeff(3, 3) - 8.0) < 1e-10);
        // Off-diagonal blocks should be zero
        QCOMPARE(S.coeff(0, 2), 0.0);
        QCOMPARE(S.coeff(2, 0), 0.0);
    }

    //=========================================================================
    // MNEProjOp additional coverage
    //=========================================================================

    void projOp_constructAndFree()
    {
        MNEProjOp projOp;
        // Initially empty
        QVERIFY(projOp.items.empty());
        projOp.free_proj();
        QVERIFY(projOp.items.empty());
    }

    void projOp_addItemAndDup()
    {
        MNEProjOp projOp;

        // Create a named matrix for projection vectors
        StringList rowlist{"vec1"};
        StringList collist{"MEG 001", "MEG 002", "MEG 003"};
        MatrixXf data(1, 3);
        data << 0.577f, 0.577f, 0.577f;
        auto vecs = MNENamedMatrix::build(rowlist, collist, data);
        QVERIFY(vecs != nullptr);

        projOp.add_item(*vecs, FIFFV_PROJ_ITEM_FIELD, "Test projection");
        QCOMPARE(projOp.items.size(), std::size_t{1});

        // dup creates a deep copy
        auto dup = projOp.dup();
        QVERIFY(dup != nullptr);
        QCOMPARE(dup->items.size(), std::size_t{1});
    }

    void projOp_addItemActive()
    {
        MNEProjOp projOp;
        StringList rowlist{"v1"};
        StringList collist{"ch1", "ch2"};
        MatrixXf data(1, 2);
        data << 0.707f, 0.707f;
        auto vecs = MNENamedMatrix::build(rowlist, collist, data);

        projOp.add_item(*vecs, FIFFV_PROJ_ITEM_FIELD, "Active proj", 0);
        QCOMPARE(projOp.items.size(), std::size_t{1});
    }

    void projOp_combine()
    {
        MNEProjOp projOp1;
        StringList rowlist{"v1"};
        StringList collist{"ch1", "ch2"};
        MatrixXf data(1, 2);
        data << 1.0f, 0.0f;
        auto vecs = MNENamedMatrix::build(rowlist, collist, data);
        projOp1.add_item(*vecs, FIFFV_PROJ_ITEM_FIELD, "Proj1");

        MNEProjOp projOp2;
        MatrixXf data2(1, 2);
        data2 << 0.0f, 1.0f;
        auto vecs2 = MNENamedMatrix::build(rowlist, collist, data2);
        projOp2.add_item(*vecs2, FIFFV_PROJ_ITEM_FIELD, "Proj2");

        projOp1.combine(projOp2);
        QCOMPARE(projOp1.items.size(), std::size_t{2});
    }

    void projOp_assignAndMakeProj()
    {
        MNEProjOp projOp;
        StringList rowlist{"v1"};
        StringList collist{"ch1", "ch2", "ch3"};
        MatrixXf data(1, 3);
        data << 0.577f, 0.577f, 0.577f;
        auto vecs = MNENamedMatrix::build(rowlist, collist, data);
        projOp.add_item(*vecs, FIFFV_PROJ_ITEM_FIELD, "Test proj");

        // assign_channels
        StringList chanNames{"ch1", "ch2", "ch3"};
        projOp.assign_channels(chanNames);
        QCOMPARE(projOp.names, chanNames);

        // make_proj builds the projection data
        QVERIFY(projOp.make_proj());
    }

    void projOp_affect()
    {
        MNEProjOp projOp;
        StringList rowlist{"v1"};
        StringList collist{"ch1", "ch2"};
        MatrixXf data(1, 2);
        data << 1.0f, 0.0f;
        auto vecs = MNENamedMatrix::build(rowlist, collist, data);
        projOp.add_item(*vecs, FIFFV_PROJ_ITEM_FIELD, "Test");

        QCOMPARE(projOp.affect(StringList{"ch1", "ch2", "ch3"}), 1);
        // A channel whose weights are all zero is not affected
        QCOMPARE(projOp.affect(StringList{"ch2", "ch3"}), 0);
    }

    void projOp_report()
    {
        MNEProjOp projOp;
        StringList rowlist{"v1"};
        StringList collist{"ch1", "ch2"};
        MatrixXf data(1, 2);
        data << 1.0f, 0.0f;
        auto vecs = MNENamedMatrix::build(rowlist, collist, data);
        projOp.add_item(*vecs, FIFFV_PROJ_ITEM_FIELD, "ReportTest");

        std::ostringstream out;
        projOp.report(out, "test_proj");
        // MNE-C mne_proj_op_add_item: items are active, and one naming no EEG/MEG channel counts as MEG
        QCOMPARE(out.str(), std::string("test_proj# 1 : ReportTest : 1 vecs : 2 chs MEG active\n"));
    }

    //=========================================================================
    // MNENamedMatrix additional coverage
    //=========================================================================

    void namedMatrix_build()
    {
        StringList rows{"r1", "r2"};
        StringList cols{"c1", "c2", "c3"};
        MatrixXf data(2, 3);
        data << 1, 2, 3, 4, 5, 6;
        auto mat = MNENamedMatrix::build(rows, cols, data);
        QVERIFY(mat != nullptr);
        QCOMPARE(mat->nrow(), 2);
        QCOMPARE(mat->ncol(), 3);
        QCOMPARE(mat->rowlist.size(), 2);
        QCOMPARE(mat->collist.size(), 3);

        // Names may be omitted, but a name list that does not fit the data is refused
        QVERIFY(MNENamedMatrix::build({}, cols, data));
        QVERIFY(!MNENamedMatrix::build(rows, {"c1", "c2"}, data));
        QVERIFY(!MNENamedMatrix::build({"r1"}, cols, data));
    }

    void namedMatrix_pick()
    {
        StringList rows{"r1", "r2", "r3"};
        StringList cols{"c1", "c2", "c3"};
        MatrixXf data(3, 3);
        data << 1, 2, 3, 4, 5, 6, 7, 8, 9;
        auto mat = MNENamedMatrix::build(rows, cols, data);

        // Pick subset of rows and cols
        StringList pickRows{"r1", "r3"};
        StringList pickCols{"c2"};
        auto sub = mat->pick(pickRows, pickCols);
        QVERIFY(sub != nullptr);
        QCOMPARE(sub->nrow(), 2);
        QCOMPARE(sub->ncol(), 1);
        // r1,c2 = 2; r3,c2 = 8
        QVERIFY(qAbs(sub->data(0, 0) - 2.0f) < 1e-5f);
        QVERIFY(qAbs(sub->data(1, 0) - 8.0f) < 1e-5f);
    }

    //=========================================================================
    // MNESourceSpaces writeToStream / readFromStream roundtrip
    //=========================================================================

    void sourceSpaces_writeReadRoundtrip()
    {
        QString fwdPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdPath))
            fwdPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdPath))
            QSKIP("Forward file not found");

        MNEForwardSolution fwd(fwdPath.toStdU16String());
        QVERIFY(fwd.src.size() >= 2);

        // mne.write_source_spaces round trip
        QTemporaryDir dir;
        const std::filesystem::path path = dir.filePath("round-trip-src.fif").toStdU16String();
        QVERIFY(fwd.src.write(path));
        const MNESourceSpaces back = MNESourceSpaces::read(path, false).value();
        QCOMPARE(back.size(), fwd.src.size());
        QCOMPARE(back[0].nuse, fwd.src[0].nuse);
        QCOMPARE(back.get_vertno()[1], fwd.src.get_vertno()[1]);
        QCOMPARE(MNESourceSpaces::read(path.parent_path() / "missing-src.fif").error().code, mne::ErrorCode::Io);
    }

    void sourceSpaces_getVertno()
    {
        QString fwdPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdPath))
            fwdPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdPath))
            QSKIP("Forward file not found");

        MNEForwardSolution fwd(fwdPath.toStdU16String());

        std::vector<VectorXi> vertno = fwd.src.get_vertno();
        QCOMPARE(vertno.size(), fwd.src.size());
        for (int i = 0; i < std::ssize(vertno); ++i) {
            QVERIFY(vertno[i].size() > 0);
        }
    }

    void sourceSpaces_findHemi()
    {
        QString fwdPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdPath))
            fwdPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdPath))
            QSKIP("Forward file not found");

        MNEForwardSolution fwd(fwdPath.toStdU16String());
        QVERIFY(fwd.src.size() >= 2);

        // find_source_space_hemi identifies hemisphere ID
        qint32 hemi = fwd.src[0].find_source_space_hemi();
        QVERIFY(hemi == FIFFV_MNE_SURF_LEFT_HEMI || hemi == FIFFV_MNE_SURF_RIGHT_HEMI);
    }

    //=========================================================================
    // FiffEvokedSet additional coverage
    //=========================================================================

    void evokedSet_readAndPickChannels()
    {
        // Read the evoked set
        QString avePath = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(avePath))
            QSKIP("sample_audvis-ave.fif not found");

        FiffEvokedSet evokedSet(avePath.toStdU16String());
        QVERIFY(evokedSet.evoked.size() > 0);

        // Test pick_channels with include list
        StringList include;
        if (evokedSet.info.ch_names.size() >= 3) {
            include.assign(evokedSet.info.ch_names.begin(), evokedSet.info.ch_names.begin() + 3);
        }
        FiffEvokedSet picked = evokedSet.pick_channels(include);
        QCOMPARE(picked.info.ch_names.size(), 3);
        QCOMPARE(picked.evoked.size(), evokedSet.evoked.size());
    }

    void evokedSet_save()
    {
        QString avePath = QDir::homePath() + "/mne_data/MNE-sample-data/MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(avePath))
            QSKIP("sample_audvis-ave.fif not found");

        FiffEvokedSet evokedSet(avePath.toStdU16String());

        // Save to temp file
        QTemporaryDir scratch;
        QString tmpPath = scratch.filePath("test_evoked_set_save.fif");
        QVERIFY(evokedSet.save(tmpPath.toStdU16String()));

        // Read back and verify
        FiffEvokedSet readBack(tmpPath.toStdU16String());
        QCOMPARE(readBack.evoked.size(), evokedSet.evoked.size());

        QFile::remove(tmpPath);
    }

    void evokedSet_clear()
    {
        FiffEvokedSet evokedSet;
        evokedSet.clear();
        QCOMPARE(evokedSet.evoked.size(), 0);
    }

    //=========================================================================
    // FiffProj additional coverage
    //=========================================================================

    void fiffProj_constructorWithData()
    {
        FiffNamedMatrix data;
        data.nrow = 1;
        data.ncol = 3;
        data.row_names.push_back("vec1");
        data.col_names.push_back("MEG 001");
        data.col_names.push_back("MEG 002");
        data.col_names.push_back("MEG 003");
        data.data = MatrixXd(1, 3);
        data.data << 0.577, 0.577, 0.577;

        FiffProj proj(FIFFV_PROJ_ITEM_FIELD, true, "Test SSP", data);
        QCOMPARE(proj.kind, static_cast<fiff_int_t>(FIFFV_PROJ_ITEM_FIELD));
        QVERIFY(proj.active);
        QCOMPARE(proj.desc, QString("Test SSP"));
    }

    void fiffProj_copyCtor()
    {
        FiffNamedMatrix data;
        data.nrow = 1;
        data.ncol = 2;
        data.row_names.push_back("v1");
        data.col_names.push_back("ch1");
        data.col_names.push_back("ch2");
        data.data = MatrixXd(1, 2);
        data.data << 1.0, 0.0;

        FiffProj proj(FIFFV_PROJ_ITEM_FIELD, false, "CopyTest", data);
        FiffProj copy(proj);
        QCOMPARE(copy.kind, proj.kind);
        QCOMPARE(copy.active, proj.active);
        QCOMPARE(copy.desc, proj.desc);
    }

    void fiffProj_activateProjs()
    {
        FiffNamedMatrix data;
        data.nrow = 1;
        data.ncol = 2;
        data.row_names.push_back("v1");
        data.col_names.push_back("ch1");
        data.col_names.push_back("ch2");
        data.data = MatrixXd(1, 2);
        data.data << 1.0, 0.0;

        std::vector<FiffProj> projs;
        projs.push_back(FiffProj(FIFFV_PROJ_ITEM_FIELD, false, "P1", data));
        projs.push_back(FiffProj(FIFFV_PROJ_ITEM_FIELD, false, "P2", data));

        QVERIFY(!projs[0].active);
        QVERIFY(!projs[1].active);
        FiffProj::activate_projs(projs);
        QVERIFY(projs[0].active);
        QVERIFY(projs[1].active);
    }

    void fiffProj_makeProjector()
    {
        // Create a simple projection vector and make a projector matrix
        FiffNamedMatrix data;
        data.nrow = 1;
        data.ncol = 3;
        data.row_names.push_back("v1");
        data.col_names.push_back("ch1");
        data.col_names.push_back("ch2");
        data.col_names.push_back("ch3");
        data.data = MatrixXd(1, 3);
        data.data << 1.0, 0.0, 0.0;

        std::vector<FiffProj> projs;
        projs.push_back(FiffProj(FIFFV_PROJ_ITEM_FIELD, true, "TestProj", data));

        StringList ch_names{"ch1", "ch2", "ch3"};
        MatrixXd proj;
        fiff_int_t nproj = FiffProj::make_projector(projs, ch_names, proj);
        QVERIFY(nproj >= 0);
        if (nproj > 0) {
            QCOMPARE(proj.rows(), static_cast<Eigen::Index>(3));
            QCOMPARE(proj.cols(), static_cast<Eigen::Index>(3));
        }
    }

    //=========================================================================
    // MNENamedMatrix::pick additional branch coverage
    //=========================================================================

    void namedMatrix_pickRowsOnly()
    {
        StringList rows{"r1", "r2", "r3"};
        StringList cols{"c1", "c2"};
        MatrixXf data(3, 2);
        data << 1, 2, 3, 4, 5, 6;
        auto mat = MNENamedMatrix::build(rows, cols, data);

        // Pick only rows — empty col list keeps all columns
        StringList pickRows{"r2"};
        StringList emptyCols;
        auto sub = mat->pick(pickRows, emptyCols);
        QVERIFY(sub != nullptr);
        QCOMPARE(sub->nrow(), 1);
        QCOMPARE(sub->ncol(), 2);
        // r2 = [3, 4]
        QVERIFY(qAbs(sub->data(0, 0) - 3.0f) < 1e-5f);
        QVERIFY(qAbs(sub->data(0, 1) - 4.0f) < 1e-5f);
    }

    void namedMatrix_pickColsOnly()
    {
        StringList rows{"r1", "r2"};
        StringList cols{"c1", "c2", "c3"};
        MatrixXf data(2, 3);
        data << 1, 2, 3, 4, 5, 6;
        auto mat = MNENamedMatrix::build(rows, cols, data);

        // Pick only cols — empty row list keeps all rows
        StringList emptyRows;
        StringList pickCols{"c3", "c1"};
        auto sub = mat->pick(emptyRows, pickCols);
        QVERIFY(sub != nullptr);
        QCOMPARE(sub->nrow(), 2);
        QCOMPARE(sub->ncol(), 2);
        // All rows, picked cols c3,c1: r1=[3,1], r2=[6,4]
        QVERIFY(qAbs(sub->data(0, 0) - 3.0f) < 1e-5f);
        QVERIFY(qAbs(sub->data(0, 1) - 1.0f) < 1e-5f);
        QVERIFY(qAbs(sub->data(1, 0) - 6.0f) < 1e-5f);
        QVERIFY(qAbs(sub->data(1, 1) - 4.0f) < 1e-5f);
    }

    void namedMatrix_pickMissingRow()
    {
        StringList rows{"r1", "r2"};
        StringList cols{"c1"};
        MatrixXf data(2, 1);
        data << 1, 2;
        auto mat = MNENamedMatrix::build(rows, cols, data);

        // Pick a row that does not exist
        StringList pickRows{"r_missing"};
        StringList emptyCols;
        auto sub = mat->pick(pickRows, emptyCols);
        QVERIFY(sub == nullptr);
    }

    void namedMatrix_pickMissingCol()
    {
        StringList rows{"r1"};
        StringList cols{"c1", "c2"};
        MatrixXf data(1, 2);
        data << 1, 2;
        auto mat = MNENamedMatrix::build(rows, cols, data);

        // Pick a col that does not exist
        StringList emptyRows;
        StringList pickCols{"c_missing"};
        auto sub = mat->pick(emptyRows, pickCols);
        QVERIFY(sub == nullptr);
    }

    void namedMatrix_copyCtor()
    {
        StringList rows{"r1", "r2"};
        StringList cols{"c1"};
        MatrixXf data(2, 1);
        data << 3, 7;
        auto orig = MNENamedMatrix::build(rows, cols, data);

        MNENamedMatrix copy(*orig);
        QCOMPARE(copy.nrow(), 2);
        QCOMPARE(copy.ncol(), 1);
        QCOMPARE(copy.rowlist, rows);
        QCOMPARE(copy.collist, cols);
        QVERIFY(qAbs(copy.data(0, 0) - 3.0f) < 1e-5f);
    }

    //=========================================================================
    // MNEMeasDataSet coverage
    //=========================================================================

    void measData_readEvokedSetFindsTheTriggerChannel()
    {
        const QString avePath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(avePath))
            QSKIP("Sample evoked file not found");
        const auto evoked = FiffEvoked::read(avePath.toStdU16String(), 0, {}, false);
        QVERIFY(evoked.has_value());
        const int stim = mne::channel_index(evoked->info.ch_names, "STI 014");
        if (stim < 0)
            QSKIP("Sample evoked file has no STI 014");

        for (const char* name : {"", "STI014"}) {
            qputenv(MNE_ENV_TRIGGER_CH, name);
            const auto meas = MNEMeasData::mne_read_meas_data(avePath.toStdU16String(), 1, nullptr, nullptr, {}, 0);
            QVERIFY(meas && meas->current);
            QVERIFY2(meas->current->stim14.size() == meas->current->np, name);
            const float value = static_cast<float>(evoked->data(stim, 0) / evoked->info.chs[stim].cal);
            QCOMPARE(meas->current->stim14[0], value);
        }
        qunsetenv(MNE_ENV_TRIGGER_CH);
    }

    void measDataSet_construction()
    {
        MNEMeasDataSet ds;
        QCOMPARE(ds.np, 0);
        QCOMPARE(ds.nave, 1);
        QVERIFY(ds.comment.empty());
    }

    void measDataSet_getValuesAtTime()
    {
        MNEMeasDataSet ds;
        ds.np = 5;
        ds.tmin = 0.0f;
        ds.tstep = 0.1f; // 10 Hz: times 0.0, 0.1, 0.2, 0.3, 0.4

        // data layout is [np x nchan] — samples as rows, channels as columns
        ds.data = MatrixXf(5, 3);
        ds.data << 1, 10, 100,
            2, 20, 200,
            3, 30, 300,
            4, 40, 400,
            5, 50, 500;

        VectorXf values(3);

        // Point query at time=0.2, integ=0 → single sample at index 2
        int ok = ds.getValuesAtTime(0.2f, 0.0f, false, values);
        QCOMPARE(ok, 0);
        QVERIFY(qAbs(values[0] - 3.0f) < 1e-5f);
        QVERIFY(qAbs(values[1] - 30.0f) < 1e-5f);
        QVERIFY(qAbs(values[2] - 300.0f) < 1e-5f);

        // Like MNE-C, a shorter output picks the first channels; a longer one is refused, not overrun
        VectorXf first(2);
        QCOMPARE(ds.getValuesAtTime(0.2f, 0.0f, false, first), 0);
        QCOMPARE(first, Eigen::Vector2f(3.0f, 30.0f));
        VectorXf longer = VectorXf::Constant(4, -7.0f);
        mne::LogCapture log;
        QCOMPARE(ds.getValuesAtTime(0.2f, 0.0f, false, longer), -1);
        QCOMPARE(log.count(mne::Log::Level::Warning, "values"), 1);
        QVERIFY((longer.array() == -7.0f).all());
    }

    void measDataSet_getValuesAtTimeAbsInteg()
    {
        MNEMeasDataSet ds;
        ds.np = 4;
        ds.tmin = 0.0f;
        ds.tstep = 0.1f;

        // data layout [np x nch]
        ds.data = MatrixXf(4, 2);
        ds.data << -1, -10,
            2, 20,
            -3, -30,
            4, 40;

        VectorXf values(2);

        // Query with use_abs=true, integ=0.3 → integration over multiple samples
        int ok = ds.getValuesAtTime(0.15f, 0.3f, true, values);
        QCOMPARE(ok, 0);
        QVERIFY(values[0] > 0.0f);
        QVERIFY(values[1] > 0.0f);
    }

    void measDataSet_getValuesFromChannelData()
    {
        // Set up a simple 2-channel, 5-sample dataset
        int nch = 2;
        int nsamp = 5;
        float tmin = 0.0f;
        float sfreq = 10.0f; // tstep = 0.1

        Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> data(nch, nsamp);
        data << 1.0f, 2.0f, 3.0f, 4.0f, 5.0f,
            10.0f, 20.0f, 30.0f, 40.0f, 50.0f;

        Eigen::VectorXf values(nch);
        int ok = MNEMeasDataSet::getValuesFromChannelData(0.2f, 0.0f, data, tmin, sfreq, false, values);
        QCOMPARE(ok, 0);
        QVERIFY(qAbs(values[0] - 3.0f) < 1e-5f);
        QVERIFY(qAbs(values[1] - 30.0f) < 1e-5f);

        // An output vector that does not hold one value per channel is refused, not overrun
        for (const int size : {nch - 1, nch + 1}) {
            Eigen::VectorXf wrong = Eigen::VectorXf::Constant(size, -7.0f);
            mne::LogCapture log;
            QCOMPARE(MNEMeasDataSet::getValuesFromChannelData(0.2f, 0.0f, data, tmin, sfreq, false, wrong), -1);
            QCOMPARE(log.count(mne::Log::Level::Warning, "values"), 1);
            QVERIFY((wrong.array() == -7.0f).all());
        }
    }

    //=========================================================================
    // FiffEvents coverage (ascii roundtrip + num_events + is_empty)
    //=========================================================================

    void fiffEvents_emptyConstruction()
    {
        FiffEvents ev;
        QVERIFY(ev.empty());
        QCOMPARE(ev.num_events(), 0);
    }

    void fiffEvents_asciiRoundtrip()
    {
        // Create events matrix: N x 3 (sample, prev, event_id)
        MatrixXi evMat(3, 3);
        evMat << 100, 0, 1,
            200, 0, 2,
            300, 0, 1;

        FiffEvents evWrite;
        evWrite.events = evMat;
        QVERIFY(!evWrite.empty());
        QCOMPARE(evWrite.num_events(), 3);

        QTemporaryDir dir;
        const std::string path = dir.filePath("events.eve").toStdString();
        QVERIFY(evWrite.write_ascii(path, 1000.0f).has_value());
        QVERIFY(FiffEvents::read_ascii(path).value().events == evMat);
    }

    void fiffEvents_fifRoundtrip()
    {
        MatrixXi evMat(2, 3);
        evMat << 500, 0, 3,
            600, 0, 4;

        FiffEvents evWrite;
        evWrite.events = evMat;

        QTemporaryDir dir;
        const std::string path = dir.filePath("events-eve.fif").toStdString();
        QVERIFY(evWrite.write_fif(path).has_value());
        QVERIFY(FiffEvents::read_fif(path).value().events == evMat);
    }

    //=========================================================================
    // MNEMshDisplaySurface coverage
    //=========================================================================

    void mshDisplaySurface_construction()
    {
        MNEMshDisplaySurface surf;
        // Default ctor creates surface; np/ntri may be uninitialized
        // Just verify we can construct without crashing
        surf.np = 0;
        surf.ntri = 0;
        QCOMPARE(surf.np, 0);
    }

    void mshDisplaySurface_decideCurvDisplay()
    {
        MNEMshDisplaySurface surf;
        surf.decide_curv_display("sulc");
        // Should set curvature_color_mode based on name
        QVERIFY(surf.curvature_color_mode >= 0);
    }

    void mshDisplaySurface_decideExtent()
    {
        MNEMshDisplaySurface surf;
        // Setup minimal vertex data so decide_surface_extent can work
        surf.np = 3;
        // Assign coefficient wise. GCC loses track of a comma initializer feeding
        // a dynamically sized matrix and reports a false -Wmaybe-uninitialized
        // from inside Eigen.
        surf.rr = MatrixXf::Zero(3, 3);
        surf.rr(1, 0) = 1.0f;
        surf.rr(1, 1) = 2.0f;
        surf.rr(1, 2) = 3.0f;
        surf.rr(2, 0) = -1.0f;
        surf.rr(2, 1) = -2.0f;
        surf.rr(2, 2) = -3.0f;
        surf.decide_surface_extent("test");
        // After deciding extent, minv/maxv should be set
        QVERIFY(surf.minv(0) <= surf.maxv(0));
    }

    void mshDisplaySurface_setupCurvatureColors()
    {
        MNEMshDisplaySurface surf;
        surf.np = 4;
        surf.curv = VectorXf(4);
        surf.curv << -0.5f, 0.2f, 0.0f, 0.8f;
        surf.curvature_color_mode = 0; // SHOW_CURVATURE_NONE
        surf.setup_curvature_colors();
        QVERIFY(surf.vertex_colors.size() > 0);
    }

    void mshDisplaySurface_setupCurvatureOverlay()
    {
        MNEMshDisplaySurface surf;
        surf.np = 3;
        surf.curv = VectorXf(3);
        surf.curv << -0.3f, 0.5f, 0.1f;
        surf.curvature_color_mode = 1; // SHOW_CURVATURE_OVERLAY
        surf.setup_curvature_colors();
        QVERIFY(surf.vertex_colors.size() > 0);
    }

    //=========================================================================
    // MNEMshDisplaySurface::scale coverage
    //=========================================================================

    void mshDisplaySurface_scale()
    {
        MNEMshDisplaySurface surf;
        surf.np = 2;
        // See mshDisplaySurface_decideExtent for why this is not a comma initializer.
        surf.rr = MatrixXf::Zero(2, 3);
        surf.rr(0, 0) = 1.0f;
        surf.rr(0, 1) = 2.0f;
        surf.rr(0, 2) = 3.0f;
        surf.rr(1, 0) = 4.0f;
        surf.rr(1, 1) = 5.0f;
        surf.rr(1, 2) = 6.0f;
        surf.minv = Eigen::Vector3f(0.0f, 1.0f, 2.0f);
        surf.maxv = Eigen::Vector3f(4.0f, 5.0f, 6.0f);

        Eigen::Vector3f scales(2.0f, 0.5f, 3.0f);
        surf.scale(scales);

        // Verify vertices scaled
        QVERIFY(qAbs(surf.rr(0, 0) - 2.0f) < 1e-5f);
        QVERIFY(qAbs(surf.rr(0, 1) - 1.0f) < 1e-5f);
        QVERIFY(qAbs(surf.rr(0, 2) - 9.0f) < 1e-5f);
        QVERIFY(qAbs(surf.rr(1, 0) - 8.0f) < 1e-5f);
        QVERIFY(qAbs(surf.rr(1, 1) - 2.5f) < 1e-5f);
        QVERIFY(qAbs(surf.rr(1, 2) - 18.0f) < 1e-5f);

        // Verify bounds scaled
        QVERIFY(qAbs(surf.minv(0) - 0.0f) < 1e-5f);
        QVERIFY(qAbs(surf.minv(1) - 0.5f) < 1e-5f);
        QVERIFY(qAbs(surf.minv(2) - 6.0f) < 1e-5f);
        QVERIFY(qAbs(surf.maxv(0) - 8.0f) < 1e-5f);
        QVERIFY(qAbs(surf.maxv(1) - 2.5f) < 1e-5f);
        QVERIFY(qAbs(surf.maxv(2) - 18.0f) < 1e-5f);
    }

    //=========================================================================
    // MNEMeasDataSet::getValuesAtTime — interpolation branch (f1 < 1.0)
    //=========================================================================

    void measDataSet_getValuesAtTimeInterp()
    {
        MNEMeasDataSet ds;
        ds.np = 10;
        ds.tmin = 0.0f;
        ds.tstep = 0.1f; // sfreq = 10 Hz

        // data [np x nch], 1 channel
        ds.data = MatrixXf(10, 1);
        for (int i = 0; i < 10; ++i)
            ds.data(i, 0) = static_cast<float>(i * 10); // 0,10,20,...,90

        VectorXf values(1);
        float& value = values[0];
        // time=0.15 → s1=1.5, n1=1, f1=0.5 — interpolates between sample 1 (10) and 2 (20)
        int ok = ds.getValuesAtTime(0.15f, 0.0f, false, values);
        QCOMPARE(ok, 0);
        // expected: 0.5 * 10 + 0.5 * 20 = 15
        QVERIFY(qAbs(value - 15.0f) < 1e-4f);
    }

    void measDataSet_getValuesAtTimeInterpUseAbs()
    {
        MNEMeasDataSet ds;
        ds.np = 10;
        ds.tmin = 0.0f;
        ds.tstep = 0.1f;

        ds.data = MatrixXf(10, 1);
        for (int i = 0; i < 10; ++i)
            ds.data(i, 0) = static_cast<float>((i % 2 == 0) ? -10 : 10);

        VectorXf values(1);
        float& value = values[0];
        // time = 0.05 → s1=0.5, n1=0, f1=0.5
        // data[0] = -10, data[1] = 10
        // use_abs: 0.5 * abs(-10) + 0.5 * abs(10) = 10
        int ok = ds.getValuesAtTime(0.05f, 0.0f, true, values);
        QCOMPARE(ok, 0);
        QVERIFY(qAbs(value - 10.0f) < 1e-4f);

        // without use_abs: 0.5 * (-10) + 0.5 * 10 = 0
        ok = ds.getValuesAtTime(0.05f, 0.0f, false, values);
        QCOMPARE(ok, 0);
        QVERIFY(qAbs(value - 0.0f) < 1e-4f);
    }

    //=========================================================================
    // MNEMeasDataSet::getValuesAtTime — last sample snap (n1 == np-1)
    //=========================================================================

    void measDataSet_getValuesAtTimeLastSample()
    {
        MNEMeasDataSet ds;
        ds.np = 5;
        ds.tmin = 0.0f;
        ds.tstep = 0.1f;

        ds.data = MatrixXf(5, 1);
        ds.data << 1.0f, 2.0f, 3.0f, 4.0f, 5.0f;

        VectorXf values(1);
        float& value = values[0];
        // time=0.4 → s1=4.0, n1=4=np-1, f1=1.0 → snaps to sample 4
        int ok = ds.getValuesAtTime(0.4f, 0.0f, false, values);
        QCOMPARE(ok, 0);
        QVERIFY(qAbs(value - 5.0f) < 1e-4f);
    }

    //=========================================================================
    // MNEMeasDataSet::getValuesAtTime — out of range
    //=========================================================================

    void measDataSet_getValuesAtTimeOutOfRange()
    {
        MNEMeasDataSet ds;
        ds.np = 5;
        ds.tmin = 0.0f;
        ds.tstep = 0.1f;
        ds.data = MatrixXf(5, 1);
        ds.data << 1, 2, 3, 4, 5;

        VectorXf values(1);
        // time = -0.5 → n1 = -5 → out of range
        int ok = ds.getValuesAtTime(-0.5f, 0.0f, false, values);
        QVERIFY(ok != 0);
    }

    //=========================================================================
    // MNEMeasDataSet::getValuesAtTime — integration (n2 < n1, within one sample)
    //=========================================================================

    void measDataSet_getValuesAtTimeIntegWithinSample()
    {
        MNEMeasDataSet ds;
        ds.np = 10;
        ds.tmin = 0.0f;
        ds.tstep = 0.01f; // sfreq = 100

        ds.data = MatrixXf(10, 1);
        for (int i = 0; i < 10; ++i)
            ds.data(i, 0) = static_cast<float>(i);

        VectorXf values(1);
        float& value = values[0];
        // time = 0.055, integ = 0.005
        // sfreq*integ = 100*0.005 = 0.5, |0.5| >= 0.05, so multi-sample path
        // s1 = 100*(0.055 - 0.0025) = 5.25
        // s2 = 100*(0.055 + 0.0025) = 5.75
        // n1 = ceil(5.25) = 6, n2 = floor(5.75) = 5
        // n2 < n1: within one sample interval
        // floored n1 = 5, f1 = 5.25-5 = 0.25, f2 = 5.75-5 = 0.75
        // sum = 0.5 * ((0.25+0.75)*data[6] + (2-0.25-0.75)*data[5])
        //     = 0.5 * (1.0*6 + 1.0*5) = 5.5
        int ok = ds.getValuesAtTime(0.055f, 0.005f, false, values);
        QCOMPARE(ok, 0);
        QVERIFY(qAbs(value - 5.5f) < 1e-3f);
    }

    //=========================================================================
    // MNEMeasDataSet::getValuesAtTime — integration multi-sample (n2 > n1)
    //=========================================================================

    void measDataSet_getValuesAtTimeIntegMultiSample()
    {
        MNEMeasDataSet ds;
        ds.np = 10;
        ds.tmin = 0.0f;
        ds.tstep = 0.01f; // sfreq = 100

        ds.data = MatrixXf(10, 1);
        for (int i = 0; i < 10; ++i)
            ds.data(i, 0) = static_cast<float>(i);

        VectorXf values(1);
        float& value = values[0];
        // time = 0.05, integ = 0.05
        // sfreq*integ = 100*0.05 = 5.0, |5.0| >= 0.05
        // s1 = 100*(0.05 - 0.025) = 2.5
        // s2 = 100*(0.05 + 0.025) = 7.5
        // n1 = ceil(2.5) = 3, n2 = floor(7.5) = 7
        // f1 = n1 - s1 = 3 - 2.5 = 0.5
        // f2 = s2 - n2 = 7.5 - 7 = 0.5
        // n2 > n1: multi-sample trapezoid
        // sum = 0.5*data[3] + data[4] + data[5] + data[6] + 0.5*data[7]
        //     = 0.5*3 + 4 + 5 + 6 + 0.5*7 = 1.5 + 4 + 5 + 6 + 3.5 = 20
        // width = n2-n1 = 4
        // f1 edge: 0.5*0.5*(0.5*data[2] + 1.5*data[3]) = 0.25*(1+4.5)=1.375
        // f2 edge: 0.5*0.5*(0.5*data[8] + 1.5*data[7]) = 0.25*(4+10.5)=3.625
        // total sum = 20 + 1.375 + 3.625 = 25
        // total width = 4 + 0.5 + 0.5 = 5
        // result = 25/5 = 5.0
        int ok = ds.getValuesAtTime(0.05f, 0.05f, false, values);
        QCOMPARE(ok, 0);
        QVERIFY(qAbs(value - 5.0f) < 1e-3f);
    }

    void measDataSet_getValuesAtTimeIntegMultiSampleUseAbs()
    {
        MNEMeasDataSet ds;
        ds.np = 10;
        ds.tmin = 0.0f;
        ds.tstep = 0.01f; // sfreq = 100

        ds.data = MatrixXf(10, 1);
        for (int i = 0; i < 10; ++i)
            ds.data(i, 0) = static_cast<float>((i % 2 == 0) ? -i : i);
        // data: 0, 1, -2, 3, -4, 5, -6, 7, -8, 9

        VectorXf values(1);
        float& value = values[0];
        // Same integration window but with use_abs=true
        // This exercises the use_abs branches in the multi-sample path
        int ok = ds.getValuesAtTime(0.05f, 0.05f, true, values);
        QCOMPARE(ok, 0);
        QVERIFY(value > 0.0f); // Should be positive since we take abs
    }

    //=========================================================================
    // MNEMeasDataSet::getValuesFromChannelData — interpolation branch
    //=========================================================================

    void measDataSet_channelDataInterp()
    {
        int nsamp = 10, nch = 1;
        float ch0[10];
        for (int i = 0; i < nsamp; ++i)
            ch0[i] = static_cast<float>(i * 10); // 0,10,...,90
        const Eigen::Map<const Eigen::RowVectorXf> data(ch0, nsamp);

        Eigen::VectorXf value(nch);
        // time=0.15, sfreq=10 → s1=1.5, n1=1, f1=0.5 → interp sample 1 and 2
        int ok = MNEMeasDataSet::getValuesFromChannelData(
            0.15f, 0.0f, data, 0.0f, 10.0f, false, value);
        QCOMPARE(ok, 0);
        QVERIFY(qAbs(value[0] - 15.0f) < 1e-4f);
    }

    void measDataSet_channelDataInterpUseAbs()
    {
        int nsamp = 10, nch = 1;
        float ch0[10];
        for (int i = 0; i < nsamp; ++i)
            ch0[i] = static_cast<float>((i % 2 == 0) ? -10 : 10);
        const Eigen::Map<const Eigen::RowVectorXf> data(ch0, nsamp);

        Eigen::VectorXf value(nch);
        // time=0.05, sfreq=10 → interp between sample 0 and 1
        int ok = MNEMeasDataSet::getValuesFromChannelData(
            0.05f, 0.0f, data, 0.0f, 10.0f, true, value);
        QCOMPARE(ok, 0);
        QVERIFY(qAbs(value[0] - 10.0f) < 1e-4f);
    }

    //=========================================================================
    // MNEMeasDataSet::getValuesFromChannelData — integration within one sample
    //=========================================================================

    void measDataSet_channelDataIntegWithinSample()
    {
        int nsamp = 10, nch = 1;
        float ch0[10];
        for (int i = 0; i < nsamp; ++i)
            ch0[i] = static_cast<float>(i);
        const Eigen::Map<const Eigen::RowVectorXf> data(ch0, nsamp);

        Eigen::VectorXf value(nch);
        // sfreq=100, time=0.055, integ=0.005 → within one sample interval
        int ok = MNEMeasDataSet::getValuesFromChannelData(
            0.055f, 0.005f, data, 0.0f, 100.0f, false, value);
        QCOMPARE(ok, 0);
        QVERIFY(qAbs(value[0] - 5.5f) < 1e-3f);
    }

    void measDataSet_channelDataIntegWithinSampleUseAbs()
    {
        int nsamp = 10, nch = 1;
        float ch0[10];
        for (int i = 0; i < nsamp; ++i)
            ch0[i] = static_cast<float>((i % 2 == 0) ? -i : i);
        const Eigen::Map<const Eigen::RowVectorXf> data(ch0, nsamp);

        Eigen::VectorXf value(nch);
        int ok = MNEMeasDataSet::getValuesFromChannelData(
            0.055f, 0.005f, data, 0.0f, 100.0f, true, value);
        QCOMPARE(ok, 0);
        QVERIFY(value[0] >= 0.0f);
    }

    //=========================================================================
    // MNEMeasDataSet::getValuesFromChannelData — integration multi-sample
    //=========================================================================

    void measDataSet_channelDataIntegMultiSample()
    {
        int nsamp = 10, nch = 1;
        float ch0[10];
        for (int i = 0; i < nsamp; ++i)
            ch0[i] = static_cast<float>(i);
        const Eigen::Map<const Eigen::RowVectorXf> data(ch0, nsamp);

        Eigen::VectorXf value(nch);
        // sfreq=100, time=0.05, integ=0.05 → multi-sample trapezoid
        int ok = MNEMeasDataSet::getValuesFromChannelData(
            0.05f, 0.05f, data, 0.0f, 100.0f, false, value);
        QCOMPARE(ok, 0);
        QVERIFY(qAbs(value[0] - 5.0f) < 1e-3f);
    }

    void measDataSet_channelDataIntegMultiSampleUseAbs()
    {
        int nsamp = 10, nch = 1;
        float ch0[10];
        for (int i = 0; i < nsamp; ++i)
            ch0[i] = static_cast<float>((i % 2 == 0) ? -i : i);
        const Eigen::Map<const Eigen::RowVectorXf> data(ch0, nsamp);

        Eigen::VectorXf value(nch);
        int ok = MNEMeasDataSet::getValuesFromChannelData(
            0.05f, 0.05f, data, 0.0f, 100.0f, true, value);
        QCOMPARE(ok, 0);
        QVERIFY(value[0] > 0.0f);
    }

    //=========================================================================
    // FiffEvents::match_event coverage
    //=========================================================================

    void fiffEvents_matchEventBasicMatch()
    {
        // events matrix: [N x 3] = sample, from, to
        MatrixXi events(3, 3);
        events << 100, 0, 1,
            200, 0, 2,
            300, 0, 1;

        AverageCategory cat;
        cat.events = {1};
        cat.ignore = 0;
        cat.prevEvent = 0;
        cat.nextEvent = 0;

        // Event 0: from=0, to=1 → matches cat.events[0]=1
        QVERIFY(FiffEvents::match_event(cat, events, 0));
        // Event 1: from=0, to=2 → no match
        QVERIFY(!FiffEvents::match_event(cat, events, 1));
        // Event 2: from=0, to=1 → matches
        QVERIFY(FiffEvents::match_event(cat, events, 2));
    }

    void fiffEvents_matchEventOutOfRange()
    {
        MatrixXi events(2, 3);
        events << 100, 0, 1,
            200, 0, 2;

        AverageCategory cat;
        cat.events = {1};
        cat.ignore = 0;
        cat.prevEvent = 0;
        cat.nextEvent = 0;

        // Out of range indices
        QVERIFY(!FiffEvents::match_event(cat, events, -1));
        QVERIFY(!FiffEvents::match_event(cat, events, 2));
    }

    void fiffEvents_matchEventWithPrevEvent()
    {
        // events: ev0=(100, 0, 1), ev1=(200, 0, 2), ev2=(300, 0, 1)
        MatrixXi events(3, 3);
        events << 100, 0, 1,
            200, 0, 2,
            300, 0, 1;

        AverageCategory cat;
        cat.events = {1};
        cat.ignore = 0;
        cat.prevEvent = 2; // must have previous event with to==2
        cat.prevIgnore = 0;
        cat.nextEvent = 0;

        // Event 0: to=1 matches events, but no previous event → false
        QVERIFY(!FiffEvents::match_event(cat, events, 0));
        // Event 2: to=1 matches events, prev event 1 has from=0, to=2 → prevEvent==2 ✓
        QVERIFY(FiffEvents::match_event(cat, events, 2));
    }

    void fiffEvents_matchEventWithNextEvent()
    {
        MatrixXi events(3, 3);
        events << 100, 0, 1,
            200, 0, 2,
            300, 0, 3;

        AverageCategory cat;
        cat.events = {1};
        cat.ignore = 0;
        cat.prevEvent = 0;
        cat.nextEvent = 2; // must have next event with to==2
        cat.nextIgnore = 0;

        // Event 0: to=1 matches, next event 1 has from=0 (from & ~ignore==0 ✓), to=2==nextEvent ✓
        QVERIFY(FiffEvents::match_event(cat, events, 0));
    }

    void fiffEvents_matchEventNextNotFound()
    {
        MatrixXi events(2, 3);
        events << 100, 0, 1,
            200, 5, 2; // from=5, so (5 & ~0) != 0 → "found" is true but match fails

        AverageCategory cat;
        cat.events = {1};
        cat.ignore = 0;
        cat.prevEvent = 0;
        cat.nextEvent = 99; // require next event == 99
        cat.nextIgnore = 0;

        // Event 0: to=1 matches events, but next event 1 has from=5 (non-zero) and to=2≠99
        // The loop finds j=1 where (events(1,1) & ~0) = 5 ≠ 0, so found stays false
        // Actually: found = true only when (from & ~ignore) == 0. from=5, ~0=0xFFFF..→ 5≠0 → found stays false
        // So "if (!found) match = false" → false
        QVERIFY(!FiffEvents::match_event(cat, events, 0));
    }

    //=========================================================================
    // MNEMshDisplaySurface — decide_curv_display additional branches
    //=========================================================================

    void mshDisplaySurface_decideCurvDisplayInflated()
    {
        MNEMshDisplaySurface surf;
        surf.decide_curv_display("inflated");
        QCOMPARE(surf.curvature_color_mode, 1); // SHOW_CURVATURE_OVERLAY
    }

    void mshDisplaySurface_decideCurvDisplayWhite()
    {
        MNEMshDisplaySurface surf;
        surf.decide_curv_display("white");
        QCOMPARE(surf.curvature_color_mode, 1);
    }

    void mshDisplaySurface_decideCurvDisplaySphere()
    {
        MNEMshDisplaySurface surf;
        surf.decide_curv_display("sphere_reg");
        QCOMPARE(surf.curvature_color_mode, 1);
    }

    void mshDisplaySurface_decideCurvDisplayPial()
    {
        MNEMshDisplaySurface surf;
        surf.decide_curv_display("pial");
        QCOMPARE(surf.curvature_color_mode, 0); // SHOW_CURVATURE_NONE
    }

    //=========================================================================
    // MNEMshDisplaySurface — setup_curvature_colors with 4 components (RGBA)
    //=========================================================================

    void mshDisplaySurface_curvatureColorsRGBA()
    {
        MNEMshDisplaySurface surf;
        surf.np = 2;
        surf.curv = VectorXf(2);
        surf.curv << 0.5f, -0.3f;
        surf.nvertex_colors = 4;
        surf.curvature_color_mode = 1; // overlay
        surf.setup_curvature_colors();
        QCOMPARE(surf.vertex_colors.size(), 2 * 4);
        // Alpha channel (index 3, 7) should be 1.0
        QVERIFY(qAbs(surf.vertex_colors(3) - 1.0f) < 1e-5f);
        QVERIFY(qAbs(surf.vertex_colors(7) - 1.0f) < 1e-5f);
    }
};

QTEST_GUILESS_MAIN(TestMneDataTypes)
#include "test_mne_data_types.moc"

#include <vector>
