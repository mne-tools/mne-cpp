/**
 * @file test_disp3d_headless.cpp
 * @brief Headless tests for disp3D library: Interpolation functions,
 *        GeometryInfo (surface distances & projection), MeshFactory geometry,
 *        DataLoader I/O paths, and RtSourceDataController / RtSensorDataController setters.
 */
#include <QTest>
#include <QCoreApplication>
#include <QVector3D>
#include <QMatrix4x4>
#include <QColor>
#include <QFile>

#include <disp3D/helpers/interpolation.h>
#include <disp3D/helpers/geometryinfo.h>
#include <disp3D/geometry/meshfactory.h>
#include <disp3D/core/dataloader.h>
#include <disp3D/workers/rtsourcedatacontroller.h>
#include <disp3D/workers/rtsensordatacontroller.h>

#include <fiff/fiff.h>

using namespace DISP3DLIB;
using namespace Eigen;

class TestDisp3dHeadless : public QObject
{
    Q_OBJECT

private:
    QString dataPath() const {
        return QCoreApplication::applicationDirPath() +
               "/../resources/data/mne-cpp-test-data/";
    }

private slots:

    //=========================================================================
    // Interpolation - static functions
    //=========================================================================
    void interpolation_linearFunction()
    {
        QCOMPARE(Interpolation::linear(0.0), 0.0);
        QCOMPARE(Interpolation::linear(1.5), 1.5);
        QCOMPARE(Interpolation::linear(-2.0), -2.0);
    }

    void interpolation_gaussianFunction()
    {
        // gaussian(0) should be 1 (exp(0) = 1)
        QVERIFY(qAbs(Interpolation::gaussian(0.0) - 1.0) < 1e-10);
        // gaussian should always be positive
        QVERIFY(Interpolation::gaussian(1.0) > 0.0);
        QVERIFY(Interpolation::gaussian(-1.0) > 0.0);
        // gaussian should be symmetric
        QVERIFY(qAbs(Interpolation::gaussian(1.0) - Interpolation::gaussian(-1.0)) < 1e-10);
        // gaussian should decrease with distance
        QVERIFY(Interpolation::gaussian(0.0) > Interpolation::gaussian(1.0));
    }

    void interpolation_squareFunction()
    {
        // square(0) should be large (1)
        double sq0 = Interpolation::square(0.0);
        QVERIFY(qAbs(sq0 - 1.0) < 1e-10);
        // square(x) = max(-(1/9)*x^2 + 1, 0); at x=1: 8/9
        // Note: implementation uses float (1.0f/9.0f), so tolerance must account for float precision
        double sq1 = Interpolation::square(1.0);
        QVERIFY(qAbs(sq1 - 8.0/9.0) < 1e-5);
    }

    void interpolation_cubicFunction()
    {
        double c0 = Interpolation::cubic(0.0);
        // At 0, result depends on specific cubic formula
        QVERIFY(std::isfinite(c0));
        QVERIFY(Interpolation::cubic(1.0) != Interpolation::cubic(2.0));
    }

    void interpolation_interpolateSignalRef()
    {
        // Create a simple 3x3 interpolation matrix (identity-like)
        SparseMatrix<float> mat(3, 3);
        mat.insert(0, 0) = 1.0f;
        mat.insert(1, 1) = 1.0f;
        mat.insert(2, 2) = 1.0f;
        mat.makeCompressed();

        VectorXf data(3);
        data << 1.0f, 2.0f, 3.0f;

        VectorXf result = Interpolation::interpolateSignal(mat, data);
        QCOMPARE(result.size(), (Index)3);
        QVERIFY(result.isApprox(data, 1e-6f));
    }

    void interpolation_interpolateSignalSPtr()
    {
        auto mat = QSharedPointer<SparseMatrix<float>>::create(3, 3);
        mat->insert(0, 0) = 0.5f;
        mat->insert(0, 1) = 0.5f;
        mat->insert(1, 1) = 1.0f;
        mat->insert(2, 2) = 1.0f;
        mat->makeCompressed();

        auto data = QSharedPointer<VectorXf>::create(3);
        (*data) << 2.0f, 4.0f, 6.0f;

        VectorXf result = Interpolation::interpolateSignal(mat, data);
        QCOMPARE(result.size(), (Index)3);
        QVERIFY(qAbs(result(0) - 3.0f) < 1e-5f); // 0.5*2 + 0.5*4
        QVERIFY(qAbs(result(1) - 4.0f) < 1e-5f);
        QVERIFY(qAbs(result(2) - 6.0f) < 1e-5f);
    }

