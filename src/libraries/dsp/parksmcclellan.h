//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     parksmcclellan.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Parks–McClellan equiripple FIR design via the Remez exchange algorithm.
 *
 * The Parks–McClellan algorithm computes the optimal linear-phase FIR filter
 * that minimises the maximum (Chebyshev / minimax) deviation between the
 * desired and the realised frequency response over a set of pass- and
 * stop-band intervals. The solution is uniquely characterised by the
 * equiripple property: at least @c N+2 alternations of the error function
 * occur on the union of pass- and stop-bands, where @c N is the filter order.
 *
 * The implementation here is a direct C++ translation of the original
 * Fortran code of McClellan, Parks and Rabiner. It exchanges trial extremal
 * frequencies until the alternation condition is satisfied, then constructs
 * the impulse response by interpolating the resulting Chebyshev polynomial
 * back to the time domain. Practical limits: 9 ≤ NumTaps ≤ 128, transition
 * width 0.02–0.30 (normalised to π), and odd tap counts for high-pass and
 * notch types so a linear-phase type-I structure is realisable.
 *
 * Used as one of the design back-ends of @ref FilterKernel and ultimately
 * of @ref FirFilter; cosine-tapered designs are the alternative when speed
 * matters more than equiripple optimality.
 */

#ifndef PARKSMCCLELLAN_H
#define PARKSMCCLELLAN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

/**
 * DECLARE CLASS ParksMcClellan
 *
 * @brief Parks-McClellan equiripple FIR filter design algorithm (Remez exchange).
 */
class DSPSHARED_EXPORT ParksMcClellan : public QObject
{
    Q_OBJECT
    Q_ENUMS(TPassType) //makes enum available to the class' Qt meta object

public:
    enum TPassType {LPF, HPF, BPF, NOTCH };

    ParksMcClellan();

    //=========================================================================================================
    /**
     * NumTaps must be odd for high pass and notch filters. Max number of taps is 128.
     * The arrays can handle up to 256 taps, but 128 is a good practical limit for convergence.
     * The minimum number of taps is 9 (maybe < 9, I forget the exact lower limit or what sets it)
     * OmegaC is the 3 dB corner freq for low pass and high pass filters.
     * It is the center freq for band pass and notch filters.
     * BW is the bandwidth for bandpass and notch filters (ignored on low and high pass).
     * OmegaC and BW are in terms of Pi. e.g. OmegaC = 0.5 centers a BPF at Omega = Pi/2.
     * The PM algorithm however uses frequencies in terms of 2Pi, so we need to to this: Edge[j] /= 2.0
     * ParksWidth is the width of the transition bands. For simplicity, we only use one width,
     * but the algorithm allows for unique values on every band edge.
     * Practical limits for ParksWidth are 0.02 - 0.15 for BPF and Notch,  0.02 - 0.30 for LPF and HPF.
     * TPassType is defined in the header file. LPF = Low Pass Filter, etc.
     * You should note our 0.01 minimum width for each band. This limit works well for the algorithm.
     * You will also find that OmegaC and BW need to be scaled a bit, depending ParksWidth, to get the
     * 3 dB corner frequencies to come in on target.

     * e.g. NewParksMcClellan(33, 0.7, 0.2, 0.1, HPF);
     * gives a 33 tap high pass filter with 3 dB corner at 0.7 with a transition bandwidth of 0.1
     * The FIR coefficients are placed in FirCoeff, starting at index 0.
     */
    ParksMcClellan(int NumTaps,
                   double OmegaC,
                   double BW,
                   double ParksWidth,
                   TPassType PassType);

    ~ParksMcClellan();

    //=========================================================================================================
    /**
     * Using nothrow prevents an exception from being thrown. new will instead return NULL.
     * These array are much larger than actually needed. See the notes in the orig fortran file.
     */
    void init(int NumTaps,
              double OmegaC,
              double BW,
              double ParksWidth,
              TPassType PassType);

    //=========================================================================================================
    /**
     */
    void CalcParkCoeff2(int NBANDS, int NFILT);

    //=========================================================================================================
    /**
     * Function to calculate the lagrange interpolation coefficients for use in the function gee.
     */
    double LeGrangeInterp2(int K, int N, int M);

    //=========================================================================================================
    /**
     * Function to evaluate the frequency response using the Lagrange interpolation
     * formula in the barycentric form.
     */
    double GEE2(int K, int N);

    //=========================================================================================================
    /**
     */
    int Remez2(int GridIndex);

    //=========================================================================================================
    /**
     * This was added by IowaHills and is used in Remez() in 6 places.
     */
    bool ErrTest(int k,
                 int Nut,
                 double Comp,
                 double *Err);

    //=========================================================================================================
    /**
     * This was added by IowaHills and is called from CalcParkCoeff2().
     * Calculation of the coefficients of the best approximation using the inverse discrete fourier transform.
     */
    void CalcCoefficients();

    Eigen::RowVectorXd FirCoeff; /**< containt the generated filter coefficients. */

private:
    int HalfTapCount;
    Eigen::VectorXi ExchangeIndex;
    Eigen::VectorXd LeGrangeD;
    Eigen::VectorXd Alpha;
    Eigen::VectorXd CosOfGrid;
    Eigen::VectorXd DesPlus;
    Eigen::VectorXd Coeff;
    Eigen::VectorXd Edge;
    Eigen::VectorXd BandMag;
    Eigen::VectorXd InitWeight;
    Eigen::VectorXd DesiredMag;
    Eigen::VectorXd Grid;
    Eigen::VectorXd Weight;

    bool InitDone2;
};
} // NAMESPACE UTILSLIB

#endif // PARKSMCCLELLAN_H
