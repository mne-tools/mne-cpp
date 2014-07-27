//=============================================================================================================
/**
* @file     adaptivemp.cpp
* @author   Martin Henfling <martin.henfling@tu-ilmenau.de>
*           Daniel Knobl <daniel.knobl@tu-ilmenau.de>
*
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Daniel Knobl and Martin Henfling All rights reserved.
*
* ported to mne-cpp by Martin Henfling and Daniel Knobl in May 2014
* original code was implemented in Matlab Code by Maciej Gratkowski
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
* @brief    Implemetation of the Matching Pursuit Algorithm introduced by Stephane Mallat and Zhifeng Zhang.
*           Matlabimplemetation of Maciej Gratkowski is used as Source and reference.
*
*/

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

AdaptiveMp::AdaptiveMp()
{

}

//*************************************************************************************************************

//MP Algorithm of M. Gratkowski
QList<GaborAtom> AdaptiveMp::matching_pursuit(MatrixXd signal, qint32 max_iterations, qreal epsilon)
{
    max_it = max_iterations;
    Eigen::FFT<double> fft;
    MatrixXd residuum = signal; //residuum initialised with signal
    qint32 sample_count = signal.rows();
    qint32 channel_count = signal.cols();
    signal_energy = 0;//VectorXd::Zero(channel_count);
    qreal residuum_energy = 0;//VectorXd residuum_energy = VectorXd::Zero(channel_count);
    qreal energy_threshold = 0;//VectorXd energy_threshold = VectorXd::Zero(channel_count);

    //calculate signal_energy
    for(qint32 channel = 0; channel < channel_count; channel++)
    {
        for(qint32 sample = 0; sample < sample_count; sample++)
            signal_energy/*[channel]*/ += (signal(sample, channel) * signal(sample, channel));

        energy_threshold/*[channel]*/ = 0.01 * epsilon * signal_energy;//[channel];
        residuum_energy/*[channel]*/ = signal_energy;//[channel];
    }

    //TODO multichannel!! Not working yet: problem is energy_threshold-->when should i increase signal_channel? may change order of loops for that
    //connecting atoms to the channel, thing to think about....

    while(it < max_iterations && (energy_threshold/*[signal_channel]*/ < residuum_energy/*[signal_channel]*/ ))
    {
        //variables for dyadic sampling
        qreal s = 1;                            //scale
        qint32 j = 1;
        qreal max_scalar_product = 0;             //inner product for choosing the best matching atom
        qreal k = 0;                            //for modulation 2*pi*k/N
        quint32 p = floor(sample_count / 2);      //translation
        GaborAtom *gabor_Atom = new GaborAtom();
        gabor_Atom->sample_count = sample_count;
        gabor_Atom->energy = 0;
        qreal phase = 0;

        while(s < sample_count)
        {
            k = 0;                              //for modulation 2*pi*k/N
            p = floor(sample_count / 2);         //translation
            VectorXd envelope = GaborAtom::gauss_function(sample_count, s, p);
            VectorXcd fft_envelope = RowVectorXcd::Zero(sample_count);
            fft.fwd(fft_envelope, envelope);

            while(k < sample_count/2)
            {
                p = floor(sample_count/2);
                VectorXcd modulation = modulation_function(sample_count, k);
                VectorXcd modulated_resid = VectorXcd::Zero(sample_count);
                VectorXcd fft_modulated_resid = VectorXcd::Zero(sample_count);
                VectorXcd fft_m_e_resid = VectorXcd::Zero(sample_count);
                VectorXd corr_coeffs = VectorXd::Zero(sample_count);

                //iteration for multichannel
                for(qint32 chn = 0; chn < channel_count; chn++)
                {
                    quint32 max_index = 0;
                    qreal maximum = 0;
                    phase = 0;

                    //complex correlation of signal and sinus-modulated gaussfunction
                    for(qint32 l = 0; l< sample_count; l++)
                        modulated_resid[l] = residuum(l, chn) * modulation[l];
                    //modulated_resid = residuum.row(chn) * modulation;

                    fft.fwd(fft_modulated_resid, modulated_resid);

                    for( qint32 m = 0; m < sample_count; m++)
                    fft_m_e_resid[m] = fft_modulated_resid[m] * conj(fft_envelope[m]);
                    //fft_m_e_resid = fft_modulated_resid * conj(fft_envelope);

                    fft.inv(corr_coeffs, fft_m_e_resid);
                    maximum = corr_coeffs[0];

                    //find index of maximum correlation-coefficient to use in translation
                    for(qint32 i = 1; i < corr_coeffs.rows(); i++)
                        if(maximum < corr_coeffs[i])
                        {
                            maximum = corr_coeffs[i];
                            max_index = i;
                        }

                    //adapting translation p to create atomtranslation correctly ToDo: is this correct, think about it again
                    if(max_index >= p) p = max_index - p + 1;
                    else p = max_index + p;

                    VectorXd atom_parameters = calculate_atom(sample_count, s, p, k, chn, residuum, RETURNPARAMETERS);

                    if(abs(atom_parameters[4]) > abs(max_scalar_product)&& p < sample_count && p > 0)
                    {
                        //set highest scalarproduct, in comparison to best matching atom
                        max_scalar_product             = atom_parameters[4];
                        gabor_Atom->scale              = atom_parameters[0];
                        gabor_Atom->translation        = atom_parameters[1];
                        gabor_Atom->modulation         = atom_parameters[2];
                        gabor_Atom->phase              = atom_parameters[3];
                        gabor_Atom->max_scalar_product = max_scalar_product;
                    }

                }
                k += pow(2.0,(-j))*sample_count/2;

            }
            j++;
            s = pow(2.0,j);
        }
        std::cout << "\n" << "===============" << " found parameters " << it + 1 << "===============" << ":\n\n"<<
                     "scale: " << gabor_Atom->scale << " trans: " << gabor_Atom->translation <<
                     " modu: " << gabor_Atom->modulation << " phase: " << gabor_Atom->phase << " sclr_prdct: " << gabor_Atom->max_scalar_product << "\n";

        //replace atoms with s==N and p = floor(N/2) by such atoms that do not have an envelope
        k = 0;
        s = sample_count;
        p = floor(sample_count / 2);
        j = floor(log(sample_count) / log(2));
        phase = 0;

        for(qint32 chn = 0; chn < channel_count; ++chn)
        {
            while(k < sample_count / 2)
            {
                VectorXd parameters_no_envelope = calculate_atom(sample_count, s, p, k, chn, residuum, RETURNPARAMETERS);

                if(abs(parameters_no_envelope[4]) > abs(max_scalar_product) && p < sample_count && p > 0)// && gaborAtom->scale == s && gaborAtom->translation == p)
                {
                    //set highest scalarproduct, in comparison to best matching atom
                    max_scalar_product             = parameters_no_envelope[4];
                    gabor_Atom->scale              = parameters_no_envelope[0];
                    gabor_Atom->translation        = parameters_no_envelope[1];
                    gabor_Atom->modulation         = parameters_no_envelope[2];
                    gabor_Atom->phase              = parameters_no_envelope[3];
                    gabor_Atom->max_scalar_product = max_scalar_product;
                }
                k += pow(2.0,(-j))*sample_count/2;

            }

        }
        std::cout << "      after comparison to NoEnvelope " << ":\n"<< "scale: " << gabor_Atom->scale << " trans: " << gabor_Atom->translation <<
                     " modu: " << gabor_Atom->modulation << " phase: " << gabor_Atom->phase << " sclr_prdct: " << gabor_Atom->max_scalar_product << "\n\n";

        //Maximisation Simplex Algorithm implemented by Bozoa Jia, adapted to the MP Algorithm by Martin Henfling. Copyright (C) 2010 Botao Jia
        //todo change to clean use of EIGEN, @present its mixed with Namespace std and <vector>
        //iteration for multichannel
        for(qint32 chn = 0; chn < channel_count; chn++)
        {
            //simplexfunction to find minimum of target among parameters s, p, k
            std::vector<double> init;

            init.push_back(gabor_Atom->scale);
            init.push_back(gabor_Atom->translation);
            init.push_back(gabor_Atom->modulation);

            double tol = 1E8 * std::numeric_limits<double>::epsilon();
            std::vector<std::vector<double> > x = std::vector<std::vector<double> >();
            qint32 iterations = 1E3;
            qint32 N = init.size();                     //space dimension

            VectorXd atom_fxc_params = VectorXd::Zero(5); //initialisation for contraction coefficients

            const qreal a=1.0, b=0.2, g=0.5, h=0.5;  //coefficients a = 1, b = 0.2, g = 0.5, h = 0.5
                                                     //a: reflection  -> xr step away from worst siplex found
                                                     //b: expansion   -> xe if better with a so go in this direction with b
                                                     //g: contraction -> xc calc new worst point an bring closer to middle of simplex
                                                     //h: full contraction to x1
            std::vector<double> xcentroid_old(N,0);  //simplex center * (N+1)
            std::vector<double> xcentroid_new(N,0);  //simplex center * (N+1)
            std::vector<double> vf(N+1,0);           //f evaluated at simplex vertices
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
                    VectorXd atom_fx = VectorXd::Zero(sample_count);

                    if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                        atom_fx = calculate_atom(sample_count, sample_count, floor(sample_count / 2), x[i][2], chn, residuum, RETURNATOM);

                    else
                        atom_fx = calculate_atom(sample_count, x[i][0], x[i][1], x[i][2], chn, residuum, RETURNATOM);

                    //create targetfunction of realGaborAtom and Residuum
                    double target = 0;
                    for(qint32 k = 0; k < atom_fx.rows(); k++)
                    {
                        target -=atom_fx[k]*residuum(k,0);
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

                VectorXd atom_fxr = VectorXd::Zero(sample_count);

                if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                    atom_fxr = calculate_atom(sample_count, sample_count, floor(sample_count / 2), xr[2], chn, residuum, RETURNATOM);

                else
                    atom_fxr = calculate_atom(sample_count, xr[0], xr[1], xr[2], chn, residuum, RETURNATOM);

                //create targetfunction of realGaborAtom and Residuum
                double fxr = 0;
                for(qint32 k = 0; k < atom_fxr.rows(); k++) fxr -=atom_fxr[k]*residuum(k,chn);//ToDo: old residuum(k,0)

                //double fxr = target;//record function at xr

                if(vf[x1]<=fxr && fxr<=vf[xn]) std::copy(xr.begin(), xr.end(), x[xnp1].begin());

                //expansion:
                else if(fxr<vf[x1])
                {
                    std::vector<double> xe(N,0);

                    for( qint32 i=0; i<N; ++i) xe[i]=xr[i]+b*(xr[i]-xg[i]);

                    VectorXd atom_fxe = VectorXd::Zero(sample_count);

                    if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                        atom_fxe = calculate_atom(sample_count, sample_count, floor(sample_count / 2), xe[2], chn, residuum, RETURNATOM);

                    else
                        atom_fxe = calculate_atom(sample_count, xe[0], xe[1], xe[2], chn, residuum, RETURNATOM);

                    //create targetfunction of realGaborAtom and Residuum
                    double fxe = 0;
                    for(qint32 k = 0; k < atom_fxe.rows(); k++) fxe -=atom_fxe[k]*residuum(k,chn);//ToDo: old residuum(k,0)

                    if( fxe < fxr ) std::copy(xe.begin(), xe.end(), x[xnp1].begin() );
                    else std::copy(xr.begin(), xr.end(), x[xnp1].begin() );
                }//expansion finished,  xe is not used outside the scope

                //contraction:
                else if( fxr > vf[xn] )
                {
                    std::vector<double> xc(N,0);

                    for( qint32 i=0; i<N; ++i)
                        xc[i]=xg[i]+g*(x[xnp1][i]-xg[i]);

                    if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                        atom_fxc_params = AdaptiveMp::calculate_atom(sample_count, sample_count, floor(sample_count / 2), xc[2], chn, residuum, RETURNPARAMETERS);

                    else
                        atom_fxc_params = AdaptiveMp::calculate_atom(sample_count, xc[0], xc[1], xc[2], chn, residuum, RETURNPARAMETERS);

                    VectorXd atom_fxc = gabor_Atom->create_real(gabor_Atom->sample_count, atom_fxc_params[0], atom_fxc_params[1], atom_fxc_params[2], atom_fxc_params[3]);

                    atom_fxc_params[4] = 0;

                    for(qint32 i = 0; i < sample_count; i++)
                        atom_fxc_params[4] += atom_fxc[i] * residuum(i, chn);

                    //create targetfunction of realGaborAtom and Residuum
                    double fxc = 0;

                    for(qint32 k = 0; k < atom_fxc.rows(); k++)
                        fxc -=atom_fxc[k]*residuum(k,chn);//ToDo: old residuum(k,0)

                    if( fxc < vf[xnp1] )
                        std::copy(xc.begin(), xc.end(), x[xnp1].begin() );

                    else
                        for( quint32 i=0; i<x.size(); ++i )
                            if( i!=x1 )
                                for(qint32 j=0; j<N; ++j)
                                    x[i][j] = x[x1][j] + h * ( x[i][j]-x[x1][j] );
                }//contraction finished, xc is not used outside the scope
            }//optimization is finished

            if(gabor_Atom->scale == sample_count && gabor_Atom->translation == floor(sample_count / 2))
                atom_fxc_params = AdaptiveMp::calculate_atom(sample_count, sample_count, floor(sample_count / 2), x[x1][2], chn, residuum, RETURNPARAMETERS);

            else
                atom_fxc_params = AdaptiveMp::calculate_atom(sample_count, x[x1][0], x[x1][1], x[x1][2], chn, residuum, RETURNPARAMETERS);

            if(abs(atom_fxc_params[4]) > abs(max_scalar_product) /*&& atom_fxc_params[0] < sample_count && atom_fxc_params[0] > 0*/ && atom_fxc_params[1] < sample_count && atom_fxc_params[1] > 0)//ToDo: find a way to make the simplex not running out of bounds
            {
                max_scalar_product = atom_fxc_params[4];             //scalarProduct
                gabor_Atom->scale              = atom_fxc_params[0];//scale
                gabor_Atom->translation        = atom_fxc_params[1];//translation
                gabor_Atom->modulation         = atom_fxc_params[2];//phase
                gabor_Atom->phase              = atom_fxc_params[3];
                gabor_Atom->max_scalar_product   = max_scalar_product;
            }

            if(cnt==iterations)//max number of iteration achieves before tol is satisfied
                std::cout<<"Simplex Iteration limit of "<<iterations<<" achieved in channel " << chn << ", result may not be optimal";

        }//end Maximisation for channels Copyright (C) 2010 Botao Jia

        std::cout <<  "      after simplex optimization " << ":\n"<< "scale: " << gabor_Atom->scale << " trans: " << gabor_Atom->translation <<
                      " modu: " << gabor_Atom->modulation << " phase: " << gabor_Atom->phase << " sclr_prdct: " << gabor_Atom->max_scalar_product << "\n\n";

        //calc multichannel parameters phase and max_scalar_product
        for(qint32 chn = 0; chn < channel_count; chn++)
        {
            VectorXd channel_params = calculate_atom(sample_count, gabor_Atom->scale, gabor_Atom->translation, gabor_Atom->modulation, chn, residuum, RETURNPARAMETERS);
            gabor_Atom->phase_list.append(channel_params[3]);
            gabor_Atom->max_scalar_list.append(channel_params[4]);

            //substract best matching Atom from Residuum in each channel
            VectorXd bestMatch = gabor_Atom->create_real(gabor_Atom->sample_count, gabor_Atom->scale, gabor_Atom->translation, gabor_Atom->modulation, gabor_Atom->phase_list.at(chn));

            for(qint32 j = 0; j < gabor_Atom->sample_count; j++)
            {
                residuum(j,chn) -= gabor_Atom->max_scalar_list.at(chn) * bestMatch[j];
                gabor_Atom->energy += (gabor_Atom->max_scalar_list.at(chn) * bestMatch[j]) * (gabor_Atom->max_scalar_list.at(chn) * bestMatch[j]);
            }

            gabor_Atom->residuum = residuum;
        }

        residuum_energy -= gabor_Atom->energy;
        current_energy += gabor_Atom->energy;

        std::cout << "energy of residuum: " << residuum_energy << "\n";

        atom_list.append(*gabor_Atom);

        delete gabor_Atom;
        it++;
        send_result();

    }//end iterations    
    emit finished();
    return atom_list;
}

