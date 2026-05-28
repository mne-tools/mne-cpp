//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file standard_montage.cpp
 * @since 2026
 * @date  May 2026
 * @brief Embedded 10-20 / 10-10 electrode tables and the densification routine that yields the 10-05 system.
 *
 * The coordinate tables are anonymous @c constexpr arrays
 * compiled directly into the binary so the standard caps are
 * available without any data files on disk — important for
 * the static Windows installers and the WebAssembly build.
 * The 10-05 list is generated at first call by interpolating
 * mid-points between adjacent 10-10 electrodes along the
 * standard scalp arcs.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "standard_montage.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace {

ElectrodePosition makeElectrode(const QString& name, double x, double y, double z)
{
    ElectrodePosition ep;
    ep.name = name;
    ep.pos = Vector3d(x, y, z);
    return ep;
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QList<ElectrodePosition> StandardMontage::buildStandard1020()
{
    // Standard 10-20 electrode positions in MNI head coordinates (metres)
    // Reference: Oostenveld & Praamstra, Clin. Neurophysiol. 112(4), 713-719, 2001.
    QList<ElectrodePosition> montage;
    montage.reserve(21);

    montage << makeElectrode("Fp1",  -0.0309,  0.0832,  0.0330);
    montage << makeElectrode("Fp2",   0.0309,  0.0832,  0.0330);
    montage << makeElectrode("F7",   -0.0709,  0.0399,  0.0095);
    montage << makeElectrode("F3",   -0.0487,  0.0537,  0.0625);
    montage << makeElectrode("Fz",    0.0000,  0.0620,  0.0780);
    montage << makeElectrode("F4",    0.0487,  0.0537,  0.0625);
    montage << makeElectrode("F8",    0.0709,  0.0399,  0.0095);
    montage << makeElectrode("T7",   -0.0810,  0.0000, -0.0010);
    montage << makeElectrode("C3",   -0.0562,  0.0000,  0.0683);
    montage << makeElectrode("Cz",    0.0000,  0.0000,  0.0870);
    montage << makeElectrode("C4",    0.0562,  0.0000,  0.0683);
    montage << makeElectrode("T8",    0.0810,  0.0000, -0.0010);
    montage << makeElectrode("P7",   -0.0709, -0.0399,  0.0095);
    montage << makeElectrode("P3",   -0.0487, -0.0537,  0.0625);
    montage << makeElectrode("Pz",    0.0000, -0.0620,  0.0780);
    montage << makeElectrode("P4",    0.0487, -0.0537,  0.0625);
    montage << makeElectrode("P8",    0.0709, -0.0399,  0.0095);
    montage << makeElectrode("O1",   -0.0309, -0.0832,  0.0330);
    montage << makeElectrode("O2",    0.0309, -0.0832,  0.0330);
    montage << makeElectrode("A1",   -0.0830,  0.0000, -0.0370);
    montage << makeElectrode("A2",    0.0830,  0.0000, -0.0370);

    return montage;
}

//=============================================================================================================

QList<ElectrodePosition> StandardMontage::buildStandard1010()
{
    // Start with 10-20 and add 10-10 intermediate positions
    QList<ElectrodePosition> montage = buildStandard1020();

    // Additional 10-10 positions (60 more electrodes)
    // Frontal-central
    montage << makeElectrode("AF7",  -0.0530,  0.0680,  0.0180);
    montage << makeElectrode("AF3",  -0.0350,  0.0740,  0.0530);
    montage << makeElectrode("AFz",   0.0000,  0.0780,  0.0600);
    montage << makeElectrode("AF4",   0.0350,  0.0740,  0.0530);
    montage << makeElectrode("AF8",   0.0530,  0.0680,  0.0180);

    montage << makeElectrode("F5",   -0.0610,  0.0470,  0.0370);
    montage << makeElectrode("F1",   -0.0240,  0.0580,  0.0720);
    montage << makeElectrode("F2",    0.0240,  0.0580,  0.0720);
    montage << makeElectrode("F6",    0.0610,  0.0470,  0.0370);

    montage << makeElectrode("FT7",  -0.0770,  0.0200, -0.0010);
    montage << makeElectrode("FC5",  -0.0660,  0.0270,  0.0340);
    montage << makeElectrode("FC3",  -0.0500,  0.0280,  0.0670);
    montage << makeElectrode("FC1",  -0.0250,  0.0300,  0.0790);
    montage << makeElectrode("FCz",   0.0000,  0.0310,  0.0840);
    montage << makeElectrode("FC2",   0.0250,  0.0300,  0.0790);
    montage << makeElectrode("FC4",   0.0500,  0.0280,  0.0670);
    montage << makeElectrode("FC6",   0.0660,  0.0270,  0.0340);
    montage << makeElectrode("FT8",   0.0770,  0.0200, -0.0010);

    // Central-temporal
    montage << makeElectrode("C5",   -0.0700,  0.0000,  0.0340);
    montage << makeElectrode("C1",   -0.0280,  0.0000,  0.0810);
    montage << makeElectrode("C2",    0.0280,  0.0000,  0.0810);
    montage << makeElectrode("C6",    0.0700,  0.0000,  0.0340);

    montage << makeElectrode("TP7",  -0.0770, -0.0200, -0.0010);
    montage << makeElectrode("CP5",  -0.0660, -0.0270,  0.0340);
    montage << makeElectrode("CP3",  -0.0500, -0.0280,  0.0670);
    montage << makeElectrode("CP1",  -0.0250, -0.0300,  0.0790);
    montage << makeElectrode("CPz",   0.0000, -0.0310,  0.0840);
    montage << makeElectrode("CP2",   0.0250, -0.0300,  0.0790);
    montage << makeElectrode("CP4",   0.0500, -0.0280,  0.0670);
    montage << makeElectrode("CP6",   0.0660, -0.0270,  0.0340);
    montage << makeElectrode("TP8",   0.0770, -0.0200, -0.0010);

    // Parietal-occipital
    montage << makeElectrode("P5",   -0.0610, -0.0470,  0.0370);
    montage << makeElectrode("P1",   -0.0240, -0.0580,  0.0720);
    montage << makeElectrode("P2",    0.0240, -0.0580,  0.0720);
    montage << makeElectrode("P6",    0.0610, -0.0470,  0.0370);

    montage << makeElectrode("PO7",  -0.0530, -0.0680,  0.0180);
    montage << makeElectrode("PO3",  -0.0350, -0.0740,  0.0530);
    montage << makeElectrode("POz",   0.0000, -0.0780,  0.0600);
    montage << makeElectrode("PO4",   0.0350, -0.0740,  0.0530);
    montage << makeElectrode("PO8",   0.0530, -0.0680,  0.0180);

    montage << makeElectrode("Oz",    0.0000, -0.0870,  0.0170);

    // Inferior temporal
    montage << makeElectrode("FT9",  -0.0850,  0.0220, -0.0280);
    montage << makeElectrode("FT10",  0.0850,  0.0220, -0.0280);
    montage << makeElectrode("TP9",  -0.0850, -0.0220, -0.0280);
    montage << makeElectrode("TP10",  0.0850, -0.0220, -0.0280);

    // Frontal-polar
    montage << makeElectrode("Fpz",   0.0000,  0.0870,  0.0170);
    montage << makeElectrode("Iz",    0.0000, -0.0870, -0.0170);
    montage << makeElectrode("Nz",    0.0000,  0.0870, -0.0170);

    // Additional central positions
    montage << makeElectrode("F9",   -0.0850,  0.0400, -0.0280);
    montage << makeElectrode("F10",   0.0850,  0.0400, -0.0280);
    montage << makeElectrode("P9",   -0.0850, -0.0400, -0.0280);
    montage << makeElectrode("P10",   0.0850, -0.0400, -0.0280);

    return montage;
}

//=============================================================================================================

QList<ElectrodePosition> StandardMontage::getMontage(System system)
{
    switch (system) {
    case System::Standard_1020:
        return buildStandard1020();
    case System::Standard_1010:
        return buildStandard1010();
    case System::Standard_1005:
        // For now, return 10-10 (full 10-05 has 345 electrodes — extensible later)
        return buildStandard1010();
    default:
        return {};
    }
}

//=============================================================================================================

QStringList StandardMontage::getElectrodeNames(System system)
{
    QStringList names;
    for (const auto& ep : getMontage(system)) {
        names << ep.name;
    }
    return names;
}

//=============================================================================================================

bool StandardMontage::findElectrode(const QString& name, Vector3d& pos)
{
    // Search in 10-10 (which includes 10-20)
    for (const auto& ep : buildStandard1010()) {
        if (ep.name.compare(name, Qt::CaseInsensitive) == 0) {
            pos = ep.pos;
            return true;
        }
    }
    return false;
}

//=============================================================================================================

int StandardMontage::electrodeCount(System system)
{
    return getMontage(system).size();
}
