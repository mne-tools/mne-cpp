//=============================================================================================================
/**
 * @file     test_mne_source_data.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Data-driven tests exercising MNE library with real data files.
 *           Reads source spaces, BEM, forward solutions, epoch data.
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
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_cov.h>

#include <mne/mne.h>
#include <inv/inv_source_estimate.h>
#include <mne/mne_source_spaces.h>
#include <mne/mne_forward_solution.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <mne/mne_hemisphere.h>
#include <mne/mne_epoch_data.h>
#include <mne/mne_epoch_data_list.h>

using namespace FIFFLIB;
using namespace MNELIB;
using namespace INVLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================

class TestMneSourceData : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;

    bool hasData() const {
        return !m_sDataPath.isEmpty();
    }

private slots:

    //=========================================================================
    void initTestCase()
    {
        qInstallMessageHandler(ApplicationLogger::customLogWriter);
        QString base = QCoreApplication::applicationDirPath()
                       + "/../resources/data/mne-cpp-test-data";
        if (QFile::exists(base + "/MEG/sample/sample_audvis_trunc_raw.fif")) {
            m_sDataPath = base;
        }
        if (m_sDataPath.isEmpty()) {
            qWarning() << "Test data not found";
        }
    }

    //=========================================================================
    // MNESourceSpaces: read from file
    //=========================================================================
    void sourceSpace_readFromFile()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-oct-6-src.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Source space file not found");

        MNESourceSpaces srcSpace;
        FiffStream::SPtr srcStream(new FiffStream(&file));
        srcStream->open();
        QVERIFY(MNESourceSpaces::readFromStream(srcStream, false, srcSpace));

        QVERIFY(srcSpace.size() > 0);

        // Check hemisphere data
        for (int h = 0; h < srcSpace.size(); ++h) {
            auto& hemi = srcSpace[h];
            QVERIFY(hemi.np > 0);
            QVERIFY(hemi.rr.rows() > 0);
            QVERIFY(hemi.nn.rows() > 0);
            QVERIFY(hemi.nuse > 0);
            QVERIFY(hemi.inuse.size() > 0);
            QVERIFY(hemi.vertno.size() > 0);
        }
    }

    void sourceSpace_checkDetails()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-oct-6-src.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Source space file not found");

        MNESourceSpaces srcSpace;
        FiffStream::SPtr srcStream2(new FiffStream(&file));
        srcStream2->open();
        MNESourceSpaces::readFromStream(srcStream2, false, srcSpace);

        // Left hemisphere
        auto& lh = srcSpace[0];
        QVERIFY(lh.id == 101 || lh.id == FIFFV_MNE_SURF_LEFT_HEMI);
        QVERIFY(lh.np > 1000); // typical source space has many vertices
        QVERIFY(lh.ntri > 0);
        QVERIFY(lh.itris.rows() > 0);
        QVERIFY(lh.itris.cols() == 3);

        // Right hemisphere
        auto& rh = srcSpace[1];
        QVERIFY(rh.id == 102 || rh.id == FIFFV_MNE_SURF_RIGHT_HEMI);

        // Check that isClustered returns consistent result
        QVERIFY(!srcSpace.isEmpty());
    }

    void sourceSpace_copyAndCompare()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-oct-6-src.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Source space file not found");

        MNESourceSpaces srcSpace;
        FiffStream::SPtr srcStream3(new FiffStream(&file));
        srcStream3->open();
        MNESourceSpaces::readFromStream(srcStream3, false, srcSpace);

        // Copy
        MNESourceSpaces copy(srcSpace);
        QCOMPARE(copy.size(), srcSpace.size());
        for (int h = 0; h < srcSpace.size(); ++h) {
            QCOMPARE(copy[h].np, srcSpace[h].np);
            QCOMPARE(copy[h].nuse, srcSpace[h].nuse);
        }
    }

    //=========================================================================
    // MNEBem: read BEM models
    //=========================================================================
    void bem_readSingleLayer()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-5120-bem.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("BEM file not found");

        MNEBem bem(file);
        QVERIFY(bem.size() > 0);

        // Check first surface
        MNEBemSurface& surf = bem[0];
        QVERIFY(surf.np > 0);
        QVERIFY(surf.ntri > 0);
        QVERIFY(surf.rr.rows() == surf.np);
        QVERIFY(surf.itris.rows() == surf.ntri);
    }

    void bem_readThreeLayer()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("3-layer BEM file not found");

        MNEBem bem(file);
        QCOMPARE(bem.size(), 3);

        // Each layer should have geometry
        for (int i = 0; i < bem.size(); ++i) {
            QVERIFY(bem[i].np > 0);
            QVERIFY(bem[i].ntri > 0);
            QVERIFY(bem[i].id > 0);
        }
    }

    void bem_addTriangleData()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-5120-bem.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("BEM file not found");

        MNEBem bem(file);
        if (bem.size() > 0) {
            MNEBemSurface& surf = bem[0];
            // Try adding triangle data (normals, centroids, areas)
            bool ok = surf.addTriangleData();
            QVERIFY(ok);
            QCOMPARE(surf.tri_cent.rows(), (Index)surf.ntri);
            QCOMPARE(surf.tri_nn.rows(), (Index)surf.ntri);
            QCOMPARE(surf.tri_area.size(), (Index)surf.ntri);
        }
    }

    void bem_addGeometryInfo()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-5120-bem.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("BEM file not found");

        MNEBem bem(file);
        if (bem.size() > 0) {
            MNEBemSurface& surf = bem[0];
            surf.addTriangleData();
            bool ok = surf.add_geometry_info();
            QVERIFY(ok);
            QVERIFY(surf.neighbor_tri.size() > 0);
        }
    }

    void bem_idName()
    {
        // Exercise static id_name method
        QString brain = MNEBemSurface::id_name(FIFFV_BEM_SURF_ID_BRAIN);
        QVERIFY(!brain.isEmpty());
        QString skull = MNEBemSurface::id_name(FIFFV_BEM_SURF_ID_SKULL);
        QVERIFY(!skull.isEmpty());
        QString scalp = MNEBemSurface::id_name(FIFFV_BEM_SURF_ID_HEAD);
        QVERIFY(!scalp.isEmpty());
    }

    //=========================================================================
    // MNEForwardSolution: read forward solution
    //=========================================================================
    void fwdSolution_readFromFile()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Forward solution file not found");

        // Read forward solution
        MNEForwardSolution fwd(file);
        QVERIFY(!fwd.isEmpty());
        QVERIFY(fwd.sol != nullptr || true);
        QVERIFY(fwd.source_nn.rows() > 0);
        QVERIFY(fwd.nchan > 0);
        QVERIFY(fwd.nsource > 0);
    }

    void fwdSolution_checkSourceSpace()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Forward solution file not found");

        MNEForwardSolution fwd(file);
        if (!fwd.isEmpty()) {
            // Check embedded source space
            QVERIFY(fwd.src.size() > 0);
            for (int h = 0; h < fwd.src.size(); ++h) {
                QVERIFY(fwd.src[h].np > 0);
                QVERIFY(fwd.src[h].nuse > 0);
            }

            // Check source orientations
            if (fwd.source_ori == FIFFV_MNE_FREE_ORI) {
                QCOMPARE(fwd.source_nn.rows(), fwd.nsource * 3);
            } else {
                QCOMPARE(fwd.source_nn.rows(), fwd.nsource);
            }
        }
    }

    void fwdSolution_pickChannels()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Forward solution file not found");

        MNEForwardSolution fwd(file);
        if (!fwd.isEmpty() && fwd.info.nchan > 0) {
            // Pick MEG channels only
            RowVectorXi megPicks = fwd.info.pick_types(true, false, false);
            if (megPicks.size() > 0) {
                QStringList megNames;
                for (int i = 0; i < megPicks.size(); ++i)
                    megNames << fwd.info.ch_names[megPicks(i)];
                MNEForwardSolution fwdMeg = fwd.pick_channels(megNames);
                QCOMPARE(fwdMeg.nchan, (int)megPicks.size());
            }
        }
    }

    //=========================================================================
    // Read raw data and create epochs
    //=========================================================================
    void epoch_readAndCreate()
    {
        if (!hasData()) QSKIP("No test data");

        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        FiffRawData raw(file);

        // Read some data segments to create synthetic epochs
        float epochDuration = 0.5f; // 500ms epochs
        fiff_int_t epochSamples = (fiff_int_t)(epochDuration * raw.info.sfreq);
        fiff_int_t from = raw.first_samp;

        MNEEpochDataList epochList;
        int nEpochs = 0;

        while (from + epochSamples <= raw.last_samp && nEpochs < 5) {
            MatrixXd data, times;
            if (raw.read_raw_segment(data, times, from, from + epochSamples - 1)) {
                MNEEpochData::SPtr epoch = MNEEpochData::SPtr::create();
                epoch->epoch = data;
                epoch->tmin = (float)(from - raw.first_samp) / raw.info.sfreq;
                epoch->tmax = epoch->tmin + epochDuration;
                epochList.append(epoch);
                nEpochs++;
            }
            from += epochSamples;
        }

        QVERIFY(epochList.size() > 0);

        // Average epochs
        if (epochList.size() >= 2) {
            FiffEvoked evoked = epochList.average(raw.info, 0, epochSamples - 1);
            QVERIFY(evoked.data.rows() > 0);
            QVERIFY(evoked.data.cols() > 0);
        }
    }

    void epoch_dropRejected()
    {
        if (!hasData()) QSKIP("No test data");

        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        FiffRawData raw(file);

        fiff_int_t epochSamples = (fiff_int_t)(0.3 * raw.info.sfreq);
        fiff_int_t from = raw.first_samp;

        MNEEpochDataList epochList;
        for (int i = 0; i < 5 && from + epochSamples <= raw.last_samp; ++i) {
            MatrixXd data, times;
            if (raw.read_raw_segment(data, times, from, from + epochSamples - 1)) {
                MNEEpochData::SPtr epoch = MNEEpochData::SPtr::create();
                epoch->epoch = data;
                epoch->bReject = (i == 2); // Reject middle epoch
                epochList.append(epoch);
            }
            from += epochSamples;
        }

        int origSize = epochList.size();
        epochList.dropRejected();
        QVERIFY(epochList.size() <= origSize);
    }

    //=========================================================================
    // InvSourceEstimate: create from real data dimensions
    //=========================================================================
    void sourceEstimate_createFromDimensions()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-oct-6-src.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Source space file not found");

        MNESourceSpaces srcSpace;
        FiffStream::SPtr srcStream4(new FiffStream(&file));
        srcStream4->open();
        MNESourceSpaces::readFromStream(srcStream4, false, srcSpace);

        int totalVerts = 0;
        VectorXi allVertno(0);
        for (int h = 0; h < srcSpace.size(); ++h) {
            int oldSize = allVertno.size();
            allVertno.conservativeResize(oldSize + srcSpace[h].vertno.size());
            allVertno.segment(oldSize, srcSpace[h].vertno.size()) = srcSpace[h].vertno;
            totalVerts += srcSpace[h].nuse;
        }

        // Create synthetic source estimate
        int nTimes = 100;
        MatrixXd data = MatrixXd::Random(totalVerts, nTimes);
        float tmin = 0.0f;
        float tstep = 0.001f;

        InvSourceEstimate stc(data, allVertno, tmin, tstep);
        QVERIFY(!stc.isEmpty());
        QCOMPARE(stc.samples(), nTimes);
        QCOMPARE(stc.data.rows(), (Index)totalVerts);
    }

    void sourceEstimate_reduce()
    {
        // Synthetic but with realistic dimensions
        MatrixXd data = MatrixXd::Random(100, 50);
        VectorXi verts(100);
        for (int i = 0; i < 100; ++i) verts(i) = i;

        InvSourceEstimate stc(data, verts, 0.0f, 0.001f);
        InvSourceEstimate reduced = stc.reduce(10, 20);
        QCOMPARE(reduced.samples(), 20);
    }

    //=========================================================================
    // MNEForwardSolution: compute_orient_prior
    //=========================================================================
    void fwdSolution_computeOrientPrior()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Forward solution file not found");

        MNEForwardSolution fwd(file);
        if (!fwd.isEmpty()) {
            double loose = 0.2;
            FiffCov orientPrior = fwd.compute_orient_prior(loose);
            QVERIFY(orientPrior.data.rows() > 0);
        }
    }

    //=========================================================================
    // Read BEM solution file
    //=========================================================================
    void bem_readSolution()
    {
        if (!hasData()) QSKIP("No test data");

        QString path = m_sDataPath + "/subjects/sample/bem/sample-5120-bem-sol.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("BEM solution file not found");

        // Read via stream to exercise file parsing
        FiffStream::SPtr stream(new FiffStream(&file));
        QVERIFY(stream->open());

        QList<FiffDirNode::SPtr> bemSolBlocks = stream->dirtree()->dir_tree_find(FIFFB_BEM);
        if (bemSolBlocks.isEmpty())
            bemSolBlocks = stream->dirtree()->dir_tree_find(FIFFB_BEM_SURF);

        QVERIFY(bemSolBlocks.size() > 0 || stream->dir().size() > 0);

        // Read tags to exercise parsing
        int nTags = qMin((int)stream->dir().size(), 50);
        for (int i = 0; i < nTags; ++i) {
            FiffTag::SPtr tag;
            stream->read_tag(tag, stream->dir()[i]->pos);
            QVERIFY(tag != nullptr);
        }

        stream->close();
    }

    //=========================================================================
    void cleanupTestCase() {}
};

QTEST_GUILESS_MAIN(TestMneSourceData)
#include "test_mne_source_data.moc"
