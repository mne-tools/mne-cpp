#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <Eigen/SparseCore>
#include <cmath>

#include <utils/generics/mne_logger.h>

#include <mne/mne_hemisphere.h>
#include <mne/mne_forward_solution.h>
#include <inv/inv_source_estimate.h>
#include <mne/mne_source_spaces.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <mne/mne_epoch_data.h>
#include <mne/mne_epoch_data_list.h>
#include <mne/mne_named_matrix.h>
#include <mne/mne_cov_matrix.h>
#include <mne/mne_proj_op.h>
#include <mne/mne_ctf_comp_data.h>
#include <mne/mne_inverse_operator.h>

#include <fiff/fiff.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_cov.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans.h>

using namespace MNELIB;
using namespace INVLIB;
using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

class TestMneHemisphereEpoch : public QObject
{
    Q_OBJECT

private:
    QString m_sDataPath;

    bool hasData() const { return !m_sDataPath.isEmpty(); }

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
    // MNEHemisphere
    //=========================================================================
    void hemisphere_defaultCtor()
    {
        MNEHemisphere h;
        // np defaults to -1 (uninitialized sentinel)
        QCOMPARE(h.np, -1);
        QVERIFY(!h.isClustered());
    }

    void hemisphere_copyCtor()
    {
        MNEHemisphere h;
        h.np = 4;
        h.ntri = 2;
        h.type = 1;
        h.rr = MatrixX3f::Random(4, 3);
        h.nn = MatrixX3f::Random(4, 3);
        h.itris = MatrixX3i(2, 3);
        h.itris << 0, 1, 2,  1, 2, 3;

        MNEHemisphere h2(h);
        QCOMPARE(h2.np, 4);
        QCOMPARE(h2.ntri, 2);
        QVERIFY(h2.rr.isApprox(h.rr));
    }

    void hemisphere_equalityEmpty()
    {
        MNEHemisphere a;
        MNEHemisphere b;
        QVERIFY(a == b);
    }

    void hemisphere_equalityPopulated()
    {
        MNEHemisphere a;
        a.np = 3; a.ntri = 1; a.type = 1; a.id = 101;
        a.rr = MatrixX3f::Ones(3, 3);
        a.nn = MatrixX3f::Zero(3, 3);
        a.itris = MatrixX3i(1, 3); a.itris << 0, 1, 2;
        a.nuse = 3;
        a.inuse = VectorXi::Ones(3);
        a.vertno = VectorXi(3); a.vertno << 0, 1, 2;

        MNEHemisphere b = a;
        // Verify copy has same basic members
        QCOMPARE(b.np, a.np);
        QCOMPARE(b.ntri, a.ntri);
        QCOMPARE(b.type, a.type);
        QVERIFY(b.rr.isApprox(a.rr));
    }

    void hemisphere_clear()
    {
        MNEHemisphere h;
        h.np = 10;
        h.ntri = 5;
        h.rr = MatrixX3f::Random(10, 3);
        h.clear();
        // After clear, np may be -1 (reset to default), not 0
        QVERIFY(h.np == 0 || h.np == -1);
    }

    void hemisphere_isClustered()
    {
        MNEHemisphere h;
        QVERIFY(!h.isClustered());
    }

    //=========================================================================
    // MNEBemSurface
    //=========================================================================
    void bemSurface_defaultCtor()
    {
        MNEBemSurface bs;
        QCOMPARE(bs.id, -1);
        QCOMPARE(bs.np, -1);
        QCOMPARE(bs.ntri, -1);
    }

    void bemSurface_copyCtor()
    {
        MNEBemSurface bs;
        bs.id = FIFFV_BEM_SURF_ID_BRAIN;
        bs.np = 4;
        bs.ntri = 2;

        MNEBemSurface bs2(bs);
        QCOMPARE(bs2.id, (int)FIFFV_BEM_SURF_ID_BRAIN);
        QCOMPARE(bs2.np, 4);
    }

