//=============================================================================================================
/**
* @file     adaptivemp.h
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>
*
* @version  1.0
* @date     July, 2014
*
* ported to mne-cpp by Martin Henfling and Daniel Knobl in May 2014
* original code was implemented in Matlab Code by Maciej Gratkowski
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    ADAPIVEMP class declaration, providing the implemetation of the Matching Pursuit Algorithm
*           introduced by Stephane Mallat and Zhifeng Zhang. Matlabimplemetation of Maciej Gratkowski is
*           used as Source and reference.
*
*/

#ifndef ADAPTIVEMP_H
#define ADAPTIVEMP_H

//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>

//*************************************************************************************************************
//=============================================================================================================
// Utils INCLUDES
//=============================================================================================================

#include <utils/mp/atom.h>
#include <utils/utils_global.h>

//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <unsupported/Eigen/FFT>
#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace std;


//=============================================================================================================

enum ReturnValue{RETURNATOM, RETURNPARAMETERS}; /**< deciding whether to return a real atom or its parameters*/

//*************************************************************************************************************
/**
* DECLARE CLASS adaptiveMP
*
* @brief The adaptiveMP class provides functions several calculating functions to run the Matching Pursuit Algorithm
*/
class UTILSSHARED_EXPORT AdaptiveMp : public QThread
{
    Q_OBJECT

public:

    typedef QList<GaborAtom> gabor_atom_list;

    /**
    * adaptiveMP_adaptiveMP
    *
    * ### MP toolbox function ###
    *
    * Constructor
    *
    * constructs adaptiveMP class
    *
    */
    AdaptiveMp();

    //=========================================================================================================

    qint32 it = 0;
    qint32 max_it = 0;
    qreal signal_energy = 0;//VectorXd signal_energy;// = VectorXd::Zero(channel_count);
    //VectorXd residuum_energy;// = VectorXd::Zero(channel_count);
    //VectorXd energy_threshold;// = VectorXd::Zero(channel_count);
    qreal current_energy = 0;
    QList<GaborAtom> atom_list;
    MatrixXd signal;
    qint32 max_iterations;
    qreal epsilon;

    //=========================================================================================================
    /**
    * adaptiveMP_matching_pursuit
    *
    * ### MP Algorithm ###
    *
    * running the MP Algorithm introduced by Mallat and Zhang
    *
    * @param[in] signal    Matrix containing single or mulitchannel signals
    * @param[in] max_it    maximum number of iterations of MP Algorithm
    * @param[in] epsilon   threshold for number of iterations of MP Algorithm
    *
    * @return result of MP Algorithm as QList of GaborAtoms
    */
    //QList<GaborAtom> matching_pursuit (MatrixXd signal, qint32 max_iterations, qreal epsilon);

    //=========================================================================================================
    /**
    * adaptiveMP_modulation_function
    *
    * ### MP toolbox root function ###
    *
    * calculates a complex function for modulating signals in MP Algorithm
    *
    * @param[in] N    number of samples
    * @param[in] k    factor of modulationfrequency
    *
    * @return complex modulationvector
    */
    VectorXcd modulation_function(qint32 N, qreal k);

    //=========================================================================================================
    /**
    * adaptiveMP_calculate_atom
    *
    * ### MP toolbox root function ###
    *
    * calculates real gabor atoms for MP Algorithm
    *
    * @param[in] sampleCount    number of samples in the atom
    * @param[in] scale          scale of atom
    * @param[in] translation    translation of atom
    * @param[in] modulation     modulation of atom
    * @param[in] channel        number of signalchannels
    * @param[in] residuum       the signalresiduun after each MP Algorithm iterationstep
    * @param[in] returnValue    declare what kind of information should be returned
    *
    * @return depending on returnValue returning the real atom calculated or the manipulated parameters: scale, translation, modulation, phase, scalarproduct
    */
    VectorXd calculate_atom(qint32 sampleCount, qreal scale, quint32 translation, qreal modulation, qint32 channel, MatrixXd residuum, ReturnValue return_value);

//=========================================================================================================

public slots:
    void send_result();
    void matching_pursuit (MatrixXd signal, qint32 max_iterations, qreal epsilon);
    void process();
    void recieve_input(MatrixXd signal, qint32 max_iterations, qreal epsilon);

    //=========================================================================================================

signals:
    void current_result(qint32 current_iteration, qint32 max_iteration, qreal current_energy, qreal max_energy, gabor_atom_list atom_list);
    void finished();
};

}   // NAMESPACE

#endif // ADAPTIVEMP_H
