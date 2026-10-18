//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     electrodes.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    ElectrodesPlugin — mne_inspect plugin that loads an sEEG / ECoG
 *           implantation from BIDS, FIFF or CSV into an IEEGLIB::IeegImplantation,
 *           draws it through DISP3DLIB::ElectrodeObject, and publishes the
 *           selection through MultimodalScene.
 *
 *           NOTE: mne_inspect does not yet ship a plugin loader. The plugin
 *           is built as a standalone static library that the host owns and
 *           drives directly. See plugins/README.md for the wiring still
 *           required.
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

#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>

#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISP3DLIB
{
class MultimodalScene;
}

namespace ELECTRODESPLUGIN
{

//=============================================================================================================
/**
 * @brief Source of the electrode geometry the plugin currently holds.
 */
enum class ElectrodeSource
{
    None = 0,
    Fiff, /**< Digitiser points read from a .fif file. */
    Csv,  /**< CSV file with label,x,y,z[,array,layout] columns. */
    Bids  /**< BIDS iEEG *_electrodes.tsv with its *_coordsystem.json. */
};

//=============================================================================================================
/**
 * @brief mne_inspect plugin: load and publish a digitised electrode set.
 *
 * Responsibilities:
 *   - Load sEEG / ECoG contact positions from BIDS iEEG (`*_electrodes.tsv`),
 *     FIFF (`FiffDigPointSet`) or CSV (`label,x,y,z[,lead,type]`).
 *   - Group contacts into the depth leads, strips and grids of an
 *     `IEEGLIB::IeegImplantation`.
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
     * or `FIFFV_POINT_EXTRA`, and groups them into a single lead named after
     * the file. Existing data is replaced.
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
     *   label, x, y, z [, lead [, type]]
     *
     * Positions are in metres. `type` is one of `depth`, `strip`, `grid`
     * (case-insensitive); without it, contact spacing decides. Rows sharing
     * the same `lead` form one lead; without one, all rows form a single lead
     * named after the file.
     *
     * @param[in] path   Path to a .csv file.
     * @return true if at least one contact row parsed successfully.
     */
    bool loadCsv(const QString& path);

    //=========================================================================================================
    /**
     * Load sEEG / ECoG contacts from a BIDS iEEG `*_electrodes.tsv`, read and grouped into
     * leads by IEEGLIB::IeegImplantation::readBids.
     *
     * Positions in "ACPC" or "ScanRAS" are scanner RAS of the subject's T1, as MNE-BIDS reads
     * them; they are moved into the surface RAS of the FreeSurfer surfaces using the T1 header,
     * and the file is refused without a T1. Other systems are taken as surface RAS.
     *
     * @param[in] path    Path to a *_electrodes.tsv file.
     * @param[in] t1Path  The subject's mri/T1.mgz.
     * @return true if at least one contact with coordinates was placed; see @ref lastError otherwise.
     */
    bool loadBids(const QString& path, const QString& t1Path = QString());

    //=========================================================================================================
    /**
     * Load one value per contact (e.g. high-gamma power) from a text file with the columns
     * `name, value`, separated by a tab or a comma. An optional header row and `#` comments are
     * skipped. Names that match no loaded contact are ignored.
     *
     * @param[in] path   Path to a .tsv or .csv file.
     * @return true if at least one value matched a loaded contact; see @ref lastError otherwise.
     */
    bool loadValues(const QString& path);

    //=========================================================================================================
    /**
     * Replace the contact values. Names that match no loaded contact are dropped; an empty map
     * clears them. Emits @ref valuesChanged.
     */
    void setValues(const QMap<QString, float>& values);

    /**
     * @return Value per contact name; empty when none are loaded.
     */
    const QMap<QString, float>& values() const
    {
        return m_values;
    }

    /**
     * @return Why the last @ref loadBids or @ref loadValues failed, or an empty string.
     */
    QString lastError() const
    {
        return m_lastError;
    }

    //=========================================================================================================
    /**
     * Replace the implantation directly, e.g. one built in code.
     */
    void setImplantation(const IEEGLIB::IeegImplantation& implantation);

    //=========================================================================================================
    /**
     * @return Source format of the most recent successful load.
     */
    ElectrodeSource source() const
    {
        return m_source;
    }

    //=========================================================================================================
    /**
     * @return Path of the most recent successful load. Empty if none.
     */
    QString sourcePath() const
    {
        return m_sourcePath;
    }

    //=========================================================================================================
    /**
     * @return Number of leads currently held.
     */
    int leadCount() const;

    //=========================================================================================================
    /**
     * @return Total number of contacts across all arrays.
     */
    int contactCount() const;

    //=========================================================================================================
    /**
     * @return Underlying renderable. Owned by the plugin; never null.
     */
    DISP3DLIB::ElectrodeObject* electrodeObject() const
    {
        return m_object.get();
    }

    //=========================================================================================================
    /**
     * @return Stable scene-layer id used by @ref attachScene.
     */
    QString sceneLayerId() const
    {
        return QStringLiteral("electrodes");
    }

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
     * Find the contact whose position is closest to @p worldPosition (in
     * MRI surface RAS coordinates) and select it. Used for cross-modality
     * jumps: when the user picks an MRI voxel or surface vertex, the
     * Pick dock asks the electrodes plugin to highlight the nearest contact.
     *
     * @return Name of the highlighted contact, or empty string if no
     *         contacts are loaded.
     */
    QString highlightNearest(const QVector3D& worldPosition);

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

    /** Emitted when the contact values change, including when new electrodes clear them. */
    void valuesChanged();

    /** Emitted when the contact selection changes (either via pick or
     *  @ref selectContact). Empty string means cleared. */
    void contactPicked(const QString& contactName);

private:
    void publishToScene();
    void rebuild(const IEEGLIB::IeegImplantation& implantation);

    std::unique_ptr<DISP3DLIB::ElectrodeObject> m_object;
    DISP3DLIB::MultimodalScene* m_scene = nullptr;
    ElectrodeSource m_source = ElectrodeSource::None;
    QString m_sourcePath;
    QString m_lastError;
    QMap<QString, float> m_values;
};

} // namespace ELECTRODESPLUGIN

#endif // MNEINSPECT_ELECTRODES_PLUGIN_H