    void bemSurface_addTriangleData()
    {
        // Build a tetrahedron (4 vertices, 4 triangles)
        MNEBemSurface bs;
        bs.id = FIFFV_BEM_SURF_ID_BRAIN;
        bs.np = 4;
        bs.ntri = 4;
        bs.rr.resize(4, 3);
        bs.rr << 1,0,0, -1,0,0, 0,1,0, 0,0,1;
        bs.itris.resize(4, 3);
        bs.itris << 0,1,2, 0,1,3, 0,2,3, 1,2,3;

        bool ok = bs.addTriangleData();
        QVERIFY(ok);
        QCOMPARE(bs.tri_cent.rows(), 4);
        QCOMPARE(bs.tri_nn.rows(), 4);
        QCOMPARE(bs.tri_area.size(), 4);
        for (int i = 0; i < 4; i++) {
            QVERIFY(bs.tri_area(i) > 0);
        }
    }

    void bemSurface_addGeometryInfo()
    {
        // Build a tetrahedron and run addTriangleData (which calls add_geometry_info)
        MNEBemSurface bs;
        bs.id = FIFFV_BEM_SURF_ID_BRAIN;
        bs.np = 4;
        bs.ntri = 4;
        bs.rr.resize(4, 3);
        bs.rr << 1,0,0, -1,0,0, 0,1,0, 0,0,1;
        bs.itris.resize(4, 3);
        bs.itris << 0,1,2, 0,1,3, 0,2,3, 1,2,3;

        bs.addTriangleData();
        // add_geometry_info creates neighbor_tri and neighbor_vert
        QVERIFY(bs.neighbor_tri.size() > 0);
    }

    void bemSurface_idName()
    {
        QString brain = MNEBemSurface::id_name(FIFFV_BEM_SURF_ID_BRAIN);
        QVERIFY(!brain.isEmpty());
    }

    void bemSurface_clear()
    {
        MNEBemSurface bs;
        bs.id = 5; bs.np = 10; bs.ntri = 5;
        bs.clear();
        QCOMPARE(bs.id, -1);
        QCOMPARE(bs.np, -1);
    }

    //=========================================================================
    // InvSourceEstimate
    //=========================================================================
    void sourceEstimate_defaultCtor()
    {
        InvSourceEstimate se;
        QVERIFY(se.isEmpty());
        QCOMPARE(se.samples(), 0);
    }

    void sourceEstimate_paramCtor()
    {
        MatrixXd data = MatrixXd::Random(10, 5);
        VectorXi verts(10);
        for (int i = 0; i < 10; ++i) verts(i) = i;

        InvSourceEstimate se(data, verts, 0.0f, 0.001f);
        QVERIFY(!se.isEmpty());
        QCOMPARE(se.samples(), 5);
        QCOMPARE(se.data.rows(), (Eigen::Index)10);
    }

    void sourceEstimate_copyCtor()
    {
        MatrixXd data = MatrixXd::Random(5, 3);
        VectorXi verts(5); verts << 0, 1, 2, 3, 4;
        InvSourceEstimate se(data, verts, 0.1f, 0.002f);

        InvSourceEstimate copy(se);
        QVERIFY(!copy.isEmpty());
        QCOMPARE(copy.samples(), 3);
        QVERIFY(copy.data.isApprox(se.data));
    }

    void sourceEstimate_reduce()
    {
        MatrixXd data = MatrixXd::Random(8, 10);
        VectorXi verts(8);
        for (int i = 0; i < 8; ++i) verts(i) = i;

        InvSourceEstimate se(data, verts, 0.0f, 0.001f);
        InvSourceEstimate reduced = se.reduce(2, 4);
        QCOMPARE(reduced.samples(), 4);
        QCOMPARE(reduced.data.rows(), (Eigen::Index)8);
    }

    void sourceEstimate_clear()
    {
        MatrixXd data = MatrixXd::Random(5, 3);
        VectorXi verts(5); verts << 0, 1, 2, 3, 4;
        InvSourceEstimate se(data, verts, 0.0f, 0.001f);
        QVERIFY(!se.isEmpty());
        se.clear();
        // After clear, check data is zeroed or empty
        QCOMPARE(se.data.rows(), (Eigen::Index)0);
    }

    void sourceEstimate_assign()
    {
        MatrixXd data = MatrixXd::Random(5, 3);
        VectorXi verts(5); verts << 0, 1, 2, 3, 4;
        InvSourceEstimate se(data, verts, 0.0f, 0.001f);

        InvSourceEstimate se2;
        se2 = se;
        QVERIFY(!se2.isEmpty());
        QCOMPARE(se2.samples(), 3);
    }

