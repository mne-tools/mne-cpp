//=============================================================================================================
/**
 * @file     rapmusic.cpp
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
 * @brief    Definition of the RapMusic Algorithm Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rapmusic.h"

#include <utils/mnemath.h>

#ifdef _OPENMP
#include <omp.h>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;
using namespace MNELIB;
using namespace FIFFLIB;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RapMusic::RapMusic()
: m_iN(0)
, m_dThreshold(0)
, m_iNumGridPoints(0)
, m_iNumChannels(0)
, m_iNumLeadFieldCombinations(0)
, m_ppPairIdxCombinations(NULL)
, m_iMaxNumThreads(1)
, m_bIsInit(false)
, m_iSamplesStcWindow(-1)
, m_fStcOverlap(-1)
{
}

//=============================================================================================================

RapMusic::RapMusic(MNEForwardSolution& p_pFwd, bool p_bSparsed, int p_iN, double p_dThr)
: m_iN(0)
, m_dThreshold(0)
, m_iNumGridPoints(0)
, m_iNumChannels(0)
, m_iNumLeadFieldCombinations(0)
, m_ppPairIdxCombinations(NULL)
, m_iMaxNumThreads(1)
, m_bIsInit(false)
, m_iSamplesStcWindow(-1)
, m_fStcOverlap(-1)
{
    //Init
    init(p_pFwd, p_bSparsed, p_iN, p_dThr);
}

//=============================================================================================================

RapMusic::~RapMusic()
{
    if(m_ppPairIdxCombinations != NULL)
        free(m_ppPairIdxCombinations);
}

//=============================================================================================================

bool RapMusic::init(MNEForwardSolution& p_pFwd, bool p_bSparsed, int p_iN, double p_dThr)
{
    //Get available thread number
    #ifdef _OPENMP
        std::cout << "OpenMP enabled" << std::endl;
        m_iMaxNumThreads = omp_get_max_threads();
    #else
        std::cout << "OpenMP disabled (to enable it: VS2010->Project Properties->C/C++->Language, then modify OpenMP Support)" << std::endl;
        m_iMaxNumThreads = 1;
    #endif
        std::cout << "Available Threats: " << m_iMaxNumThreads << std::endl << std::endl;

    //Initialize RAP MUSIC
    std::cout << "##### Initialization RAP MUSIC started ######\n\n";

    m_iN = p_iN;
    m_dThreshold = p_dThr;

//    //Grid check
//    if(p_pMatGrid != NULL)
//    {
//        if ( p_pMatGrid->rows() != p_pMatLeadField->cols() / 3 )
//        {
//            std::cout << "Grid does not fit to given Lead Field!\n";
//            return false;
//        }
//    }

//    m_pMatGrid = p_pMatGrid;

    //Lead Field check
    if ( p_pFwd.sol->data.cols() % 3 != 0 )
    {
        std::cout << "Gain matrix is not associated with a 3D grid!\n";
        return false;
    }

    m_iNumGridPoints = p_pFwd.sol->data.cols()/3;

    m_iNumChannels = p_pFwd.sol->data.rows();

//    m_pMappedMatLeadField = new Eigen::Map<MatrixXT>
//        (   p_pMatLeadField->data(),
//            p_pMatLeadField->rows(),
//            p_pMatLeadField->cols() );

    m_ForwardSolution = p_pFwd;

    //##### Calc lead field combination #####

    std::cout << "Calculate gain matrix combinations. \n";

    m_iNumLeadFieldCombinations = MNEMath::nchoose2(m_iNumGridPoints+1);

    m_ppPairIdxCombinations = (Pair **)malloc(m_iNumLeadFieldCombinations * sizeof(Pair *));

    calcPairCombinations(m_iNumGridPoints, m_iNumLeadFieldCombinations, m_ppPairIdxCombinations);

    std::cout << "Gain matrix combinations calculated. \n\n";

    //##### Calc lead field combination end #####

    std::cout << "Number of grid points: " << m_iNumGridPoints << "\n\n";

    std::cout << "Number of combinated points: " << m_iNumLeadFieldCombinations << "\n\n";

    std::cout << "Number of sources to find: " << m_iN << "\n\n";

    std::cout << "Threshold: " << m_dThreshold << "\n\n";

    //Init end

    std::cout << "##### Initialization RAP MUSIC completed ######\n\n\n";

    Q_UNUSED(p_bSparsed);

    m_bIsInit = true;

    return m_bIsInit;
}

//=============================================================================================================

const char* RapMusic::getName() const
{
    return "RAP MUSIC";
}

//=============================================================================================================

const MNESourceSpace& RapMusic::getSourceSpace() const
{
    return m_ForwardSolution.src;
}

//=============================================================================================================

MNESourceEstimate RapMusic::calculateInverse(const FiffEvoked &p_fiffEvoked, bool pick_normal)
{
    Q_UNUSED(pick_normal);

    MNESourceEstimate p_sourceEstimate;

    if(p_fiffEvoked.data.rows() != m_iNumChannels)
    {
        std::cout << "Number of FiffEvoked channels (" << p_fiffEvoked.data.rows() << ") doesn't match the number of channels (" << m_iNumChannels << ") of the forward solution." << std::endl;
        return p_sourceEstimate;
    }
//    else
//        std::cout << "Number of FiffEvoked channels (" << p_fiffEvoked.data.rows() << ") matchs the number of channels (" << m_iNumChannels << ") of the forward solution." << std::endl;

    //
    // Rap MUSIC Source estimate
    //
    p_sourceEstimate.data = MatrixXd::Zero(m_ForwardSolution.nsource, p_fiffEvoked.data.cols());

    //Results
    p_sourceEstimate.vertices = VectorXi(m_ForwardSolution.src[0].vertno.size() + m_ForwardSolution.src[1].vertno.size());
    p_sourceEstimate.vertices << m_ForwardSolution.src[0].vertno, m_ForwardSolution.src[1].vertno;

    p_sourceEstimate.times = p_fiffEvoked.times;
    p_sourceEstimate.tmin = p_fiffEvoked.times[0];
    p_sourceEstimate.tstep = p_fiffEvoked.times[1] - p_fiffEvoked.times[0];

    if(m_iSamplesStcWindow <= 3) //if samples per stc aren't set -> use full window
    {
        QList< DipolePair<double> > t_RapDipoles;
        calculateInverse(p_fiffEvoked.data, t_RapDipoles);

        for(qint32 i = 0; i < t_RapDipoles.size(); ++i)
        {
            double dip1 = sqrt( pow(t_RapDipoles[i].m_Dipole1.phi_x(),2) +
                                pow(t_RapDipoles[i].m_Dipole1.phi_y(),2) +
                                pow(t_RapDipoles[i].m_Dipole1.phi_z(),2) ) * t_RapDipoles[i].m_vCorrelation;

            double dip2 = sqrt( pow(t_RapDipoles[i].m_Dipole2.phi_x(),2) +
                                pow(t_RapDipoles[i].m_Dipole2.phi_y(),2) +
                                pow(t_RapDipoles[i].m_Dipole2.phi_z(),2) ) * t_RapDipoles[i].m_vCorrelation;

            RowVectorXd dip1Time = RowVectorXd::Constant(p_fiffEvoked.data.cols(), dip1);
            RowVectorXd dip2Time = RowVectorXd::Constant(p_fiffEvoked.data.cols(), dip2);

            p_sourceEstimate.data.block(t_RapDipoles[i].m_iIdx1, 0, 1, p_fiffEvoked.data.cols()) = dip1Time;
            p_sourceEstimate.data.block(t_RapDipoles[i].m_iIdx2, 0, 1, p_fiffEvoked.data.cols()) = dip2Time;
        }
    }
    else
    {
        bool first = true;
        bool last = false;

        qint32 t_iNumSensors = p_fiffEvoked.data.rows();
        qint32 t_iNumSteps = p_fiffEvoked.data.cols();

        qint32 t_iSamplesOverlap = (qint32)floor(((float)m_iSamplesStcWindow)*m_fStcOverlap);
        qint32 t_iSamplesDiscard = t_iSamplesOverlap/2;

        MatrixXd data = MatrixXd::Zero(t_iNumSensors, m_iSamplesStcWindow);

        qint32 curSample = 0;
        qint32 curResultSample = 0;
        qint32 stcWindowSize = m_iSamplesStcWindow - 2*t_iSamplesDiscard;

        while(!last)
        {
            QList< DipolePair<double> > t_RapDipoles;

            //Data
            if(curSample + m_iSamplesStcWindow >= t_iNumSteps) //last
            {
                last = true;
                data = p_fiffEvoked.data.block(0, p_fiffEvoked.data.cols()-m_iSamplesStcWindow, t_iNumSensors, m_iSamplesStcWindow);
            }
            else
                data = p_fiffEvoked.data.block(0, curSample, t_iNumSensors, m_iSamplesStcWindow);

            curSample += (m_iSamplesStcWindow - t_iSamplesOverlap);
            if(first)
                curSample -= t_iSamplesDiscard; //shift on start t_iSamplesDiscard backwards

            //Calculate
            calculateInverse(data, t_RapDipoles);

            //Assign Result
            if(last)
                stcWindowSize = p_sourceEstimate.data.cols() - curResultSample;

            for(qint32 i = 0; i < t_RapDipoles.size(); ++i)
            {
                double dip1 = sqrt( pow(t_RapDipoles[i].m_Dipole1.phi_x(),2) +
                                    pow(t_RapDipoles[i].m_Dipole1.phi_y(),2) +
                                    pow(t_RapDipoles[i].m_Dipole1.phi_z(),2) ) * t_RapDipoles[i].m_vCorrelation;

                double dip2 = sqrt( pow(t_RapDipoles[i].m_Dipole2.phi_x(),2) +
                                    pow(t_RapDipoles[i].m_Dipole2.phi_y(),2) +
                                    pow(t_RapDipoles[i].m_Dipole2.phi_z(),2) ) * t_RapDipoles[i].m_vCorrelation;

                RowVectorXd dip1Time = RowVectorXd::Constant(stcWindowSize, dip1);
                RowVectorXd dip2Time = RowVectorXd::Constant(stcWindowSize, dip2);

                p_sourceEstimate.data.block(t_RapDipoles[i].m_iIdx1, curResultSample, 1, stcWindowSize) = dip1Time;
                p_sourceEstimate.data.block(t_RapDipoles[i].m_iIdx2, curResultSample, 1, stcWindowSize) = dip2Time;

            }

            curResultSample += stcWindowSize;

            if(first)
                first = false;
        }
    }

    return p_sourceEstimate;
}

//=============================================================================================================

MNESourceEstimate RapMusic::calculateInverse(const MatrixXd &data, float tmin, float tstep, bool pick_normal) const
{
    Q_UNUSED(pick_normal);

    MNESourceEstimate p_sourceEstimate;

    if(data.rows() != m_iNumChannels)
    {
        std::cout << "Number of FiffEvoked channels (" << data.rows() << ") doesn't match the number of channels (" << m_iNumChannels << ") of the forward solution." << std::endl;
        return p_sourceEstimate;
    }
//    else
//        std::cout << "Number of FiffEvoked channels (" << data.rows() << ") matchs the number of channels (" << m_iNumChannels << ") of the forward solution." << std::endl;

    //
    // Rap MUSIC Source estimate
    //
    p_sourceEstimate.data = MatrixXd::Zero(m_ForwardSolution.nsource, data.cols());

    //Results
    p_sourceEstimate.vertices = VectorXi(m_ForwardSolution.src[0].vertno.size() + m_ForwardSolution.src[1].vertno.size());
    p_sourceEstimate.vertices << m_ForwardSolution.src[0].vertno, m_ForwardSolution.src[1].vertno;

    p_sourceEstimate.times = RowVectorXf::Zero(data.cols());
    p_sourceEstimate.times[0] = tmin;
    for(qint32 i = 1; i < p_sourceEstimate.times.size(); ++i)
        p_sourceEstimate.times[i] = p_sourceEstimate.times[i-1] + tstep;
    p_sourceEstimate.tmin = tmin;
    p_sourceEstimate.tstep = tstep;

    QList< DipolePair<double> > t_RapDipoles;
    calculateInverse(data, t_RapDipoles);

    for(qint32 i = 0; i < t_RapDipoles.size(); ++i)
    {
        double dip1 = sqrt( pow(t_RapDipoles[i].m_Dipole1.phi_x(),2) +
                            pow(t_RapDipoles[i].m_Dipole1.phi_y(),2) +
                            pow(t_RapDipoles[i].m_Dipole1.phi_z(),2) ) * t_RapDipoles[i].m_vCorrelation;

        double dip2 = sqrt( pow(t_RapDipoles[i].m_Dipole2.phi_x(),2) +
                            pow(t_RapDipoles[i].m_Dipole2.phi_y(),2) +
                            pow(t_RapDipoles[i].m_Dipole2.phi_z(),2) ) * t_RapDipoles[i].m_vCorrelation;

        RowVectorXd dip1Time = RowVectorXd::Constant(data.cols(), dip1);
        RowVectorXd dip2Time = RowVectorXd::Constant(data.cols(), dip2);

        p_sourceEstimate.data.block(t_RapDipoles[i].m_iIdx1, 0, 1, data.cols()) = dip1Time;
        p_sourceEstimate.data.block(t_RapDipoles[i].m_iIdx2, 0, 1, data.cols()) = dip2Time;
    }

    return p_sourceEstimate;
}

//=============================================================================================================

MNESourceEstimate RapMusic::calculateInverse(const MatrixXd& p_matMeasurement, QList< DipolePair<double> > &p_RapDipoles) const
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

    std::cout << "##### Calculation of RAP MUSIC started ######\n\n";

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

        //Multithreading correlation calculation
        #ifdef _OPENMP
        #pragma omp parallel num_threads(m_iMaxNumThreads)
        #endif
        {
        #ifdef _OPENMP
        #pragma omp for
        #endif
            for(int i = 0; i < m_iNumLeadFieldCombinations; i++)
            {
                //new Version: calculate matrix multiplication before
                //Create Lead Field combinations -> It would be better to use a pointer construction, to increase performance
                MatrixX6T t_matProj_G(t_matProj_LeadField.rows(),6);

                int idx1 = m_ppPairIdxCombinations[i]->x1;
                int idx2 = m_ppPairIdxCombinations[i]->x2;

                RapMusic::getGainMatrixPair(t_matProj_LeadField, t_matProj_G, idx1, idx2);

                t_vecRoh(i) = RapMusic::subcorr(t_matProj_G, t_matU_B);//t_vecRoh holds the correlations roh_k
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

        //subcorr benchmark
        end_subcorr = clock();

        float t_fSubcorrElapsedTime = ( (float)(end_subcorr-start_subcorr) / (float)CLOCKS_PER_SEC ) * 1000.0f;
        std::cout << "Time Elapsed: " << t_fSubcorrElapsedTime << " ms" << std::endl;

        //Find the maximum of correlation - can't put this in the for loop because it's running in different threads.
        double t_val_roh_k;

        VectorXT::Index t_iMaxIdx;

        t_val_roh_k = t_vecRoh.maxCoeff(&t_iMaxIdx);//p_vecCor = ^roh_k

        //get positions in sparsed leadfield from index combinations;
        int t_iIdx1 = m_ppPairIdxCombinations[t_iMaxIdx]->x1;
        int t_iIdx2 = m_ppPairIdxCombinations[t_iMaxIdx]->x2;

        // (Idx+1) because of MATLAB positions -> starting with 1 not with 0
        std::cout << "Iteration: " << r+1 << " of " << t_iMaxSearch
            << "; Correlation: " << t_val_roh_k<< "; Position (Idx+1): " << t_iIdx1+1 << " - " << t_iIdx2+1 <<"\n\n";

        //Calculations with the max correlated dipole pair G_k_1 -> ToDo Obsolet when taking direkt Projected Lead Field
        MatrixX6T t_matG_k_1(m_ForwardSolution.sol->data.rows(),6);
        RapMusic::getGainMatrixPair(m_ForwardSolution.sol->data, t_matG_k_1, t_iIdx1, t_iIdx2);

        MatrixX6T t_matProj_G_k_1(t_matOrthProj.rows(), t_matG_k_1.cols());
        t_matProj_G_k_1 = t_matOrthProj * t_matG_k_1;//Subtract the found sources from the current found source
//         MatrixX6T t_matProj_G_k_1(t_matProj_LeadField.rows(), 6);
//         getLeadFieldPair(t_matProj_LeadField, t_matProj_G_k_1, t_iIdx1, t_iIdx2);

        //Calculate source direction
        //source direction (p_pMatPhi) for current source r (phi_k_1)
        Vector6T t_vec_phi_k_1(6);
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

    std::cout << "##### Calculation of RAP MUSIC completed ######"<< std::endl << std::endl << std::endl;

    end = clock();

    float t_fElapsedTime = ( (float)(end-start) / (float)CLOCKS_PER_SEC ) * 1000.0f;
    std::cout << "Total Time Elapsed: " << t_fElapsedTime << " ms" << std::endl << std::endl;

    //garbage collecting
    delete t_pMatPhi_s;

    return p_SourceEstimate;
}

//=============================================================================================================

int RapMusic::calcPhi_s(const MatrixXT& p_matMeasurement, MatrixXT* &p_pMatPhi_s) const
{
    //Calculate p_pMatPhi_s
    MatrixXT t_matF;//t_matF = makeSquareMat(p_pMatMeasurement); //FF^T -> ToDo Check this
    if (p_matMeasurement.cols() > p_matMeasurement.rows())
        t_matF = makeSquareMat(p_matMeasurement); //FF^T
    else
        t_matF = MatrixXT(p_matMeasurement);

    Eigen::JacobiSVD<MatrixXT> t_svdF(t_matF, Eigen::ComputeThinU);

    int t_r = getRank(t_svdF.singularValues().asDiagonal());

    int t_iCols = t_r;//t_r < m_iN ? m_iN : t_r;

    if (p_pMatPhi_s != NULL)
        delete p_pMatPhi_s;

    //m_iNumChannels has to be equal to t_svdF.matrixU().rows()
    p_pMatPhi_s = new MatrixXT(m_iNumChannels, t_iCols);

    //assign the signal subspace
    memcpy(p_pMatPhi_s->data(), t_svdF.matrixU().data(), sizeof(double) * m_iNumChannels * t_iCols);

    return t_r;
}

//=============================================================================================================

double RapMusic::subcorr(MatrixX6T& p_matProj_G, const MatrixXT& p_matU_B)
{
    //Orthogonalisierungstest wegen performance weggelassen -> ohne is es viel schneller

    Matrix6T t_matSigma_A(6, 6);
    Matrix6XT t_matU_A_T(6, p_matProj_G.rows()); //rows and cols are changed, because of CV_SVD_U_T

    Eigen::JacobiSVD<MatrixXT> t_svdProj_G(p_matProj_G, Eigen::ComputeThinU);

    t_matSigma_A = t_svdProj_G.singularValues().asDiagonal();
    t_matU_A_T = t_svdProj_G.matrixU().transpose();

    //lt. Mosher 1998 ToDo: Only Retain those Components of U_A and U_B that correspond to nonzero singular values
    //for U_A and U_B the number of columns corresponds to their ranks
    MatrixXT t_matU_A_T_full;
    //reduce to rank only when directions aren't calculated, otherwise use the full t_matU_A_T

    useFullRank(t_matU_A_T, t_matSigma_A, t_matU_A_T_full, IS_TRANSPOSED);//lt. Mosher 1998: Only Retain those Components of U_A that correspond to nonzero singular values -> for U_A the number of columns corresponds to their ranks

    MatrixXT t_matCor(t_matU_A_T_full.rows(), p_matU_B.cols());

    //Step 2: compute the subspace correlation
    t_matCor = t_matU_A_T_full*p_matU_B;//lt. Mosher 1998: C = U_A^T * U_B

    VectorXT t_vecSigma_C;

    if (t_matCor.cols() > t_matCor.rows())
    {
        MatrixXT t_matCor_H(t_matCor.cols(), t_matCor.rows());
        t_matCor_H = t_matCor.adjoint(); //for complex it has to be adjunct
        //ToDo -> use instead adjointInPlace

        Eigen::JacobiSVD<MatrixXT> t_svdCor_H(t_matCor_H);

        t_vecSigma_C = t_svdCor_H.singularValues();
    }
    else
    {
        Eigen::JacobiSVD<MatrixXT> t_svdCor(t_matCor);

        t_vecSigma_C = t_svdCor.singularValues();
    }

    //Step 3
    double t_dRetSigma_C;
    t_dRetSigma_C = t_vecSigma_C(0); //Take only the correlation of the first principal components

    //garbage collecting
    //ToDo

    return t_dRetSigma_C;
}

//=============================================================================================================

double RapMusic::subcorr(MatrixX6T& p_matProj_G, const MatrixXT& p_matU_B, Vector6T& p_vec_phi_k_1)
{
    //Orthogonalisierungstest wegen performance weggelassen -> ohne is es viel schneller

    Matrix6T sigma_A(6, 6);
    Matrix6XT U_A_T(6, p_matProj_G.rows()); //rows and cols are changed, because of CV_SVD_U_T
    Matrix6T V_A(6, 6);

    Eigen::JacobiSVD<MatrixXT> svdOfProj_G(p_matProj_G, Eigen::ComputeThinU | Eigen::ComputeThinV);

    sigma_A = svdOfProj_G.singularValues().asDiagonal();
    U_A_T = svdOfProj_G.matrixU().transpose();
    V_A = svdOfProj_G.matrixV();

    //lt. Mosher 1998 ToDo: Only Retain those Components of U_A and U_B that correspond to nonzero singular values
    //for U_A and U_B the number of columns corresponds to their ranks
    //-> reduce to rank only when directions aren't calculated, otherwise use the full U_A_T

    Matrix6XT t_matCor(6, p_matU_B.cols());

    //Step 2: compute the subspace correlation
    t_matCor = U_A_T*p_matU_B;//lt. Mosher 1998: C = U_A^T * U_B

    VectorXT sigma_C;

    //Step 4
    Matrix6XT U_C;

    if (t_matCor.cols() > t_matCor.rows())
    {
        MatrixX6T Cor_H(t_matCor.cols(), 6);
        Cor_H = t_matCor.adjoint(); //for complex it has to be adjunct

        Eigen::JacobiSVD<MatrixXT> svdOfCor_H(Cor_H, Eigen::ComputeThinV);

        U_C = svdOfCor_H.matrixV(); //because t_matCor Hermitesch U and V are exchanged
        sigma_C = svdOfCor_H.singularValues();
    }
    else
    {
        Eigen::JacobiSVD<MatrixXT> svdOfCor(t_matCor, Eigen::ComputeThinU);

        U_C = svdOfCor.matrixU();
        sigma_C = svdOfCor.singularValues();
    }

    Matrix6T sigma_a_inv;
    sigma_a_inv = sigma_A.inverse();

    Matrix6XT X;
    X = (V_A*sigma_a_inv)*U_C;//X = V_A*Sigma_A^-1*U_C

    Vector6T X_max;//only for the maximum c - so instead of X->cols use 1
    X_max = X.col(0);

    double norm_X = 1/(X_max.norm());

    //Multiply a scalar with an Array -> linear transform
    p_vec_phi_k_1 = X_max*norm_X;//u1 = x1/||x1|| this is the orientation

    //garbage collecting
    //ToDo

    //Step 3
    double ret_sigma_C;
    ret_sigma_C = sigma_C(0); //Take only the correlation of the first principal components

    //garbage collecting
    //ToDo

    return ret_sigma_C;
}

//=============================================================================================================

void RapMusic::calcA_k_1(   const MatrixX6T& p_matG_k_1,
                            const Vector6T& p_matPhi_k_1,
                            const int p_iIdxk_1,
                            MatrixXT& p_matA_k_1)
{
    //Calculate A_k_1 = [a_theta_1..a_theta_k_1] matrix for subtraction of found source
    VectorXT t_vec_a_theta_k_1(p_matG_k_1.rows(),1);

    t_vec_a_theta_k_1 = p_matG_k_1*p_matPhi_k_1; // a_theta_k_1 = G_k_1*phi_k_1   this corresponds to the normalized signal component in subspace r

    p_matA_k_1.block(0,p_iIdxk_1,p_matA_k_1.rows(),1) = t_vec_a_theta_k_1;
}

//=============================================================================================================

void RapMusic::calcOrthProj(const MatrixXT& p_matA_k_1, MatrixXT& p_matOrthProj) const
{
    //Calculate OrthProj=I-A_k_1*(A_k_1'*A_k_1)^-1*A_k_1' //Wetterling -> A_k_1 = Gain

    MatrixXT t_matA_k_1_tmp(p_matA_k_1.cols(), p_matA_k_1.cols());
    t_matA_k_1_tmp = p_matA_k_1.adjoint()*p_matA_k_1;//A_k_1'*A_k_1 = A_k_1_tmp -> A_k_1' has to be adjoint for complex

    int t_size = t_matA_k_1_tmp.cols();

    while (!t_matA_k_1_tmp.block(0,0,t_size,t_size).fullPivLu().isInvertible())
    {
        --t_size;
    }

    MatrixXT t_matA_k_1_tmp_inv(t_matA_k_1_tmp.rows(), t_matA_k_1_tmp.cols());
    t_matA_k_1_tmp_inv.setZero();

    t_matA_k_1_tmp_inv.block(0,0,t_size,t_size) = t_matA_k_1_tmp.block(0,0,t_size,t_size).inverse();//(A_k_1_tmp)^-1 = A_k_1_tmp_inv

    t_matA_k_1_tmp = MatrixXT::Zero(p_matA_k_1.rows(), p_matA_k_1.cols());

    t_matA_k_1_tmp = p_matA_k_1*t_matA_k_1_tmp_inv;//(A_k_1*A_k_1_tmp_inv) = A_k_1_tmp

    MatrixXT t_matA_k_1_tmp2(p_matA_k_1.rows(), p_matA_k_1.rows());

    t_matA_k_1_tmp2 = t_matA_k_1_tmp*p_matA_k_1.adjoint();//(A_k_1_tmp)*A_k_1' -> here A_k_1' is only transposed - it has to be adjoint

    MatrixXT I(m_iNumChannels,m_iNumChannels);
    I.setIdentity();

    p_matOrthProj = I-t_matA_k_1_tmp2; //OrthProj=I-A_k_1*(A_k_1'*A_k_1)^-1*A_k_1';

    //garbage collecting
    //ToDo
}

//=============================================================================================================

void RapMusic::calcPairCombinations(    const int p_iNumPoints,
                                        const int p_iNumCombinations,
                                        Pair** p_ppPairIdxCombinations) const
{
    int idx1 = 0;
    int idx2 = 0;

    //Process Code in {m_max_num_threads} threads -> When compile with Intel Compiler -> probably obsolete
    #ifdef _OPENMP
    #pragma omp parallel num_threads(m_iMaxNumThreads) private(idx1, idx2)
    #endif
    {
    #ifdef _OPENMP
    #pragma omp for
    #endif
        for (int i = 0; i < p_iNumCombinations; ++i)
        {
            RapMusic::getPointPair(p_iNumPoints, i, idx1, idx2);

            Pair* t_pairCombination = new Pair();
            t_pairCombination->x1 = idx1;
            t_pairCombination->x2 = idx2;

            p_ppPairIdxCombinations[i] = t_pairCombination;
        }
    }
}

//=============================================================================================================

void RapMusic::getPointPair(const int p_iPoints, const int p_iCurIdx, int &p_iIdx1, int &p_iIdx2)
{
    int ii = p_iPoints*(p_iPoints+1)/2-1-p_iCurIdx;
    int K = (int)floor((sqrt((double)(8*ii+1))-1)/2);

    p_iIdx1 = p_iPoints-1-K;

    p_iIdx2 = (p_iCurIdx-p_iPoints*(p_iPoints+1)/2 + (K+1)*(K+2)/2)+p_iIdx1;
}

//=============================================================================================================
//ToDo don't make a real copy
void RapMusic::getGainMatrixPair(   const MatrixXT& p_matGainMarix,
                                    MatrixX6T& p_matGainMarix_Pair,
                                    int p_iIdx1, int p_iIdx2)
{
    p_matGainMarix_Pair.block(0,0,p_matGainMarix.rows(),3) = p_matGainMarix.block(0, p_iIdx1*3, p_matGainMarix.rows(), 3);

    p_matGainMarix_Pair.block(0,3,p_matGainMarix.rows(),3) = p_matGainMarix.block(0, p_iIdx2*3, p_matGainMarix.rows(), 3);
}

//=============================================================================================================

void RapMusic::insertSource(    int p_iDipoleIdx1, int p_iDipoleIdx2,
                                const Vector6T &p_vec_phi_k_1,
                                double p_valCor,
                                QList< DipolePair<double> > &p_RapDipoles)
{
    DipolePair<double> t_pRapDipolePair;

    t_pRapDipolePair.m_iIdx1 = p_iDipoleIdx1; //p_iDipoleIdx1+1 because of MATLAB index
    t_pRapDipolePair.m_iIdx2 = p_iDipoleIdx2;

    t_pRapDipolePair.m_Dipole1.x() = 0; //m_bGridInitialized ? (*m_pMatGrid)(p_iDipoleIdx1, 0) : 0;
    t_pRapDipolePair.m_Dipole1.y() = 0; //m_bGridInitialized ? (*m_pMatGrid)(p_iDipoleIdx1, 1) : 0;
    t_pRapDipolePair.m_Dipole1.z() = 0; //m_bGridInitialized ? (*m_pMatGrid)(p_iDipoleIdx1, 2) : 0;

    t_pRapDipolePair.m_Dipole2.x() = 0; //m_bGridInitialized ? (*m_pMatGrid)(p_iDipoleIdx2, 0) : 0;
    t_pRapDipolePair.m_Dipole2.y() = 0; //m_bGridInitialized ? (*m_pMatGrid)(p_iDipoleIdx2, 1) : 0;
    t_pRapDipolePair.m_Dipole2.z() = 0; //m_bGridInitialized ? (*m_pMatGrid)(p_iDipoleIdx2, 2) : 0;

    t_pRapDipolePair.m_Dipole1.phi_x() = p_vec_phi_k_1[0];
    t_pRapDipolePair.m_Dipole1.phi_y() = p_vec_phi_k_1[1];
    t_pRapDipolePair.m_Dipole1.phi_z() = p_vec_phi_k_1[2];

    t_pRapDipolePair.m_Dipole2.phi_x() = p_vec_phi_k_1[3];
    t_pRapDipolePair.m_Dipole2.phi_y() = p_vec_phi_k_1[4];
    t_pRapDipolePair.m_Dipole2.phi_z() = p_vec_phi_k_1[5];

    t_pRapDipolePair.m_vCorrelation = p_valCor;

    p_RapDipoles.append(t_pRapDipolePair);
}

//=============================================================================================================

void RapMusic::setStcAttr(int p_iSampStcWin, float p_fStcOverlap)
{
    m_iSamplesStcWindow = p_iSampStcWin;
    m_fStcOverlap = p_fStcOverlap;
}
