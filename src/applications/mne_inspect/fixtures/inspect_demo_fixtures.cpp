//=============================================================================================================
/**
 * @file     inspect_demo_fixtures.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Implementation of the in-memory inspect demo fixtures.
 */

#include "inspect_demo_fixtures.h"

#include <fiff/fiff_file.h>
#include <mri/mri_types.h>

#include <QColor>
#include <QString>
#include <QVector3D>

#include <cmath>

using DISP3DLIB::ElectrodeArray;
using DISP3DLIB::ElectrodeContact;
using DISP3DLIB::ElectrodeLayout;
using MRILIB::MriSlice;
using MRILIB::MriVolData;

namespace MNEINSPECT
{

namespace {

ElectrodeArray makeDepthStrip(const QString& label,
                              const QVector3D& origin,
                              int nContacts,
                              float spacing = 0.005f)
{
    ElectrodeArray arr;
    arr.label  = label;
    arr.layout = ElectrodeLayout::Depth;
    for (int i = 0; i < nContacts; ++i) {
        ElectrodeContact c;
        c.name     = QStringLiteral("%1%2").arg(label).arg(i);
        c.position = origin + QVector3D(0.0f, 0.0f, static_cast<float>(i) * spacing);
        arr.contacts.append(c);
    }
    return arr;
}

ElectrodeArray makeEcogStrip(const QString& label,
                             const QVector3D& origin,
                             int nContacts,
                             float spacing = 0.008f)
{
    ElectrodeArray arr;
    arr.label    = label;
    arr.layout   = ElectrodeLayout::Strip;
    arr.gridRows = 1;
    arr.gridCols = nContacts;
    for (int i = 0; i < nContacts; ++i) {
        ElectrodeContact c;
        c.name     = QStringLiteral("%1%2").arg(label).arg(i + 1);
        c.position = origin + QVector3D(static_cast<float>(i) * spacing, 0.0f, 0.0f);
        arr.contacts.append(c);
    }
    return arr;
}

ElectrodeArray makeEcogGrid(const QString& label,
                            const QVector3D& origin,
                            int rows,
                            int cols,
                            float spacing = 0.006f)
{
    ElectrodeArray arr;
    arr.label    = label;
    arr.layout   = ElectrodeLayout::Grid;
    arr.gridRows = rows;
    arr.gridCols = cols;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            ElectrodeContact contact;
            contact.name = QStringLiteral("%1_R%2C%3").arg(label).arg(r + 1).arg(c + 1);
            contact.position = origin + QVector3D(static_cast<float>(c) * spacing,
                                                  static_cast<float>(r) * spacing,
                                                  0.0f);
            arr.contacts.append(contact);
        }
    }
    return arr;
}

}  // namespace

QVector<ElectrodeArray> demoOneDepthStrip()
{
    ElectrodeArray arr;
    arr.label  = QStringLiteral("LA");
    arr.layout = ElectrodeLayout::Depth;
    for (int i = 0; i < 4; ++i) {
        ElectrodeContact c;
        c.name     = QStringLiteral("LA%1").arg(i);
        c.position = QVector3D(0.0f, 0.0f, static_cast<float>(i) * 0.01f);
        arr.contacts.append(c);
    }
    return {arr};
}

QVector<ElectrodeArray> demoFourArrayMontage()
{
    QVector<ElectrodeArray> out;
    out.append(makeDepthStrip(QStringLiteral("LA"),
                              QVector3D(-0.035f, 0.010f, 0.0f), 6));
    out.append(makeDepthStrip(QStringLiteral("RA"),
                              QVector3D( 0.035f, 0.010f, 0.0f), 6));
    out.append(makeEcogStrip(QStringLiteral("LG"),
                             QVector3D(-0.020f, -0.020f, 0.030f), 6));
    out.append(makeEcogGrid(QStringLiteral("RG"),
                            QVector3D( 0.010f, -0.020f, 0.030f), 4, 4));
    return out;
}

std::unique_ptr<MriVolData> demoMriSlab(int dim)
{
    if (dim <= 0) {
        dim = 32;
    }

    auto vol = std::make_unique<MriVolData>();
    vol->fileName = QStringLiteral("<demo:slab>");
    vol->version  = MRILIB::MRI_MGH_VERSION;
    vol->width    = dim;
    vol->height   = dim;
    vol->depth    = dim;
    vol->nframes  = 1;
    vol->type     = MRILIB::MRI_FLOAT;
    vol->dof      = 0;
    vol->rasGood  = true;

    // 2 mm isotropic so the cube covers roughly +/- 32 mm — small enough
    // to stay near the synthetic electrode positions but large enough that
    // the ortho slices look like a real subject in the screenshots.
    vol->xsize = 2.0f;
    vol->ysize = 2.0f;
    vol->zsize = 2.0f;

    // Standard RAS direction cosines (identity orientation).
    vol->x_ras = Eigen::Vector3f(1.0f, 0.0f, 0.0f);
    vol->y_ras = Eigen::Vector3f(0.0f, 1.0f, 0.0f);
    vol->z_ras = Eigen::Vector3f(0.0f, 0.0f, 1.0f);
    vol->c_ras = Eigen::Vector3f(0.0f, 0.0f, 0.0f);

    // voxelSurfRasT mirrors computeVox2Ras(); MriVolData itself never reads
    // this member back (computeVox2Ras rebuilds it from xsize/ysize/zsize),
    // so leaving the default identity FiffCoordTrans is fine.

    // Fill `depth` slices with a centred Gaussian blob whose intensity
    // tapers off in z, so the three ortho cuts each show a recognisable
    // bright spot at the volume centre.
    vol->slices.reserve(dim);
    const float cx = (dim - 1) * 0.5f;
    const float cy = (dim - 1) * 0.5f;
    const float cz = (dim - 1) * 0.5f;
    const float sigma = static_cast<float>(dim) * 0.18f;
    const float twoSigmaSq = 2.0f * sigma * sigma;

    for (int k = 0; k < dim; ++k) {
        MriSlice slice;
        slice.width        = dim;
        slice.height       = dim;
        slice.pixelFormat  = FIFFV_MRI_PIXEL_FLOAT;
        slice.dimx         = vol->xsize * 1e-3f;   // metres per pixel
        slice.dimy         = vol->ysize * 1e-3f;
        slice.scale        = 1.0f;
        slice.pixelsFloat.resize(dim * dim);

        for (int j = 0; j < dim; ++j) {
            for (int i = 0; i < dim; ++i) {
                const float dx = static_cast<float>(i) - cx;
                const float dy = static_cast<float>(j) - cy;
                const float dz = static_cast<float>(k) - cz;
                const float r2 = dx * dx + dy * dy + dz * dz;
                const float v  = std::exp(-r2 / twoSigmaSq);
                slice.pixelsFloat[j * dim + i] = v;
            }
        }
        vol->slices.append(slice);
    }

    return vol;
}

}  // namespace MNEINSPECT
