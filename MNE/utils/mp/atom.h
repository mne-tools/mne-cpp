//MATCHING PURSUIT

#ifndef ATOM_H
#define ATOM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <vector>
#include <math.h>
#include <utils/utils_global.h>
#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <Eigen/unsupported/FFT>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>

namespace UTILSLIB
{
    //=============================================================================================================
    // NAMESPACES

    using namespace Eigen;

    //*************************************************************************************************************

    #define PI 3.1415926535897932384626433832795

    // Atomklasse zum Erstellen und Abrufen von Atomen und deren Parameter
    class UTILSSHARED_EXPORT Atom// : public QObject
    {
       // Q_OBJECT

    public:

        bool SaveToRam;
        qint32 SampleCount;
        qreal Scale;
        qint32 Translation;
        qreal Modulation;
    };

    class UTILSSHARED_EXPORT GaborAtom : public Atom
    {

    public:

        GaborAtom(qint32 sampleCount, qreal scale, qint32 translation, qreal modulation, qreal phase, bool saveToRam = false);
        qreal Phase;
        static VectorXd GaborAtom::GaussFunction (qint32 sampleCount, qreal scale, qint32 translation);
        VectorXcd GaborAtom::CreateComplex();
        VectorXd GaborAtom::CreateReal();
    };

}
#endif // ATOM_H
