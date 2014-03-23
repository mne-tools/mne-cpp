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
//using namespace Eigen;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

adaptiveMP::adaptiveMP()
{

}

//*************************************************************************************************************

//MP Algorithmus of M. Gratkowski
QList<GaborAtom> adaptiveMP::MatchingPursuit (MatrixXd signal,qint32 max_it,qreal epsilon)
{
        QList<GaborAtom> atomList;
        Eigen::FFT<double> fft;        
        MatrixXd residuum = signal; //residuum initialised with signal
        qint32 it = 0;                  //iterationscounter
        qint32 sampleCount = signal.rows();
        qint32 channelCount = signal.cols();
        VectorXd signalEnergy(channelCount);

        //calculate signalenergy
        for(qint32 channel = 0; channel < channelCount; channel++)
            for(qint32 sample = 0; sample < sampleCount; sample++)
                signalEnergy[channel] = (signal(sample, channel) * signal(sample, channel));

        while(it < max_it)
        {
            //variables for dyadic sampling
            qreal s = 1;                                                    //scale
            qint32 j = 1;
            VectorXd maxScalarProduct = RowVectorXd::Zero(channelCount);

            while(s < sampleCount)
            {
                qreal k = 0;                                                //for modulation 2*pi*k/N
                qint32 p = floor(sampleCount / 2);                          //translation
                VectorXd envelope = GaborAtom::GaussFunction(sampleCount, s, p);
                VectorXcd fftEnvelope = RowVectorXcd::Zero(sampleCount);
                fft.fwd(fftEnvelope, envelope);

                while(k <= sampleCount/2)
                {
                    p = floor(sampleCount/2);
                    VectorXcd modulation = ModulationFunction(sampleCount, k);
                    VectorXcd modulatedResid(sampleCount);
                    VectorXcd fftModulatedResid;
                    VectorXcd fftMEResid(sampleCount);
                    VectorXd corrCoeffs;
                    GaborAtom *gaborAtom = new GaborAtom(sampleCount, s, p, k, 0);

                    //iteration for multichannel
                    for(qint32 chn = 0; chn < channelCount; chn++)
                    {
                        qint32 maxIndex = 0;
                        qreal maximum;// = corrCoeffs[0];
                        qreal phase = 0;
                        qreal scalarProduct = 0;

                        //complex correlation of signal and sinus-modulated gaussfunction
                        for(qint32 l = 0; l< sampleCount; l++)
                            modulatedResid[l] = residuum(l, chn) * modulation[l];
                        fft.fwd(fftModulatedResid, modulatedResid);

                        for( qint32 m = 0; m < sampleCount; m++)
                           fftMEResid[m] = fftModulatedResid[m] * conj(fftEnvelope[m]);

                        fft.inv(corrCoeffs, fftMEResid);
                        maximum = corrCoeffs[0];

                        //find index of maximum correlation-coefficient to use in translation
                        for(qint32 i = 1; i < corrCoeffs.rows(); i++)
                            if(maximum < corrCoeffs[i])
                            {
                                maximum = corrCoeffs[i];
                                maxIndex = i;
                            }

                        //adapting translation p to create atomtranslation correctly
                        if(maxIndex >= p) p = maxIndex - p;
                        else p = maxIndex + p;

                        //create complex Gaboratom
                        VectorXcd complexGaborAtom = gaborAtom->CreateComplex();

                        //calculate Inner Product: preparation to find the parameter phase
                        std::complex<double> innerProduct(0, 0);

                        for(qint32 i = 0; i < sampleCount; i++)
                            innerProduct += residuum(i, chn) * conj(complexGaborAtom[i]);

                        //calculate Phase to create realGaborAtoms
                        phase = atan(imag(innerProduct) / real(innerProduct));
                        VectorXd realGaborAtom = gaborAtom ->CreateReal();

                        //Hint: inner Product is used to select the best matching Atom
                        for(qint32 i= 0; i < sampleCount; i++)
                            scalarProduct += realGaborAtom[i] * residuum(i, chn);

                        //dbg why difference between complex innerProduct and qreal scalarProduct??

                        //innerProduct = 0;//(0, 0);
                        //for(qint32 i= 0; i < sampleCount; i++)
                        //    innerProduct += (realGaborAtom[i] * residuum(i, chn), 0);

                        //std::cout << real(innerProduct) << std::endl;
                        //std::cout <<"\n"<< scalarProduct << std::endl;

                        if(abs(scalarProduct) > abs(maxScalarProduct[chn]))
                        {
                            //set highest scalarproduct, in comparison to best matching atom
                            maxScalarProduct[chn] = scalarProduct;
                            gaborAtom->Scale = s;
                            gaborAtom->Translation = p;
                            gaborAtom->Modulation = k;
                            gaborAtom->Phase = phase;
                            atomList.append(*gaborAtom);
                        }


                    }

                    k = k + pow(2,(-j))*sampleCount/2;

                }

                j++;
                s = pow(2,j);
            }

            it++;
        }

        //printf("ich verlasse mPursuit");
        return atomList;
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

        modulation[n] = std::polar(1 / sqrt(N), 2 * 3.1416 * k / N * n);
    }
    //printf("ich verlasse modulationFunction");
    return modulation;
}