    //=========================================================================
    // MNESourceSpaces
    //=========================================================================
    void sourceSpace_defaultCtor()
    {
        MNESourceSpaces ss;
        QVERIFY(ss.isEmpty());
        QCOMPARE(ss.size(), 0);
    }

    void sourceSpace_copyCtor()
    {
        MNESourceSpaces ss;
        MNESourceSpaces ss2(ss);
        QVERIFY(ss2.isEmpty());
    }

    //=========================================================================
    // MNEForwardSolution
    //=========================================================================
    void fwdSol_defaultCtor()
    {
        MNEForwardSolution fwd;
        QVERIFY(fwd.isEmpty());
        QVERIFY(!fwd.isFixedOrient());
    }

    void fwdSol_tripletSelection()
    {
        VectorXi sel(3);
        sel << 0, 2, 5;
        MNEForwardSolution fwd;
        VectorXi result = fwd.tripletSelection(sel);
        // Expect {0,1,2, 6,7,8, 15,16,17}
        QCOMPARE(result.size(), (Eigen::Index)9);
        QCOMPARE(result(0), 0);
        QCOMPARE(result(1), 1);
        QCOMPARE(result(2), 2);
        QCOMPARE(result(3), 6);
        QCOMPARE(result(4), 7);
        QCOMPARE(result(5), 8);
        QCOMPARE(result(6), 15);
        QCOMPARE(result(7), 16);
        QCOMPARE(result(8), 17);
    }

    void fwdSol_isEmpty()
    {
        MNEForwardSolution fwd;
        QVERIFY(fwd.isEmpty());
        fwd.nchan = 5;
        QVERIFY(!fwd.isEmpty());
    }

    void fwdSol_clear()
    {
        MNEForwardSolution fwd;
        fwd.nchan = 10;
        fwd.clear();
        QVERIFY(fwd.isEmpty());
    }

    //=========================================================================
    // MNEEpochDataList
    //=========================================================================
    void epochDataList_defaultCtor()
    {
        MNEEpochDataList list;
        QCOMPARE(list.size(), 0);
    }

    void epochDataList_dropRejected()
    {
        MNEEpochDataList list;
        for (int i = 0; i < 5; ++i) {
            MNEEpochData::SPtr e = MNEEpochData::SPtr::create();
            e->epoch = MatrixXd::Random(2, 10);
            e->bReject = (i == 1 || i == 3); // reject epochs 1 and 3
            list.append(e);
        }
        QCOMPARE(list.size(), 5);
        list.dropRejected();
        QCOMPARE(list.size(), 3);
    }

    void epochDataList_pickChannels()
    {
        MNEEpochDataList list;
        for (int i = 0; i < 3; ++i) {
            MNEEpochData::SPtr e = MNEEpochData::SPtr::create();
            e->epoch = MatrixXd::Random(5, 10); // 5 channels, 10 samples
            list.append(e);
        }

        RowVectorXi sel(2);
        sel << 0, 3; // pick channels 0 and 3
        list.pick_channels(sel);

        for (int i = 0; i < list.size(); ++i) {
            QCOMPARE(list[i]->epoch.rows(), (Eigen::Index)2);
        }
    }

    //=========================================================================
    // MNENamedMatrix
    //=========================================================================
    void namedMatrix_defaultCtor()
    {
        MNENamedMatrix nm;
        QCOMPARE(nm.nrow, 0);
        QCOMPARE(nm.ncol, 0);
    }

    void namedMatrix_build()
    {
        QStringList rows; rows << "R1" << "R2";
        QStringList cols; cols << "C1" << "C2" << "C3";
        MatrixXf data(2, 3);
        data << 1, 2, 3, 4, 5, 6;

        auto nm = MNENamedMatrix::build(2, 3, rows, cols, data);
        QVERIFY(nm != nullptr);
        QCOMPARE(nm->nrow, 2);
        QCOMPARE(nm->ncol, 3);
        QCOMPARE(nm->rowlist.size(), 2);
        QCOMPARE(nm->collist.size(), 3);
    }

    void namedMatrix_copyCtor()
    {
        QStringList rows; rows << "A" << "B";
        QStringList cols; cols << "X" << "Y";
        MatrixXf data(2, 2);
        data << 1, 2, 3, 4;

        auto nm = MNENamedMatrix::build(2, 2, rows, cols, data);
        MNENamedMatrix copy(*nm);
        QCOMPARE(copy.nrow, 2);
        QCOMPARE(copy.ncol, 2);
        QVERIFY(copy.data.isApprox(nm->data));
    }

