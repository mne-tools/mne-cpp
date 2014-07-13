//=============================================================================================================
/**
* @file     atom.cpp
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>
*
* @version  1.0
* @date     July, 2014
*
* ported to mne-cpp by Martin Henfling and Daniel Knobl in May 2014
* from original code by Marcij Gratkowski
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
* @brief    Implemetation of root funtions needed to perform the Matching Pursuit Algorithm introduced
*           by Stephane Mallat and Zhifeng Zhang.
*           Matlabimplemetation of Marcij Gratkowski is used as Source and reference.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "atom.h"

//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>
#include <algorithm>    // std::sort
#include <vector>       // std::vector

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QStringList GaborAtom::CreateStringValues()
{
    VectorXd atomValues = create_real(GaborAtom::sample_count, GaborAtom::scale, GaborAtom::translation, GaborAtom::modulation, GaborAtom::phase);
    QStringList atomStringValues;
    for(qint32 i = 0; i < atomValues.rows(); i++)
        atomStringValues.append(QString("%1").arg(atomValues[i]));
    return atomStringValues;
}

//*************************************************************************************************************

GaborAtom::GaborAtom()
{
    energy = 0;
    max_scalar_product = 0;
}

//*************************************************************************************************************

VectorXd GaborAtom::gauss_function (qint32 sample_count, qreal scale, qint32 translation)
{
    VectorXd gauss = VectorXd::Zero(sample_count);

    for(qint32 n = 0; n < sample_count; n++)
    {
        qreal t = (qreal(n)-translation)/scale;
        gauss[n] = exp(-PI * pow(t, 2))*pow(sqrt(scale),(-1))*pow(qreal(2),(0.25));
    }

    return gauss;
}

//*************************************************************************************************************

VectorXcd GaborAtom::create_complex(qint32 sample_count, qreal scale, qint32 translation, qreal modulation)
{
    VectorXcd complex_atom(sample_count);
    qreal norm_atom = 0;

    //if scale == signalLength and translation == middle of signal there is no window or envelope necessary
    //thats a simplification to save calculation time and reduces the windowing effects
    if(scale == sample_count && translation == floor(sample_count / 2))
        for(qint32 i = 0; i < sample_count; i++)
            complex_atom[i] = std::polar(1 / sqrt(qreal(sample_count)), 2 * PI * modulation / qreal(sample_count) * qreal(i));

    //else scale is smaler than signalLength and translation is not in the middle an envelopement is required
    else
        for(qint32 i = 0; i < sample_count; i++)
        {
            qreal t = (qreal(i) - qreal(translation)) / qreal(scale);
            complex_atom[i] = std::polar(1 / sqrt(sample_count) * pow(2.0, 0.25) * exp( -PI * pow(t, 2.0))
                                              , 2 * PI * modulation / qreal(sample_count) * qreal(i));
        }

    //normalization
    for(qint32 i = 0; i < sample_count; i++)
        norm_atom += pow(abs(complex_atom[i]), 2.0);

    norm_atom = sqrt(norm_atom);

    if(norm_atom != 0)
        for(qint32 i = 0; i < sample_count; i++)
            complex_atom[i] = complex_atom[i] / norm_atom;

    return complex_atom;
}

//*************************************************************************************************************

VectorXd GaborAtom::create_real(qint32 sample_count, qreal scale, qint32 translation, qreal modulation, qreal phase)
{
    VectorXd real_atom(sample_count);
    qreal norm_atom = 0;

    if(scale == sample_count)
        for(qint32 i = 0; i < sample_count; i++)
            real_atom[i] = 1 / sqrt(sample_count) * cos( 2 * PI * modulation / sample_count * qreal(i) + phase);
    else
    {
        VectorXd envelope = GaborAtom::gauss_function(sample_count, scale, translation);
        for(qint32 i = 0; i < sample_count; i++)
        {
            qreal t = (qreal(i) - translation) / scale;
            real_atom[i] = envelope[i] * cos(2 * PI * modulation / sample_count * qreal(i) + phase);
            Q_UNUSED(t);
        }
    }

    //normalization
    for(qint32 i = 0; i < sample_count; i++)
        norm_atom += pow(real_atom[i], 2);

    norm_atom = sqrt(norm_atom);

    if(norm_atom != 0)
        for(qint32 i = 0; i < sample_count; i++)
            real_atom[i] = real_atom[i] / norm_atom;

    return real_atom; //length of the vector realAtom is 1 after normalization
}

//*************************************************************************************************************

ChirpAtom::ChirpAtom(qint32 sample_count, qreal scale, qint32 translation, qreal modulation, qreal phase, qreal chirp, bool saveToRam)
{
    ChirpAtom::sample_count = sample_count;
    ChirpAtom::scale = scale;
    ChirpAtom::translation = translation;
    ChirpAtom::modulation = modulation;
    ChirpAtom::phase = phase;
    ChirpAtom::chirp = chirp;
    ChirpAtom::SaveToRam = saveToRam;
}

//*************************************************************************************************************

VectorXcd ChirpAtom::CreateComplex()
{
    VectorXcd vec;
    return vec;
}

//*************************************************************************************************************

VectorXd ChirpAtom::CreateReal()
{
    VectorXd vec;
    return vec;
}

//*************************************************************************************************************

QStringList ChirpAtom::CreateStringValues()
{
    VectorXd atomValues = CreateReal();
    QStringList atomStringValues;
    for(qint32 i = 0; i < atomValues.rows(); i++)
        atomStringValues.append(QString("%1").arg(atomValues[i]));
    return atomStringValues;
}
