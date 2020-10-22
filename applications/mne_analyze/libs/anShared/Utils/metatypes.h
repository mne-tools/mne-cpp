//=============================================================================================================
/**
 * @file     metatypes.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief     Register your QMetatypes here.
 *
 */

#ifndef METATYPES_H
#define METATYPES_H

#include <inverse/dipoleFit/ecd.h>
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
Q_DECLARE_METATYPE(INVERSELIB::ECD);
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
