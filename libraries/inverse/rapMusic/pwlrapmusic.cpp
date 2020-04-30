//=============================================================================================================
/**
 * @file     pwlrapmusic.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of the PwlRapMusic Algorithm Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pwlrapmusic.h"

#ifdef _OPENMP
#include <omp.h>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;
using namespace MNELIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PwlRapMusic::PwlRapMusic()
: RapMusic()
{
}

//=============================================================================================================

PwlRapMusic::PwlRapMusic(MNEForwardSolution& p_pFwd, bool p_bSparsed, int p_iN, double p_dThr)
: RapMusic(p_pFwd, p_bSparsed, p_iN, p_dThr)
{
    //Init
    init(p_pFwd, p_bSparsed, p_iN, p_dThr);
}

//=============================================================================================================

PwlRapMusic::~PwlRapMusic()
{
}

//=============================================================================================================

const char* PwlRapMusic::getName() const
{
    return "Powell RAP MUSIC";
}

//=============================================================================================================

MNESourceEstimate PwlRapMusic::calculateInverse(const FiffEvoked &p_fiffEvoked, bool pick_normal)
{
    return RapMusic::calculateInverse(p_fiffEvoked, pick_normal);
}

//=============================================================================================================

MNESourceEstimate PwlRapMusic::calculateInverse(const MatrixXd &data, float tmin, float tstep) const
{
    return RapMusic::calculateInverse(data, tmin, tstep);
}

//=============================================================================================================

MNESourceEstimate PwlRapMusic::calculateInverse(const MatrixXd& p_matMeasurement, QList< DipolePair<double> > &p_RapDipoles) const
{
    MNESourceEstimate p_SourceEstimate;

    //if not initialized -> break
    if(!m_bIsInit)
    {
        std::cout << "RAP MUSIC wasn't initialized!"; //ToDo: catch this earlier
        return p_SourceEstimate; //false
    }

    //Test if data are correct
    if(p_matMeasurement.rows() != m_iNumChannels)
    {
        std::cout << "Lead Field channels do not fit to number of measurement channels!"; //ToDo: catch this earlier
        return p_SourceEstimate;
    }

    //Inits
    //Stop the time for benchmark purpose
    clock_t start, end;
    start = clock();

//    //Map HPCMatrix to Eigen Matrix
//    Eigen::Map<MatrixXT>
//        t_MappedMatMeasurement(	p_pMatMeasurement->data(),
//        p_pMatMeasurement->rows(),
//        p_pMatMeasurement->cols() );

    //Calculate the signal subspace (t_pMatPhi_s)
    MatrixXT* t_pMatPhi_s = NULL;//(m_iNumChannels, m_iN < t_r ? m_iN : t_r);
    int t_r = calcPhi_s(/*(MatrixXT)*/p_matMeasurement, t_pMatPhi_s);

    int t_iMaxSearch = m_iN < t_r ? m_iN : t_r; //The smallest of Rank and Iterations

    if (t_r < m_iN)
    {
        std::cout << "Warning: Rank " << t_r << " of the measurement data is smaller than the " << m_iN;
        std::cout << " sources to find." << std::endl;
        std::cout << "         Searching now for " << t_iMaxSearch << " correlated sources.";
        std::cout << std::endl << std::endl;
    }

    //Create Orthogonal Projector
    //OrthProj
    MatrixXT t_matOrthProj(m_iNumChannels,m_iNumChannels);
    t_matOrthProj.setIdentity();

    //A_k_1
    MatrixXT t_matA_k_1(m_iNumChannels, t_iMaxSearch);
    t_matA_k_1.setZero();

//    if (m_pMatGrid != NULL)
//    {
//        if(p_pRapDipoles != NULL)
//            p_pRapDipoles->initRapDipoles(m_pMatGrid);
//        else
//            p_pRapDipoles = new RapDipoles<T>(m_pMatGrid);
//    }
//    else
//    {
//        if(p_pRapDipoles != NULL)
//            delete p_pRapDipoles;