    void namedMatrix_pick()
    {
        QStringList rows; rows << "R1" << "R2" << "R3";
        QStringList cols; cols << "C1" << "C2" << "C3";
        MatrixXf data(3, 3);
        data << 1, 2, 3, 4, 5, 6, 7, 8, 9;

        auto nm = MNENamedMatrix::build(3, 3, rows, cols, data);

        QStringList pickRows; pickRows << "R1" << "R3";
        QStringList pickCols; pickCols << "C2";
        auto picked = nm->pick(pickRows, 2, pickCols, 1);
        QVERIFY(picked != nullptr);
        QCOMPARE(picked->nrow, 2);
        QCOMPARE(picked->ncol, 1);
    }

    //=========================================================================
    // MNECovMatrix
    //=========================================================================
    void covMatrix_createDense()
    {
        QStringList names; names << "CH1" << "CH2" << "CH3";
        // Packed lower triangle for 3x3: elements [0,0], [1,0], [1,1], [2,0], [2,1], [2,2]
        VectorXd cov(6);
        cov << 1, 0, 1, 0, 0, 1; // identity

        auto cm = MNECovMatrix::create_dense(1, 3, names, cov);
        QVERIFY(cm != nullptr);
        QCOMPARE(cm->ncov, 3);
        QVERIFY(!cm->is_diag());
    }

    void covMatrix_createDiag()
    {
        QStringList names; names << "CH1" << "CH2";
        VectorXd diag(2);
        diag << 4.0, 9.0;

        auto cm = MNECovMatrix::create_diag(1, 2, names, diag);
        QVERIFY(cm != nullptr);
        QVERIFY(cm->is_diag());
        QCOMPARE(cm->cov_diag.size(), (Eigen::Index)2);
    }

    void covMatrix_dup()
    {
        QStringList names; names << "A" << "B";
        VectorXd diag(2); diag << 2.0, 3.0;

        auto cm = MNECovMatrix::create_diag(1, 2, names, diag);
        auto dup = cm->dup();
        QVERIFY(dup != nullptr);
        QCOMPARE(dup->ncov, 2);
        QVERIFY(dup->is_diag());
    }

    //=========================================================================
    // MNEProjOp
    //=========================================================================
    void projOp_defaultCtor()
    {
        MNEProjOp op;
        QCOMPARE(op.nitems, 0);
    }

    void projOp_addItem()
    {
        MNEProjOp op;
        QStringList rows; rows << "R1";
        QStringList cols; cols << "CH1" << "CH2" << "CH3";
        MatrixXf data(1, 3);
        data << 1.0f, 0.0f, 0.0f;

        auto nm = MNENamedMatrix::build(1, 3, rows, cols, data);
        op.add_item(nm.get(), 1, "test proj");
        QCOMPARE(op.nitems, 1);
    }

    void projOp_dup()
    {
        MNEProjOp op;
        QStringList rows; rows << "R1";
        QStringList cols; cols << "CH1" << "CH2";
        MatrixXf data(1, 2);
        data << 1.0f, 0.0f;

        auto nm = MNENamedMatrix::build(1, 2, rows, cols, data);
        op.add_item(nm.get(), 1, "test");

        MNEProjOp* dup = op.dup();
        QVERIFY(dup != nullptr);
        QCOMPARE(dup->nitems, 1);
        delete dup;
    }

    void projOp_freeProj()
    {
        MNEProjOp op;
        op.free_proj();
        QCOMPARE(op.nitems, 0);
    }

    //=========================================================================
    // MNECTFCompData
    //=========================================================================
    void ctfCompData_defaultCtor()
    {
        MNECTFCompData cd;
        QVERIFY(!cd.calibrated);
    }

    void ctfCompData_copyCtor()
    {
        MNECTFCompData cd;
        cd.kind = 1;
        cd.calibrated = true;
        cd.data = std::make_unique<MNENamedMatrix>();
        cd.data->nrow = 2;
        cd.data->ncol = 3;

        MNECTFCompData cd2(cd);
        QCOMPARE(cd2.kind, 1);
        QVERIFY(cd2.calibrated);
        QVERIFY(cd2.data != nullptr);
        QCOMPARE(cd2.data->nrow, 2);
        QCOMPARE(cd2.data->ncol, 3);
    }

