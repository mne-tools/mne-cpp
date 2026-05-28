//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     mne.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Andreas Griesshammer <ag@fieldlineinc.com>
 * @since    0.1.0
 * @date     August 2012
 * @brief    Implementation of the @ref MNELIB::MNE static facade.
 *
 * Defines the matlab-style wrapper bodies declared in @c mne.h, dispatching
 * to @ref MNESourceSpaces, @ref MNEBem, @ref MNEHemisphere,
 * @ref MNEForwardSolution and @ref MNEEpochDataList and reusing the
 * FIFF readers/writers from FIFFLIB. Logic is intentionally kept inside
 * the static methods so the class itself remains empty and trivially
 * copyable.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne.h"
#include <fiff/fiff.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

// read_events, read_events_from_fif, read_events_from_ascii, write_events_to_fif,
// write_events_to_ascii implementations moved to FiffEvents class in fiff_events.cpp

//=============================================================================================================

void MNE::setup_compensators(FiffRawData& raw,
                             fiff_int_t dest_comp,
                             bool keep_comp)
{
    // Set up projection
    if (raw.info.projs.size() == 0) {
        qInfo("No projector specified for these data\n");
    } else {
        // Activate the projection items
        for (qint32 k = 0; k < raw.info.projs.size(); ++k) {
            raw.info.projs[k].active = true;
        }

        qInfo("%lld projection items activated\n",raw.info.projs.size());
        // Create the projector
//        fiff_int_t nproj = MNE::make_projector_info(raw.info, raw.proj); Using the member function instead
        fiff_int_t nproj = raw.info.make_projector(raw.proj);

        if (nproj == 0)  {
            qInfo("The projection vectors do not apply to these channels\n");
        } else {
            qInfo("Created an SSP operator (subspace dimension = %d)\n",nproj);
        }
    }

    // Set up the CTF compensator
//    qint32 current_comp = MNE::get_current_comp(raw.info);
    qint32 current_comp = raw.info.get_current_comp();
    if (current_comp > 0)
        qInfo("Current compensation grade : %d\n",current_comp);

    if (keep_comp)
        dest_comp = current_comp;

    if (current_comp != dest_comp)
    {
        qDebug() << "This part needs to be debugged";
        if(MNE::make_compensator(raw.info, current_comp, dest_comp, raw.comp))
        {
//            raw.info.chs = MNE::set_current_comp(raw.info.chs,dest_comp);
            raw.info.set_current_comp(dest_comp);
            qInfo("Appropriate compensator added to change to grade %d.\n",dest_comp);
        }
        else
        {
            qCritical("Could not make the compensator\n");
            return;
        }
    }
}

//=============================================================================================================

// compute_proj implementation moved to FiffProj::compute_from_raw in fiff_proj.cpp
