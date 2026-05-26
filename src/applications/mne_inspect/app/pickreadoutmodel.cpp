//=============================================================================================================
/**
 * @file     pickreadoutmodel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    PickReadoutModel implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pickreadoutmodel.h"

#include "plugins/electrodes/electrodes.h"
#include "plugins/mri_slices/mri_slices.h"

#include <disp3D/scene/multimodalscene.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStringList>

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEINSPECT;
using namespace DISP3DLIB;

//=============================================================================================================

PickReadoutModel::PickReadoutModel(QObject* parent)
    : QObject(parent)
{
    clearRows();
}

//=============================================================================================================

PickReadoutModel::~PickReadoutModel() = default;

//=============================================================================================================

void PickReadoutModel::attachScene(MultimodalScene* scene)
{
    if (m_scene == scene) {
        return;
    }
    if (m_scene) {
        QObject::disconnect(m_scene, &MultimodalScene::picked,
                            this, &PickReadoutModel::handlePick);
    }
    m_scene = scene;
    if (m_scene) {
        QObject::connect(m_scene, &MultimodalScene::picked,
                         this, &PickReadoutModel::handlePick);
    }
}

//=============================================================================================================

void PickReadoutModel::setElectrodesPlugin(ELECTRODESPLUGIN::ElectrodesPlugin* plugin)
{
    m_electrodes = plugin;
}

//=============================================================================================================

void PickReadoutModel::setMriSlicesPlugin(MRISLICESPLUGIN::MriSlicesPlugin* plugin)
{
    m_mriSlices = plugin;
}

//=============================================================================================================

QString PickReadoutModel::text() const
{
    QStringList rows;
    rows << m_labelRow << m_worldRow << m_voxelRow << m_valueRow;
    return rows.join(QLatin1Char('\n'));
}

//=============================================================================================================

void PickReadoutModel::handlePick(const PickResult& pick)
{
    m_lastPick = pick;
    formatRows(pick);

    // Cross-modality jumps.
    switch (pick.kind) {
        case PickKind::CorticalVertex:
        case PickKind::ElectrodeContact:
            if (m_mriSlices) {
                m_mriSlices->setVoxelOfInterest(pick.world);
            }
            break;
        case PickKind::MriVoxel:
            if (m_electrodes) {
                m_electrodes->highlightNearest(pick.world);
            }
            break;
        default:
            break;
    }

    emit readoutChanged();
}

//=============================================================================================================

void PickReadoutModel::clearRows()
{
    m_labelRow = QStringLiteral("Pick: —");
    m_worldRow = QStringLiteral("World: —");
    m_voxelRow = QStringLiteral("Voxel: —");
    m_valueRow = QStringLiteral("Value: —");
}

//=============================================================================================================

QString PickReadoutModel::formatVec(const QVector3D& v)
{
    return QStringLiteral("(%1, %2, %3) m")
            .arg(v.x(), 0, 'f', 4)
            .arg(v.y(), 0, 'f', 4)
            .arg(v.z(), 0, 'f', 4);
}

//=============================================================================================================

void PickReadoutModel::formatRows(const PickResult& pick)
{
    if (pick.kind == PickKind::None) {
        clearRows();
        return;
    }

    m_worldRow = QStringLiteral("World: %1").arg(formatVec(pick.world));
    m_valueRow = std::isnan(pick.value)
            ? QStringLiteral("Value: —")
            : QStringLiteral("Value: %1").arg(pick.value, 0, 'f', 4);

    switch (pick.kind) {
        case PickKind::ElectrodeContact: {
            m_labelRow = QStringLiteral("Contact: %1")
                            .arg(pick.label.isEmpty() ? QStringLiteral("?") : pick.label);
            if (m_mriSlices && m_mriSlices->volume()) {
                const Eigen::Vector3f vox = m_mriSlices->voxelFromWorld(pick.world);
                m_voxelRow = QStringLiteral("MRI voxel: (%1, %2, %3)")
                                .arg(static_cast<int>(std::lround(vox.x())))
                                .arg(static_cast<int>(std::lround(vox.y())))
                                .arg(static_cast<int>(std::lround(vox.z())));
            } else {
                m_voxelRow = QStringLiteral("MRI voxel: —");
            }
            break;
        }
        case PickKind::CorticalVertex: {
            const QString hemi = (pick.hemisphere == 0) ? QStringLiteral("LH")
                                : (pick.hemisphere == 1) ? QStringLiteral("RH")
                                : QStringLiteral("?");
            m_labelRow = QStringLiteral("Cortex: %1 vertex %2")
                            .arg(hemi).arg(pick.objectId);
            m_voxelRow = QStringLiteral("Source: %1")
                            .arg(pick.sourceId.isEmpty() ? QStringLiteral("—")
                                                         : pick.sourceId);
            break;
        }
        case PickKind::MriVoxel: {
            m_labelRow = QStringLiteral("MRI: %1")
                            .arg(pick.sourceId.isEmpty() ? QStringLiteral("voxel")
                                                         : pick.sourceId);
            m_voxelRow = QStringLiteral("Voxel: (%1, %2, %3) ori=%4")
                            .arg(static_cast<int>(std::lround(pick.voxel.x())))
                            .arg(static_cast<int>(std::lround(pick.voxel.y())))
                            .arg(static_cast<int>(std::lround(pick.voxel.z())))
                            .arg(pick.sliceOrientation);
            break;
        }
        case PickKind::Sensor:
        case PickKind::Dipole:
        case PickKind::Bem:
        case PickKind::Custom:
            m_labelRow = QStringLiteral("%1: %2")
                            .arg(pick.sourceId.isEmpty() ? QStringLiteral("Pick")
                                                         : pick.sourceId)
                            .arg(pick.label.isEmpty() ? QStringLiteral("(unnamed)")
                                                      : pick.label);
            m_voxelRow = QStringLiteral("ObjectId: %1").arg(pick.objectId);
            break;
        case PickKind::None:
            clearRows();
            break;
    }
}