    //=========================================================================
    // MNEForwardSolution::compute_orient_prior
    //=========================================================================
    void fwdSol_computeOrientPrior()
    {
        MNEForwardSolution fwd;
        fwd.nchan = 10;
        fwd.surf_ori = true;
        fwd.source_ori = FIFFV_MNE_FREE_ORI;

        // sol with proper dimensions
        fwd.sol = FiffNamedMatrix::SDPtr(new FiffNamedMatrix());
        fwd.sol->data = MatrixXd::Random(10, 12); // 4 sources * 3 orientations
        fwd.sol->ncol = 12;
        fwd.sol->nrow = 10;

        FiffCov orient_prior = fwd.compute_orient_prior(0.2f);
        QVERIFY(orient_prior.data.cols() > 0 || orient_prior.data.rows() > 0);
    }

    //=========================================================================
    // DATA-DRIVEN TESTS: MNESourceSpaces from real file
    //=========================================================================
    void data_sourceSpace_readFromFile()
    {
        if (!hasData()) QSKIP("No test data");
        QString path = m_sDataPath + "/subjects/sample/bem/sample-oct-6-src.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Source space file not found");

        MNESourceSpaces srcSpace;
        FiffStream::SPtr srcStream(new FiffStream(&file));
        srcStream->open();
        QVERIFY(MNESourceSpaces::readFromStream(srcStream, false, srcSpace));

