//MATCHING PURSUIT

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
    VectorXd atomValues = CreateReal(GaborAtom::SampleCount, GaborAtom::Scale, GaborAtom::Translation, GaborAtom::Modulation, GaborAtom::Phase);
    QStringList atomStringValues;
    for(qint32 i = 0; i < atomValues.rows(); i++)
        atomStringValues.append(QString("%1").arg(atomValues[i]));
    return atomStringValues;
}

GaborAtom::GaborAtom()
{
    energy = 0;
    MaxScalarProduct = 0;
}

VectorXd GaborAtom::GaussFunction (qint32 sampleCount, qreal scale, qint32 translation)
{
    VectorXd gauss = VectorXd::Zero(sampleCount);

    for(qint32 n = 0; n < sampleCount; n++)
    {
        qreal t = (qreal(n)-translation)/scale;
        gauss[n] = exp(-PI * pow(t, 2))*pow(sqrt(scale),(-1))*pow(qreal(2),(0.25));
    }

    return gauss;
}

VectorXcd GaborAtom::CreateComplex(qint32 sampleCount, qreal scale, qint32 translation, qreal modulation)
{
    VectorXcd complexAtom(sampleCount);
    qreal normAtom = 0;

    //if scale == signalLength and translation == middle of signal there is no window or envelope necessary
    //thats a simplification to save calculation time and reduces the windowing effects
    if(scale == sampleCount && translation == floor(sampleCount / 2))
        for(qint32 i = 0; i < sampleCount; i++)
            complexAtom[i] = std::polar(1 / sqrt(qreal(sampleCount)), 2 * PI * modulation / qreal(sampleCount) * qreal(i));

    //else scale is smaler than signalLength and translation is not in the middle an envelopement is required
    else
        for(qint32 i = 0; i < sampleCount; i++)
        {
            qreal t = (qreal(i) - qreal(translation)) / qreal(scale);
            complexAtom[i] = std::polar(1 / sqrt(sampleCount) * pow(2.0, 0.25) * exp( -PI * pow(t, 2.0))
                                              , 2 * PI * modulation / qreal(sampleCount) * qreal(i));
        }

    //normalization
    for(qint32 i = 0; i < sampleCount; i++)
        normAtom += pow(abs(complexAtom[i]), 2.0);

    normAtom = sqrt(normAtom);

    if(normAtom != 0)
        for(qint32 i = 0; i < sampleCount; i++)
            complexAtom[i] = complexAtom[i] / normAtom;

    return complexAtom;
}

VectorXd GaborAtom::CreateReal(qint32 sampleCount, qreal scale, qint32 translation, qreal modulation, qreal phase)
{
    VectorXd realAtom(sampleCount);
    qreal normAtom = 0;

    if(scale == sampleCount)
        for(qint32 i = 0; i < sampleCount; i++)
            realAtom[i] = 1 / sqrt(sampleCount) * cos( 2 * PI * modulation / sampleCount * qreal(i) + phase);
    else
    {
        VectorXd envelope = GaborAtom::GaussFunction(sampleCount, scale, translation);
        for(qint32 i = 0; i < sampleCount; i++)
        {
            qreal t = (qreal(i) - translation) / scale;
            realAtom[i] = envelope[i] * cos(2 * PI * modulation / sampleCount * qreal(i) + phase);
            Q_UNUSED(t);
        }
    }

    //normalization
    for(qint32 i = 0; i < sampleCount; i++)
        normAtom += pow(realAtom[i], 2);

    normAtom = sqrt(normAtom);

    if(normAtom != 0)
        for(qint32 i = 0; i < sampleCount; i++)
            realAtom[i] = realAtom[i] / normAtom;

    return realAtom; //length of the vector realAtom is 1 after normalization
}

ChirpAtom::ChirpAtom(qint32 sampleCount, qreal scale, qint32 translation, qreal modulation, qreal phase, qreal chirp, bool saveToRam)
{
    ChirpAtom::SampleCount = sampleCount;
    ChirpAtom::Scale = scale;
    ChirpAtom::Translation = translation;
    ChirpAtom::Modulation = modulation;
    ChirpAtom::Phase = phase;
    ChirpAtom::Chirp = chirp;
    ChirpAtom::SaveToRam = saveToRam;
}

VectorXcd ChirpAtom::CreateComplex()
{
    VectorXcd vec;
    return vec;
}

VectorXd ChirpAtom::CreateReal()
{
    VectorXd vec;
    return vec;
}

QStringList ChirpAtom::CreateStringValues()
{
    VectorXd atomValues = CreateReal();
    QStringList atomStringValues;
    for(qint32 i = 0; i < atomValues.rows(); i++)
        atomStringValues.append(QString("%1").arg(atomValues[i]));
    return atomStringValues;
}
