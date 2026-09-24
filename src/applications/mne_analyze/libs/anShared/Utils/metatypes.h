//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     metatypes.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     November, 2015
 * @brief     Register your QMetatypes here.
 */

#ifndef METATYPES_H
#define METATYPES_H

#include <inv/dipole_fit/inv_ecd.h>
#include <Eigen/Core>
#include "../Management/event.h"
#include "../Model/fiffrawviewmodel.h"

// IMPORTANT: You must also use qRegisterMetaTypes in AnalyzeCore::registerMetatypes to use custom types in QObject::connect() calls.
#ifndef metatype_vector3f
#define metatype_vector3f
Q_DECLARE_METATYPE(Eigen::Vector3f);
#endif

#ifndef metatype_vector3i
#define metatype_vector3i
Q_DECLARE_METATYPE(Eigen::Vector3i);
#endif

#ifndef metatype_inverselibecd
#define metatype_inverselibecd
Q_DECLARE_METATYPE(INVLIB::InvEcd);
#endif

#ifndef metatype_ANSHAREDLIB_events
#define metatype_ANSHAREDLIB_events
Q_DECLARE_METATYPE(QSharedPointer<ANSHAREDLIB::Event>);
#endif

#ifndef metatype_ANSHAREDLIB_chandata
#define metatype_ANSHAREDLIB_chandata
Q_DECLARE_METATYPE(ANSHAREDLIB::ChannelData);
#endif

#endif // ENUMS_H
