//=============================================================================================================
/**
 * @file     shot_app_common.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Implementation of the shared real-app shot helpers.
 */

#include "shot_app_common.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QHash>
#include <QRhiWidget>
#include <QWidget>

namespace DOCSHOTS
{

bool forceQRhiNullOnRhiWidgets(QWidget* root)
{
    if (!root) {
        return false;
    }
    if (auto* rw = qobject_cast<QRhiWidget*>(root)) {
        rw->setApi(QRhiWidget::Api::Null);
    }
    const QList<QRhiWidget*> children = root->findChildren<QRhiWidget*>();
    for (QRhiWidget* rw : children) {
        rw->setApi(QRhiWidget::Api::Null);
    }
    return true;
}

void pumpUntilIdle(QWidget* w, int msecs)
{
    if (w) {
        w->update();
    }
    if (msecs <= 0) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        return;
    }
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < msecs) {
        // Drain whatever is queued. We can't ask Qt "are you idle?" in
        // Qt6 (hasPendingEvents was removed), so we just spin the bounded
        // pump until the wall-clock budget is exhausted; the per-tick
        // maxtime keeps the loop responsive without busy-waiting.
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QCoreApplication::sendPostedEvents();
    }
}

namespace {

QHash<QString, AppFixtureLoaders::Loader>& registry()
{
    static QHash<QString, AppFixtureLoaders::Loader> r;
    return r;
}

}  // namespace

void AppFixtureLoaders::registerLoader(const QString& name, Loader loader)
{
    registry().insert(name, std::move(loader));
}

bool AppFixtureLoaders::apply(const QString& name, QObject* mainWindow)
{
    const auto it = registry().constFind(name);
    if (it == registry().constEnd()) {
        return false;
    }
    it.value()(mainWindow);
    return true;
}

bool AppFixtureLoaders::isRegistered(const QString& name)
{
    return registry().contains(name);
}

}  // namespace DOCSHOTS
