//=============================================================================================================
/**
 * @file     shot_app_common.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 *       the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *       and the following disclaimer in the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used to endorse or
 *       promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    Shared helpers for real-app screenshot kinds: QRhi → Null fallback,
 *           bounded event-loop pump, and a tiny named-fixture registry that
 *           per-app shot files use to advertise their setup-JSON hooks.
 *
 *           To add fixtures for another app (e.g. mne_align): in
 *           `shot_mne_align_app.cpp` call
 *           `AppFixtureLoaders::registerLoader("name", [](QObject* mw){ ... })`
 *           at static-init time (an anonymous-namespace `int dummy = []{...}()`
 *           trick works well), then reference the same name from the manifest
 *           setup block. The lambda receives the host MainWindow as a
 *           `QObject*`; `qobject_cast` to the concrete type and push fixture
 *           data via the app's public accessors.
 */

#ifndef DOCSHOTS_SHOT_APP_COMMON_H
#define DOCSHOTS_SHOT_APP_COMMON_H

#include <QString>

#include <functional>

class QObject;
class QWidget;

namespace DOCSHOTS
{

//=============================================================================================================
/**
 * Walk @p root and force every QRhiWidget descendant (and @p root itself if
 * it is a QRhiWidget) to use the QRhi `Null` backend. The `Null` backend is
 * a no-op renderer that lets the widgets initialise and lay out under the
 * offscreen QPA without touching Metal / Vulkan / OpenGL — which would
 * crash on a typical headless CI box.
 *
 * MUST be called BEFORE @p root is shown. Once a QRhiWidget initialises its
 * QRhi resources, the API selection is locked.
 *
 * @param[in] root  Top-level widget; usually the application's MainWindow.
 * @return          True if the walk completed (always, in practice). False
 *                  on a null @p root.
 */
bool forceQRhiNullOnRhiWidgets(QWidget* root);

//=============================================================================================================
/**
 * Spin the application event loop for up to @p msecs milliseconds, returning
 * early once the loop reports no further pending events. Used after `show()`
 * to give Qt a chance to lay out the dock widgets, populate the toolbar,
 * resize child widgets, etc. before we call `grab()`.
 *
 * @param[in] w      Widget whose `update()` is requested at the start.
 *                   Pass `nullptr` to skip the update request.
 * @param[in] msecs  Hard upper bound on the pump duration.
 */
void pumpUntilIdle(QWidget* w, int msecs = 250);

//=============================================================================================================
/**
 * Per-app named fixture registry. Shot renderers consume a setup-JSON key
 * such as `"fixtures": ["demo_electrodes", "demo_mri"]` and call
 * `AppFixtureLoaders::apply` to push each fixture into the live MainWindow.
 * Per-app shot files register their loaders at static-init time via
 * `AppFixtureLoaders::registerLoader`.
 */
class AppFixtureLoaders
{
public:
    using Loader = std::function<void(QObject* /*mainWindow*/)>;

    /** Register a loader by name. Last-write-wins if the name is reused. */
    static void registerLoader(const QString& name, Loader loader);

    /** Apply a previously registered loader. Returns false if @p name is unknown. */
    static bool apply(const QString& name, QObject* mainWindow);

    /** True if @p name has been registered. */
    static bool isRegistered(const QString& name);
};

}  // namespace DOCSHOTS

#endif  // DOCSHOTS_SHOT_APP_COMMON_H
