//MATCHING PURSUIT
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "adaptivemp.h"
#include <vector>

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

//MP Algorithm of M. Gratkowski
QList<GaborAtom> adaptiveMP::MatchingPursuit (MatrixXd signal,qint32 max_it,qreal epsilon)
{
    QList<GaborAtom> atomList;
    Eigen::FFT<double> fft;
    MatrixXd residuum = signal; //residuum initialised with signal
    qint32 it = 0;              //iterationscounter
    qint32 sampleCount = signal.rows();
    qint32 channelCount = signal.cols();
    qint32 signalChannel = 0;
    VectorXd signalEnergy = VectorXd::Zero(channelCount);
    VectorXd residuumEnergy = VectorXd::Zero(channelCount);
    VectorXd energyThreshold = VectorXd::Zero(channelCount);

    //calculate signalenergy
    for(qint32 channel = 0; channel < channelCount; channel++)
    {
        for(qint32 sample = 0; sample < sampleCount; sample++)
            signalEnergy[channel] += (signal(sample, channel) * signal(sample, channel));

        energyThreshold[channel] = epsilon * signalEnergy[channel];
        residuumEnergy[channel] = signalEnergy[channel];
    }


    //TODO multichannel!! Not working yet: problem is energyThreshold-->when should i increase signalChannel? may change order of loops for that
    //connecting atoms to the channel, thing to think about....

    while(it < max_it && energyThreshold[signalChannel] <= residuumEnergy[signalChannel] )
    {
        //variables for dyadic sampling
        qreal s = 1;                            //scale
        qint32 j = 1;
        qreal maxScalarProduct = 0;             //inner product for choosing the best matching atom
        qreal k = 0;                            //for modulation 2*pi*k/N
        qint32 p = floor(sampleCount / 2);      //translation
        GaborAtom *gaborAtom = new GaborAtom();
        gaborAtom->SampleCount = sampleCount;
        qreal phase = 0;

        while(s < sampleCount)
        {
            k = 0;                              //for modulation 2*pi*k/N
            p = floor(sampleCount / 2);         //translation
            VectorXd envelope = GaborAtom::GaussFunction(sampleCount, s, p);
            VectorXcd fftEnvelope = RowVectorXcd::Zero(sampleCount);
            fft.fwd(fftEnvelope, envelope);

            while(k < sampleCount/2)
            {
                p = floor(sampleCount/2);
                VectorXcd modulation = ModulationFunction(sampleCount, k);
                VectorXcd modulatedResid = VectorXcd::Zero(sampleCount);
                VectorXcd fftModulatedResid = VectorXcd::Zero(sampleCount);
                VectorXcd fftMEResid = VectorXcd::Zero(sampleCount);
                VectorXd corrCoeffs = VectorXd::Zero(sampleCount);

                //iteration for multichannel
                for(qint32 chn = 0; chn < channelCount; ++chn)
                {
                    qint32 maxIndex = 0;
                    qreal maximum = 0;
                    phase = 0;

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
                    if(maxIndex >= p) p = maxIndex - p + 1;
                    else p = maxIndex + p;

                    VectorXd atomParameters = adaptiveMP::CalculateAtom(sampleCount, s, p, k, chn, residuum, RETURNPARAMETERS);

                    if(abs(atomParameters[4]) > abs(maxScalarProduct))
                    {
                        //set highest scalarproduct, in comparison to best matching atom
                        maxScalarProduct            = atomParameters[4];
                        gaborAtom->Scale            = atomParameters[0];
                        gaborAtom->Translation      = atomParameters[1];
                        gaborAtom->Modulation       = atomParameters[2];
                        gaborAtom->Phase            = atomParameters[3];
                        gaborAtom->MaxScalarProduct = maxScalarProduct;
                    }

                }
                k += pow(2.0,(-j))*sampleCount/2;

            }
            j++;
            s = pow(2.0,j);
        }
        std::cout << "gefundene Parameter " << it << ":\n   " << gaborAtom->Scale << "     " << gaborAtom->Translation <<
                     "      " << gaborAtom->Modulation << "      " << gaborAtom->Phase << "\n";

        //replace atoms with s==N and p = floor(N/2) by such atoms that do not have an envelope
        k = 0;
        s = sampleCount;
        p = floor(sampleCount / 2);
        j = floor(log2(sampleCount));
        phase = 0;

        for(qint32 chn = 0; chn < channelCount; ++chn)
        {
            while(k < sampleCount / 2)
            {
                VectorXd parametersNoEnvelope = adaptiveMP::CalculateAtom(sampleCount, s, p, k, chn, residuum, RETURNPARAMETERS);

                if(abs(parametersNoEnvelope[4]) > abs(maxScalarProduct))// && gaborAtom->Scale == s && gaborAtom->Translation == p)
                {
                    //set highest scalarproduct, in comparison to best matching atom
                    maxScalarProduct            = parametersNoEnvelope[4];
                    gaborAtom->Scale            = parametersNoEnvelope[0];
                    gaborAtom->Translation      = parametersNoEnvelope[1];
                    gaborAtom->Modulation       = parametersNoEnvelope[2];
                    gaborAtom->Phase            = parametersNoEnvelope[3];
                    gaborAtom->MaxScalarProduct = maxScalarProduct;
                }
                k += pow(2.0,(-j))*sampleCount/2;

            }

        }
        std::cout << "gefundene Parameter nach ersetung NoEnvelope " << it << ":\n   " << gaborAtom->Scale << "     " << gaborAtom->Translation <<
                     "      " << gaborAtom->Modulation << "      " << gaborAtom->Phase << "\n";

        //Maximisation Simplex Algorithm implemented by Bozoa Jia, adapted to the MP Algorithm by Martin Henfling. Copyright (C) 2010 Botao Jia
        //todo change to clean use of EIGEN, @present its mixed with Namespace std and <vector>
        //iteration for multichannel
        for(qint32 chn = 0; chn < channelCount; ++chn)
        {
            //simplexfunction to find minimum of target among parameters s, p, k
            std::vector<double> init;

            init.push_back(gaborAtom->Scale);
            init.push_back(gaborAtom->Translation);
            init.push_back(gaborAtom->Modulation);

            double tol = 1E8 * std::numeric_limits<double>::epsilon();
            std::vector<std::vector<double> > x = std::vector<std::vector<double> >();
            qint32 iterations = 1E3;
            qint32 N = init.size();                     //space dimension

            VectorXd atomFxcParams = VectorXd::Zero(5); //initialisation for contraction coefficients

            const qreal a=1.0, b=0.2, g=0.5, h=0.5;  //coefficients
                                                     //a: reflection  -> xr step away from worst siplex found
                                                     //b: expansion   -> xe if better with a so go in this direction with b
                                                     //g: contraction -> xc calc new worst point an bring closer to middle of simplex
                                                     //h: full contraction to x1
            std::vector<double> xcentroid_old(N,0);  //simplex center * (N+1)
            std::vector<double> xcentroid_new(N,0);  //simplex center * (N+1)
            std::vector<double> vf(N+1,0);           //f evaluated at simplex vertexes
            qint32 x1 = 0, xn = 0, xnp1 = 0;         //x1:   f(x1) = min { f(x1), f(x2)...f(x_{n+1} }
                                                     //xnp1: f(xnp1) = max { f(x1), f(x2)...f(x_{n+1} }
                                                     //xn:   f(xn)<f(xnp1) && f(xn)> all other f(x_i)
            qint32 cnt = 0; //iteration step number

            if(x.size()== 0) //if no initial simplex is specified
            {
                //construct the trial simplex
                //based upon the initial guess parameters
                std::vector<double> del( init );
                std::transform(del.begin(), del.end(), del.begin(),
                std::bind2nd( std::divides<double>() , 20) );//'20' is picked
                                                     //assuming initial trail close to true

                for(qint32 i = 0; i < N; ++i)
                {
                    std::vector<double> tmp( init );
                    tmp[i] +=  del[i];
                    x.push_back( tmp );
                }

                x.push_back(init);//x.size()=N+1, x[i].size()=N

                //xcentriod
                std::transform(init.begin(), init.end(), xcentroid_old.begin(), std::bind2nd(std::multiplies<double>(), N+1) );
            }//constructing the simplex finished

            //optimization begins
            for(cnt=0; cnt<iterations; ++cnt)
            {
                for(qint32 i=0; i < N+1; ++i)
                {
                    VectorXd atomFx = VectorXd::Zero(sampleCount);

                    if(gaborAtom->Scale == sampleCount && gaborAtom->Translation == floor(sampleCount / 2))
                        atomFx = adaptiveMP::CalculateAtom(sampleCount, sampleCount, floor(sampleCount / 2), x[i][2], chn, residuum, RETURNATOM);

                    else
                        atomFx = adaptiveMP::CalculateAtom(sampleCount, x[i][0], x[i][1], x[i][2], chn, residuum, RETURNATOM);

                    //create targetfunction of realGaborAtom and Residuum
                    double target = 0;
                    for(qint32 k = 0; k < atomFx.rows(); k++)
                    {
                        target -=atomFx[k]*residuum(k,0);
                    }

                    vf[i] = target;
                }

                x1=0; xn=0; xnp1=0;//find index of max, second max, min of vf.

                for(quint32 i=0; i < vf.size(); ++i)
                {
                    if(vf[i]<vf[x1])      x1 = i;
                    if(vf[i]>vf[xnp1])    xnp1 = i;

                }

                xn = x1;

                for(quint32 i=0; i<vf.size();++i) if(vf[i]<vf[xnp1] && vf[i]>vf[xn])  xn=i;

                //x1, xn, xnp1 are found

                std::vector<double> xg(N, 0);//xg: centroid of the N best vertexes

                for(quint32 i=0; i<x.size(); ++i) if(i!=xnp1) std::transform(xg.begin(), xg.end(), x[i].begin(), xg.begin(), std::plus<double>() );

                std::transform(xg.begin(), xg.end(), x[xnp1].begin(), xcentroid_new.begin(), std::plus<double>());
                std::transform(xg.begin(), xg.end(), xg.begin(), std::bind2nd(std::divides<double>(), N) );
                //xg found, xcentroid_new updated

                //termination condition
                double diff=0;          //calculate the difference of the simplex centers

                //see if the difference is less than the termination criteria
                for(qint32 i=0; i<N; ++i) diff += fabs(xcentroid_old[i]-xcentroid_new[i]);

                if (diff/N < tol) break;              //terminate the optimizer
                else xcentroid_old.swap(xcentroid_new); //update simplex center

                //reflection:
                std::vector<double> xr(N,0);

                for( qint32 i=0; i<N; ++i) xr[i]=xg[i]+a*(xg[i]-x[xnp1][i]);
                //reflection, xr found

                VectorXd atomFxr = VectorXd::Zero(sampleCount);

                if(gaborAtom->Scale == sampleCount && gaborAtom->Translation == floor(sampleCount / 2))
                    atomFxr = adaptiveMP::CalculateAtom(sampleCount, sampleCount, floor(sampleCount / 2), xr[2], chn, residuum, RETURNATOM);

                else
                    atomFxr = adaptiveMP::CalculateAtom(sampleCount, xr[0], xr[1], xr[2], chn, residuum, RETURNATOM);

                //create targetfunction of realGaborAtom and Residuum
                double fxr = 0;
                for(qint32 k = 0; k < atomFxr.rows(); k++) fxr -=atomFxr[k]*residuum(k,0);

                //double fxr = target;//record function at xr

                if(vf[x1]<=fxr && fxr<=vf[xn]) std::copy(xr.begin(), xr.end(), x[xnp1].begin());

                //expansion:
                else if(fxr<vf[x1])
                {
                    std::vector<double> xe(N,0);

                    for( qint32 i=0; i<N; ++i) xe[i]=xr[i]+b*(xr[i]-xg[i]);

                    VectorXd atomFxe = VectorXd::Zero(sampleCount);

                    if(gaborAtom->Scale == sampleCount && gaborAtom->Translation == floor(sampleCount / 2))
                        atomFxe = adaptiveMP::CalculateAtom(sampleCount, sampleCount, floor(sampleCount / 2), xe[2], chn, residuum, RETURNATOM);

                    else
                        atomFxe = adaptiveMP::CalculateAtom(sampleCount, xe[0], xe[1], xe[2], chn, residuum, RETURNATOM);

                    //create targetfunction of realGaborAtom and Residuum
                    double fxe = 0;
                    for(qint32 k = 0; k < atomFxe.rows(); k++) fxe -=atomFxe[k]*residuum(k,0);

                    if( fxe < fxr ) std::copy(xe.begin(), xe.end(), x[xnp1].begin() );
                    else std::copy(xr.begin(), xr.end(), x[xnp1].begin() );
                }//expansion finished,  xe is not used outside the scope

                //contraction:
                else if( fxr > vf[xn] )
                {
                    std::vector<double> xc(N,0);

                    for( qint32 i=0; i<N; ++i)
                        xc[i]=xg[i]+g*(x[xnp1][i]-xg[i]);

                    if(gaborAtom->Scale == sampleCount && gaborAtom->Translation == floor(sampleCount / 2))
                        atomFxcParams = adaptiveMP::CalculateAtom(sampleCount, sampleCount, floor(sampleCount / 2), xc[2], chn, residuum, RETURNPARAMETERS);

                    else
                        atomFxcParams = adaptiveMP::CalculateAtom(sampleCount, xc[0], xc[1], xc[2], chn, residuum, RETURNPARAMETERS);

                    VectorXd atomFxc = gaborAtom->CreateReal(gaborAtom->SampleCount, atomFxcParams[0], atomFxcParams[1], atomFxcParams[2], atomFxcParams[3]);

                    atomFxcParams[4] = 0;

                    for(qint32 i = 0; i < sampleCount; i++)
                        atomFxcParams[4] += atomFxc[i] * residuum(i, chn);

                    //create targetfunction of realGaborAtom and Residuum
                    double fxc = 0;

                    for(qint32 k = 0; k < atomFxc.rows(); k++)
                        fxc -=atomFxc[k]*residuum(k,0);

                    if( fxc < vf[xnp1] )
                        std::copy(xc.begin(), xc.end(), x[xnp1].begin() );

                    else
                        for( quint32 i=0; i<x.size(); ++i )
                            if( i!=x1 )
                                for(qint32 j=0; j<N; ++j)
                                    x[i][j] = x[x1][j] + h * ( x[i][j]-x[x1][j] );
                }//contraction finished, xc is not used outside the scope
            }//optimization is finished

            if(gaborAtom->Scale == sampleCount && gaborAtom->Translation == floor(sampleCount / 2))
                atomFxcParams = adaptiveMP::CalculateAtom(sampleCount, sampleCount, floor(sampleCount / 2), x[x1][2], chn, residuum, RETURNPARAMETERS);

            else
                atomFxcParams = adaptiveMP::CalculateAtom(sampleCount, x[x1][0], x[x1][1], x[x1][2], chn, residuum, RETURNPARAMETERS);

            if(abs(atomFxcParams[4]) > abs(maxScalarProduct))
            {
                maxScalarProduct = atomFxcParams[4];             //scalarProduct
                gaborAtom->Scale              = atomFxcParams[0];//Scale
                gaborAtom->Translation        = atomFxcParams[1];//Translation
                gaborAtom->Modulation         = atomFxcParams[2];//Phase
                gaborAtom->Phase              = atomFxcParams[3];
                gaborAtom->MaxScalarProduct   = maxScalarProduct;
            }

            std::cout <<  "Parameter nach Optimierung " << it << ":\n   " << gaborAtom->Scale << "     " << gaborAtom->Translation <<
                                "      " << gaborAtom->Modulation << "      " << gaborAtom->Phase << "\n\n";
            if(cnt==iterations)//max number of iteration achieves before tol is satisfied
                std::cout<<"Simplex Iteration limit of "<<iterations<<" achieved, result may not be optimal"  <<std::endl;

            //substract best matching Atom from Residuum in each channel
            VectorXd bestMatch = gaborAtom->CreateReal(gaborAtom->SampleCount, gaborAtom->Scale, gaborAtom->Translation, gaborAtom->Modulation, gaborAtom->Phase);

            for(qint32 j = 0; j < gaborAtom->SampleCount; j++)
            {
                residuum(j,chn) -= gaborAtom->MaxScalarProduct * bestMatch[j];
                residuumEnergy[chn] -= (gaborAtom->MaxScalarProduct * bestMatch[j]) * (gaborAtom->MaxScalarProduct * bestMatch[j]);
            }

            gaborAtom->Residuum = residuum;
            atomList.append(*gaborAtom);
        }//end Maximisation Copyright (C) 2010 Botao Jia

        delete gaborAtom;
        it++;
    }//end iterations
    return atomList;
}

