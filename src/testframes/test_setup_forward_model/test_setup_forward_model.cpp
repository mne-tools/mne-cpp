// test_setup_forward_model.cpp - Pattern 2 test for mne_setup_forward_model

#include "../../tools/forward/mne_setup_forward_model/mne_setup_forward_model_settings.h"
#include "../../tools/forward/mne_setup_forward_model/setupforwardmodel.h"

#include <QTest>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTemporaryDir>
#include <QCoreApplication>
#include <QFileInfo>

using namespace MNESETUPFORWARDMODEL;

class TestSetupForwardModel : public QObject
{
    Q_OBJECT

public:
    TestSetupForwardModel() {}

private:
    bool createSyntheticTriFile(const QString& filePath, float scale = 100.0f)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;
        QTextStream out(&file);
        out << "4\n";
        out << QString("%1 %2 %3\n").arg(0.0 * scale).arg(0.0 * scale).arg(1.0 * scale);
        out << QString("%1 %2 %3\n").arg(1.0 * scale).arg(0.0 * scale).arg(0.0 * scale);
        out << QString("%1 %2 %3\n").arg(0.0 * scale).arg(1.0 * scale).arg(0.0 * scale);
        out << QString("%1 %2 %3\n").arg(0.0 * scale).arg(0.0 * scale).arg(0.0 * scale);
        out << "4\n";
        out << "1 2 3\n";
        out << "1 2 4\n";
        out << "1 3 4\n";
        out << "2 3 4\n";
        file.close();
        return true;
    }

    void createBemDirectory(const QString& subjectsDir, const QString& subject, bool homogeneous = false)
    {
        QString bemDir = subjectsDir + "/" + subject + "/bem";
        QDir().mkpath(bemDir);
        createSyntheticTriFile(bemDir + "/inner_skull.tri");
        if (!homogeneous) {
            createSyntheticTriFile(bemDir + "/outer_skull.tri", 110.0f);
            createSyntheticTriFile(bemDir + "/outer_skin.tri", 120.0f);
        }
    }

