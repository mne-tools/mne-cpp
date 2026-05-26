//=============================================================================================================
/**
 * @file     electrodes.h
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
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    ElectrodesPlugin — mne_inspect plugin that loads a digitised
 *           electrode set (EEG / sEEG / ECoG) from FIFF or CSV, surfaces an
 *           ElectrodeArray collection (Depth / Strip / Grid layouts), and
 *           publishes the selection through MultimodalScene.
 *
 *           NOTE: mne_inspect does not yet ship a plugin loader. The plugin
 *           is built as a standalone static library that the host owns and
 *           drives directly. See plugins/README.md for the wiring still
 *           required.
 *
 */

#ifndef MNEINSPECT_ELECTRODES_PLUGIN_H
#define MNEINSPECT_ELECTRODES_PLUGIN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/renderable/electrodeobject.h>
#include <disp3D/scene/pickresult.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QString>
#include <QVector>

#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISP3DLIB {
    class MultimodalScene;
}

namespace ELECTRODESPLUGIN
{

//=============================================================================================================
/**
 * @brief Source of the electrode geometry the plugin currently holds.
 */
enum class ElectrodeSource {
    None = 0,
    Fiff,    /**< Digitiser points read from a .fif file. */
    Csv      /**< CSV file with label,x,y,z[,array,layout] columns. */
};

//=============================================================================================================
/**
 * @brief mne_inspect plugin: load and publish a digitised electrode set.
 *
 * Responsibilities:
 *   - Load EEG / sEEG / ECoG contact positions from FIFF (`FiffDigPointSet`)
 *     or CSV (`label,x,y,z[,array,layout]`).
 *   - Group contacts into `DISP3DLIB::ElectrodeArray` records with the
 *     appropriate `Depth` / `Strip` / `Grid` layout.
 *   - Hand the resulting `DISP3DLIB::ElectrodeObject` to the host through
 *     @ref electrodeObject() and register it as a `SceneLayerKind::Electrode`
 *     layer when @ref attachScene() has been called.
 *   - Translate scene picks (`DISP3DLIB::PickResult` with
 *     `kind == ElectrodeContact`) into a contact-name selection and emit
 *     @ref contactPicked.
 *
 * The plugin is a `QObject` so the host can connect to its signals, but it
 * carries no GUI or QRhi state.
 */
class ElectrodesPlugin : public QObject
{
    Q_OBJECT

public:
    explicit ElectrodesPlugin(QObject* parent = nullptr);
    ~ElectrodesPlugin() override;

    //=========================================================================================================
    /**
     * Load digitiser points from a FIFF file.
     *
     * Reads `FiffDigPointSet`, keeps points whose kind is `FIFFV_POINT_EEG`
     * or `FIFFV_POINT_EXTRA`, and groups them into a single Depth-layout
     * array labelled by the file basename. Existing data is replaced.
     *
     * @param[in] path   Path to a .fif file containing digitiser points.
     * @return true if at least one usable point was found.
     */
    bool loadFiff(const QString& path);

    //=========================================================================================================
    /**
     * Load contacts from a CSV file.
     *
     * Expected columns (header optional, comma-separated):
     *   label, x, y, z [, array_label [, layout]]
     *
     * `layout` is one of `depth`, `strip`, `grid` (case-insensitive). Rows
     * sharing the same `array_label` are grouped into one
     * `DISP3DLIB::ElectrodeArray`. When `array_label` is missing all rows
     * collapse into a single array named after the file.
     *
     * @param[in] path   Path to a .csv file.
     * @return true if at least one contact row parsed successfully.
     */
    bool loadCsv(const QString& path);

    //=========================================================================================================
    /**
     * Replace the held arrays directly. Useful for tests and for callers
     * that materialise the geometry themselves (e.g. coregistration wizard).
     */
    void setArrays(const QVector<DISP3DLIB::ElectrodeArray>& arrays);

    //=========================================================================================================
    /**
     * @return Source format of the most recent successful load.
     */
    ElectrodeSource source() const { return m_source; }

    //=========================================================================================================
    /**
     * @return Path of the most recent successful load. Empty if none.
     */
    QString sourcePath() const { return m_sourcePath; }

    //=========================================================================================================
    /**
     * @return Number of arrays currently held.
     */
    int arrayCount() const;

    //=========================================================================================================
    /**
     * @return Total number of contacts across all arrays.
     */
    int contactCount() const;

    //=========================================================================================================
    /**
     * @return Underlying renderable. Owned by the plugin; never null.
     */
    DISP3DLIB::ElectrodeObject* electrodeObject() const { return m_object.get(); }

    //=========================================================================================================
    /**
     * @return Stable scene-layer id used by @ref attachScene.
     */
    QString sceneLayerId() const { return QStringLiteral("electrodes"); }

    //=========================================================================================================
    /**
     * Attach the plugin to a `MultimodalScene`. Registers (or updates) a
     * `SceneLayerKind::Electrode` layer carrying the current
     * `ElectrodeObject` payload, and subscribes to the scene's `picked`
     * signal so that contact picks are routed through @ref handlePick.
     *
     * Passing a null pointer detaches.
     */
    void attachScene(DISP3DLIB::MultimodalScene* scene);

    //=========================================================================================================
    /**
     * @return Currently selected contact name, or an empty string.
     */
    QString selectedContact() const;

    //=========================================================================================================
    /**
     * Programmatically select a contact by name and emit @ref contactPicked.
     */
    void selectContact(const QString& name);

    //=========================================================================================================
    /**
     * Clear the contact selection.
     */
    void clearSelection();

public slots:
    //=========================================================================================================
    /**
     * Pick callback. If `pick.kind == PickKind::ElectrodeContact` and the
     * pick originated from this plugin's layer, update the contact
     * selection and emit @ref contactPicked.
     */
    void handlePick(const DISP3DLIB::PickResult& pick);

signals:
    /** Emitted when the underlying ElectrodeObject is replaced. */
    void electrodesChanged();

    /** Emitted when the contact selection changes (either via pick or
     *  @ref selectContact). Empty string means cleared. */
    void contactPicked(const QString& contactName);

private:
    void publishToScene();
    void rebuildFromArrays(const QVector<DISP3DLIB::ElectrodeArray>& arrays);

    std::unique_ptr<DISP3DLIB::ElectrodeObject> m_object;
    DISP3DLIB::MultimodalScene*                 m_scene = nullptr;
    ElectrodeSource                             m_source = ElectrodeSource::None;
    QString                                     m_sourcePath;
};

} // namespace ELECTRODESPLUGIN

#endif // MNEINSPECT_ELECTRODES_PLUGIN_H
