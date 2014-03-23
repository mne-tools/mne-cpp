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

GaborAtom::GaborAtom(qint32 sampleCount, qreal scale, qint32 translation, qreal modulation, qreal phase, bool saveToRam)
{
    GaborAtom::SampleCount = sampleCount;
    GaborAtom::Scale = scale;
    GaborAtom::Translation = translation;
    GaborAtom::Modulation = modulation;
    GaborAtom::Phase = phase;
    GaborAtom::SaveToRam = saveToRam;
}

VectorXd GaborAtom::GaussFunction (qint32 sampleCount, qreal scale, qint32 translation)
{
    VectorXd gauss = VectorXd::Zero(sampleCount);

    for(qint32 n = 0; n < sampleCount; n++)
    {
        qreal t = (n-translation)/scale;
        gauss[n] = exp(-PI * pow(t, 2))*pow(sqrt(scale),(-1))*pow(2,(0.25));
    }

    return gauss;
}

VectorXcd GaborAtom::CreateComplex()
{
    VectorXcd complexAtom(GaborAtom::SampleCount);
    qreal normAtom = 0;

    if(GaborAtom::Scale == GaborAtom::SampleCount && GaborAtom::Translation == floor(GaborAtom::SampleCount / 2))
        for(qint32 i = 0; i < GaborAtom::SampleCount; i++)
            complexAtom[i] = std::polar(1 / sqrt(GaborAtom::SampleCount), 2 * PI * GaborAtom::Modulation / GaborAtom::SampleCount * i);
     else
        for(qint32 i = 0; i < GaborAtom::SampleCount; i++)
        {
            qreal t = (i - GaborAtom::Translation) / GaborAtom::Scale;
            complexAtom[i] = std::polar(1 / sqrt(GaborAtom::SampleCount) * pow(2, 0.25) * exp( -PI * pow(t, 2))
                                              , 2 * PI * GaborAtom::Modulation / GaborAtom::SampleCount * i);

        }

    //normalization
    for(qint32 i = 0; i < GaborAtom::SampleCount; i++)
        normAtom += abs(complexAtom[i]);

    if(normAtom != 0)
        for(qint32 i = 0; i < GaborAtom::SampleCount; i++)
            complexAtom[i] = complexAtom[i] / normAtom;


    return complexAtom;
}

VectorXd GaborAtom::CreateReal()
{
    VectorXd realAtom(GaborAtom::SampleCount);
    qreal normAtom = 0;

    if(GaborAtom::Scale == GaborAtom::SampleCount)
        for(qint32 i = 0; i < GaborAtom::SampleCount; i++)
            realAtom[i] = 1 / sqrt(GaborAtom::SampleCount) * cos( 2 * PI * GaborAtom::Modulation / GaborAtom::SampleCount * i + GaborAtom::Phase);
    else
    {
        VectorXd envelope = GaborAtom::GaussFunction(GaborAtom::SampleCount, GaborAtom::Scale, GaborAtom::Translation);
        for(qint32 i = 0; i < GaborAtom::SampleCount; i++)
        {
            qreal t = (i - GaborAtom::Translation) / GaborAtom::Scale;
            realAtom[i] = envelope[i] * cos(2 * PI * GaborAtom::Modulation / GaborAtom::SampleCount * i + GaborAtom::Phase);
        }
    }

    //normalization
    for(qint32 i = 0; i < GaborAtom::SampleCount; i++)
        normAtom += realAtom[i];

    if(normAtom != 0)
        for(qint32 i = 0; i < GaborAtom::SampleCount; i++)
            realAtom[i] = realAtom[i] / normAtom;

    return realAtom;
}
