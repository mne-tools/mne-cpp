//=============================================================================================================
/**
* @file     atom.h
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>
*
* @version  1.0
* @date     July, 2014
*
* atom class declaration
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
* @brief    ATOM class declaration, providing core features and parameters of Atoms used in Matching Pursiut Algorithm
*           GaborAtom and ChirpAtom inherited from Atom class
*
*/

#ifndef ATOM_H
#define ATOM_H

//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>

//*************************************************************************************************************
//=============================================================================================================
// UTILS INCLUDES
//=============================================================================================================

#include <utils/utils_global.h>

//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <unsupported/Eigen/FFT>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>

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

//=============================================================================================================

#define PI 3.1415926535897932384626433832795

//=============================================================================================================

/**
* providing basic core parameters any Atom like GaborAtoms need for further calculations
*
* @brief basic core parameters of atoms
*/
class UTILSSHARED_EXPORT Atom// : public QObject // Atom class to build and call atoms or their parameters
{
    // Q_OBJECT
public:

    bool SaveToRam;
    qint32 sample_count;
    qreal scale;
    qint32 translation;
    qreal modulation;
    qreal max_scalar_product;
    MatrixXd residuum;
    qreal energy;
    //qreal NormAtom;

};

//=============================================================================================================

/**
* GaborAtom class inherited from Atom with additional functions
*
* @brief GaborAtom used in adaptive MP Algorithm
*/
class UTILSSHARED_EXPORT GaborAtom : public Atom
{

public:
    qreal phase;

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * GaborAtom class inherited from Atom with additional functions
    */
    GaborAtom();

    //=========================================================================================================
    /**
    * GaborAtom_gauss_function
    *
    * ### MP toolbox root function ###
    *
    * calculates gaussfunction for GaborAtoms
    *
    * @param[in] sampleCount    number of samples in the atom
    * @param[in] scale          scale of atom
    * @param[in] translation    translation of atom
    *
    * @return Vector with gaussfunction content
    */
    static VectorXd gauss_function (qint32 sample_count, qreal scale, quint32 translation);

    //=========================================================================================================
    /**
    * GaborAtom_create_complex
    *
    * ### MP toolbox root function ###
    *
    * calculates complex gabor atoms for MP Algorithm
    *
    * @param[in] sampleCount    number of samples in the atom
    * @param[in] scale          scale of atom
    * @param[in] translation    translation of atom
    * @param[in] modulation     modulation of atom
    *
    * @return complex Vector with GaborAtom
    */
    VectorXcd create_complex(qint32 sample_count, qreal scale, quint32 translation, qreal modulation);

    //=========================================================================================================
    /**
    * GaborAtom_create_complex
    *
    * ### MP toolbox root function ###
    *
    * calculates real gabor atoms for MP Algorithm
    *
    * @param[in] sampleCount    number of samples in the atom
    * @param[in] scale          scale of atom
    * @param[in] translation    translation of atom
    * @param[in] modulation     modulation of atom
    * @param[in] phase          phase of the complex atom
    *
    * @return real Vector with GaborAtom
    */
    VectorXd create_real(qint32 sample_count, qreal scale, quint32 translation, qreal modulation, qreal phase);

    //=========================================================================================================
    /**
    * adaptiveMP_calculate_atom
    *
    * ### MP toolbox communication function ###
    *
    * creates String of GaborAtoms
    *
    * @return GaborAtom as String
    */
    QStringList CreateStringValues();

};

    class UTILSSHARED_EXPORT ChirpAtom : public Atom
    {

    public:

        qreal phase;
        qreal chirp;

        ChirpAtom(qint32 sampleCount, qreal scale, quint32 translation, qreal modulation, qreal phase, qreal chirp, bool saveToRam = false);

        VectorXcd CreateComplex();
        VectorXd CreateReal();
        QStringList CreateStringValues();

    };


}
#endif // ATOM_H