private slots:
    void testSettingsDefaults()
    {
        const char* argv[] = {"test"};
        int argc = 1;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        QCOMPARE(settings.scalpConductivity(), 0.3f);
        QCOMPARE(settings.skullConductivity(), 0.006f);
        QCOMPARE(settings.brainConductivity(), 0.3f);
        QCOMPARE(settings.homogeneous(), false);
        QCOMPARE(settings.useSurfFormat(), false);
        QCOMPARE(settings.icoLevel(), -1);
        QCOMPARE(settings.noSolution(), false);
        QCOMPARE(settings.swap(), true);
        QCOMPARE(settings.meters(), false);
        QCOMPARE(settings.innerShift(), 0.0f);
        QCOMPARE(settings.outerShift(), 0.0f);
        QCOMPARE(settings.scalpShift(), 0.0f);
        QCOMPARE(settings.overwrite(), false);
        QVERIFY(settings.modelName().isEmpty());
    }

    void testSettingsSubject()
    {
        const char* argv[] = {"test", "--subject", "sample"};
        int argc = 3;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        QCOMPARE(settings.subject(), QString("sample"));
    }

    void testSettingsSubjectsDir()
    {
        const char* argv[] = {"test", "--subjects-dir", "/tmp/subjects"};
        int argc = 3;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        QCOMPARE(settings.subjectsDir(), QString("/tmp/subjects"));
    }

    void testSettingsConductivities()
    {
        const char* argv[] = {"test", "--scalpc", "0.5", "--skullc", "0.01", "--brainc", "0.4"};
        int argc = 7;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        QCOMPARE(settings.scalpConductivity(), 0.5f);
        QCOMPARE(settings.skullConductivity(), 0.01f);
        QCOMPARE(settings.brainConductivity(), 0.4f);
    }

    void testSettingsModelName()
    {
        const char* argv[] = {"test", "--model", "mymodel"};
        int argc = 3;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        QCOMPARE(settings.modelName(), QString("mymodel"));
    }

    void testSettingsHomog()
    {
        const char* argv[] = {"test", "--homog"};
        int argc = 2;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        QCOMPARE(settings.homogeneous(), true);
    }

    void testSettingsSurf()
    {
        const char* argv[] = {"test", "--surf"};
        int argc = 2;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        QCOMPARE(settings.useSurfFormat(), true);
    }

    void testSettingsIco()
    {
        const char* argv[] = {"test", "--ico", "4"};
        int argc = 3;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        QCOMPARE(settings.icoLevel(), 4);
    }

    void testSettingsNosol()
    {
        const char* argv[] = {"test", "--nosol"};
        int argc = 2;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        QCOMPARE(settings.noSolution(), true);
    }

    void testSettingsNoswap()
    {
        const char* argv[] = {"test", "--noswap"};
        int argc = 2;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        QCOMPARE(settings.swap(), false);
    }

    void testSettingsMeters()
    {
        const char* argv[] = {"test", "--meters"};
        int argc = 2;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        QCOMPARE(settings.meters(), true);
    }

    void testSettingsShifts()
    {
        const char* argv[] = {"test", "--innershift", "2.5", "--outershift", "3.0", "--scalpshift", "1.5"};
        int argc = 7;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        QCOMPARE(settings.innerShift(), 2.5f);
        QCOMPARE(settings.outerShift(), 3.0f);
        QCOMPARE(settings.scalpShift(), 1.5f);
    }

    void testSettingsOverwrite()
    {
        const char* argv[] = {"test", "--overwrite"};
        int argc = 2;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        QCOMPARE(settings.overwrite(), true);
    }

    void testSettingsCombined()
    {
        const char* argv[] = {"test",
            "--subject", "sample", "--subjects-dir", "/tmp/sub",
            "--homog", "--nosol", "--noswap", "--meters", "--overwrite",
            "--scalpc", "0.5", "--skullc", "0.01", "--brainc", "0.4",
            "--model", "mymodel",
            "--innershift", "2.0", "--outershift", "3.0", "--scalpshift", "1.0",
            "--ico", "5", "--surf"};
        int argc = 27;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        QCOMPARE(settings.subject(), QString("sample"));
        QCOMPARE(settings.subjectsDir(), QString("/tmp/sub"));
        QCOMPARE(settings.homogeneous(), true);
        QCOMPARE(settings.noSolution(), true);
        QCOMPARE(settings.swap(), false);
        QCOMPARE(settings.meters(), true);
        QCOMPARE(settings.overwrite(), true);
        QCOMPARE(settings.scalpConductivity(), 0.5f);
        QCOMPARE(settings.skullConductivity(), 0.01f);
        QCOMPARE(settings.brainConductivity(), 0.4f);
        QCOMPARE(settings.modelName(), QString("mymodel"));
        QCOMPARE(settings.innerShift(), 2.0f);
        QCOMPARE(settings.outerShift(), 3.0f);
        QCOMPARE(settings.scalpShift(), 1.0f);
        QCOMPARE(settings.icoLevel(), 5);
        QCOMPARE(settings.useSurfFormat(), true);
    }

    void testRunNoSubjectsDir()
    {
        const char* argv[] = {"test", "--subject", "sample"};
        int argc = 3;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 1);
    }

    void testRunNoSubject()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData()};
        int argc = 3;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 1);
    }

    void testRunMissingSubjectDir()
    {
        const char* argv[] = {"test", "--subjects-dir", "/nonexistent/path", "--subject", "sample"};
        int argc = 5;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 1);
    }

    void testRunMissingBemDir()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QDir().mkpath(tmpDir.path() + "/sample");
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(), "--subject", "sample"};
        int argc = 5;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 1);
    }

    void testRunMissingInnerSkull()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QDir().mkpath(tmpDir.path() + "/sample/bem");
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(), "--subject", "sample"};
        int argc = 5;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 1);
    }

    void testRunMissingOuterSkull()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        createBemDirectory(tmpDir.path(), "sample", true);
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(), "--subject", "sample"};
        int argc = 5;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 1);
    }

    void testRunHomogNoSol()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        createBemDirectory(tmpDir.path(), "sample", true);
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(),
            "--subject", "sample", "--homog", "--nosol", "--overwrite"};
        int argc = 8;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 0);
        QString bemDir = tmpDir.path() + "/sample/bem";
        QDir dir(bemDir);
        QVERIFY(!dir.entryList(QStringList() << "*-bem.fif", QDir::Files).isEmpty());
        QVERIFY(!dir.entryList(QStringList() << "*.pnt", QDir::Files).isEmpty());
        QVERIFY(!dir.entryList(QStringList() << "*.surf", QDir::Files).isEmpty());
    }

    void testRunThreeLayerNoSol()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        createBemDirectory(tmpDir.path(), "sample", false);
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(),
            "--subject", "sample", "--nosol", "--overwrite"};
        int argc = 7;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 0);
        QString bemDir = tmpDir.path() + "/sample/bem";
        QDir dir(bemDir);
        QCOMPARE(dir.entryList(QStringList() << "*.pnt", QDir::Files).size(), 3);
        QCOMPARE(dir.entryList(QStringList() << "*.surf", QDir::Files).size(), 3);
    }

    void testRunWithModelName()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        createBemDirectory(tmpDir.path(), "sample", true);
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(),
            "--subject", "sample", "--homog", "--nosol", "--overwrite", "--model", "test_model"};
        int argc = 10;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 0);
        QVERIFY(QFileInfo::exists(tmpDir.path() + "/sample/bem/test_model-bem.fif"));
    }

    void testRunOverwriteProtection()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        createBemDirectory(tmpDir.path(), "sample", true);
        QByteArray dirBytes = tmpDir.path().toUtf8();
        {
            const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(),
                "--subject", "sample", "--homog", "--nosol", "--overwrite", "--model", "ow_test"};
            int argc = 10;
            MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
            SetupForwardModel model(settings);
            QCOMPARE(model.run(), 0);
        }
        {
            const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(),
                "--subject", "sample", "--homog", "--nosol", "--model", "ow_test"};
            int argc = 9;
            MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
            SetupForwardModel model(settings);
            QCOMPARE(model.run(), 1);
        }
    }

    void testRunWithShift()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        createBemDirectory(tmpDir.path(), "sample", true);
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(),
            "--subject", "sample", "--homog", "--nosol", "--overwrite", "--innershift", "2.0"};
        int argc = 10;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 0);
    }

    void testRunNoswap()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        createBemDirectory(tmpDir.path(), "sample", true);
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(),
            "--subject", "sample", "--homog", "--nosol", "--overwrite", "--noswap"};
        int argc = 9;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 0);
    }

    void testRunWithMeters()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString bemDir = tmpDir.path() + "/sample/bem";
        QDir().mkpath(bemDir);
        createSyntheticTriFile(bemDir + "/inner_skull.tri", 0.1f);
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(),
            "--subject", "sample", "--homog", "--nosol", "--overwrite", "--meters"};
        int argc = 9;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 0);
    }

    void testRunWithCustomConductivities()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        createBemDirectory(tmpDir.path(), "sample", false);
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(),
            "--subject", "sample", "--nosol", "--overwrite",
            "--scalpc", "0.5", "--skullc", "0.01", "--brainc", "0.4"};
        int argc = 13;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 0);
    }

    void testRunThreeLayerWithShifts()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        createBemDirectory(tmpDir.path(), "sample", false);
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(),
            "--subject", "sample", "--nosol", "--overwrite",
            "--innershift", "1.0", "--outershift", "2.0", "--scalpshift", "3.0"};
        int argc = 13;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 0);
    }

    void testRunSubjectNamedSurface()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString bemDir = tmpDir.path() + "/sample/bem";
        QDir().mkpath(bemDir);
        createSyntheticTriFile(bemDir + "/sample-inner_skull.tri");
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(),
            "--subject", "sample", "--homog", "--nosol", "--overwrite"};
        int argc = 8;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 0);
    }

    void testPntFileContents()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        createBemDirectory(tmpDir.path(), "sample", true);
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(),
            "--subject", "sample", "--homog", "--nosol", "--overwrite"};
        int argc = 8;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 0);
        QString bemDir = tmpDir.path() + "/sample/bem";
        QDir dir(bemDir);
        QStringList pntFiles = dir.entryList(QStringList() << "*.pnt", QDir::Files);
        QVERIFY(!pntFiles.isEmpty());
        QFile pntFile(bemDir + "/" + pntFiles.first());
        QVERIFY(pntFile.open(QIODevice::ReadOnly | QIODevice::Text));
        QTextStream in(&pntFile);
        int nVerts;
        in >> nVerts;
        QCOMPARE(nVerts, 4);
        pntFile.close();
    }

    void testRunHomogWithSolution()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        createBemDirectory(tmpDir.path(), "sample", true);
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(),
            "--subject", "sample", "--homog", "--overwrite"};
        int argc = 7;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 0);
        QString bemDir = tmpDir.path() + "/sample/bem";
        QDir dir(bemDir);
        QVERIFY(!dir.entryList(QStringList() << "*-sol.fif", QDir::Files).isEmpty());
    }

    void testRunThreeLayerWithSolution()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        createBemDirectory(tmpDir.path(), "sample", false);
        QByteArray dirBytes = tmpDir.path().toUtf8();
        const char* argv[] = {"test", "--subjects-dir", dirBytes.constData(),
            "--subject", "sample", "--overwrite"};
        int argc = 5;
        MNESetupForwardModelSettings settings(&argc, const_cast<char**>(argv));
        SetupForwardModel model(settings);
        QCOMPARE(model.run(), 0);
        QString bemDir = tmpDir.path() + "/sample/bem";
        QDir dir(bemDir);
        QVERIFY(!dir.entryList(QStringList() << "*-sol.fif", QDir::Files).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestSetupForwardModel)
#include "test_setup_forward_model.moc"
