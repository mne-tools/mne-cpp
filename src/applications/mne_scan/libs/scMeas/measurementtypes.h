//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     measurementtypes.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Contains the declaration of the MeasurementTypes class.
 */

#ifndef MEASUREMENTTYPES_H
#define MEASUREMENTTYPES_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scmeas_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>

//=============================================================================================================
// DEFINE NAMESPACE SCMEASLIB
//=============================================================================================================

namespace SCMEASLIB
{

//=============================================================================================================
/**
 * Class MeasurementTypes to register measurement classes to QMetaType types
 *
 * @brief Class MeasurementTypes to register measurement classes to QMetaType types
 */
class SCMEASSHARED_EXPORT MeasurementTypes : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a MeasurementTypes Object.
     */
    explicit MeasurementTypes(QObject *parent = 0);

    //=========================================================================================================
    /**
     * Call to register MeasurementTypes
     */
    static void registerTypes();
    
signals:
    
public slots:
    
};
} //NAMESPACE

#endif // MEASUREMENTTYPES_H