//*************************************************************************************************************

VectorXcd adaptiveMP::ModulationFunction(qint32 N, qreal k)
{
    VectorXcd modulation = VectorXcd::Zero(N);

    for(qint32 n = 0; n < N; n++)
    {
        modulation[n] = std::polar(1 / sqrt(qreal(N)), 2 * PI * k / qreal(N) * qreal(n));
    }
    return modulation;
}

//*************************************************************************************************************

VectorXd adaptiveMP::CalculateAtom(qint32 sampleCount, qreal scale, qint32 translation, qreal modulation, qint32 channel, MatrixXd residuum, ReturnValue returnValue)
{
    GaborAtom *gaborAtom = new GaborAtom();
    qreal phase = 0;
    //create complex Gaboratom
    VectorXcd complexGaborAtom = gaborAtom->CreateComplex(sampleCount, scale, translation, modulation);

    //calculate Inner Product: preparation to find the parameter phase
    std::complex<double> innerProduct(0, 0);

    for(qint32 i = 0; i < sampleCount; i++)
        innerProduct += residuum(i, channel) * conj(complexGaborAtom[i]);

    //calculate Phase to create realGaborAtoms
    phase = std::arg(innerProduct);
    if (phase < 0) phase = 2 * PI - phase;
    VectorXd realGaborAtom = gaborAtom->CreateReal(sampleCount, scale, translation, modulation, phase);

    delete gaborAtom;

    switch(returnValue)
    {
        case RETURNPARAMETERS:
        {
            qreal scalarProduct = 0;

            for(qint32 i = 0; i < sampleCount; i++)
                scalarProduct += realGaborAtom[i] * residuum(i, channel);

            VectorXd atomParameters = VectorXd::Zero(5);

            atomParameters[0] = scale;
            atomParameters[1] = translation;
            atomParameters[2] = modulation;
            atomParameters[3] = phase;
            atomParameters[4] = scalarProduct;

            return atomParameters;
        }
        case RETURNATOM: {return realGaborAtom;} //returns normalized realGaborAtom
    }
}


