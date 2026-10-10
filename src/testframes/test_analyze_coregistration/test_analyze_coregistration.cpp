//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_analyze_coregistration.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 * @brief    Drives the mne_analyze co-registration panel as a user does and checks the transforms it writes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <applications/mne_analyze/plugins/coregistration/coregistration.h>

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/event.h>
#include <anShared/Model/bemdatamodel.h>

#include <disp/viewers/coregsettingsview.h>

#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_dig_point_set.h>

#include <QtTest>
#include <QDockWidget>

#include <memory>

using namespace COREGISTRATIONPLUGIN;
using namespace ANSHAREDLIB;
using namespace DISPLIB;
using namespace FIFFLIB;

//=============================================================================================================

class TestAnalyzeCoregistration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void fitsFiducialsByIdentity();
};

//=============================================================================================================

namespace
{

QString sampleFile(const QString& name)
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/" + name;
}

} // namespace

//=============================================================================================================

void TestAnalyzeCoregistration::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("MNE-CPP-Test"));
    QCoreApplication::setApplicationName(QStringLiteral("test_analyze_coregistration"));
    if (!QFile::exists(sampleFile("MEG/sample/sample_audvis-ave.fif")))
        QSKIP("Sample test data not found");
}

//=============================================================================================================

void TestAnalyzeCoregistration::fitsFiducialsByIdentity()
{
    auto data = QSharedPointer<AnalyzeData>::create();
    CoRegistration plugin;
    plugin.setGlobalData(data);
    plugin.init();
    std::unique_ptr<QDockWidget> dock(plugin.getControl());
    auto* view = qobject_cast<CoregSettingsView*>(dock->widget());
    QVERIFY(view);

    // Fitting before anything is loaded only warns
    emit view->fitFiducials();
    emit view->fitICP();

    // The head surface, selected in the panel's list
    const QString bemPath = sampleFile("subjects/sample/bem/sample-1280-1280-1280-bem.fif");
    auto bem = data->loadModel<BemDataModel>(bemPath);
    plugin.handleEvent(QSharedPointer<Event>::create(SELECTED_MODEL_CHANGED, nullptr, QVariant::fromValue(bem.staticCast<AbstractModel>())));

    // MRI fiducials as stored by mne (LPA, nasion, RPA) are the head fiducials moved by the reference transform.
    // Stored in another order than the digitizer's, they must still be matched by identity.
    const QString avePath = sampleFile("MEG/sample/sample_audvis-ave.fif");
    const FiffCoordTrans reference = FiffCoordTrans::readTransform(sampleFile("MEG/sample/all-trans.fif"), FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
    QFile aveFile(avePath);
    FiffDigPointSet fids = FiffDigPointSet(aveFile).pickTypes({FIFFV_POINT_CARDINAL});
    QCOMPARE(fids.size(), 3);
    fids.applyTransform(reference);
    FiffDigPointSet reordered;
    for (int ident : {FIFFV_POINT_NASION, FIFFV_POINT_RPA, FIFFV_POINT_LPA}) {
        for (int i = 0; i < fids.size(); ++i) {
            if (fids[i].ident == ident)
                reordered << fids[i];
        }
    }
    QTemporaryDir dir;
    const QString fidPath = dir.filePath("sample-fiducials.fif");
    QVERIFY(reordered.write(fidPath));

    emit view->digFileChanged(avePath);
    emit view->fidFileChanged(fidPath);
    emit view->fitFiducials();
    const QString transPath = dir.filePath("fit-trans.fif");
    emit view->storeTrans(transPath);

    const FiffCoordTrans fitted = FiffCoordTrans::readTransform(transPath, FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
    QVERIFY2((fitted.trans - reference.trans).cwiseAbs().maxCoeff() < 1e-4f,
             qPrintable(QStringLiteral("max deviation %1").arg((fitted.trans - reference.trans).cwiseAbs().maxCoeff())));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestAnalyzeCoregistration)
#include "test_analyze_coregistration.moc"
