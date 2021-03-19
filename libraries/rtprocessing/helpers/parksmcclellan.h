//=============================================================================================================
/**
 * @file     parksmcclellan.h
 * @since    0.1.0
 *
 * ported to mne-cpp by Christoph Dinh and Florian Schlembach in February, 2014
 *
 *
 * October 19, 2013
 * From: http://www.iowahills.com/A7ExampleCodePage.html
 *
 * The original fortran code came from the Parks McClellan article on Wikipedia.
 * http://en.wikipedia.org/wiki/Parks%E2%80%93McClellan_filter_design_algorithm
 *
 * This code is quite different from the original. The original code had 69 goto statements,
 * which made it nearly impossible to follow. And of course, it was Fortran code, so many changes
 * had to be made regardless of style.
 *
 * Our first step was to get a C version of the code working with as few changes as possible.
 * Then, in our desire to see if the code could be made more understandable, we decided to
 * remove as many goto statements as possible. We checked our work by comparing the coefficients
 * between this code and our original translation on more than 1000 filters while varying all the parameters.
 *
 * Ultimately, we were able to reduce the goto count from 69 to 7, all of which are in the Remez
 * function. Of the 7 remaining, 3 of these are at the very bottom of the function, and go
 * back to the very top of the function. These could have been removed, but our goal was to
 * clarify the code, not restyle it, and since they are clear, we let them be.
 *
 * The other 4 goto statements are intertwined in a rather nasty way. We recommend you print out
 * the Remez code, tape the sheets end to end, and trace out the goto's. It wasn't apparent to
 * us that they can be removed without an extensive study of the code.
 *
 * For better or worse, we also removed any code that was obviously related to Hilbert transforms
 * and Differentiators. We did this because we aren't interested in these, and we also don't
 * believe this algorithm does a very good job with them (far too much ripple).
 *
 * We added the functions CalcCoefficients() and ErrTest() as a way to simplify things a bit.
 *
 * We also found 3 sections of code that never executed. Two of the sections were just a few lines
 * that the goto's always went around. The third section represented nearly half of the CalcCoefficients()
 * function. This statement always tested the same, which never allowed the code to execute.
 * if(GRID[1] < 0.01 && GRID[NGRID] > 0.49) KKK = 1;
 * This may be due to the 0.01 minimum width limit we set for the bands.
 *
 * Note our use of MIN_TEST_VAL. The original code wasn't guarding against division by zero.
 * Limiting the return values as we have also helped the algorithm's convergence behavior.
 *
 * In an effort to improve readability, we made a large number of variable name changes and also
 * deleted a large number of variables. We left many variable names in tact, in part as an aid when
 * comparing to the original code, and in part because a better name wasn't obvious.
 *
 * This code is essentially straight c, and should compile with few, if any changes. Take note
 * of the 4 commented lines at the bottom of CalcParkCoeff2. These lines replace the code from the
 * original Ouch() function. They warn of the possibility of convergence failure, but you will
 * find that the iteration count NITER, isn't always an indicator of convergence problems when
 * it is less than 3, as stated in the original Fortran code comments.
 *
 * If you find a problem with this code, please leave us a note on:
 * http://www.iowahills.com/feedbackcomments.html
 */

#ifndef PARKSMCCLELLAN_H
#define PARKSMCCLELLAN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtprocessing_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

/**
 * DECLARE CLASS ParksMcClellan
 *
 * @brief The ParksMcClellan class provides the ParksMcClellan filter desing algorithm.
 */
class RTPROCESINGSHARED_EXPORT ParksMcClellan : public QObject
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
} // NAMESPACE RTPROCESSINGLIB

#endif // PARKSMCCLELLAN_H
