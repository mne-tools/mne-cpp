//=============================================================================================================
/**
* @file     rtsensordataworker.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief     RtSensorDataWorker class declaration..
*
*/

#ifndef DISP3DLIB_RTINTERPOLATIONMATWORKER_H
#define DISP3DLIB_RTINTERPOLATIONMATWORKER_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../../items/common/types.h"
#include <mne/mne_bem_surface.h>
#include <fiff/fiff_evoked.h>
#include "../../../../helpers/geometryinfo/geometryinfo.h"
#include "../../../../helpers/interpolation/interpolation.h"
#include <mne/mne_bem_surface.h>
#include <disp/helpers/colormap.h>
#include <utils/ioutils.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_types.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QMutex>
#include <QVector3D>
#include <QSharedPointer>
#include <QLinkedList>
#include <QTimer>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* This worker calculates the interpolation matrix.
*
* @brief This worker calculates the interpolation matrix.
*/
class DISP3DSHARED_EXPORT RtInterpolationMatWorker : public QObject
{
    Q_OBJECT

public:
    RtInterpolationMatWorker();

public:
    //=========================================================================================================
    /**
     * This function sets the function that is used in the interpolation process.
     * Warning: Using this function can take some seconds because recalculation are required.
     *
     * @param[in] sInterpolationFunction     Function that computes interpolation coefficients using the distance values.
     */
    void setInterpolationFunction(const QString &sInterpolationFunction);

    //=========================================================================================================
    /**
     * This function sets the cancel distance used in distance calculations for the interpolation.
     * Distances higher than this are ignored, i.e. the respective coefficients are set to zero.
     * Warning: Using this function can take some seconds because recalculation are required.
     *
     * @param[in] dCancelDist           The new cancel distance value in meters.
     */
    void setCancelDistance(double dCancelDist);

    //=========================================================================================================
    /**
    * Sets the members InterpolationData.bemSurface, InterpolationData.vecSensorPos and m_numSensors.
    * In the end calls calculateSurfaceData().
    *
    * @param[in] bemSurface                The MNEBemSurface that holds the mesh information
    * @param[in] vecSensorPos              The QVector that holds the sensor positons in x, y and z coordinates.
    * @param[in] fiffEvoked                Holds all information about the sensors.
    * @param[in] iSensorType               Type of the sensor: FIFFV_EEG_CH or FIFFV_MEG_CH.
    *
    * @return Returns the created interpolation matrix.
    */
    void setInterpolationInfo(const MNELIB::MNEBemSurface &bemSurface,
                              const QVector<Vector3f> &vecSensorPos,
                              const FIFFLIB::FiffInfo &fiffInfo,
                              int iSensorType);

    //=========================================================================================================
    /**
    * Sets bad channels and recalculate interpolation matrix.
    *
    * @param[in] info                 The fiff info including the new bad channels.
    */
    void setBadChannels(const FIFFLIB::FiffInfo& info);

protected:    
    //=========================================================================================================
    /**
    * Calculate the interpolation operator based on the set interpolation info.
    */
    void calculateInterpolationOperator();

    bool                m_bInterpolationInfoIsInit;                 /**< Flag if this thread's interpoaltion data was initialized. */

    //=============================================================================================================
    /**
     * The struct specifing all data that is used in the interpolation process
     */
    struct InterpolationData {
        int                                     iSensorType;                      /**< Type of the sensor: FIFFV_EEG_CH or FIFFV_MEG_CH. */
        double                                  dCancelDistance;                  /**< Cancel distance for the interpolaion in meters. */

        QSharedPointer<SparseMatrix<float> >    pInterpolationMatrix;             /**< Interpolation matrix that holds all coefficients for a signal interpolation. */
        QSharedPointer<MatrixXd>                pDistanceMatrix;                  /**< Distance matrix that holds distances from sensors positions to the near vertices in meters. */
        QSharedPointer<QVector<qint32>>         pVecMappedSubset;                 /**< Vector index position represents the id of the sensor and the qint in each cell is the vertex it is mapped to. */

        MNELIB::MNEBemSurface                   bemSurface;                       /**< Holds all vertex information that is needed (public member rr). */
        FIFFLIB::FiffInfo                       fiffInfo;                         /**< Contains all information about the sensors. */

        double (*interpolationFunction) (double);                                 /**< Function that computes interpolation coefficients using the distance values. */
    } m_lInterpolationData; /**< Container for the interpolation data. */

signals:
    void newInterpolationMatrixCalculated(QSharedPointer<Eigen::SparseMatrix<float>> matInterpolationOperator);
};

} // NAMESPACE

#endif //DISP3DLIB_RTINTERPOLATIONMATWORKER_H
