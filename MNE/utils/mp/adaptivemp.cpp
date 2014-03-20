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
using namespace Eigen;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

adaptiveMP::adaptiveMP()
{

}

//*************************************************************************************************************

//MP Algorithmus von M. Gratkowski (nur einkanalig - single trial)
QList<Atom> adaptiveMP::MatchingPursuit (VectorXd signal,qint32 max_it,qreal epsilon)
{
        QList<Atom> atom;
        Eigen::FFT<double> fft;
        qreal signalEnergy = 0;
        VectorXd residuum = signal; //Residuum initialisiert mit signal
        qint32 it = 0;                  //Iterationszähler

        //Berechnen der Signalenergie
        for(qint32 sample = 0; sample < signal.rows(); sample++)
            signalEnergy += (signal[sample] * signal[sample]);

        while(it < max_it)
        {
            //Variablen für dyadische Abtastung
            qreal s = 1;
            qint32 j = 1;

            while(s < signal.rows())
            {
                qreal k = 0;

                qint32 p = floor(signal.rows()/2);    //halbe Sampleanzahl abgerundet bspw. 9/2 = 4
                VectorXd envelope = GaussFunction(signal.rows(), s, p);
                VectorXcd fftEnvelope = RowVectorXcd::Zero(signal.rows());
                fft.fwd(fftEnvelope, envelope);

                for(qint32 i = 0; i < signal.rows(); i++)
                    std::cout << i<< "     "<< (abs(fftEnvelope[i]))<<"\n";

                for(qint32 i = 0; i < signal.rows(); i++)
                    std::cout << i<< "     "<< (envelope[i])<<"\n";


                //QList<qreal> fftEnvelope = //todo hier fehlt fft

               while(k <= signal.rows()/2)
                {
                    qint32 p = floor(signal.rows()/2);
                    VectorXcd modulation = ModulationFunction(signal.rows(), k);

                    k = k + pow(2,(-j))*signal.rows()/2;

                }

                j++;
                s = pow(2,j);
            }

            it++;
        }

        //printf("ich verlasse mPursuit");
        return atom;
}

//*************************************************************************************************************

VectorXd adaptiveMP::GaussFunction (qint32 N, qreal s, qint32 p)
{
    VectorXd gauss = VectorXd::Zero(N);

    for(qint32 n = 0; n < N; n++)
    {
        qreal t = (n-p)/s;
        gauss[n] = exp(-3.1416*pow(t, 2))*pow(sqrt(s),(-1))*pow(2,(0.25));
    }
    //printf("ich verlasse gaussFunction");
    return gauss;
}

//*************************************************************************************************************

VectorXcd adaptiveMP::ModulationFunction(qint32 N, qreal k)
{
    VectorXcd modulation = VectorXcd::Zero(N);

    for(qint32 n = 0; n < N; n++)
    {

        modulation[n] = std::polar(1/sqrt(N),exp(2*3.1416*k/N*n));
    }
    //printf("ich verlasse modulationFunction");
    return modulation;
}



