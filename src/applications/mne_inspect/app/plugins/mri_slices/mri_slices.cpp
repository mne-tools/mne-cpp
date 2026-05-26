//=============================================================================================================
/**
 * @file     mri_slices.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    MriSlicesPlugin implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_slices.h"

#include <disp3D/scene/multimodalscene.h>

#include <mri/mri_slicer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QFileInfo>
#include <QImage>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MRISLICESPLUGIN;
using namespace DISP3DLIB;
using namespace MRILIB;

namespace {

QImage sliceImageToQImage(const MriSliceImage& slice)
{
    QImage img(slice.width, slice.height, QImage::Format_Grayscale8);
    for (int y = 0; y < slice.height; ++y) {
        uchar* row = img.scanLine(y);
        for (int x = 0; x < slice.width; ++x) {
            // pixels is row-major (y, x) and normalised to [0, 1].
            float v = slice.pixels(y, x);
            if (v < 0.0f) v = 0.0f;
            if (v > 1.0f) v = 1.0f;
            row[x] = static_cast<uchar>(v * 255.0f + 0.5f);
        }
    }
    return img;
}

DISP3DLIB::SliceOrientation toDispOrientation(MRILIB::SliceOrientation o)
{
    switch (o) {
        case MRILIB::SliceOrientation::Axial:    return DISP3DLIB::SliceOrientation::Axial;
        case MRILIB::SliceOrientation::Coronal:  return DISP3DLIB::SliceOrientation::Coronal;
        case MRILIB::SliceOrientation::Sagittal: return DISP3DLIB::SliceOrientation::Sagittal;
    }
    return DISP3DLIB::SliceOrientation::Axial;
}

} // anonymous namespace

//=============================================================================================================

MriSlicesPlugin::MriSlicesPlugin(QObject* parent)
    : QObject(parent)
{
}

//=============================================================================================================

MriSlicesPlugin::~MriSlicesPlugin() = default;

//=============================================================================================================

std::array<QString, 3> MriSlicesPlugin::sceneLayerIds()
{
    return {
        QStringLiteral("mri_axial"),
        QStringLiteral("mri_coronal"),
        QStringLiteral("mri_sagittal"),
    };
}

//=============================================================================================================

bool MriSlicesPlugin::loadVolume(const QString& path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix != QLatin1String("mgh") && suffix != QLatin1String("mgz")) {
        qWarning() << "MriSlicesPlugin::loadVolume: unsupported format" << suffix
                   << "— only MGH/MGZ are wired in v2.3.0";
        return false;
    }

    auto vol = std::make_unique<MriVolData>();
    if (!vol->read(path) || !vol->isValid()) {
        qWarning() << "MriSlicesPlugin::loadVolume: failed to read" << path;
        return false;
    }

    m_volume = std::move(vol);
    m_sourcePath = path;
    m_crosshair = volumeCenter();
    rebuildSlices();
    emit volumeChanged();
    emit crosshairChanged(QVector3D(m_crosshair.x(), m_crosshair.y(), m_crosshair.z()));
    return true;
}

//=============================================================================================================

void MriSlicesPlugin::setCrosshair(const Eigen::Vector3f& rasPoint)
{
    if ((rasPoint - m_crosshair).norm() < 1e-6f) {
        return;
    }
    m_crosshair = rasPoint;
    rebuildSlices();
    emit crosshairChanged(QVector3D(m_crosshair.x(), m_crosshair.y(), m_crosshair.z()));
}

//=============================================================================================================

void MriSlicesPlugin::setCrosshair(const QVector3D& rasPoint)
{
    setCrosshair(Eigen::Vector3f(rasPoint.x(), rasPoint.y(), rasPoint.z()));
}

//=============================================================================================================

void MriSlicesPlugin::attachScene(MultimodalScene* scene)
{
    if (m_scene == scene) {
        return;
    }

    if (m_scene) {
        QObject::disconnect(m_scene, &MultimodalScene::picked,
                            this, &MriSlicesPlugin::handlePick);
        for (const QString& id : sceneLayerIds()) {
            m_scene->removeLayer(id);
        }
    }

    m_scene = scene;

    if (m_scene) {
        QObject::connect(m_scene, &MultimodalScene::picked,
                         this, &MriSlicesPlugin::handlePick);
        publishToScene();
    }
}

//=============================================================================================================

void MriSlicesPlugin::handlePick(const PickResult& pick)
{
    if (pick.kind != PickKind::MriVoxel) {
        return;
    }
    setCrosshair(pick.world);
}

//=============================================================================================================

Eigen::Vector3f MriSlicesPlugin::volumeCenter() const
{
    if (!m_volume) {
        return Eigen::Vector3f::Zero();
    }
    const Eigen::Matrix4f vox2ras = m_volume->computeVox2Ras();
    const Eigen::Vector4f vc(m_volume->dimX() * 0.5f,
                              m_volume->dimY() * 0.5f,
                              m_volume->dimZ() * 0.5f,
                              1.0f);
    const Eigen::Vector4f ras = vox2ras * vc;
    return Eigen::Vector3f(ras.x(), ras.y(), ras.z());
}

//=============================================================================================================

void MriSlicesPlugin::rebuildSlices()
{
    for (auto& s : m_slices) {
        s.reset();
    }

    if (!m_volume) {
        publishToScene();
        return;
    }

    const QVector<MriSliceImage> ortho =
        MriSlicer::extractOrthogonal(*m_volume, m_crosshair);

    // extractOrthogonal documents the result order as {axial, coronal, sagittal}.
    if (ortho.size() != 3) {
        qWarning() << "MriSlicesPlugin: unexpected ortho slice count" << ortho.size();
        publishToScene();
        return;
    }

    const Eigen::Matrix4f vox2rasF = m_volume->computeVox2Ras();
    const Eigen::Matrix4d vox2rasD = vox2rasF.cast<double>();

    for (int i = 0; i < 3; ++i) {
        const MriSliceImage& src = ortho[i];
        auto so = std::make_unique<SliceObject>();
        so->setSlice(sliceImageToQImage(src),
                     toDispOrientation(src.orientation),
                     src.sliceIndex,
                     vox2rasD);
        m_slices[i] = std::move(so);
    }

    publishToScene();
}

//=============================================================================================================

void MriSlicesPlugin::publishToScene()
{
    if (!m_scene) {
        return;
    }

    const std::array<QString, 3> ids = sceneLayerIds();
    const std::array<const char*, 3> names {
        "MRI Axial", "MRI Coronal", "MRI Sagittal"
    };

    for (int i = 0; i < 3; ++i) {
        if (!m_slices[i]) {
            m_scene->removeLayer(ids[i]);
            continue;
        }
        SceneLayer layer;
        layer.id = ids[i];
        layer.displayName = QString::fromLatin1(names[i]);
        layer.kind = SceneLayerKind::MriSlice;
        layer.drawOrder = i;
        // Non-owning shared_ptr: the plugin retains ownership of the object.
        SliceObject* raw = m_slices[i].get();
        layer.payload = std::shared_ptr<void>(raw, [](void*){});
        m_scene->addLayer(std::move(layer));
    }
}
