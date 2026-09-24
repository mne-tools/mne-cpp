//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     pickreadoutmodel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    PickReadoutModel — testable model behind MNE Inspect's "Pick"
 *           dock. Listens to MultimodalScene::picked, formats kind-specific
 *           text rows (contact name / world / voxel / value), and drives
 *           cross-modality jumps (surface → MRI crosshair, MRI → nearest
 *           electrode contact).
 *
 *           Decoupled from any QWidget so it can be exercised by
 *           test_mne_inspect_multimodal headlessly.
 */

#ifndef MNEINSPECT_PICKREADOUTMODEL_H
#define MNEINSPECT_PICKREADOUTMODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <disp3D/scene/pickresult.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QString>
#include <QVector3D>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISP3DLIB {
    class MultimodalScene;
}

namespace ELECTRODESPLUGIN {
    class ElectrodesPlugin;
}

namespace MRISLICESPLUGIN {
    class MriSlicesPlugin;
}

namespace MNEINSPECT
{

//=============================================================================================================
/**
 * @brief Reactive model behind the "Pick" dock.
 *
 * Subscribes to @ref DISP3DLIB::MultimodalScene::picked and exposes the
 * latest pick as four formatted rows. Optionally drives cross-modality
 * jumps:
 *   - On a `CorticalVertex` or `ElectrodeContact` pick, asks the attached
 *     `MriSlicesPlugin` to snap its crosshair to the pick world position.
 *   - On a `MriVoxel` pick, asks the attached `ElectrodesPlugin` to
 *     highlight the nearest contact.
 *
 * Either plugin pointer can be null (no cross-modality jump performed).
 */
class PickReadoutModel : public QObject
{
    Q_OBJECT

public:
    explicit PickReadoutModel(QObject* parent = nullptr);
    ~PickReadoutModel() override;

    //=========================================================================================================
    /**
     * Subscribe to the scene's `picked` signal. Passing nullptr detaches.
     */
    void attachScene(DISP3DLIB::MultimodalScene* scene);

    //=========================================================================================================
    /**
     * Provide an electrodes plugin so MRI-voxel picks can highlight the
     * nearest contact. Pass nullptr to disable the jump.
     */
    void setElectrodesPlugin(ELECTRODESPLUGIN::ElectrodesPlugin* plugin);

    //=========================================================================================================
    /**
     * Provide an MRI-slices plugin so surface / contact picks can snap
     * the ortho crosshair to the pick position, and so MRI voxel
     * intensity / voxel-index can be looked up for the readout.
     */
    void setMriSlicesPlugin(MRISLICESPLUGIN::MriSlicesPlugin* plugin);

    //=========================================================================================================
    /** @return Most recent pick payload (default-constructed before the
     *  first hit). */
    const DISP3DLIB::PickResult& lastPick() const { return m_lastPick; }

    //=========================================================================================================
    /** @return Single-line "label" row for the dock (e.g. "Contact: LA0"). */
    QString labelRow() const { return m_labelRow; }

    //=========================================================================================================
    /** @return Single-line "world position" row (RAS millimetres). */
    QString worldRow() const { return m_worldRow; }

    //=========================================================================================================
    /** @return Single-line "voxel / kind-specific extra" row. */
    QString voxelRow() const { return m_voxelRow; }

    //=========================================================================================================
    /** @return Single-line "value" row (overlay value, intensity, …). */
    QString valueRow() const { return m_valueRow; }

    //=========================================================================================================
    /** @return Multi-line concatenation of all four rows. */
    QString text() const;

public slots:
    //=========================================================================================================
    /**
     * Drive the model from a synthesised pick. Called by the scene's
     * `picked` signal in normal operation; tests can call this directly.
     */
    void handlePick(const DISP3DLIB::PickResult& pick);

signals:
    /** Emitted whenever any of the four rows change. */
    void readoutChanged();

private:
    void clearRows();
    void formatRows(const DISP3DLIB::PickResult& pick);
    static QString formatVec(const QVector3D& v);

    DISP3DLIB::MultimodalScene*           m_scene = nullptr;
    ELECTRODESPLUGIN::ElectrodesPlugin*   m_electrodes = nullptr;
    MRISLICESPLUGIN::MriSlicesPlugin*     m_mriSlices = nullptr;
    DISP3DLIB::PickResult                 m_lastPick;
    QString                               m_labelRow;
    QString                               m_worldRow;
    QString                               m_voxelRow;
    QString                               m_valueRow;
};

} // namespace MNEINSPECT

#endif // MNEINSPECT_PICKREADOUTMODEL_H
