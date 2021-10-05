//=============================================================================================================
/**
 * @file     ecd_set.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the Electric Current Dipole Set (ECDSet) Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecd_set.h"
#include <fiff/fiff_types.h>

//ToDo don't use access and unlink -> use QT stuff instead -> QFile
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <io.h>
#else
#include <unistd.h>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QFile>
#include <QString>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define X 0
#define Y 1
#define Z 2

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

//TODO OBSOLETE - USE ALREADY DEFINED ONE
static fiff_int_t swap_int (fiff_int_t source)
{
    unsigned char *csource =  (unsigned char *)(&source);
    fiff_int_t result;
    unsigned char *cresult =  (unsigned char *)(&result);

    cresult[0] = csource[3];
    cresult[1] = csource[2];
    cresult[2] = csource[1];
    cresult[3] = csource[0];
    return (result);
}

//=============================================================================================================
//TODO OBSOLETE - USE ALREADY DEFINED ONE
static float swap_float (float source)
{
    unsigned char *csource =  (unsigned char *)(&source);
    float result;
    unsigned char *cresult =  (unsigned char *)(&result);

    cresult[0] = csource[3];
    cresult[1] = csource[2];
    cresult[2] = csource[1];
    cresult[3] = csource[0];
    return result;
}

//=============================================================================================================

namespace INVERSELIB {

typedef struct {
    int   dipole;               /* Which dipole in a multi-dipole set */
    float begin,end;            /* Fitting time range */
    float r0[3];                /* Sphere model origin */
    float rd[3];                /* Dipole location */
    float Q[3];                 /* Dipole amplitude */
    float goodness;             /* Goodness-of-fit */
    int   errors_computed;      /* Have we computed the errors */
    float noise_level;          /* Noise level used for error computations */
    float single_errors[5];     /* Single parameter error limits */
    float error_matrix[5][5];   /* This fully describes the conf. ellipsoid */
    float conf_vol;             /* The xyz confidence volume */
    float khi2;                 /* The khi^2 value */
    float prob;                 /* Probability to exceed khi^2 by chance */
    float noise_est;            /* Total noise estimate */
} *bdipEcd,bdipEcdRec;

} // Namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ECDSet::ECDSet()
{
}

//=============================================================================================================

ECDSet::ECDSet(const ECDSet &p_ECDSet)
: dataname(p_ECDSet.dataname)
, m_qListDips(p_ECDSet.m_qListDips)
{
}

//=============================================================================================================

ECDSet::~ECDSet()
{
}

//=============================================================================================================

void ECDSet::addEcd(const ECD& p_ecd)
{
    m_qListDips.append(p_ecd);
}

//=============================================================================================================

ECDSet ECDSet::read_dipoles_dip(const QString& fileName)
{
    ECDSet  set;

    QFile inputFile(fileName);
    if (inputFile.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        QTextStream in(&inputFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            QStringList list = line.split(QRegExp("\\s+"));

            if(list[0].contains("#") || list.size() != 11) {
                continue;
            }
            else {
                ECD     one;
                one.valid = true;
                one.time = list[1].toFloat() / 1000.0f;
                one.rd[X] = list[3].toFloat() / 1000.0f;
                one.rd[Y] = list[4].toFloat() / 1000.0f;
                one.rd[Z] = list[5].toFloat() / 1000.0f;
                one.Q[X] = list[7].toFloat() / 1e9f;
                one.Q[Y] = list[8].toFloat() / 1e9f;
                one.Q[Z] = list[9].toFloat() / 1e9f;
                one.good = list[10].toFloat() / 100.0f;
                set << one;
            }
        }
        inputFile.close();

        printf("Read %d dipoles in dip format from %s\n",set.size(),fileName.toUtf8().data());
    }
    else {
        printf("Not able to read from: %s\n", fileName.toUtf8().data());
    }

    return set;
}

//=============================================================================================================

