//=============================================================================================================
/**
 * @file     newparksmcclellan.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @version  2.1.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    Declaration of the legacy Parks-McClellan helper used by mne_browse.
 *
 */

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
