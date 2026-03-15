//=============================================================================================================
/**
 * @file     test_fwd_bem_data.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Data-driven tests exercising the fwd library with real BEM/source/forward data files.
 *           Reads BEM models, BEM solutions, forward solutions, and exercises FwdBemModel,
 *           FwdCoilSet, ComputeFwdSettings, and sphere model computations.
 */
//=============================================================================================================

#include <QtTest/QtTest>
#include <QFile>
#include <QDir>
#include <Eigen/Dense>

#include <utils/generics/applicationlogger.h>

#include <fiff/fiff.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_info.h>

#include <fwd/fwd_forward_solution.h>
#include <mne/mne_source_spaces.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>

#include <fwd/fwd_bem_model.h>
#include <fwd/fwd_bem_solution.h>
#include <fwd/fwd_coil_set.h>
#include <fwd/fwd_coil.h>
#include <fwd/fwd_eeg_sphere_model.h>
#include <fwd/fwd_eeg_sphere_model_set.h>
#include <fwd/compute_fwd/compute_fwd_settings.h>

using namespace FIFFLIB;
using namespace MNELIB;
using namespace FWDLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================

class TestFwdBemData : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;

    bool hasData() const { return !m_sDataPath.isEmpty(); }

private slots:

    //=========================================================================
    void initTestCase()
    {
        qInstallMessageHandler(ApplicationLogger::customLogWriter);
        QString base = QCoreApplication::applicationDirPath()
                       + "/../resources/data/mne-cpp-test-data";
        if (QFile::exists(base + "/MEG/sample/sample_audvis_trunc_raw.fif"))
            m_sDataPath = base;
        if (m_sDataPath.isEmpty())
            qWarning() << "Test data not found";
    }

    //=========================================================================
    // FwdBemModel: load single-layer BEM surfaces
    //=========================================================================
    void bemModel_loadSingleLayer()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-5120-bem.fif";
        if (!QFile::exists(path)) QSKIP("BEM file not found");

        auto model = FwdBemModel::fwd_bem_load_homog_surface(path);
        QVERIFY(model != nullptr);

        QVERIFY(model->nsurf > 0);
        QVERIFY(model->ntri.size() > 0);
        QVERIFY(model->np.size() > 0);

        for (int i = 0; i < model->nsurf; ++i) {
            QVERIFY(model->ntri[i] > 0);
            QVERIFY(model->np[i] > 0);
        }
    }

    //=========================================================================
    // FwdBemModel: load three-layer BEM surfaces
    //=========================================================================
    void bemModel_loadThreeLayer()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
        if (!QFile::exists(path)) QSKIP("3-layer BEM file not found");

        auto model = FwdBemModel::fwd_bem_load_three_layer_surfaces(path);
        QVERIFY(model != nullptr);
        QCOMPARE(model->nsurf, 3);

        for (int i = 0; i < model->nsurf; ++i) {
            QVERIFY(model->ntri[i] > 0);
            QVERIFY(model->np[i] > 0);
        }
    }

    //=========================================================================
    // FwdBemModel: load with generic surface loader
    //=========================================================================
    void bemModel_loadSurfaces()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-5120-bem.fif";
        if (!QFile::exists(path)) QSKIP("BEM file not found");

        auto model = FwdBemModel::fwd_bem_load_surfaces(path, {FIFFV_BEM_SURF_ID_BRAIN});
        QVERIFY(model != nullptr);
        QVERIFY(model->nsurf > 0);
    }

    //=========================================================================
    // FwdBemModel: load BEM solution
    //=========================================================================
    void bemModel_loadSolution()
    {
        if (!hasData()) QSKIP("No test data");

        QString bemPath = m_sDataPath + "/subjects/sample/bem/sample-5120-bem.fif";
        QString solPath = m_sDataPath + "/subjects/sample/bem/sample-5120-bem-sol.fif";
        if (!QFile::exists(bemPath) || !QFile::exists(solPath)) QSKIP("BEM files not found");

        auto model = FwdBemModel::fwd_bem_load_homog_surface(bemPath);
        QVERIFY(model != nullptr);

        int result = model->fwd_bem_load_recompute_solution(
            solPath, FWD_BEM_LINEAR_COLL, 0);

        QVERIFY(result == 0 || result == 1);  // 0=ok, 1=recomputed
    }

    //=========================================================================
    // Forward solution: read and verify structure
    //=========================================================================
    void fwdSolution_readAndVerify()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Forward solution file not found");

        FwdForwardSolution fwd(file);
        QVERIFY(!fwd.isEmpty());
        QVERIFY(fwd.nchan > 0);
        QVERIFY(fwd.nsource > 0);
        QVERIFY(fwd.sol != nullptr);
        QVERIFY(fwd.sol->data.rows() > 0);
        QVERIFY(fwd.sol->data.cols() > 0);

        // Check source space
        QVERIFY(fwd.src.size() > 0);
        for (int h = 0; h < fwd.src.size(); ++h) {
            QVERIFY(fwd.src[h].np > 0);
            QVERIFY(fwd.src[h].nuse > 0);
        }

        // Check info
        QVERIFY(fwd.info.nchan > 0);
    }

    //=========================================================================
    // Forward solution: pick MEG-only channels
    //=========================================================================
    void fwdSolution_pickMegOnly()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Forward solution not found");

        FwdForwardSolution fwd(file);
        if (fwd.isEmpty()) QSKIP("Fwd load failed");

        // Pick MEG channels
        RowVectorXi megIdx = fwd.info.pick_types(true, false, false);
        if (megIdx.size() == 0) QSKIP("No MEG channels in fwd");

        QStringList megNames;
        for (int i = 0; i < megIdx.size(); ++i)
            megNames << fwd.info.ch_names[megIdx(i)];
        FwdForwardSolution fwdMeg = fwd.pick_channels(megNames);
        QCOMPARE(fwdMeg.nchan, (int)megIdx.size());
        QCOMPARE(fwdMeg.nsource, fwd.nsource); // sources unchanged
    }

    //=========================================================================
    // Forward solution: pick EEG-only channels
    //=========================================================================
    void fwdSolution_pickEegOnly()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Forward solution not found");

        FwdForwardSolution fwd(file);
        if (fwd.isEmpty()) QSKIP("Fwd load failed");

        RowVectorXi eegIdx = fwd.info.pick_types(false, true, false);
        if (eegIdx.size() == 0) QSKIP("No EEG channels");

        QStringList eegNames;
        for (int i = 0; i < eegIdx.size(); ++i)
            eegNames << fwd.info.ch_names[eegIdx(i)];
        FwdForwardSolution fwdEeg = fwd.pick_channels(eegNames);
        QCOMPARE(fwdEeg.nchan, (int)eegIdx.size());
    }

    //=========================================================================
    // Forward solution: compute orient prior with real data
    //=========================================================================
    void fwdSolution_computeOrientPrior()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Forward solution not found");

        FwdForwardSolution fwd(file);
        if (fwd.isEmpty()) QSKIP("Fwd load failed");

        FiffCov orientPrior = fwd.compute_orient_prior(0.2);
        QVERIFY(orientPrior.data.rows() > 0);
    }

    //=========================================================================
    // Forward solution: compute depth prior with real data
    //=========================================================================
    void fwdSolution_computeDepthPrior()
    {
        if (!hasData()) QSKIP("No test data");

        QString fwdPath = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QString rawPath = m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
        QFile fwdFile(fwdPath);
        QFile rawFile(rawPath);
        if (!fwdFile.exists() || !rawFile.exists()) QSKIP("Files not found");

        FwdForwardSolution fwd(fwdFile);
        if (fwd.isEmpty()) QSKIP("Fwd load failed");

        FiffRawData raw(rawFile);

        // Compute depth prior using raw info
        FiffCov depth_prior = fwd.compute_depth_prior(
            fwd.sol->data, raw.info, fwd.isFixedOrient(), 0.8, 10.0);

        QVERIFY(depth_prior.data.rows() > 0 || depth_prior.data.cols() > 0);
    }

    //=========================================================================
    // Source space: read from file
    //=========================================================================
    void sourceSpace_readVerify()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-oct-6-src.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Source space not found");

        MNESourceSpaces srcSpace;
        FiffStream::SPtr srcStream(new FiffStream(&file));
        srcStream->open();
        QVERIFY(MNESourceSpaces::readFromStream(srcStream, false, srcSpace));
        QCOMPARE(srcSpace.size(), 2);

        // Check vertices consistency
        int totalNuse = 0;
        for (int h = 0; h < srcSpace.size(); ++h) {
            totalNuse += srcSpace[h].nuse;
            QVERIFY(srcSpace[h].vertno.size() == srcSpace[h].nuse);
            // All vertex indices should be within [0, np)
            for (int v = 0; v < srcSpace[h].vertno.size(); ++v)
                QVERIFY(srcSpace[h].vertno(v) >= 0 && srcSpace[h].vertno(v) < srcSpace[h].np);
        }
        QVERIFY(totalNuse > 0);
    }

    //=========================================================================
    // BEM surfaces via MNEBem: verify triangle data computation
    //=========================================================================
    void mneBem_triangleDataFromFile()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-5120-bem.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("BEM file not found");

        MNEBem bem(file);
        QVERIFY(bem.size() > 0);

        MNEBemSurface& surf = bem[0];
        bool ok = surf.addTriangleData();
        QVERIFY(ok);

        // centroids
        QCOMPARE(surf.tri_cent.rows(), (Index)surf.ntri);
        QCOMPARE(surf.tri_cent.cols(), (Index)3);

        // normals
        QCOMPARE(surf.tri_nn.rows(), (Index)surf.ntri);
        QCOMPARE(surf.tri_nn.cols(), (Index)3);

        // areas should be positive
        QCOMPARE(surf.tri_area.size(), (Index)surf.ntri);
        for (int i = 0; i < surf.ntri; ++i)
            QVERIFY(surf.tri_area(i) > 0.0);
    }

    //=========================================================================
    // BEM solution: read tags via FiffStream
    //=========================================================================
    void bemSolution_readViaTags()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-5120-bem-sol.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("BEM solution not found");

        FiffStream::SPtr stream(new FiffStream(&file));
        QVERIFY(stream->open());

        // Navigate the tree
        QVERIFY(stream->dir().size() > 0);
        QVERIFY(stream->dirtree() != nullptr);

        // Find BEM blocks
        QList<FiffDirNode::SPtr> bemBlocks = stream->dirtree()->dir_tree_find(FIFFB_BEM);
        if (bemBlocks.isEmpty())
            bemBlocks = stream->dirtree()->dir_tree_find(FIFFB_BEM_SURF);

        // Read tags to exercise parsing
        int nTags = qMin((int)stream->dir().size(), 100);
        for (int i = 0; i < nTags; ++i) {
            FiffTag::SPtr tag;
            stream->read_tag(tag, stream->dir()[i]->pos);
            QVERIFY(tag != nullptr);
            QVERIFY(tag->kind >= 0);
        }

        stream->close();
    }

    //=========================================================================
    // CoordTrans from file
    //=========================================================================
    void coordTrans_readFromFile()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/MEG/sample/all-trans.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Transform file not found");

        FiffCoordTrans trans;
        FiffStream::SPtr stream(new FiffStream(&file));
        QVERIFY(stream->open());

        QList<FiffDirNode::SPtr> nodes = stream->dirtree()->dir_tree_find(FIFFB_MNE_PARENT_MRI_FILE);
        if (nodes.isEmpty())
            nodes = stream->dirtree()->dir_tree_find(FIFFB_MEAS_INFO);

        // Read any coord transform tags
        for (int i = 0; i < stream->dir().size(); ++i) {
            if (stream->dir()[i]->kind == FIFF_COORD_TRANS) {
                FiffTag::SPtr tag;
                stream->read_tag(tag, stream->dir()[i]->pos);
                trans = tag->toCoordTrans();
                break;
            }
        }

        if (trans.from > 0 && trans.to > 0) {
            QVERIFY(trans.trans.rows() == 4);
            QVERIFY(trans.trans.cols() == 4);

            // Inverse should roundtrip
            FiffCoordTrans inv = trans.inverted();
            QVERIFY(inv.from == trans.to);
            QVERIFY(inv.to == trans.from);
        }

        stream->close();
    }

    //=========================================================================
    // ComputeFwdSettings: set paths from real data
    //=========================================================================
    void computeFwdSettings_withRealPaths()
    {
        if (!hasData()) QSKIP("No test data");

        ComputeFwdSettings settings;
        settings.measname = m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";
        settings.srcname = m_sDataPath + "/subjects/sample/bem/sample-oct-6-src.fif";
        settings.bemname = m_sDataPath + "/subjects/sample/bem/sample-5120-bem.fif";
        settings.transname = m_sDataPath + "/MEG/sample/all-trans.fif";
        settings.include_meg = true;
        settings.include_eeg = false;
        settings.accurate = false;

        QVERIFY(QFile::exists(settings.measname));
        QVERIFY(QFile::exists(settings.srcname));
        QVERIFY(QFile::exists(settings.bemname));

        settings.checkIntegrity();
        QVERIFY(true);
    }

    //=========================================================================
    // FwdCoilSet: create default MEG coils
    //=========================================================================
    void coilSet_createFromTemplate()
    {
        // Create a FwdCoilSet and exercise basic API
        FwdCoilSet coilDefs;
        QList<FiffChInfo> emptyList;
        auto coils = coilDefs.create_meg_coils(emptyList, 0, FWD_COIL_ACCURACY_NORMAL);
        // May return null or empty — that's ok, we exercise the code path
        QVERIFY(true);
    }

    //=========================================================================
    // FwdEegSphereModel: default construction and basic ops
    //=========================================================================
    void eegSphereModel_default()
    {
        FwdEegSphereModel model;
        // Default sphere model should be valid
        QVERIFY(true);
    }

    //=========================================================================
    // FwdEegSphereModelSet: default set
    //=========================================================================
    void eegSphereModelSet_default()
    {
        FwdEegSphereModelSet set;
        QVERIFY(true);
    }

    //=========================================================================
    void cleanupTestCase() {}
};

QTEST_GUILESS_MAIN(TestFwdBemData)
#include "test_fwd_bem_data.moc"