    void interpolation_createInterpolationMat()
    {
        // Simple setup: 4 vertices, 2 sensors projected to vertices 0 and 2
        VectorXi projectedSensors(2);
        projectedSensors << 0, 2;

        // Distance table: nVertices x nSensors (4 x 2)
        auto matDist = QSharedPointer<MatrixXd>::create(4, 2);
        (*matDist) << 0.0, 0.3,
                      0.1, 0.2,
                      0.2, 0.0,
                      0.3, 0.1;

        auto mat = Interpolation::createInterpolationMat(
            projectedSensors, matDist, Interpolation::linear, 0.5);

        QVERIFY(mat != nullptr);
        QCOMPARE(mat->rows(), (Index)4);
        QCOMPARE(mat->cols(), (Index)2);
    }

    //=========================================================================
    // GeometryInfo - projection & scdc
    //=========================================================================
    void geometryInfo_projectSensors()
    {
        // 4 vertices in a plane
        MatrixX3f verts(4, 3);
        verts << 0, 0, 0,
                 1, 0, 0,
                 0, 1, 0,
                 1, 1, 0;

        // 2 sensor positions
        MatrixX3f sensors(2, 3);
        sensors << 0.1f, 0.1f, 0.5f,
                   0.9f, 0.9f, 0.5f;

        VectorXi projected = GeometryInfo::projectSensors(verts, sensors);
        QCOMPARE(projected.size(), (Index)2);
        // Nearest to (0.1, 0.1, 0.5) should be vertex 0
        QCOMPARE(projected(0), 0);
        // Nearest to (0.9, 0.9, 0.5) should be vertex 3
        QCOMPARE(projected(1), 3);
    }

    void geometryInfo_scdcBasic()
    {
        // Simple mesh: 4 vertices forming a square
        MatrixX3f verts(4, 3);
        verts << 0, 0, 0,
                 1, 0, 0,
                 0, 1, 0,
                 1, 1, 0;

        // Connectivity: each vertex connected to adjacent vertices
        std::vector<VectorXi> neighbors(4);
        VectorXi n0(2); n0 << 1, 2; neighbors[0] = n0;
        VectorXi n1(2); n1 << 0, 3; neighbors[1] = n1;
        VectorXi n2(2); n2 << 0, 3; neighbors[2] = n2;
        VectorXi n3(2); n3 << 1, 2; neighbors[3] = n3;

        VectorXi subset(2);
        subset << 0, 3;

        auto distMatrix = GeometryInfo::scdc(verts, neighbors, subset, 10.0);
        QVERIFY(distMatrix != nullptr);
        QCOMPARE(distMatrix->rows(), (Index)4);
        QCOMPARE(distMatrix->cols(), (Index)2);

        // Distance from vertex 0 to itself should be 0
        QVERIFY(qAbs((*distMatrix)(0, 0)) < 1e-6);
    }

    //=========================================================================
    // MeshFactory - sphere & plate creation
    //=========================================================================
    void meshFactory_createSphere()
    {
        auto sphere = MeshFactory::createSphere(
            QVector3D(0, 0, 0), 0.01f, Qt::red, 1);
        QVERIFY(sphere != nullptr);
    }

    void meshFactory_createSphereSubdiv0()
    {
        auto sphere = MeshFactory::createSphere(
            QVector3D(1, 2, 3), 0.05f, Qt::blue, 0);
        QVERIFY(sphere != nullptr);
    }

    void meshFactory_createPlate()
    {
        QMatrix4x4 orientation;
        orientation.setToIdentity();
        auto plate = MeshFactory::createPlate(
            QVector3D(0, 0, 0), orientation, Qt::green, 0.02f);
        QVERIFY(plate != nullptr);
    }

