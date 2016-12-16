//=============================================================================================================
/**
* @file     fwd_eeg_sphere_model_set.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the FwdEegSphereModelSet Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_eeg_sphere_model_set.h"

//ToDo don't use access and unlink -> use Qt stuff instead
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <io.h>
#else
#include <unistd.h>
#endif


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QString>


//*************************************************************************************************************
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

#ifndef R_OK
#define R_OK    4       /* Test for read permission.  */
#endif


#if defined(_WIN32) || defined(_WIN64)
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif


char *mne_strdup_2(const char *s)
{
    char *res;
    if (s == NULL)
        return NULL;
    res = (char*) malloc(strlen(s)+1);
    strcpy(res,s);
    return res;
}


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdEegSphereModelSet::FwdEegSphereModelSet()
{
}


////*************************************************************************************************************

//FwdEegSphereModelSet::FwdEegSphereModelSet(const FwdEegSphereModelSet &p_FwdEegSphereModelSet)
//: m_qListModels(p_FwdEegSphereModelSet.m_qListModels)
//{

//}


//*************************************************************************************************************

FwdEegSphereModelSet::~FwdEegSphereModelSet()
{

}


//*************************************************************************************************************

void FwdEegSphereModelSet::fwd_free_eeg_sphere_model_set(FwdEegSphereModelSet* s)

{
    int k;
    if (!s)
        return;
    for (k = 0; k < s->nmodel; k++)
        FwdEegSphereModel::fwd_free_eeg_sphere_model(s->models[k]);
    FREE_2(s->models);
    FREE_2(s);

    return;
}


//*************************************************************************************************************

FwdEegSphereModelSet* FwdEegSphereModelSet::fwd_new_eeg_sphere_model_set()
{
    FwdEegSphereModelSet* s = MALLOC_2(1,FwdEegSphereModelSet);

    s->models  = NULL;
    s->nmodel  = 0;
    return s;
}


//*************************************************************************************************************

FwdEegSphereModelSet* FwdEegSphereModelSet::fwd_add_to_eeg_sphere_model_set(FwdEegSphereModelSet* s, FwdEegSphereModel* m)
{
    if (!s)
        s = fwd_new_eeg_sphere_model_set();

    s->models = REALLOC_2(s->models,s->nmodel+1,FwdEegSphereModel*);
    s->models[s->nmodel++] = m;
    return s;
}


//*************************************************************************************************************
//fwd_eeg_sphere_models.c
FwdEegSphereModelSet* FwdEegSphereModelSet::fwd_add_default_eeg_sphere_model(FwdEegSphereModelSet* s)
{
    static const int   def_nlayer        = 4;
    static const float def_unit_rads[]   = {0.90,0.92,0.97,1.0};
    static const float def_sigmas[]      = {0.33,1.0,0.4e-2,0.33};

    return FwdEegSphereModelSet::fwd_add_to_eeg_sphere_model_set(s,FwdEegSphereModel::fwd_create_eeg_sphere_model("Default",
                                                                         def_nlayer,def_unit_rads,def_sigmas));
}