bool ECDSet::save_dipoles_bdip(const QString& fileName)
/*
   * Save dipoles in the bdip format employed by xfit
   */
{
    FILE        *out = NULL;
    bdipEcdRec  one_out;
    ECD         one;
    int         k,p;
    int         nsave;

    if (fileName.isEmpty() || this->size() == 0)
        return true;

    if ((out = fopen(fileName.toUtf8().data(),"w")) == NULL) {
        printf(fileName.toUtf8().data());
        return false;
    }

    for (k = 0, nsave = 0; k < this->size(); k++) {
        one = m_qListDips[k];
        if (one.valid) {
            one_out.dipole = swap_int(1);
            one_out.begin  = swap_float(one.time);
            for (p = 0; p < 3; p++) {
                one_out.r0[p] = swap_float(0.0);
                one_out.rd[p] = swap_float(one.rd[p]);
                one_out.Q[p]  = swap_float(one.Q[p]);
            }
            one_out.goodness = swap_float(one.good);
            one_out.errors_computed = swap_int(0);
            one_out.khi2            = swap_float(one.khi2);
            if (fwrite(&one_out,sizeof(bdipEcdRec),1,out) != 1) {
                printf("Failed to write a dipole");
                goto bad;
            }
            nsave++;
        }
    }
    if (fclose(out) != 0) {
        out = NULL;
        printf(fileName.toUtf8().data());
        goto bad;
    }
    printf("Save %d dipoles in bdip format to %s\n",nsave,fileName.toUtf8().data());
    return true;

bad : {
        if (out) {
            fclose(out);
            unlink(fileName.toUtf8().data());
        }
        return false;
    }
}

//=============================================================================================================

bool ECDSet::save_dipoles_dip(const QString& fileName) const
{
    FILE *out = NULL;
    int  k,nsave;
    ECD  one;

    if (fileName.isEmpty() || this->size() == 0)
        return true;
    if ((out = fopen(fileName.toUtf8().data(),"w")) == NULL) {
        printf(fileName.toUtf8().data());
        return false;
    }
    fprintf(out,"# CoordinateSystem \"Head\"\n");
    fprintf (out,"# %7s %7s %8s %8s %8s %8s %8s %8s %8s %6s\n",
             "begin","end","X (mm)","Y (mm)","Z (mm)","Q(nAm)","Qx(nAm)","Qy(nAm)","Qz(nAm)","g/%");
    for (k = 0, nsave = 0; k < this->size(); k++) {
        one = this->m_qListDips[k];
        if (one.valid) {
            fprintf(out,"  %7.1f %7.1f %8.2f %8.2f %8.2f %8.3f %8.3f %8.3f %8.3f %6.1f\n",
                    1000*one.time,1000*one.time,
                    1000*one.rd[X],1000*one.rd[Y],1000*one.rd[Z],
                    1e9*one.Q.norm(),1e9*one.Q[X],1e9*one.Q[Y],1e9*one.Q[Z],100.0*one.good);
            nsave++;
        }
    }
    fprintf(out,"## Name \"%s dipoles\" Style \"Dipoles\"\n","ALL");
    if (fclose(out) != 0) {
        out = NULL;
        printf(fileName.toUtf8().data());
        goto bad;
    }
    printf("Save %d dipoles in dip format to %s\n",nsave,fileName.toUtf8().data());
    return true;

bad : {
        if (out) {
            fclose(out);
            unlink(fileName.toUtf8().data());
        }
        return false;
    }
}

//=============================================================================================================

const ECD& ECDSet::operator[] (int idx) const
{
    if (idx>=m_qListDips.length())
    {
        qWarning("Warning: Required ECD doesn't exist! Returning ECD '0'.");
        idx=0;
    }
    return m_qListDips[idx];
}

//=============================================================================================================

ECD& ECDSet::operator[] (int idx)
{
    if (idx >= m_qListDips.length())
    {
        qWarning("Warning: Required ECD doesn't exist! Returning ECD '0'.");
        idx = 0;
    }
    return m_qListDips[idx];
}

//=============================================================================================================

ECDSet &ECDSet::operator<<(const ECD &p_ecd)
{
    this->m_qListDips.append(p_ecd);
    return *this;
}
