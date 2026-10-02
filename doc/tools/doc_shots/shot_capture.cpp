//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     shot_capture.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 *
 * @brief    Implementation of the deterministic documentation screenshot capture.
 */

#include "shot_capture.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QSaveFile>
#include <QFontDatabase>
#include <QFontInfo>
#include <QImage>
#include <QImageWriter>
#include <QLocale>
#include <QMutex>
#include <QMutexLocker>
#include <QPixmap>
#include <QScreen>
#include <QSet>
#include <QStringList>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTextStream>
#include <QThread>
#include <QThreadPool>
#include <QWidget>

#include <algorithm>

namespace DOCSHOTS
{

namespace
{

QString sizeText(const QSize& size)
{
    return QStringLiteral("%1x%2").arg(size.width()).arg(size.height());
}

QString sizeDiagnostics(const QWidget& window, const QSize& requested)
{
    QStringList lines{QStringLiteral("window is %1, requested %2 (minimum size hint %3)")
                          .arg(sizeText(window.size()), sizeText(requested), sizeText(window.minimumSizeHint()))};

    // Explicit minimum sizes are what keeps a window from shrinking; name the largest ones.
    QList<const QWidget*> constrained;
    for (const QWidget* child : window.findChildren<QWidget*>()) {
        if (child->isVisibleTo(&window) && !child->minimumSize().isEmpty()) {
            constrained.append(child);
        }
    }
    std::sort(constrained.begin(), constrained.end(), [](const QWidget* a, const QWidget* b) {
        const QSize sa = a->minimumSize();
        const QSize sb = b->minimumSize();
        return sa.width() * sa.height() > sb.width() * sb.height();
    });
    for (const QWidget* child : constrained.mid(0, 5)) {
        lines << QStringLiteral("  %1 '%2' has minimum size %3")
                     .arg(QString::fromLatin1(child->metaObject()->className()), child->objectName(),
                          sizeText(child->minimumSize()));
    }
    return lines.join(QLatin1Char('\n'));
}

void printOnce(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    static QSet<QString> seen;
    static QMutex mutex;
    {
        const QMutexLocker lock(&mutex);
        if (seen.contains(message)) {
            return;
        }
        seen.insert(message);
    }
    QTextStream(stderr) << qFormatLogMessage(type, context, message) << "\n";
}

} // namespace

void installDeduplicatingMessageHandler()
{
    qInstallMessageHandler(printOnce);
}

void prepareProcessEnvironment()
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    // Without a console, Windows Qt logs to OutputDebugString and CI would show no diagnostics.
    qputenv("QT_FORCE_STDERR_LOGGING", "1");
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
    qputenv("QT_SCALE_FACTOR", "1");
    qputenv("QT_FONT_DPI", "96");
    qunsetenv("QT_SCREEN_SCALE_FACTORS");
    qunsetenv("QT_STYLE_OVERRIDE");
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
}