//*************************************************************************************************************

VectorXcd AdaptiveMp::modulation_function(qint32 N, qreal k)
{
    VectorXcd modulation = VectorXcd::Zero(N);

    for(qint32 n = 0; n < N; n++)
    {
        modulation[n] = std::polar(1 / sqrt(qreal(N)), 2 * PI * k / qreal(N) * qreal(n));
    }
    return modulation;
}

//*************************************************************************************************************

VectorXd AdaptiveMp::calculate_atom(qint32 sample_count, qreal scale, quint32 translation, qreal modulation, qint32 channel, MatrixXd residuum, ReturnValue return_value)
{
    GaborAtom *gabor_Atom = new GaborAtom();
    qreal phase = 0;
    //create complex Gaboratom
    VectorXcd complex_gabor_atom = gabor_Atom->create_complex(sample_count, scale, translation, modulation);

    //calculate Inner Product: preparation to find the parameter phase
    std::complex<double> inner_product(0, 0);

    for(qint32 i = 0; i < sample_count; i++)
        inner_product += residuum(i, channel) * conj(complex_gabor_atom[i]);

    //calculate phase to create realGaborAtoms
    phase = std::arg(inner_product);
    if (phase < 0) phase = 2 * PI - phase;
    VectorXd real_gabor_atom = gabor_Atom->create_real(sample_count, scale, translation, modulation, phase);

    delete gabor_Atom;

    switch(return_value)
    {
        case RETURNPARAMETERS:
        {
            qreal scalar_product = 0;

            for(qint32 i = 0; i < sample_count; i++)
                scalar_product += real_gabor_atom[i] * residuum(i, channel);

            VectorXd atom_parameters = VectorXd::Zero(5);

            atom_parameters[0] = scale;
            atom_parameters[1] = translation;
            atom_parameters[2] = modulation;
            atom_parameters[3] = phase;
            atom_parameters[4] = scalar_product;

            return atom_parameters;
        }
        case RETURNATOM: {return real_gabor_atom;} //returns normalized realGaborAtom
    }
}

//*************************************************************************************************************

void AdaptiveMp::send_result()
{
    emit current_result(it, max_it, current_energy, signal_energy, atom_list);
}

//*************************************************************************************************************

void AdaptiveMp::process()
{
    //recieve_input(signal, max_iterations, epsilon);
    matching_pursuit(signal, max_iterations, epsilon);
}

//*************************************************************************************************************

void AdaptiveMp::recieve_input(Eigen::MatrixXd signal, qint32 max_iterations, qreal epsilon)
{
  matching_pursuit(signal, max_iterations, epsilon);
}

//*************************************************************************************************************

AdaptiveMp::~AdaptiveMp()
{
}