//        p_pRapDipoles = new RapDipoles<T>();
//    }
    p_RapDipoles.clear();

    std::cout << "##### Calculation of PWL RAP MUSIC started ######\n\n";

    MatrixXT t_matProj_Phi_s(t_matOrthProj.rows(), t_pMatPhi_s->cols());
    //new Version: Calculate projection before
    MatrixXT t_matProj_LeadField(m_ForwardSolution.sol->data.rows(), m_ForwardSolution.sol->data.cols());

    for(int r = 0; r < t_iMaxSearch ; ++r)
    {
        t_matProj_Phi_s = t_matOrthProj*(*t_pMatPhi_s);

        //new Version: Calculating Projection before
        t_matProj_LeadField = t_matOrthProj * m_ForwardSolution.sol->data;//Subtract the found sources from the current found source

        //###First Option###
        //Step 1: lt. Mosher 1998 -> Maybe tmp_Proj_Phi_S is already orthogonal -> so no SVD needed -> U_B = tmp_Proj_Phi_S;
        Eigen::JacobiSVD< MatrixXT > t_svdProj_Phi_S(t_matProj_Phi_s, Eigen::ComputeThinU);
        MatrixXT t_matU_B;
        useFullRank(t_svdProj_Phi_S.matrixU(), t_svdProj_Phi_S.singularValues().asDiagonal(), t_matU_B);

        //Inits
        VectorXT t_vecRoh(m_iNumLeadFieldCombinations,1);
        t_vecRoh.setZero();

        //subcorr benchmark
        //Stop the time
        clock_t start_subcorr, end_subcorr;
        start_subcorr = clock();

        double t_val_roh_k;

        //Powell
        int t_iCurrentRow = 2;

        int t_iIdx1 = -1;
        int t_iIdx2 = -1;

        int t_iMaxIdx_old = -1;

        int t_iMaxFound = 0;

        Eigen::VectorXi t_pVecIdxElements(m_iNumGridPoints);

        PowellIdxVec(t_iCurrentRow, m_iNumGridPoints, t_pVecIdxElements);

        int t_iNumVecElements = m_iNumGridPoints;

        while(t_iMaxFound == 0)
        {

            //Multithreading correlation calculation
            #ifdef _OPENMP
            #pragma omp parallel num_threads(m_iMaxNumThreads)
            #endif
            {
            #ifdef _OPENMP
            #pragma omp for
            #endif
                for(int i = 0; i < t_iNumVecElements; i++)
                {
                    int k = t_pVecIdxElements(i);
                    //new Version: calculate matrix multiplication before
                    //Create Lead Field combinations -> It would be better to use a pointer construction, to increase performance
                    MatrixX6T t_matProj_G(t_matProj_LeadField.rows(),6);

                    int idx1 = m_ppPairIdxCombinations[k]->x1;
                    int idx2 = m_ppPairIdxCombinations[k]->x2;

                    RapMusic::getGainMatrixPair(t_matProj_LeadField, t_matProj_G, idx1, idx2);

                    t_vecRoh(k) = RapMusic::subcorr(t_matProj_G, t_matU_B);//t_vecRoh holds the correlations roh_k
                }
            }

    //         if(r==0)
    //         {
    //             std::fstream filestr;
    //             std::stringstream filename;
    //             filename << "Roh_gold.txt";
    //
    //             filestr.open ( filename.str().c_str(), std::fstream::out);
    //             for(int i = 0; i < m_iNumLeadFieldCombinations; ++i)
    //             {
    //               filestr << t_vecRoh(i) << "\n";
    //             }
    //             filestr.close();
    //
    //             //exit(0);
    //         }

            //Find the maximum of correlation - can't put this in the for loop because it's running in different threads.

            VectorXT::Index t_iMaxIdx;

            t_val_roh_k = t_vecRoh.maxCoeff(&t_iMaxIdx);//p_vecCor = ^roh_k

            if((int)t_iMaxIdx == t_iMaxIdx_old)
            {
                t_iMaxFound = 1;
                break;
            }
            else
            {
                t_iMaxIdx_old = t_iMaxIdx;
                //get positions in sparsed leadfield from index combinations;
                t_iIdx1 = m_ppPairIdxCombinations[t_iMaxIdx]->x1;
                t_iIdx2 = m_ppPairIdxCombinations[t_iMaxIdx]->x2;
            }

            //set new index
            if(t_iIdx1 == t_iCurrentRow)
                t_iCurrentRow = t_iIdx2;
            else
                t_iCurrentRow = t_iIdx1;

            PowellIdxVec(t_iCurrentRow, m_iNumGridPoints, t_pVecIdxElements);
        }

        //subcorr benchmark
        end_subcorr = clock();

        float t_fSubcorrElapsedTime = ( (float)(end_subcorr-start_subcorr) / (float)CLOCKS_PER_SEC ) * 1000.0f;
        std::cout << "Time Elapsed: " << t_fSubcorrElapsedTime << " ms" << std::endl;

        // (Idx+1) because of MATLAB positions -> starting with 1 not with 0
        std::cout << "Iteration: " << r+1 << " of " << t_iMaxSearch
            << "; Correlation: " << t_val_roh_k<< "; Position (Idx+1): " << t_iIdx1+1 << " - " << t_iIdx2+1 <<"\n\n";

        //Calculations with the max correlated dipole pair G_k_1
        MatrixX6T t_matG_k_1(m_ForwardSolution.sol->data.rows(),6);
        RapMusic::getGainMatrixPair(m_ForwardSolution.sol->data, t_matG_k_1, t_iIdx1, t_iIdx2);

        MatrixX6T t_matProj_G_k_1(t_matOrthProj.rows(), t_matG_k_1.cols());
        t_matProj_G_k_1 = t_matOrthProj * t_matG_k_1;//Subtract the found sources from the current found source
//         MatrixX6T t_matProj_G_k_1(t_matProj_LeadField.rows(), 6);
//         getLeadFieldPair(t_matProj_LeadField, t_matProj_G_k_1, t_iIdx1, t_iIdx2);

        //Calculate source direction
        //source direction (p_pMatPhi) for current source r (phi_k_1)
        Vector6T t_vec_phi_k_1(6, 1);
        RapMusic::subcorr(t_matProj_G_k_1, t_matU_B, t_vec_phi_k_1);//Correlate the current source to calculate the direction

        //Set return values
        RapMusic::insertSource(t_iIdx1, t_iIdx2, t_vec_phi_k_1, t_val_roh_k, p_RapDipoles);

        //Stop Searching when Correlation is smaller then the Threshold
        if (t_val_roh_k < m_dThreshold)
        {
            std::cout << "Searching stopped, last correlation " << t_val_roh_k;
            std::cout << " is smaller then the given threshold " << m_dThreshold << std::endl << std::endl;
            break;
        }

        //Calculate A_k_1 = [a_theta_1..a_theta_k_1] matrix for subtraction of found source
        RapMusic::calcA_k_1(t_matG_k_1, t_vec_phi_k_1, r, t_matA_k_1);

        //Calculate new orthogonal Projector (Pi_k_1)
        calcOrthProj(t_matA_k_1, t_matOrthProj);

        //garbage collecting
        //ToDo
    }

    std::cout << "##### Calculation of PWL RAP MUSIC completed ######"<< std::endl << std::endl << std::endl;

    end = clock();

    float t_fElapsedTime = ( (float)(end-start) / (float)CLOCKS_PER_SEC ) * 1000.0f;
    std::cout << "Total Time Elapsed: " << t_fElapsedTime << " ms" << std::endl << std::endl;

    //garbage collecting
    delete t_pMatPhi_s;

    return p_SourceEstimate;
}

//=============================================================================================================

int PwlRapMusic::PowellOffset(int p_iRow, int p_iNumPoints)
{

    return p_iRow*p_iNumPoints - (( (p_iRow-1)*p_iRow) / 2); //triangular series 1 3 6 10 ... = (num_pairs*(num_pairs+1))/2
}

//=============================================================================================================

void PwlRapMusic::PowellIdxVec(int p_iRow, int p_iNumPoints, Eigen::VectorXi& p_pVecElements)
{

    //     if(p_pVecElements != NULL)
    //         delete[] p_pVecElements;
    //
    //     p_pVecElements = new int(p_iNumPoints);

    //col combination index
    for(int i = 0; i <= p_iRow; ++i)//=p_iNumPoints-1
        p_pVecElements(i) = PwlRapMusic::PowellOffset(i+1,p_iNumPoints)-(p_iNumPoints-p_iRow);

    //row combination index
    int off = PwlRapMusic::PowellOffset(p_iRow,p_iNumPoints);
    int length = p_iNumPoints - p_iRow;
    int k=0;
    for(int i = p_iRow; i < p_iRow+length; ++i)//=p_iNumPoints-1
    {
        p_pVecElements(i) = off+k;
        k = k + 1;
    }
}
