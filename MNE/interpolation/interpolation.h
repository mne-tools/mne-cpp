//=============================================================================================================
/**
* @file     interpolation.h
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Mai, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief     Interpolation class declaration.
*
*/

#ifndef INTERPOLATION_INTERPOLATION_H
#define INTERPOLATION_INTERPOLATION_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "interpolation_global.h"
#include <limits>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QVector>

//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Sparse>

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SWP
//=============================================================================================================

namespace INTERPOLATION {

#define DOUBLE_INFINITY std::numeric_limits<double>::infinity()

//*************************************************************************************************************
//=============================================================================================================
// SWP FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* Calculates and stores a matrix which provides the information how much each vertex is influenced by every sensor.
* On this basis, the user can use sensor data to calculate interpolated values for each sensor of the mesh.
* The user can choose the function that is used in the interpolation. Three functions are provided.
* Interpolation weights are calculated using the following formula:
* ![weight matrix](./interpolation_formula.png)
* where w are interpolation weights, d are distances, m is the number of columsn and f is the used function.
*
* @brief This class holds methods for creating distance-based weight matrices and for interpolating signals
*/

class INTERPOLATIONSHARED_EXPORT Interpolation
{

public:
    typedef QSharedPointer<Interpolation> SPtr;            /**< Shared pointer type for Interpolation. */
    typedef QSharedPointer<const Interpolation> ConstSPtr; /**< Const shared pointer type for Interpolation. */

    //=========================================================================================================
    /**
    * deleted default constructor (static class).
    */
    Interpolation() = delete;

    //=========================================================================================================
    /**
     * This method calculates the weight matrix that is later needed for the interpolation of signals.
     * The matrix will have n rows, where n is the number of rows of the passed distance table (i.e. the number of vertices of the mesh that the table is based on),
     * and m columns, where m is the number of mapped sensors on the mesh (based on a prior sensor-to-mesh mapping).
     * <i>createInterplationMat</i> calculates the matrix according to the following scheme:
     *    -# if the vertex belongs to a sensor: The value at the position of the sensor is 1 and all other values in this row are set to 0
     *    -# if not: the values are calculated to give a total of 1 (a lot of values will stay 0, because they are too far away to influence) by using the above mentioned formula
     *
     * @brief <i>createInterpolationMat</i> Calculate weight matrix for later interpolation
     * @param pProjectedSensors Vector of IDs of sensor vertices
     * @param pDistanceTable Matrix that contains all needed distances
     * @param interpolationFunction Function that computes interpolation coefficients using the distance values
     * @param dcancelDist Distances higher than this are ignored, i.e. the respective coefficients are set to zero
     * @return A shared pointer to the distance matrix created
     */
    static QSharedPointer<Eigen::SparseMatrix<double> > createInterpolationMat(const QSharedPointer<QVector<qint32>> pProjectedSensors, const QSharedPointer<Eigen::MatrixXd> pDistanceTable, double (*interpolationFunction) (double), const double dCancelDist = DOUBLE_INFINITY);

    //=========================================================================================================
        /**
         * The interpolation essentially corresponds to a matrix * vector multiplication. A vector of sensor data (i.e. a vector of double-values)
         * is multiplied with the result of the <i>createInterpolationMat</i>.
         * The result is a vector that contains interpolated values for all vertices of the mesh that was used to create the weight matrix,
         * i.e. in first instance the distance table that the weight matrix is based on.
         *
         * @brief <i>interpolateSignals</i> Interpolate sensor data
         * @param pInterpolationMatrix The weight matrix which should be used for multiplying
         * @param vecMeasurementData A vector with measured sensor data
         * @return Interpolated values for all vertices of the mesh
         */
    static QSharedPointer<Eigen::VectorXf> interpolateSignal(const QSharedPointer<Eigen::SparseMatrix<double> > pInterpolationMatrix, const Eigen::VectorXd &vecMeasurementData);

    //=========================================================================================================
    /**
     * Serves as a placeholder for other functions and is needed in case a linear interpolation is wanted when calling <i>createInterplationMat</i>.
     *
     * @brief linear Returns input argument unchanged
     * @param dIn Distance value
     * @return Same value interpreted as a interpolation weight
     */
    static double linear(const double dIn);

    //=========================================================================================================
    /**
     * Calculates interpolation weights based on distance values
     *
     * @brief <i>gaussian</i> Returns interpolation weight that corresponds to gauss curve with sigma set to 1
     * @param dIn Distance value
     * @return The function value of the gauss curve at d
     */
    static double gaussian(const double dIn);
    //=========================================================================================================
    /**
     * Calculates interpolation weights based on distance values
     *
     * @brief <i>square</i> Returns interpolation weight that corresponds to negative parabel with an y-offset of 1.
     * @param dIn Distance value
     * @return The function value of the negative parabel at d
     */
    static double square(const double dIn);
    //=========================================================================================================

    /**
     * Calculates interpolation weights based on distance values
     *
     * @brief <i>square</i> Returns interpolation weight that corresponds to qubic hyperbel
     * @param dIn Distance value
     * @return The function value of the qubic hyperbel at d
     */
    static double qubic(const double dIn);

protected:

private:

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace INTERPOLATION

#endif // INTERPOLATION_INTERPOLATION_H
