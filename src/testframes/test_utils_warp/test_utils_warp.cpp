#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <QTemporaryFile>
#include <utils/warp.h>

using namespace UTILSLIB;
using namespace Eigen;

class TestUtilsWarp : public QObject
{
    Q_OBJECT

private slots:
    void testIdentityWarp()
    {
        // When source and destination landmarks are the same, warp should be identity
        MatrixXf landmarks(5, 3);
        landmarks << 0,0,0, 1,0,0, 0,1,0, 0,0,1, 1,1,1;
        MatrixXf vertices(3, 3);
        vertices << 0.5f, 0.5f, 0.0f,
                    0.0f, 0.5f, 0.5f,
                    0.5f, 0.0f, 0.5f;

        Warp warp;
        MatrixXf result = warp.calculate(landmarks, landmarks, vertices);
        QCOMPARE(result.rows(), 3);
        QCOMPARE(result.cols(), 3);
        // Result should be close to vertices
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                QVERIFY(std::abs(result(i,j) - vertices(i,j)) < 0.01f);
            }
        }
    }

    void testTranslationWarp()
    {
        // Shift all landmarks by +1 in X
        MatrixXf sLm(4, 3);
        sLm << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        MatrixXf dLm = sLm;
        dLm.col(0).array() += 1.0f;

        MatrixXf vertices(2, 3);
        vertices << 0.5f, 0.5f, 0.0f,
                    0.2f, 0.3f, 0.4f;

        Warp warp;
        MatrixXf result = warp.calculate(sLm, dLm, vertices);
        QCOMPARE(result.rows(), 2);
        // X coordinates should be shifted by ~1
        for (int i = 0; i < 2; ++i) {
            QVERIFY(std::abs(result(i,0) - (vertices(i,0) + 1.0f)) < 0.5f);
        }
    }

    void testCalculateIdentity()
    {
        // When source and destination landmarks are the same,
        // warp should be approximately identity
        MatrixXf sLm(4, 3);
        sLm << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        MatrixXf dLm = sLm; // identity warp

        QList<MatrixXf> vertList;
        MatrixXf v1(3, 3);
        v1 << 0.1f,0.2f,0.3f, 0.4f,0.5f,0.6f, 0.7f,0.8f,0.9f;
        vertList.append(v1);

        Warp warp;
        warp.calculate(sLm, dLm, vertList);
        // After identity warp, vertices should be close to original
        QCOMPARE(vertList[0].rows(), 3);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                QVERIFY(qAbs(vertList[0](i, j) - v1(i, j)) < 0.1f);
            }
        }
    }

    void testCalculateTranslation()
    {
        // When destination landmarks are shifted, vertices should shift too
        MatrixXf sLm(4, 3);
        sLm << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        MatrixXf dLm(4, 3);
        dLm << 1,0,0, 2,0,0, 1,1,0, 1,0,1; // shifted +1 in x

        QList<MatrixXf> vertList;
        MatrixXf v1(1, 3);
        v1 << 0.5f, 0.5f, 0.5f;
        vertList.append(v1);

        Warp warp;
        warp.calculate(sLm, dLm, vertList);
        // The result should be shifted approximately +1 in x
        QVERIFY(vertList[0](0, 0) > 1.0f);
    }

    void testCalculateList()
    {
        MatrixXf sLm(4, 3);
        sLm << 0,0,0, 1,0,0, 0,1,0, 0,0,1;
        MatrixXf dLm = sLm;

        QList<MatrixXf> vertList;
        MatrixXf v1(3, 3);
        v1 << 0.1f,0.2f,0.3f, 0.4f,0.5f,0.6f, 0.7f,0.8f,0.9f;
        MatrixXf v2(2, 3);
        v2 << 0.5f,0.5f,0.5f, 0.2f,0.2f,0.2f;
        vertList.append(v1);
        vertList.append(v2);

        Warp warp;
        warp.calculate(sLm, dLm, vertList);
        QCOMPARE(vertList.size(), 2);
        QCOMPARE(vertList[0].rows(), 3);
        QCOMPARE(vertList[1].rows(), 2);
    }

    void testReadsLmQString()
    {
        // Create a temp electrode file
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QTextStream out(&tmpFile);
        out << "3\n";
        out << "Fp1 -0.0309 0.0838 -0.0017\n";
        out << "Fp2 0.0303 0.0838 -0.0017\n";
        out << "F3 -0.0537 0.0556 0.0404\n";
        tmpFile.close();

        Warp warp;
        MatrixXf electrodes = warp.readsLm(tmpFile.fileName());
        QCOMPARE(electrodes.rows(), 3);
        QCOMPARE(electrodes.cols(), 3);
        QVERIFY(std::abs(electrodes(0,0) - (-0.0309f)) < 0.001f);
    }

    void testReadsLmStdString()
    {
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QTextStream out(&tmpFile);
        out << "2\n";
        out << "E1 1.0 2.0 3.0\n";
        out << "E2 4.0 5.0 6.0\n";
        tmpFile.close();

        Warp warp;
        MatrixXf electrodes = warp.readsLm(tmpFile.fileName().toStdString());
        QCOMPARE(electrodes.rows(), 2);
        QCOMPARE(electrodes.cols(), 3);
        QVERIFY(std::abs(electrodes(1, 2) - 6.0f) < 0.001f);
    }

    void testScalingWarp()
    {
        // Scale by factor 2: dLm = 2 * sLm
        MatrixXf sLm(5, 3);
        sLm << 0,0,0, 1,0,0, 0,1,0, 0,0,1, 1,1,1;
        MatrixXf dLm = sLm * 2.0f;

        MatrixXf vertices(1, 3);
        vertices << 0.5f, 0.5f, 0.5f;

        Warp warp;
        MatrixXf result = warp.calculate(sLm, dLm, vertices);
        // Result should be approximately (1,1,1)
        for (int j = 0; j < 3; ++j) {
            QVERIFY(std::abs(result(0, j) - 1.0f) < 0.5f);
        }
    }
};

QTEST_GUILESS_MAIN(TestUtilsWarp)
#include "test_utils_warp.moc"
