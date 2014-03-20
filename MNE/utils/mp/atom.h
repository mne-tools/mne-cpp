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

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QCoreApplication>

namespace UTILSLIB
{
//=============================================================================================================
// NAMESPACES


//*************************************************************************************************************



// Atomklasse zum Erstellen und Abrufen von Atomen und deren Parameter
class UTILSSHARED_EXPORT Atom : public QObject
{
    Q_OBJECT



public:
//    enum AtomType
//    {
//             Gauss,
//             Chirp
//    };

    Atom();

    qreal Samples;
    qreal Scale;
    qreal Modulation;
    qreal Phase;
    qreal ChirpValue;
    //Atom::AtomType AType;

    //QList<qreal> Create(qint32 samples, qreal scale, qreal modulation, qreal phase, qreal chirp = 0 /*,Atom::AtomType atomType = Gauss*/);
    //QStringList  CreateStringValues(qint32 samples, qreal scale, qreal modulation, qreal phase, qreal chirp/*, Atom::AtomType atomType*/);
};

}
#endif // ATOM_H
