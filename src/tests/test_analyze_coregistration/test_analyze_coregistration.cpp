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

#include <apps/mne_analyze/plugins/coregistration/coregistration.h>
#include <anShared/Management/analyzedata.h>
#include <anShared/Management/event.h>
#include <anShared/Model/bemdatamodel.h>
#include <mne/viz/backends/qt/kit/panelwidget.h>
#include <mne/viz/panels/coregistration_panel.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/base/string_utils.h>
#include <mne/base/thread_pool.h>
#include <mne/fiff/fiff_dig_point_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QDockWidget>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <future>
#include <memory>
#include <vector>

using namespace COREGISTRATIONPLUGIN;
using namespace mne::analyze;
using namespace mne::viz;
using namespace mne::viz::qt;
using namespace mne;

//=============================================================================================================

class TestAnalyzeCoregistration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void fitsFiducialsByIdentity();
    void icpUsesTheSettingsAtClickTime();
};

//=============================================================================================================

namespace
{

QString sampleFile(const QString& name)
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/" + name;
}

std::string utf8(const QString& path)
{
    return path.toStdString();
}

/** @return The co-registration panel shown in @p dock. */
CoregistrationPanel* panelOf(QDockWidget* dock)
{
    auto* widget = qobject_cast<PanelWidget*>(dock->widget());
    return widget ? dynamic_cast<CoregistrationPanel*>(&widget->panel()) : nullptr;
}

/** Loads the sample head surface, digitizer and MRI fiducials into the panel and fits the fiducials. */
void prepareFit(CoRegistration& plugin, AnalyzeData& data, CoregistrationPanel* view, const QString& sDir)
{
    auto bem = data.loadModel<BemDataModel>(sampleFile("subjects/sample/bem/sample-1280-1280-1280-bem.fif"));
    plugin.handleEvent(QSharedPointer<Event>::create(SELECTED_MODEL_CHANGED, nullptr, QVariant::fromValue(bem.staticCast<AbstractModel>())));
    const QString avePath = sampleFile("MEG/sample/sample_audvis-ave.fif");
    FiffDigPointSet fids = FiffDigPointSet(avePath.toStdString()).pick_types({FIFFV_POINT_CARDINAL});
    fids.apply_transform(FiffCoordTrans(sampleFile("MEG/sample/all-trans.fif").toStdU16String(), FIFFV_COORD_HEAD, FIFFV_COORD_MRI));
    const QString fidPath = sDir + "/sample-fiducials.fif";
    QVERIFY(fids.write(fidPath.toStdString()));
    view->form().set_value("digitizerFile", utf8(avePath));
    view->form().set_value("fiducialFile", utf8(fidPath));
    view->form().trigger("fitFiducials");
}

