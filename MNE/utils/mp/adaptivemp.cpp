//MATCHING PURSUIT
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "adaptivemp.h"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

adaptiveMP::adaptiveMP()
{

}

//*************************************************************************************************************

//MP Algorithmus von M. Gratkowski (nur einkanalig - single trial)
QList<Atom> adaptiveMP::MatchingPursuit (QList<qreal> signal,qint32 max_it,qreal epsilon)
{
        QList<Atom> atom;
        Eigen::FFT<double> fft;
        qreal signalEnergy = 0;
        QList<qreal> residuum = signal; //Residuum initialisiert mit signal
        qint32 it = 0;                  //Iterationszähler

        //Berechnen der Signalenergie
        for(qint32 sample = 0; sample < signal.length(); sample++)
            signalEnergy += (signal.at(sample) * signal.at(sample));

        while(it < max_it)
        {
            //Variablen für dyadische Abtastung
            qreal s = 1;
            qint32 j = 1;

            while(s < signal.length())
            {
                qreal k = 0;

                qint32 p = floor(signal.length()/2);    //halbe Sampleanzahl abgerundet bspw. 9/2 = 4
                RowVectorXd envelope = GaussFunction(signal.length(), s, p);
                RowVectorXcd fftEnvelope = RowVectorXcd::Zero(signal.length());
                fft.fwd(fftEnvelope, envelope);

                for(qint32 i = 0; i < signal.length(); i++)
                    std::cout << i<< "     "<< (abs(fftEnvelope(i)))<<"\n";

                for(qint32 i = 0; i < signal.length(); i++)
                    std::cout << i<< "     "<< (envelope(i))<<"\n";


                //QList<qreal> fftEnvelope = //todo hier fehlt fft

               while(k <= signal.length()/2)
                {
                    qint32 p = floor(signal.length()/2);
                    QList<std::complex<qreal>> modulation = ModulationFunction(signal.length(), k);






                    k = k + pow(2,(-j))*signal.length()/2;

                }

                j++;
                s = pow(2,j);
            }

            it++;
        }

        printf("ich verlasse mPursuit");
        return atom;
}

//*************************************************************************************************************

RowVectorXd adaptiveMP::GaussFunction (qint32 N, qreal s, qint32 p)
{
    RowVectorXd gauss = RowVectorXd::Zero(N);

    for(qint32 n = 0; n < N; n++)
    {
        qreal t = (n-p)/s;
        gauss(n) = exp(-3.1416*pow(t, 2))*pow(sqrt(s),(-1))*pow(2,(0.25));
    }
    printf("ich verlasse gaussFunction");
    return gauss;
}

//*************************************************************************************************************

QList<std::complex<qreal>> adaptiveMP::ModulationFunction(qint32 N, qreal k)
{
    QList<std::complex<qreal>> modulation;

    for(qint32 n = 0; n < N; n++)
    {

        modulation.append(std::polar(1/sqrt(N),exp(2*3.1416*k/N*n)));
    }
    printf("ich verlasse modulationFunction");
    return modulation;
}



