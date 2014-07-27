//=============================================================================================================
/**
* @file     fixdict.h
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>
*           Sebastian Krause <sebastian.krause@tu.ilmenau.de
*
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Sebastian Krause,Daniel Knobl and Martin Henfling All rights reserved.
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
* @brief    FIXDICTMP class declaration, providing the implemetation of the Matching Pursuit Algorithm
*           using precalculated atom dictionaries.
*
*/

#ifndef FIXDICTMP_H
#define FIXDICTMP_H

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

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QFile>
#include <QStringList>
#include <QtXml/QtXml>

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

//*************************************************************************************************************
/**
* DECLARE CLASS FixDictMp
*
* @brief The fixdictMP class provides functions several calculating functions to run the Matching Pursuit Algorithm
*/
class UTILSSHARED_EXPORT FixDictMp : public QThread
{
    Q_OBJECT

public:

    //typedef QList<GaborAtom> gabor_atom_list;

    /**
    * fixdictMp_fixdictMP
    *
    * ### MP toolbox function ###
    *
    * Constructor
    *
    * constructs FixDictMp class
    *
    */
    FixDictMp();

    qint32 test();

    QList<GaborAtom> matching_pursuit(QFile &currentDict, VectorXd signalSamples, qint32 iterationsCount);

    QStringList correlation(VectorXd signalSamples, QList<qreal> atomSamples, QString atomName);

    static void create_tree_dict(QString save_path);
    //=========================================================================================================

    qreal create_molecules(VectorXd compare_atom, qreal phase, qreal modulation, quint32 translation, qint32 sample_count, GaborAtom* gabor_Atom, qreal scale);
public slots:
    //void send_result();
    //void matching_pursuit (MatrixXd signal, qint32 max_iterations, qreal epsilon);
    //void process();
    //void recieve_input(MatrixXd signal, qint32 max_iterations, qreal epsilon);

    //=========================================================================================================

signals:
    //void current_result(qint32 current_iteration, qint32 max_iteration, qreal current_energy, qreal max_energy, gabor_atom_list atom_list);
    //void finished();
};//class

}//NAMESPACE

#endif // FIXDICTMP_H