bool applyDeterministicTheme(QApplication& app, QString& err)
{
    QString family;
    for (const char* file : {":/doc_shots/fonts/DejaVuSans.ttf", ":/doc_shots/fonts/DejaVuSans-Bold.ttf"}) {
        const int id = QFontDatabase::addApplicationFont(QString::fromLatin1(file));
        if (id < 0 || QFontDatabase::applicationFontFamilies(id).isEmpty()) {
            err = QStringLiteral("cannot load the bundled font %1").arg(QString::fromLatin1(file));
            return false;
        }
        family = QFontDatabase::applicationFontFamilies(id).constFirst();
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
#endif
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QApplication::setPalette(app.style()->standardPalette());

    QFont font(family);
    font.setPixelSize(kFontPixelSize);
    font.setHintingPreference(QFont::PreferNoHinting);
    QApplication::setFont(font);
    if (QFontInfo(QApplication::font()).family() != family) {
        err = QStringLiteral("application font resolves to '%1' instead of the bundled '%2' (platform %3, %4 "
                             "families available)")
                  .arg(QFontInfo(QApplication::font()).family(), family, QGuiApplication::platformName())
                  .arg(QFontDatabase::families().size());
        return false;
    }

    QGuiApplication::styleHints()->setCursorFlashTime(0);
    for (const Qt::UIEffect effect : {Qt::UI_AnimateMenu, Qt::UI_FadeMenu, Qt::UI_AnimateCombo,
                                      Qt::UI_AnimateTooltip, Qt::UI_FadeTooltip, Qt::UI_AnimateToolBox}) {
        QApplication::setEffectEnabled(effect, false);
    }
    return true;
}

QString environmentSummary()
{
    const QFontInfo font(QApplication::font());
    const QScreen* screen = QGuiApplication::primaryScreen();
    return QStringLiteral("Qt %1, platform %2, style %3, font '%4' %5px, device pixel ratio %6, locale %7")
        .arg(QString::fromLatin1(qVersion()), QGuiApplication::platformName(),
             QApplication::style() ? QApplication::style()->name() : QStringLiteral("none"), font.family())
        .arg(font.pixelSize())
        .arg(screen ? screen->devicePixelRatio() : 0.0)
        .arg(QLocale().name());
}

bool showAtSize(QWidget& window, const QSize& size, QString& err)
{
    window.resize(size);
    window.show();
    if (!waitUntilSettled(window, err)) {
        return false;
    }
    if (window.size() != size) {
        err = sizeDiagnostics(window, size);
        return false;
    }
    return true;
}

bool waitUntilSettled(QWidget& window, QString& err, int settleMs, int timeoutMs)
{
    if (!window.isVisible()) {
        err = QStringLiteral("window is not shown");
        return false;
    }
    QElapsedTimer total;
    QElapsedTimer unchanged;
    total.start();
    unchanged.start();
    QImage last;
    int frames = 0;
    while (true) {
        QCoreApplication::sendPostedEvents();
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        const QImage frame = window.grab().toImage();
        ++frames;
        if (frame != last || QThreadPool::globalInstance()->activeThreadCount() > 0) {
            last = frame;
            unchanged.restart();
        } else if (unchanged.elapsed() >= settleMs) {
            return true;
        }
        if (total.elapsed() > timeoutMs) {
            err = QStringLiteral("window did not settle within %1 ms (%2 frames grabbed, %3 thread-pool jobs "
                                 "running): something keeps repainting or loading")
                      .arg(timeoutMs)
                      .arg(frames)
                      .arg(QThreadPool::globalInstance()->activeThreadCount());
            return false;
        }
        QThread::msleep(10);
    }
}

bool captureWindow(QWidget& window, const QSize& size, const QString& outPath, QString& err)
{
    if (!waitUntilSettled(window, err)) {
        return false;
    }
    if (window.size() != size) {
        err = sizeDiagnostics(window, size);
        return false;
    }
    const QImage image = window.grab().toImage();
    if (image.size() != size) {
        err = QStringLiteral("grab is %1 for a %2 window (device pixel ratio %3); captures must use ratio 1")
                  .arg(sizeText(image.size()), sizeText(size))
                  .arg(image.devicePixelRatio());
        return false;
    }
    return savePng(image, outPath, err);
}

bool savePng(const QImage& image, const QString& outPath, QString& err)
{
    QSaveFile file(outPath);
    if (!file.open(QIODevice::WriteOnly)) {
        err = QStringLiteral("cannot open %1: %2").arg(outPath, file.errorString());
        return false;
    }
    QImageWriter writer(&file, "png");
    if (!writer.write(image.convertToFormat(QImage::Format_RGB888))) {
        err = QStringLiteral("cannot write %1: %2").arg(outPath, writer.errorString());
        return false;
    }
    if (!file.commit()) {
        err = QStringLiteral("cannot commit %1: %2").arg(outPath, file.errorString());
        return false;
    }
    return true;
}

} // namespace DOCSHOTS
