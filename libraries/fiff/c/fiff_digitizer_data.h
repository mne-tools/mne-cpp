//=============================================================================================================
/**
 * @file     fiff_digitizer_data.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    FiffDigitizerData class declaration.
 *
 */

#ifndef FIFF_DIGITIZER_DATA_H
#define FIFF_DIGITIZER_DATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../fiff_global.h"
#include "../fiff_types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QDebug>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffCoordTransOld;
class FiffDigPoint;

//=============================================================================================================
/**
 * Replaces *digitizerData, digitizerDataRec struct (analyze_types.c).
 *
 * @brief Digitization points container and description.
 */
class FIFFSHARED_EXPORT FiffDigitizerData
{
public:
    typedef QSharedPointer<FiffDigitizerData> SPtr;              /**< Shared pointer type for FiffDigitizerData. */
    typedef QSharedPointer<const FiffDigitizerData> ConstSPtr;   /**< Const shared pointer type for FiffDigitizerData. */

    //=========================================================================================================
    /**
     * Constructs the FiffDigitizerData.
     */
    FiffDigitizerData();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffDigitizerData   Digitization point descriptor which should be copied.
     */
    FiffDigitizerData(const FiffDigitizerData& p_FiffDigitizerData);

    //=========================================================================================================
    /**
     * Default constructor
     *
     * @param[in] p_IODevice   Input device to read data from.
     */
    FiffDigitizerData(QIODevice &p_IODevice);

    //=========================================================================================================
    /**
     * Destroys the digitization point description
     */
    ~FiffDigitizerData();

    void print();

public:
    QString        filename;                 /* Where did these come from */
    FIFFLIB::FiffCoordTransOld* head_mri_t;            /* This is relevant for us */
    FIFFLIB::FiffCoordTransOld* head_mri_t_adj;        /* This is the adjusted transformation */
    QList<FIFFLIB::FiffDigPoint>   points;           /* The points */
    int            coord_frame;               /* The coordinate frame of the above points */
    QList<int>     active;                   /* Which are active */
    QList<int>     discard;                  /* Which should be discarded? */
    int            npoint;                    /* How many? */
    FIFFLIB::FiffDigPoint*   mri_fids;         /* MRI coordinate system fiducials picked here */
    int            nfids;                     /* How many? */
    int            show;                      /* Should the digitizer data be shown */
    int            show_minimal;              /* Show fiducials and coils only? */
    float          *dist;                     /* Distance of each point from the head surface */
    int            *closest;                  /* Closest vertex # on the head surface */
    float          **closest_point;           /* Closest vertex locations on the head surface */
    int            dist_valid;                /* Are the above data valid at this point? */

//    typedef struct {                            /* The digitizer data will be loaded from the measurement file or elsewhere */
//      char           *filename;                 /* Where did these come from */
//      FIFFLIB::FiffCoordTransOld* head_mri_t;            /* This is relevant for us */
//      FIFFLIB::FiffCoordTransOld* head_mri_t_adj;        /* This is the adjusted transformation */
//      FIFFLIB::fiffDigPoint   points;           /* The points */
//      int            coord_frame;               /* The coordinate frame of the above points */
//      int            *active;                   /* Which are active */
//      int            *discard;                  /* Which should be discarded? */
//      int            npoint;                    /* How many? */
//      FIFFLIB::fiffDigPoint   mri_fids;         /* MRI coordinate system fiducials picked here */
//      int            nfids;                     /* How many? */
//      int            show;                      /* Should the digitizer data be shown */
//      int            show_minimal;              /* Show fiducials and coils only? */
//      float          *dist;                     /* Distance of each point from the head surface */
//      int            *closest;                  /* Closest vertex # on the head surface */
//      float          **closest_point;           /* Closest vertex locations on the head surface */
//      int            dist_valid;                /* Are the above data valid at this point? */
//    } *digitizerData,digitizerDataRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE

#endif // FIFF_DIGITIZER_DATA_H
