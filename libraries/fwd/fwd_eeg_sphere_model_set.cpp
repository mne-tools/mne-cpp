//=============================================================================================================
/**
 * @file     fwd_eeg_sphere_model_set.cpp
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
 * @brief    Definition of the FwdEegSphereModelSet Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_eeg_sphere_model_set.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QFile>

#include <Eigen/Core>

using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

/*
 * Basics...
 */
#define MALLOC_2(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC_2(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))
#define FREE_2(x) if ((char *)(x) != NULL) free((char *)(x))

#define MAXLINE 500

#ifndef FAIL
#define FAIL -1
#endif

#ifndef OK
#define OK 0
#endif

#define SEP ":\n\r"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FWDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdEegSphereModelSet::FwdEegSphereModelSet()
{
}

////=============================================================================================================

//FwdEegSphereModelSet::FwdEegSphereModelSet(const FwdEegSphereModelSet &p_FwdEegSphereModelSet)
//: m_qListModels(p_FwdEegSphereModelSet.m_qListModels)
//{

//}

//=============================================================================================================

FwdEegSphereModelSet::~FwdEegSphereModelSet()
{
    for (int k = 0; k < this->nmodel(); k++)
        delete this->models[k];
}

//=============================================================================================================

//void FwdEegSphereModelSet::fwd_free_eeg_sphere_model_set(FwdEegSphereModelSet* s)

//{
//    if (!s)
//        return;

//    return;
//}

//=============================================================================================================

//FwdEegSphereModelSet* FwdEegSphereModelSet::fwd_new_eeg_sphere_model_set()
//{
//    FwdEegSphereModelSet* s = new FwdEegSphereModelSet;

//    return s;
//}

//=============================================================================================================

FwdEegSphereModelSet* FwdEegSphereModelSet::fwd_add_to_eeg_sphere_model_set(FwdEegSphereModelSet* s, FwdEegSphereModel* m)
{
    if (!s)
        s = new FwdEegSphereModelSet;

    s->models.append(m);
    return s;
}

//=============================================================================================================
//fwd_eeg_sphere_models.c
FwdEegSphereModelSet* FwdEegSphereModelSet::fwd_add_default_eeg_sphere_model(FwdEegSphereModelSet* s)
{
    static const int   def_nlayer        = 4;
    VectorXf def_unit_rads(def_nlayer);
    def_unit_rads << 0.90f,0.92f,0.97f,1.0f;
    VectorXf def_sigmas(def_nlayer);
    def_sigmas << 0.33f,1.0f,0.4e-2f,0.33f;

    return FwdEegSphereModelSet::fwd_add_to_eeg_sphere_model_set(s,FwdEegSphereModel::fwd_create_eeg_sphere_model("Default",
                                                                         def_nlayer,def_unit_rads,def_sigmas));
}

//=============================================================================================================
//fwd_eeg_sphere_models.c
FwdEegSphereModelSet* FwdEegSphereModelSet::fwd_load_eeg_sphere_models(const QString& filename, FwdEegSphereModelSet* now)
{
    char line[MAXLINE];
    FILE *fp = NULL;
    QString name;
    VectorXf rads;
    VectorXf sigmas;
    int   nlayer  = 0;
    char *one, *two;
    QString tag;

    if (!now)
        now = fwd_add_default_eeg_sphere_model(now);

    if (filename.isEmpty())
        return now;

    QFile t_file(filename);
    if (!t_file.isReadable())	/* Never mind about an unaccesible file */
        return now;

    if ((fp = fopen(filename.toUtf8().data(),"r")) == NULL) {
        printf(filename.toUtf8().data());
        goto bad;
    }
    while (fgets(line,MAXLINE,fp) != NULL) {
        if (line[0] == '#')
            continue;
        one = strtok(line,SEP);
        if (one != NULL) {
            if (tag.isEmpty() || tag.size() == 0)
                name = one;
            else {
                name = QString("%1 %2").arg(one).arg(tag);
            }
            while (1) {
                one = strtok(NULL,SEP);
                if (one == NULL)
                    break;
                two = strtok(NULL,SEP);
                if (two == NULL)
                    break;
                rads.resize(nlayer+1);
                sigmas.resize(nlayer+1);
                if (sscanf(one,"%g",rads[nlayer]) != 1) {
                    nlayer = 0;
                    break;
                }
                if (sscanf(two,"%g",sigmas[nlayer]) != 1) {
                    nlayer = 0;
                    break;
                }
                nlayer++;
            }
            if (nlayer > 0)
                now = fwd_add_to_eeg_sphere_model_set(now,FwdEegSphereModel::fwd_create_eeg_sphere_model(name,nlayer,rads,sigmas));
            nlayer = 0;
        }
    }
    if (ferror(fp)) {
        printf(filename.toUtf8().data());
        goto bad;
    }
    fclose(fp);
    return now;

bad : {
        if (fp)
            fclose(fp);
        delete now;
        return NULL;
    }
}