    void meshFactory_createBarbell()
    {
        QMatrix4x4 orientation;
        orientation.setToIdentity();
        auto barbell = MeshFactory::createBarbell(
            QVector3D(0, 0, 0), orientation, Qt::yellow, 0.02f);
        QVERIFY(barbell != nullptr);
    }

    void meshFactory_createBatchedSpheres()
    {
        QVector<QVector3D> positions;
        positions.append(QVector3D(0, 0, 0));
        positions.append(QVector3D(0.1f, 0, 0));
        positions.append(QVector3D(0, 0.1f, 0));

        auto batched = MeshFactory::createBatchedSpheres(positions, 0.005f, Qt::white, 0);
        QVERIFY(batched != nullptr);
    }

    void meshFactory_sphereVertexCount()
    {
        int count0 = MeshFactory::sphereVertexCount(0);
        int count1 = MeshFactory::sphereVertexCount(1);
        int count2 = MeshFactory::sphereVertexCount(2);

        // Each subdivision increases vertex count
        QVERIFY(count0 > 0);
        QVERIFY(count1 > count0);
        QVERIFY(count2 > count1);

        // Icosahedron has 12 vertices at subdivision 0
        QCOMPARE(count0, 12);
    }

    //=========================================================================
    // DataLoader - I/O with sample data
    //=========================================================================
    void dataLoader_loadSensors()
    {
        QString rawFile = dataPath() + "MEG/sample/sample_audvis_trunc_raw.fif";
        if (!QFile::exists(rawFile)) {
            QSKIP("Raw sample file not available");
        }

        auto result = DataLoader::loadSensors(rawFile);
        QVERIFY(result.hasInfo);
        QVERIFY(result.info.nchan > 0);
    }

    void dataLoader_loadSensorsNonExistent()
    {
        auto result = DataLoader::loadSensors("/nonexistent/file.fif");
        QVERIFY(!result.hasInfo);
    }

    void dataLoader_loadEvoked()
    {
        QString evokedFile = dataPath() + "MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(evokedFile)) {
            QSKIP("Evoked sample file not available");
        }

        auto evoked = DataLoader::loadEvoked(evokedFile, 0);
        QVERIFY(evoked.data.rows() > 0);
    }

    void dataLoader_probeEvokedSets()
    {
        QString evokedFile = dataPath() + "MEG/sample/sample_audvis-ave.fif";
        if (!QFile::exists(evokedFile)) {
            QSKIP("Evoked sample file not available");
        }

        QStringList sets = DataLoader::probeEvokedSets(evokedFile);
        QVERIFY(sets.size() > 0);
    }

    void dataLoader_loadSourceSpace()
    {
        QString fwdFile = dataPath() + "MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif";
        if (!QFile::exists(fwdFile)) {
            QSKIP("Forward solution file not available");
        }

        auto srcSpace = DataLoader::loadSourceSpace(fwdFile);
        // Should have loaded something if file exists
        QVERIFY(!srcSpace.isEmpty());
    }

    void dataLoader_loadDipolesNonExistent()
    {
        auto dipoles = DataLoader::loadDipoles("/nonexistent/file.dip");
        QCOMPARE(dipoles.size(), 0);
    }

    void dataLoader_loadHeadToMriTransform()
    {
        QString transFile = dataPath() + "MEG/sample/all-trans.fif";
        if (!QFile::exists(transFile)) {
            QSKIP("Transform file not available");
        }

        FIFFLIB::FiffCoordTrans trans;
        bool ok = DataLoader::loadHeadToMriTransform(transFile, trans);
        QVERIFY(ok);
    }

    //=========================================================================
    // RtSourceDataController - setters
    //=========================================================================
    void rtSourceDataController_defaultCtor()
    {
        RtSourceDataController ctrl;
        // Just verify it can be constructed without crash
        QVERIFY(true);
    }

    //=========================================================================
    // RtSensorDataController - setters
    //=========================================================================
    void rtSensorDataController_defaultCtor()
    {
        RtSensorDataController ctrl;
        QVERIFY(true);
    }
};

QTEST_GUILESS_MAIN(TestDisp3dHeadless)

#include "test_disp3d_headless.moc"