//*************************************************************************************************************
//fwd_eeg_sphere_models.c
FwdEegSphereModelSet* FwdEegSphereModelSet::fwd_load_eeg_sphere_models(const QString& filename, FwdEegSphereModelSet* now)
{
    char line[MAXLINE];
    FILE *fp = NULL;
    char  *name   = NULL;
    float *rads   = NULL;
    float *sigmas = NULL;
    int   nlayer  = 0;
    char  *one,*two;
    char  *tag = NULL;

    if (!now)
        now = fwd_add_default_eeg_sphere_model(now);

    if (filename.isEmpty())
        return now;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    if (_access(filename.toLatin1().data(),R_OK) != OK)	/* Never mind about an unaccesible file */
        return now;
#else
    if (access(filename.toLatin1().data(),R_OK) != OK)	/* Never mind about an unaccesible file */
        return now;
#endif

    if ((fp = fopen(filename.toLatin1().data(),"r")) == NULL) {
        printf(filename.toLatin1().data());
        goto bad;
    }
    while (fgets(line,MAXLINE,fp) != NULL) {
        if (line[0] == '#')
            continue;
        one = strtok(line,SEP);
        if (one != NULL) {
            if (!tag || strlen(tag) == 0)
                name = mne_strdup_2(one);
            else {
                name = MALLOC_2(strlen(one)+strlen(tag)+10,char);
                sprintf(name,"%s %s",one,tag);
            }
            while (1) {
                one = strtok(NULL,SEP);
                if (one == NULL)
                    break;
                two = strtok(NULL,SEP);
                if (two == NULL)
                    break;
                rads   = REALLOC_2(rads,nlayer+1,float);
                sigmas = REALLOC_2(sigmas,nlayer+1,float);
                if (sscanf(one,"%g",rads+nlayer) != 1) {
                    nlayer = 0;
                    break;
                }
                if (sscanf(two,"%g",sigmas+nlayer) != 1) {
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
        printf(filename.toLatin1().data());
        goto bad;
    }
    fclose(fp);
    return now;

bad : {
        if (fp)
            fclose(fp);
        fwd_free_eeg_sphere_model_set(now);
        return NULL;
    }
}


//*************************************************************************************************************
//fwd_eeg_sphere_models.c
FwdEegSphereModel* FwdEegSphereModelSet::fwd_select_eeg_sphere_model(const QString& p_sName)
{
    int k;

    QString name("Default");

    if (!p_sName.isEmpty())
        name = p_sName;

    if (this->nmodel == 0) {
        printf("No EEG sphere model definitions available");
        return NULL;
    }

    for (k = 0; k < this->nmodel; k++) {
        if (strcasecmp(this->models[k]->name,name.toLatin1().data()) == 0) {
            fprintf(stderr,"Selected model: %s\n",this->models[k]->name);
            return this->models[k]->fwd_dup_eeg_sphere_model();
        }
    }
    printf("EEG sphere model %s not found.",name.toLatin1().data());
    return NULL;
}


//*************************************************************************************************************
//dipole_fit_setup.c
void FwdEegSphereModelSet::fwd_list_eeg_sphere_models(FILE *f)
{
    int k,p;
    FwdEegSphereModel* this_model;

    if ( this->nmodel < 0 )
        return;
    fprintf(f,"Available EEG sphere models:\n");
    for (k = 0; k < this->nmodel; k++) {
        this_model = this->models[k];
        fprintf(f,"\t%s : %d",this_model->name,this_model->nlayer);
        for (p = 0; p < this_model->nlayer; p++)
            fprintf(f," : %7.3f : %7.3f",this_model->layers[p].rel_rad,this_model->layers[p].sigma);
        fprintf(f,"\n");
    }
}






////*************************************************************************************************************

//void FwdEegSphereModelSet::addFwdEegSphereModel(const FwdEegSphereModel &p_FwdEegSphereModel)
//{
//    m_qListModels.append(p_FwdEegSphereModel);
//}


////*************************************************************************************************************

//const FwdEegSphereModel& FwdEegSphereModelSet::operator[] (qint32 idx) const
//{
//    if (idx>=m_qListModels.length())
//    {
//        qWarning("Warning: Required FwdEegSphereModel doesn't exist! Returning FwdEegSphereModel '0'.");
//        idx=0;
//    }
//    return m_qListModels[idx];
//}


////*************************************************************************************************************

//FwdEegSphereModel& FwdEegSphereModelSet::operator[] (qint32 idx)
//{
//    if (idx >= m_qListModels.length())
//    {
//        qWarning("Warning: Required FwdEegSphereModel doesn't exist! Returning FwdEegSphereModel '0'.");
//        idx = 0;
//    }
//    return m_qListModels[idx];
//}


////*************************************************************************************************************

//FwdEegSphereModelSet &FwdEegSphereModelSet::operator<<(const FwdEegSphereModel &p_FwdEegSphereModel)
//{
//    this->m_qListModels.append(p_FwdEegSphereModel);
//    return *this;
//}