//=============================================================================================================
//fwd_eeg_sphere_models.c
FwdEegSphereModel* FwdEegSphereModelSet::fwd_select_eeg_sphere_model(const QString& p_sName)
{
    int k;

    QString name("Default");

    if (!p_sName.isEmpty())
        name = p_sName;

    if (this->nmodel() == 0) {
        printf("No EEG sphere model definitions available");
        return NULL;
    }

    for (k = 0; k < this->nmodel(); k++) {
        if (this->models[k]->name.compare(name) == 0) {
            printf("Selected model: %s\n",this->models[k]->name.toUtf8().constData());
            return new FwdEegSphereModel(*(this->models[k]));
        }
    }
    printf("EEG sphere model %s not found.",name.toUtf8().constData());
    return NULL;
}

//=============================================================================================================
//dipole_fit_setup.c
void FwdEegSphereModelSet::fwd_list_eeg_sphere_models(FILE *f)
{
    int k,p;
    FwdEegSphereModel* this_model;

    if ( this->nmodel() < 0 )
        return;
    fprintf(f,"Available EEG sphere models:\n");
    for (k = 0; k < this->nmodel(); k++) {
        this_model = this->models[k];
        fprintf(f,"\t%s : %d",this_model->name.toUtf8().constData(),this_model->nlayer());
        for (p = 0; p < this_model->nlayer(); p++)
            fprintf(f," : %7.3f : %7.3f",this_model->layers[p].rel_rad,this_model->layers[p].sigma);
        fprintf(f,"\n");
    }
}

////=============================================================================================================

//void FwdEegSphereModelSet::addFwdEegSphereModel(const FwdEegSphereModel &p_FwdEegSphereModel)
//{
//    m_qListModels.append(p_FwdEegSphereModel);
//}

////=============================================================================================================

//const FwdEegSphereModel& FwdEegSphereModelSet::operator[] (qint32 idx) const
//{
//    if (idx>=m_qListModels.length())
//    {
//        qWarning("Warning: Required FwdEegSphereModel doesn't exist! Returning FwdEegSphereModel '0'.");
//        idx=0;
//    }
//    return m_qListModels[idx];
//}

////=============================================================================================================

//FwdEegSphereModel& FwdEegSphereModelSet::operator[] (qint32 idx)
//{
//    if (idx >= m_qListModels.length())
//    {
//        qWarning("Warning: Required FwdEegSphereModel doesn't exist! Returning FwdEegSphereModel '0'.");
//        idx = 0;
//    }
//    return m_qListModels[idx];
//}

////=============================================================================================================

//FwdEegSphereModelSet &FwdEegSphereModelSet::operator<<(const FwdEegSphereModel &p_FwdEegSphereModel)
//{
//    this->m_qListModels.append(p_FwdEegSphereModel);
//    return *this;
//}