        for (int h = 0; h < srcSpace.size(); ++h) {
            auto& hemi = srcSpace[h];
            QVERIFY(hemi.np > 0);
            QVERIFY(hemi.nuse > 0);
            QVERIFY(hemi.rr.rows() == hemi.np);
            QVERIFY(hemi.nn.rows() == hemi.np);
            QVERIFY(hemi.vertno.size() == hemi.nuse);
            QVERIFY(hemi.inuse.size() == hemi.np);
            QVERIFY(hemi.itris.rows() > 0);
        }
    }

    void data_sourceSpace_hemisphereDetails()
    {
        if (!hasData()) QSKIP("No test data");
        QString path = m_sDataPath + "/subjects/sample/bem/sample-oct-6-src.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Source space file not found");

        MNESourceSpaces srcSpace;
        FiffStream::SPtr srcStream2(new FiffStream(&file));
        srcStream2->open();
        MNESourceSpaces::readFromStream(srcStream2, false, srcSpace);
        QVERIFY(srcSpace[0].id == 101 || srcSpace[0].id == FIFFV_MNE_SURF_LEFT_HEMI);
        // Right hemisphere
        QVERIFY(srcSpace[1].id == 102 || srcSpace[1].id == FIFFV_MNE_SURF_RIGHT_HEMI);

        // Copy and verify
        MNESourceSpaces copy(srcSpace);
        QCOMPARE(copy.size(), srcSpace.size());
        QCOMPARE(copy[0].np, srcSpace[0].np);
        QCOMPARE(copy[1].nuse, srcSpace[1].nuse);
    }

    //=========================================================================
    // DATA-DRIVEN: MNEBem from real file
    //=========================================================================
    void data_bem_readSingleLayer()
    {
        if (!hasData()) QSKIP("No test data");
        QString path = m_sDataPath + "/subjects/sample/bem/sample-5120-bem.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("BEM file not found");

        MNEBem bem(file);
        QVERIFY(bem.size() > 0);
        MNEBemSurface& surf = bem[0];
        QVERIFY(surf.np > 0);
        QVERIFY(surf.ntri > 0);
        QVERIFY(surf.rr.rows() == surf.np);
        QVERIFY(surf.itris.rows() == surf.ntri);
    }

    void data_bem_readThreeLayer()
    {
        if (!hasData()) QSKIP("No test data");
        QString path = m_sDataPath + "/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("3-layer BEM not found");

        MNEBem bem(file);
        QCOMPARE(bem.size(), 3);
        for (int i = 0; i < 3; ++i) {
            QVERIFY(bem[i].np > 0);
            QVERIFY(bem[i].ntri > 0);
        }
    }

    void data_bem_addTriangleData()
    {
        if (!hasData()) QSKIP("No test data");
        QString path = m_sDataPath + "/subjects/sample/bem/sample-5120-bem.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("BEM file not found");

        MNEBem bem(file);
        if (bem.size() > 0) {
            MNEBemSurface& surf = bem[0];
            bool ok = surf.addTriangleData();
            QVERIFY(ok);
            QCOMPARE(surf.tri_cent.rows(), (Index)surf.ntri);
            QCOMPARE(surf.tri_nn.rows(), (Index)surf.ntri);
            QCOMPARE(surf.tri_area.size(), (Index)surf.ntri);
        }
    }

    void data_bem_addGeometryInfo()
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

    //=========================================================================
    // DATA-DRIVEN: MNEForwardSolution from real file
    //=========================================================================
    void data_fwdSol_readFromFile()
    {
        if (!hasData()) QSKIP("No test data");
        QString path = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Forward solution not found");

        MNEForwardSolution fwd(file);
        QVERIFY(!fwd.isEmpty());
        QVERIFY(fwd.nchan > 0);
        QVERIFY(fwd.nsource > 0);
        QVERIFY(fwd.source_nn.rows() > 0);
        QVERIFY(fwd.src.size() > 0);
    }

    void data_fwdSol_pickChannels()
    {
        if (!hasData()) QSKIP("No test data");
        QString path = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Forward solution not found");

        MNEForwardSolution fwd(file);
        if (!fwd.isEmpty() && fwd.info.nchan > 0) {
            RowVectorXi megIdx = fwd.info.pick_types(true, false, false);
            if (megIdx.size() > 0) {
                QStringList megNames;
                for (int i = 0; i < megIdx.size(); ++i)
                    megNames << fwd.info.ch_names[megIdx(i)];
                MNEForwardSolution fwdMeg = fwd.pick_channels(megNames);
                QCOMPARE(fwdMeg.nchan, (int)megIdx.size());
            }
        }
    }

    void data_fwdSol_computeOrientPrior()
    {
        if (!hasData()) QSKIP("No test data");
        QString path = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Forward solution not found");

        MNEForwardSolution fwd(file);
        if (!fwd.isEmpty()) {
            FiffCov orientPrior = fwd.compute_orient_prior(0.2);
            QVERIFY(orientPrior.data.rows() > 0);
        }
    }

    //=========================================================================
    // DATA-DRIVEN: Epochs from real raw data
    //=========================================================================
    void data_epochList_fromRawData()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        if (!file.exists()) QSKIP("Raw file not found");

        FiffRawData raw(file);
        fiff_int_t epochSamples = (fiff_int_t)(0.5 * raw.info.sfreq);
        fiff_int_t from = raw.first_samp;

        MNEEpochDataList epochList;
        int nEpochs = 0;
        while (from + epochSamples <= raw.last_samp && nEpochs < 5) {
            MatrixXd data, times;
            if (raw.read_raw_segment(data, times, from, from + epochSamples - 1)) {
                MNEEpochData::SPtr epoch = MNEEpochData::SPtr::create();
                epoch->epoch = data;
                epoch->tmin = (float)(from - raw.first_samp) / raw.info.sfreq;
                epoch->tmax = epoch->tmin + 0.5f;
                epochList.append(epoch);
                nEpochs++;
            }
            from += epochSamples;
        }
        QVERIFY(epochList.size() >= 3);

        // Average
        FiffEvoked evoked = epochList.average(raw.info, 0, epochSamples - 1);
        QVERIFY(evoked.data.rows() > 0);
        QVERIFY(evoked.data.cols() > 0);
    }

    void data_epochList_dropRejected()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        if (!file.exists()) QSKIP("Raw file not found");

        FiffRawData raw(file);
        fiff_int_t epochSamples = (fiff_int_t)(0.3 * raw.info.sfreq);
        fiff_int_t from = raw.first_samp;

        MNEEpochDataList epochList;
        for (int i = 0; i < 5 && from + epochSamples <= raw.last_samp; ++i) {
            MatrixXd data, times;
            if (raw.read_raw_segment(data, times, from, from + epochSamples - 1)) {
                MNEEpochData::SPtr epoch = MNEEpochData::SPtr::create();
                epoch->epoch = data;
                epoch->bReject = (i == 2);
                epochList.append(epoch);
            }
            from += epochSamples;
        }
        int orig = epochList.size();
        epochList.dropRejected();
        QVERIFY(epochList.size() <= orig);
    }

    void data_epochList_pickChannels()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        if (!file.exists()) QSKIP("Raw file not found");

        FiffRawData raw(file);
        fiff_int_t epochSamples = (fiff_int_t)(0.2 * raw.info.sfreq);
        fiff_int_t from = raw.first_samp;

        MNEEpochDataList epochList;
        for (int i = 0; i < 3 && from + epochSamples <= raw.last_samp; ++i) {
            MatrixXd data, times;
            if (raw.read_raw_segment(data, times, from, from + epochSamples - 1)) {
                MNEEpochData::SPtr epoch = MNEEpochData::SPtr::create();
                epoch->epoch = data;
                epochList.append(epoch);
            }
            from += epochSamples;
        }

        // Pick first 10 channels
        RowVectorXi sel(10);
        for (int i = 0; i < 10; ++i) sel(i) = i;
        epochList.pick_channels(sel);
        for (int i = 0; i < epochList.size(); ++i)
            QCOMPARE(epochList[i]->epoch.rows(), (Eigen::Index)10);
    }

    //=========================================================================
    // DATA-DRIVEN: FiffCov from real file
    //=========================================================================
    void data_noiseCov_readFromFile()
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
    }

    void data_noiseCov_pickChannels()
    {
        if (!hasData()) QSKIP("No test data");
        QString path = m_sDataPath + "/MEG/sample/sample_audvis-cov.fif";
        QFile file(path);
        if (!file.exists()) QSKIP("Covariance file not found");

        FiffCov cov(file);
        if (cov.isEmpty()) QSKIP("Failed to read cov");

        int nPick = qMin(30, cov.names.size());
        QStringList pickNames = cov.names.mid(0, nPick);
        FiffCov picked = cov.pick_channels(pickNames);
        QCOMPARE(picked.names.size(), nPick);
        QCOMPARE(picked.data.rows(), (Index)nPick);
    }

    //=========================================================================
    // DATA-DRIVEN: Projectors from real raw info
    //=========================================================================
    void data_proj_fromRawInfo()
    {
        if (!hasData()) QSKIP("No test data");
        QFile file(m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif");
        if (!file.exists()) QSKIP("Raw file not found");

        FiffRawData raw(file);
        QList<FiffProj>& projs = raw.info.projs;
        if (projs.isEmpty()) QSKIP("No projectors in raw");

        // Activate all projectors
        FiffProj::activate_projs(projs);
        for (const FiffProj& p : projs)
            QVERIFY(p.active);

        // Make projector matrix
        MatrixXd P;
        int nProj = FiffProj::make_projector(projs, raw.info.ch_names, P);
        QVERIFY(nProj > 0);
        QCOMPARE(P.rows(), (Index)raw.info.nchan);
        QCOMPARE(P.cols(), (Index)raw.info.nchan);
    }

    //=========================================================================
    // DATA-DRIVEN: MNEInverseOperator from fwd+cov
    //=========================================================================
    void data_invOp_makeFromFwdCov()
    {
        if (!hasData()) QSKIP("No test data");

        QString fwdPath = m_sDataPath + "/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif";
        QString covPath = m_sDataPath + "/MEG/sample/sample_audvis-cov.fif";
        QString rawPath = m_sDataPath + "/MEG/sample/sample_audvis_trunc_raw.fif";

        QFile fwdFile(fwdPath);
        QFile covFile(covPath);
        QFile rawFile(rawPath);
        if (!fwdFile.exists() || !covFile.exists() || !rawFile.exists())
            QSKIP("Required files not found");

        MNEForwardSolution fwd(fwdFile);
        FiffCov noiseCov(covFile);
        FiffRawData raw(rawFile);

        if (fwd.isEmpty() || noiseCov.isEmpty()) QSKIP("Failed to load");

        MNEInverseOperator invOp = MNEInverseOperator::make_inverse_operator(
            raw.info, fwd, noiseCov, 0.2f, 0.8f, false, true);
        QVERIFY(invOp.nchan > 0);
        QVERIFY(invOp.nsource > 0);
    }

    void cleanupTestCase() {}
};

QTEST_GUILESS_MAIN(TestMneHemisphereEpoch)
#include "test_mne_hemisphere_epoch.moc"
