#ifdef __cplusplus
extern "C" {
#endif

#ifndef NEWPARKSMCCLELLAN_H
#define NEWPARKSMCCLELLAN_H

extern int HalfTapCount, *ExchangeIndex;
extern double *LeGrangeD, *Alpha, *CosOfGrid, *DesPlus, *Coeff, *Edge, *BandMag, *InitWeight, *DesiredMag, *Grid, *Weight, *FirCoeff;
extern bool InitDone2;

enum TPassType {LPF, HPF, BPF, NOTCH };
void NewParksMcClellan(int NumTaps, double OmegaC, double BW, double ParksWidth, TPassType PassType);
void CalcParkCoeff2(int NBANDS, int NFILT);
double LeGrangeInterp2(int K, int N, int M);
double GEE2(int K, int N);
int Remez2(int GridIndex);
bool ErrTest(int k, int Nut, double Comp, double *Err);
void CalcCoefficients(void);

#endif // NEWPARKSMCCLELLAN_H

#ifdef __cplusplus
}
#endif