/** Runs ICP from the panel; pChange (if set) edits the panel after the click but before the fit starts. */
FiffCoordTrans runIcp(CoRegistration& plugin, CoregistrationPanel* view, const QString& sDir, const std::function<void()>& change = {})
{
    Q_UNUSED(plugin)
    view->form().set_value("rmse", std::string());

    // Hold every pool worker until the panel has been edited, so the fit is queued behind them
    ThreadPool& pool = ThreadPool::global();
    std::promise<void> gate;
    const std::shared_future<void> open = gate.get_future().share();
    std::vector<std::future<void>> held;
    held.reserve(pool.thread_count());
    for (std::size_t i = 0; i < pool.thread_count(); ++i)
        held.push_back(pool.submit([open] { open.wait(); }));
    view->form().trigger("fitIcp");
    if (change)
        change();
    gate.set_value();
    for (std::future<void>& f : held)
        f.get();

    if (!QTest::qWaitFor([view] { return !view->form().get<std::string>("rmse").empty(); }, 10000))
        return FiffCoordTrans();
    const QString transPath = sDir + "/icp-trans.fif";
    view->form().set_value("transStoreFile", utf8(transPath));
    return FiffCoordTrans(transPath.toStdU16String(), FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
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
    CoregistrationPanel* view = panelOf(dock.get());
    QVERIFY(view);

    // Fitting before anything is loaded only warns
    view->form().trigger("fitFiducials");
    view->form().trigger("fitIcp");

    // The head surface, selected in the panel's list
    const QString bemPath = sampleFile("subjects/sample/bem/sample-1280-1280-1280-bem.fif");
    auto bem = data->loadModel<BemDataModel>(bemPath);
    plugin.handleEvent(QSharedPointer<Event>::create(SELECTED_MODEL_CHANGED, nullptr, QVariant::fromValue(bem.staticCast<AbstractModel>())));

    // MRI fiducials as stored by mne (LPA, nasion, RPA) are the head fiducials moved by the reference transform.
    // Stored in another order than the digitizer's, they must still be matched by identity.
    const QString avePath = sampleFile("MEG/sample/sample_audvis-ave.fif");
    const FiffCoordTrans reference(sampleFile("MEG/sample/all-trans.fif").toStdU16String(), FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
    FiffDigPointSet fids = FiffDigPointSet(avePath.toStdString()).pick_types({FIFFV_POINT_CARDINAL});
    QCOMPARE(fids.size(), 3);
    fids.apply_transform(reference);
    FiffDigPointSet reordered;
    for (int ident : {FIFFV_POINT_NASION, FIFFV_POINT_RPA, FIFFV_POINT_LPA}) {
        for (int i = 0; i < std::ssize(fids); ++i) {
            if (fids[i].ident == ident)
                reordered.push_back(fids[i]);
        }
    }
    QTemporaryDir dir;
    const QString fidPath = dir.filePath("sample-fiducials.fif");
    QVERIFY(reordered.write(fidPath.toStdString()));

    view->form().set_value("digitizerFile", utf8(avePath));
    view->form().set_value("fiducialFile", utf8(fidPath));
    view->form().trigger("fitFiducials");
    const QString transPath = dir.filePath("fit-trans.fif");
    view->form().set_value("transStoreFile", utf8(transPath));

    const FiffCoordTrans fitted(transPath.toStdU16String(), FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
    QVERIFY2((fitted.trans - reference.trans).cwiseAbs().maxCoeff() < 1e-4f,
             qPrintable(QStringLiteral("max deviation %1").arg((fitted.trans - reference.trans).cwiseAbs().maxCoeff())));

    // A file that is not FIFF is refused as digitizer, fiducial or transform file
    const QString notTrans = dir.filePath("notes.txt");
    QFile notes(notTrans);
    QVERIFY(notes.open(QIODevice::WriteOnly));
    notes.write("not a transform");
    notes.close();
    view->form().set_value("digitizerFile", utf8(notTrans));
    view->form().set_value("fiducialFile", utf8(notTrans));
    view->form().trigger("fitFiducials");
    view->form().set_value("transStoreFile", utf8(transPath));
    QVERIFY(FiffCoordTrans(transPath.toStdU16String(), FIFFV_COORD_HEAD, FIFFV_COORD_MRI).trans.isApprox(fitted.trans));

    // Loading a file without a transform keeps the current one; the reference loads as stored
    view->form().set_value("transFile", utf8(notTrans));
    view->form().set_value("transFile", utf8(sampleFile("MEG/sample/all-trans.fif")));
    view->form().set_value("transStoreFile", utf8(transPath));
    const FiffCoordTrans loaded(transPath.toStdU16String(), FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
    QVERIFY(loaded.trans.isApprox(reference.trans));
}

//=============================================================================================================

void TestAnalyzeCoregistration::icpUsesTheSettingsAtClickTime()
{
    QTemporaryDir dir;
    auto fit = [&dir](const std::function<void(CoregistrationPanel*)>& change) {
        auto data = QSharedPointer<AnalyzeData>::create();
        CoRegistration plugin;
        plugin.setGlobalData(data);
        plugin.init();
        std::unique_ptr<QDockWidget> dock(plugin.getControl());
        CoregistrationPanel* view = panelOf(dock.get());
        prepareFit(plugin, *data, view, dir.path());
        return runIcp(plugin, view, dir.path(), change ? std::function<void()>([&] { change(view); }) : std::function<void()>());
    };

    const FiffCoordTrans clicked = fit({});
    QVERIFY(!clicked.empty());
    // ICP moves the fiducial fit onto the head shape
    const FiffCoordTrans fiducialFit(sampleFile("MEG/sample/all-trans.fif").toStdU16String(), FIFFV_COORD_HEAD, FIFFV_COORD_MRI);
    QVERIFY((clicked.trans - fiducialFit.trans).cwiseAbs().maxCoeff() > 1e-4f);
    // Unticking the head shape points after Fit ICP was clicked must not change the running fit
    const FiffCoordTrans changed = fit([](CoregistrationPanel* view) {
        view->form().set_value("useHsp", false);
    });
    QVERIFY(!changed.empty());
    QVERIFY2((changed.trans - clicked.trans).cwiseAbs().maxCoeff() < 1e-6f,
             qPrintable(QStringLiteral("max deviation %1").arg((changed.trans - clicked.trans).cwiseAbs().maxCoeff())));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestAnalyzeCoregistration)
#include "test_analyze_coregistration.moc"
