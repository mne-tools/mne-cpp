//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *

 * @file     inspect_demo_fixtures.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Implementation of the in-memory inspect demo fixtures.
 */

#include "inspect_demo_fixtures.h"

#include <mne/fiff/fiff_file.h>
#include <mne/mri/mri_types.h>

#include <QString>

#include <cmath>

using mne::IeegContact;
using mne::IeegImplantation;
using mne::IeegLeadType;
using mne::MriSlice;
using mne::MriVolData;

namespace MNEINSPECT
{

namespace
{

IeegContact makeContact(const QString& name, const QString& lead, const Eigen::Vector3f& position)
{
    IeegContact c;
    c.name = name.toStdString();
    c.lead = lead.toStdString();
    c.position = position;
    return c;
}

void addLine(std::vector<IeegContact>& contacts, const QString& lead, const Eigen::Vector3f& origin,
             const Eigen::Vector3f& step, int nContacts, int firstNumber)
{
    for (int i = 0; i < nContacts; ++i)
        contacts.push_back(makeContact(QStringLiteral("%1%2").arg(lead).arg(i + firstNumber), lead, origin + float(i) * step));
}

} // namespace

IeegImplantation demoOneDepthStrip()
{
    std::vector<IeegContact> contacts;
    addLine(contacts, QStringLiteral("LA"), Eigen::Vector3f::Zero(), Eigen::Vector3f(0.0f, 0.0f, 0.01f), 4, 0);
    IeegImplantation implantation(contacts);
    implantation.setLeadType("LA", IeegLeadType::Depth);
    return implantation;
}

IeegImplantation demoFourArrayMontage()
{
    std::vector<IeegContact> contacts;
    addLine(contacts, QStringLiteral("LA"), Eigen::Vector3f(-0.035f, 0.010f, 0.0f), Eigen::Vector3f(0.0f, 0.0f, 0.005f), 6, 0);
    addLine(contacts, QStringLiteral("RA"), Eigen::Vector3f(0.035f, 0.010f, 0.0f), Eigen::Vector3f(0.0f, 0.0f, 0.005f), 6, 0);
    addLine(contacts, QStringLiteral("LG"), Eigen::Vector3f(-0.020f, -0.020f, 0.030f), Eigen::Vector3f(0.008f, 0.0f, 0.0f), 6, 1);
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c)
            contacts.push_back(makeContact(QStringLiteral("RG%1").arg(r * 4 + c + 1), QStringLiteral("RG"),
                                           Eigen::Vector3f(0.010f + 0.006f * float(c), -0.020f + 0.006f * float(r), 0.030f)));
    }
    IeegImplantation implantation(contacts);
    implantation.setLeadType("LG", IeegLeadType::Strip);
    implantation.setLeadType("RG", IeegLeadType::Grid, 4, 4);
    return implantation;
}

std::unique_ptr<MriVolData> demoMriSlab(int dim)
{
    if (dim <= 0) {
        dim = 32;
    }

    auto vol = std::make_unique<MriVolData>();
    vol->fileName = "<demo:slab>";
    vol->version = mne::MRI_MGH_VERSION;
    vol->width = dim;
    vol->height = dim;
    vol->depth = dim;
    vol->nframes = 1;
    vol->type = mne::MRI_FLOAT;
    vol->dof = 0;
    vol->rasGood = true;

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
        slice.width = dim;
        slice.height = dim;
        slice.pixelFormat = FIFFV_MRI_PIXEL_FLOAT;
        slice.dimx = vol->xsize * 1e-3f; // metres per pixel
        slice.dimy = vol->ysize * 1e-3f;
        slice.scale = 1.0f;
        slice.pixelsFloat.resize(static_cast<qsizetype>(dim) * dim);

        for (int j = 0; j < dim; ++j) {
            for (int i = 0; i < dim; ++i) {
                const float dx = static_cast<float>(i) - cx;
                const float dy = static_cast<float>(j) - cy;
                const float dz = static_cast<float>(k) - cz;
                const float r2 = dx * dx + dy * dy + dz * dz;
                const float v = std::exp(-r2 / twoSigmaSq);
                slice.pixelsFloat[j * dim + i] = v;
            }
        }
        vol->slices.push_back(slice);
    }

    return vol;
}

} // namespace MNEINSPECT
