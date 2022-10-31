

#include "dipole_fit.h"
#include "../c/mne_meas_data_set.h"
#include "guess_data.h"

#include <string.h>
#include <QScopedPointer>

using namespace INVERSELIB;
using namespace MNELIB;
using namespace FWDLIB;

#if defined(_WIN32) || defined(_WIN64)
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION     "1.00"
#endif

#ifndef FAIL
#define FAIL -1
#endif

#ifndef OK
#define OK 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define BIG_TIME 1e6

#define SEG_LEN 10.0

#define EPS_VALUES 0.05

//=============================================================================================================
// STATIC DEFINITIONS ToDo make members
//=============================================================================================================

#define MALLOC(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))
#define FREE(x) if ((char *)(x) != NULL) free((char *)(x))

#define ALLOC_CMATRIX(x,y) mne_cmatrix((x),(y))

#define FREE_CMATRIX(m) mne_free_cmatrix((m))

static void matrix_error(int kind, int nr, int nc)

{
    if (kind == 1)
        printf("Failed to allocate memory pointers for a %d x %d matrix\n",nr,nc);
    else if (kind == 2)
        printf("Failed to allocate memory for a %d x %d matrix\n",nr,nc);
    else
        printf("Allocation error for a %d x %d matrix\n",nr,nc);
    if (sizeof(void *) == 4) {
        printf("This is probably because you seem to be using a computer with 32-bit architecture.\n");
        printf("Please consider moving to a 64-bit platform.");
    }
    printf("Cannot continue. Sorry.\n");
    exit(1);
}

float **mne_cmatrix (int nr,int nc)

{
    int i;
    float **m;
    float *whole;

    m = MALLOC(nr,float *);
    if (!m) matrix_error(1,nr,nc);
    whole = MALLOC(nr*nc,float);
    if (!whole) matrix_error(2,nr,nc);

    for(i=0;i<nr;i++)
        m[i] = whole + i*nc;
    return m;
}

void mne_free_cmatrix (float **m)
{
    if (m) {
        FREE(*m);
        FREE(m);
    }
}

//char *mne_compose_mne_name(const char *path, const char *filename)
///*
//      * Compose a filename under the "$MNE_ROOT" directory
//      */
//{
//    char *res;
//    char *mne_root;

//    if (filename == NULL) {
//        qCritical("No file name specified to mne_compose_mne_name");
//        return NULL;
//    }
//    mne_root = getenv(MNE_ENV_ROOT);
//    if (mne_root == NULL || strlen(mne_root) == 0) {
//        qCritical("Environment variable MNE_ROOT not set");
//        return NULL;
//    }
//    if (path == NULL || strlen(path) == 0) {
//        res = MALLOC(strlen(mne_root)+strlen(filename)+2,char);
//        strcpy(res,mne_root);
//        strcat(res,"/");
//        strcat(res,filename);
//    }
//    else {
//        res = MALLOC(strlen(mne_root)+strlen(filename)+strlen(path)+3,char);
//        strcpy(res,mne_root);
//        strcat(res,"/");
//        strcat(res,path);
//        strcat(res,"/");
//        strcat(res,filename);
//    }
//    return res;
//}

//============================= misc_util.c =============================

//============================= mne_named_matrix.c =============================

void mne_free_name_list(char **list, int nlist)
/*
 * Free a name list array
 */
{
    int k;
    if (list == NULL || nlist == 0)
        return;
    for (k = 0; k < nlist; k++) {
#ifdef FOO
        printf("%d %s\n",k,list[k]);
#endif
        FREE(list[k]);
    }
    FREE(list);
    return;
}

//============================= mne_ch_selections.c =============================

/*
 * Mandatory allocation functions
 */
static mneChSelection new_ch_selection()

{
    mneChSelection newsel = MALLOC(1,mneChSelectionRec);

    newsel->name.clear();
    newsel->chdef.clear();
    newsel->chspick.clear();
    newsel->chspick_nospace.clear();
    newsel->pick    = NULL;
    newsel->pick_deriv = NULL;
    newsel->ch_kind = NULL;
    newsel->ndef    = 0;
    newsel->nchan   = 0;
    newsel->kind    = MNE_CH_SELECTION_UNKNOWN;
    return newsel;
}

mneChSelection mne_ch_selection_these(const QString& selname, const QStringList& names, int nch)
/*
 * Give an explicit list of interesting channels
 */
{
    int c;
    mneChSelection sel;

    sel        = new_ch_selection();
    sel->name  = selname;
//    sel->chdef;
    sel->ndef  = nch;
    sel->kind  = MNE_CH_SELECTION_USER;

    for (c = 0; c < nch; c++)
        sel->chdef.append(names[c]);

    return sel;
}

static void omit_spaces(QStringList names, int nnames)

{
    char *c,*cc;
    for (int k = 0; k < names.size(); k++) {
        while( names[k].startsWith(" ") ) {
            names[k].remove(0,1);
        }
    }
    return;
}

int mne_ch_selection_assign_chs(mneChSelection sel,
                                MneRawData*     data)
/*
      * Make the channel picking real easy
      */
{
    int c,rc,d;
    MneRawInfo*  info;
    int nch;
    QString dash;

    if (!sel || !data)
        return 0;

    info = data->info;
    sel->chspick.clear();
    sel->chspick_nospace.clear();
    /*
   * Expansion of possible regular expressions must be added eventually
   */
    sel->chspick         = sel->chdef;
    sel->chspick_nospace = sel->chdef;
    omit_spaces(sel->chspick_nospace,sel->ndef);
    sel->nchan           = sel->ndef;

    sel->pick        = REALLOC(sel->pick,sel->nchan,int);        /* Just in case */
    sel->pick_deriv  = REALLOC(sel->pick_deriv,sel->nchan,int);
    sel->ch_kind     = REALLOC(sel->ch_kind,sel->nchan,int);

    for (c = 0; c < sel->nchan; c++) {
        sel->pick[c]       = -1;
        sel->pick_deriv[c] = -1;
        sel->ch_kind[c]    = -1;
        for (rc = 0; rc < info->nchan; rc++) {
            if (QString::compare(sel->chspick[c],info->chInfo[rc].ch_name,Qt::CaseInsensitive) == 0 ||
                    QString::compare(sel->chspick_nospace[c],info->chInfo[rc].ch_name,Qt::CaseInsensitive) == 0) {
                sel->pick[c]    = rc;
                sel->ch_kind[c] = info->chInfo[rc].kind;
                break;
            }
        }
    }
    /*
     * Maybe the derivations will help
     */
    sel->nderiv = 0;
    if (data->deriv_matched) {
        QStringList deriv_names = data->deriv_matched->deriv_data->rowlist;
        int  nderiv        = data->deriv_matched->deriv_data->nrow;

        for (c = 0; c < sel->nchan; c++) {
            if (sel->pick[c] == -1) {
                for (d = 0; d < nderiv; d++) {
                    if (QString::compare(sel->chspick[c],deriv_names[d],Qt::CaseInsensitive) == 0 &&
                            data->deriv_matched->valid && data->deriv_matched->valid[d]) {
                        sel->pick_deriv[c] = d;
                        sel->ch_kind[c]    = data->deriv_matched->chs[d].kind;
                        sel->nderiv++;
                        break;
                    }
                }
            }
        }
    }
    /*
   * Try simple channels again without the part after dashes
   */
    for (c = 0; c < sel->nchan; c++) {
        if (sel->pick[c] == -1 && sel->pick_deriv[c] == -1) {
            for (rc = 0; rc < info->nchan; rc++) {
                dash = QString(info->chInfo[rc].ch_name).mid(QString(info->chInfo[rc].ch_name).indexOf("-")+1);// strchr(info->chInfo[rc].ch_name,'-');
                if (!dash.isNull()) {
//                    *dash = '\0';
                    if (QString::compare(sel->chspick[c],info->chInfo[rc].ch_name,Qt::CaseInsensitive) == 0 ||
                            QString::compare(sel->chspick_nospace[c],info->chInfo[rc].ch_name,Qt::CaseInsensitive) == 0) {
//                        *dash = '-';
                        sel->pick[c] = rc;
                        sel->ch_kind[c] = info->chInfo[rc].kind;
                        break;
                    }
//                    *dash = '-';
                }
            }
        }
    }
    for (c = 0, nch = 0; c < sel->nchan; c++) {
        if (sel->pick[c] >= 0)
            nch++;
    }
    if (sel->nderiv > 0)
        printf("Selection %c%s%c has %d matched derived channels.\n",'"',sel->name.toUtf8().constData(),'"',sel->nderiv);
    return nch;
}

//============================= mne_get_values.c =============================

int mne_get_values_from_data (float time,         /* Interesting time point */
                              float integ,	  /* Time integration */
                              float **data,	  /* The data values (time by time) */
                              int   nsamp,	  /* How many time points? */
                              int   nch,          /* How many channels */
                              float tmin,	  /* Time of first sample */
                              float sfreq,	  /* Sampling frequency */
                              int   use_abs,      /* Use absolute values */
                              float *value)	  /* The picked values */
/*
      * Pick a signal value using linear interpolation
      */
{
    int   n1,n2,k;
    float s1,s2;
    float f1,f2;
    float sum;
    float width;
    int   ch;

    for (ch = 0; ch < nch; ch++) {
        /*
     * Find out the correct samples
     */
        if (std::fabs(sfreq*integ) < EPS_VALUES) { /* This is the single-sample case */
            s1 = sfreq*(time - tmin);
            n1 = floor(s1);
            f1 = 1.0 + n1 - s1;
            if (n1 < 0 || n1 > nsamp-1) {
                printf("Sample value out of range %d (0..%d)",n1,nsamp-1);
                return(-1);
            }
            /*
       * Avoid rounding error
       */
            if (n1 == nsamp-1) {
                if (std::fabs(f1-1.0) < 1e-3)
                    f1 = 1.0;
            }
            if (f1 < 1.0 && n1 > nsamp-2) {
                printf("Sample value out of range %d (0..%d) %.4f",n1,nsamp-1,f1);
                return(-1);
            }
            if (f1 < 1.0) {
                if (use_abs)
                    sum = f1*std::fabs(data[n1][ch]) + (1.0-f1)*std::fabs(data[n1+1][ch]);
                else
                    sum = f1*data[n1][ch] + (1.0-f1)*data[n1+1][ch];
            }
            else {
                if (use_abs)
                    sum = std::fabs(data[n1][ch]);
                else
                    sum = data[n1][ch];
            }
        }
        else {			/* Multiple samples */
            s1 = sfreq*(time - 0.5*integ - tmin);
            s2 = sfreq*(time + 0.5*integ - tmin);
            n1 = ceil(s1); n2 = floor(s2);
            if (n2 < n1) {		/* We are within one sample interval */
                n1 = floor(s1);
                if (n1 < 0 || n1 > nsamp-2)
                    return (-1);
                f1 = s1 - n1;
                f2 = s2 - n1;
                if (use_abs)
                    sum = 0.5*((f1+f2)*std::fabs(data[n1+1][ch]) + (2.0-f1-f2)*std::fabs(data[n1][ch]));
                else
                    sum = 0.5*((f1+f2)*data[n1+1][ch] + (2.0-f1-f2)*data[n1][ch]);
            }
            else {
                f1 = n1 - s1;
                f2 = s2 - n2;
                if (n1 < 0 || n1 > nsamp-1) {
                    printf("Sample value out of range %d (0..%d)",n1,nsamp-1);
                    return(-1);
                }
                if (n2 < 0 || n2 > nsamp-1) {
                    printf("Sample value out of range %d (0..%d)",n2,nsamp-1);
                    return(-1);
                }
                if (f1 != 0.0 && n1 < 1)
                    return(-1);
                if (f2 != 0.0 && n2 > nsamp-2)
                    return(-1);
                sum = 0.0;
                width = 0.0;
                if (n2 > n1) {		/* Do the whole intervals */
                    if (use_abs) {
                        sum = 0.5 * std::fabs(data[n1][ch]);
                        for (k = n1+1; k < n2; k++)
                            sum = sum + std::fabs(data[k][ch]);
                        sum = sum + 0.5 * std::fabs(data[n2][ch]);
                    }
                    else {
                        sum = 0.5*data[n1][ch];
                        for (k = n1+1; k < n2; k++)
                            sum = sum + data[k][ch];
                        sum = sum + 0.5*data[n2][ch];
                    }
                    width = n2 - n1;
                }
                /*
         * Add tails
         */
                if (use_abs) {
                    if (f1 != 0.0)
                        sum = sum + 0.5 * f1 * (f1 * std::fabs(data[n1-1][ch]) + (2.0 - f1) * std::fabs(data[n1][ch]));
                    if (f2 != 0.0)
                        sum = sum + 0.5 * f2 * (f2 * std::fabs(data[n2+1][ch]) + (2.0 - f2) * std::fabs(data[n2][ch]));
                }
                else {
                    if (f1 != 0.0)
                        sum = sum + 0.5*f1*(f1*data[n1-1][ch] + (2.0-f1)*data[n1][ch]);
                    if (f2 != 0.0)
                        sum = sum + 0.5*f2*(f2*data[n2+1][ch] + (2.0-f2)*data[n2][ch]);
                }
                width = width + f1 + f2;
                sum = sum/width;
            }
        }
        value[ch] = sum;
    }
    return (0);
}

int mne_get_values_from_data_ch (float time,      /* Interesting time point */
                                 float integ,	  /* Time integration */
                                 float **data,	  /* The data values (channel by channel) */
                                 int   nsamp,	  /* How many time points? */
                                 int   nch,       /* How many channels */
                                 float tmin,	  /* Time of first sample */
                                 float sfreq,	  /* Sampling frequency */
                                 int   use_abs,   /* Use absolute values */
                                 float *value)	  /* The picked values */
/*
      * Pick a signal value using linear interpolation
      */
{
    int   n1,n2,k;
    float s1,s2;
    float f1,f2;
    float sum;
    float width;
    int   ch;

    for (ch = 0; ch < nch; ch++) {
        /*
     * Find out the correct samples
     */
        if (std::fabs(sfreq * integ) < EPS_VALUES) { /* This is the single-sample case */
            s1 = sfreq*(time - tmin);
            n1 = floor(s1);
            f1 = 1.0 + n1 - s1;
            if (n1 < 0 || n1 > nsamp-1)
                return(-1);
            if (f1 < 1.0 && n1 > nsamp-2)
                return(-1);
            if (f1 < 1.0) {
                if (use_abs)
                    sum = f1 * std::fabs(data[ch][n1]) + (1.0 - f1) * std::fabs(data[ch][n1+1]);
                else
                    sum = f1*data[ch][n1] + (1.0-f1)*data[ch][n1+1];
            }
            else {
                if (use_abs)
                    sum = std::fabs(data[ch][n1]);
                else
                    sum = data[ch][n1];
            }
        }
        else {			/* Multiple samples */
            s1 = sfreq*(time - 0.5*integ - tmin);
            s2 = sfreq*(time + 0.5*integ - tmin);
            n1 = ceil(s1); n2 = floor(s2);
            if (n2 < n1) {		/* We are within one sample interval */
                n1 = floor(s1);
                if (n1 < 0 || n1 > nsamp-2)
                    return (-1);
                f1 = s1 - n1;
                f2 = s2 - n1;
                if (use_abs)
                    sum = 0.5*((f1+f2)*std::fabs(data[ch][n1+1]) + (2.0-f1-f2)*std::fabs(data[ch][n1]));
                else
                    sum = 0.5*((f1+f2)*data[ch][n1+1] + (2.0-f1-f2)*data[ch][n1]);
            }
            else {
                f1 = n1 - s1;
                f2 = s2 - n2;
                if (n1 < 0 || n1 > nsamp-1 || n2 < 0 || n2 > nsamp-1)
                    return(-1);
                if (f1 != 0.0 && n1 < 1)
                    return(-1);
                if (f2 != 0.0 && n2 > nsamp-2)
                    return(-1);
                sum = 0.0;
                width = 0.0;
                if (n2 > n1) {		/* Do the whole intervals */
                    if (use_abs) {
                        sum = 0.5 * std::fabs(data[ch][n1]);
                        for (k = n1+1; k < n2; k++)
                            sum = sum + std::fabs(data[ch][k]);
                        sum = sum + 0.5 * std::fabs(data[ch][n2]);
                    }
                    else {
                        sum = 0.5*data[ch][n1];
                        for (k = n1+1; k < n2; k++)
                            sum = sum + data[ch][k];
                        sum = sum + 0.5*data[ch][n2];
                    }
                    width = n2 - n1;
                }
                /*
         * Add tails
         */
                if (use_abs) {
                    if (f1 != 0.0)
                        sum = sum + 0.5 * f1 * (f1 * std::fabs(data[ch][n1-1]) + (2.0 - f1) * std::fabs(data[ch][n1]));
                    if (f2 != 0.0)
                        sum = sum + 0.5 * f2 * (f2 * std::fabs(data[ch][n2+1]) + (2.0 - f2) * std::fabs(data[ch][n2]));
                }
                else {
                    if (f1 != 0.0)
                        sum = sum + 0.5*f1*(f1*data[ch][n1-1]+ (2.0-f1)*data[ch][n1]);
                    if (f2 != 0.0)
                        sum = sum + 0.5*f2*(f2*data[ch][n2+1] + (2.0-f2)*data[ch][n2]);
                }
                width = width + f1 + f2;
                sum = sum/width;
            }
        }
        value[ch] = sum;
    }
    return (0);
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DipoleFit::DipoleFit(DipoleFitSettings* p_settings)
: settings(p_settings)
{
}

//=============================================================================================================
//todo split in initFit where the settings are handed over and the actual fit
ECDSet DipoleFit::calculateFit() const
{
    QScopedPointer<GuessData>guess (Q_NULLPTR);
    ECDSet              set;
    FwdEegSphereModel*  eeg_model = NULL;
    DipoleFitData*      fit_data = NULL;
    MneMeasData*        data     = NULL;
    MneRawData*         raw      = NULL;
    mneChSelection      sel      = NULL;

    printf("---- Setting up...\n\n");
    if (settings->include_eeg) {
        if ((eeg_model = FwdEegSphereModel::setup_eeg_sphere_model(settings->eeg_model_file,settings->eeg_model_name,settings->eeg_sphere_rad)) == NULL)
            goto out;
    }

    if ((fit_data = DipoleFitData::setup_dipole_fit_data(   settings->mriname,
                                                            settings->measname,
                                                            settings->bemname.isEmpty() ? NULL : settings->bemname.toUtf8().data(),
                                                            &settings->r0,
                                                            eeg_model,
                                                            settings->accurate,
                                                            settings->badname,
                                                            settings->noisename,
                                                            settings->grad_std,
                                                            settings->mag_std,
                                                            settings->eeg_std,
                                                            settings->mag_reg,
                                                            settings->grad_reg,
                                                            settings->eeg_reg,
                                                            settings->diagnoise,
                                                            settings->projnames,
                                                            settings->include_meg,
                                                            settings->include_eeg)) == NULL)
        goto out;

    fit_data->fit_mag_dipoles = settings->fit_mag_dipoles;
    if (settings->is_raw) {
        int c;
        float t1,t2;

        printf("\n---- Opening a raw data file...\n\n");
        if ((raw = MneRawData::mne_raw_open_file(settings->measname.isEmpty() ? NULL : settings->measname.toUtf8().data(),TRUE,FALSE,&(settings->filter))) == NULL)
            goto out;
        /*
        * A channel selection is needed to access the data
        */
        sel = mne_ch_selection_these("fit",fit_data->ch_names,fit_data->nmeg+fit_data->neeg);
        mne_ch_selection_assign_chs(sel,raw);
        for (c = 0; c < sel->nchan; c++)
            if (sel->pick[c] < 0) {
                qCritical ("All desired channels were not available");
                goto out;
            }
        printf("\tChannel selection created.\n");
        /*
        * Let's be a little generous here
        */
        t1 = raw->first_samp/raw->info->sfreq;
        t2 = (raw->first_samp+raw->nsamp-1)/raw->info->sfreq;
        if (settings->tmin < t1 + settings->integ)
            settings->tmin = t1 + settings->integ;
        if (settings->tmax > t2 - settings->integ)
            settings->tmax =  t2 - settings->integ;
        if (settings->tstep < 0)
            settings->tstep = 1.0/raw->info->sfreq;

        printf("\tOpened raw data file %s : %d MEG and %d EEG \n",
               settings->measname.toUtf8().data(),fit_data->nmeg,fit_data->neeg);
    }
    else {
        printf("\n---- Reading data...\n\n");
        if ((data = MneMeasData::mne_read_meas_data(settings->measname,
                                                    settings->setno,
                                                    NULL,
                                                    NULL,
                                                    fit_data->ch_names,
                                                    fit_data->nmeg+fit_data->neeg)) == NULL)
            goto out;
        if (settings->do_baseline)
            data->adjust_baselines(settings->bmin,settings->bmax);
        else
            printf("\tNo baseline setting in effect.\n");
        if (settings->tmin < data->current->tmin + settings->integ/2.0)
            settings->tmin = data->current->tmin + settings->integ/2.0;
        if (settings->tmax > data->current->tmin + (data->current->np-1)*data->current->tstep - settings->integ/2.0)
            settings->tmax =  data->current->tmin + (data->current->np-1)*data->current->tstep - settings->integ/2.0;
        if (settings->tstep < 0)
            settings->tstep = data->current->tstep;

        printf("\tRead data set %d from %s : %d MEG and %d EEG \n",
               settings->setno,settings->measname.toUtf8().data(),fit_data->nmeg,fit_data->neeg);
        if (!settings->noisename.isEmpty()) {
            printf("\nScaling the noise covariance...\n");
            if (DipoleFitData::scale_noise_cov(fit_data,data->current->nave) == FAIL)
                goto out;
        }
    }

    /*
     * Proceed to computing the fits
     */
    printf("\n---- Computing the forward solution for the guesses...\n\n");
    guess.reset(new GuessData( settings->guessname,
                               settings->guess_surfname,
                               settings->guess_mindist, settings->guess_exclude, settings->guess_grid, fit_data));
    if (guess.isNull())
        goto out;

    fprintf (stderr,"\n---- Fitting : %7.1f ... %7.1f ms (step: %6.1f ms integ: %6.1f ms)\n\n",
             1000*settings->tmin,1000*settings->tmax,1000*settings->tstep,1000*settings->integ);

    if (raw) {
        if (fit_dipoles_raw(settings->measname,raw,sel,fit_data,guess.take(),settings->tmin,settings->tmax,settings->tstep,settings->integ,settings->verbose) == FAIL)
            goto out;
    }
    else {
        if (fit_dipoles(settings->measname,data,fit_data,guess.take(),settings->tmin,settings->tmax,settings->tstep,settings->integ,settings->verbose,set) == FAIL)
            goto out;
    }
    printf("%d dipoles fitted\n",set.size());

out : {
        return set;
    }
}

//=============================================================================================================

int DipoleFit::fit_dipoles( const QString& dataname, MneMeasData* data, DipoleFitData* fit, GuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose, ECDSet& p_set)
{
    float *one = MALLOC(data->nchan,float);
    float time;
    ECDSet set;
    ECD   dip;
    int   s;
    int   report_interval = 10;

    set.dataname = dataname;

    printf("Fitting...%c",verbose ? '\n' : '\0');
    for (s = 0, time = tmin; time < tmax; s++, time = tmin  + s*tstep) {
        /*
     * Pick the data point
     */
        if (mne_get_values_from_data(time,integ,data->current->data,data->current->np,data->nchan,data->current->tmin,
                                     1.0/data->current->tstep,FALSE,one) == FAIL) {
            printf("Cannot pick time: %7.1f ms\n",1000*time);
            continue;
        }

        if (!DipoleFitData::fit_one(fit,guess,time,one,verbose,dip))
            printf("t = %7.1f ms : %s\n",1000*time,"error (tbd: catch)");
        else {
            set.addEcd(dip);
            if (verbose)
                dip.print(stdout);
            else {
                if (set.size() % report_interval == 0)
                    printf("%d..",set.size());
            }
        }
    }
    if (!verbose)
        printf("[done]\n");
    FREE(one);
    p_set = set;
    return OK;
}

//=============================================================================================================

int DipoleFit::fit_dipoles_raw(const QString& dataname, MneRawData* raw, mneChSelection sel, DipoleFitData* fit, GuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose, ECDSet& p_set)
{
    float *one    = MALLOC(sel->nchan,float);
    float sfreq   = raw->info->sfreq;
    float myinteg = integ > 0.0 ? 2*integ : 0.1;
    int   overlap = ceil(myinteg*sfreq);
    int   length  = SEG_LEN*sfreq;
    int   step    = length - overlap;
    int   stepo   = step + overlap/2;
    int   start   = raw->first_samp;
    int   s,picks;
    float time,stime;
    float **data  = ALLOC_CMATRIX(sel->nchan,length);
    ECD    dip;
    ECDSet set;
    int    report_interval = 10;

    set.dataname = dataname;

    /*
   * Load the initial data segment
   */
    stime = start/sfreq;
    if (MneRawData::mne_raw_pick_data_filt(raw,sel,start,length,data) == FAIL)
        goto bad;
    printf("Fitting...%c",verbose ? '\n' : '\0');
    for (s = 0, time = tmin; time < tmax; s++, time = tmin  + s*tstep) {
        picks = time*sfreq - start;
        if (picks > stepo) {		/* Need a new data segment? */
            start = start + step;
            if (MneRawData::mne_raw_pick_data_filt(raw,sel,start,length,data) == FAIL)
                goto bad;
            picks = time*sfreq - start;
            stime = start/sfreq;
        }
        /*
     * Get the values
     */
        if (mne_get_values_from_data_ch (time,integ,data,length,sel->nchan,stime,sfreq,FALSE,one) == FAIL) {
            printf("Cannot pick time: %8.3f s\n",time);
            continue;
        }
        /*
     * Fit
     */
        if (!DipoleFitData::fit_one(fit,guess,time,one,verbose,dip))
            qWarning() << "Error";
        else {
            set.addEcd(dip);
            if (verbose)
                dip.print(stdout);
            else {
                if (set.size() % report_interval == 0)
                    printf("%d..",set.size());
            }
        }
    }
    if (!verbose)
        printf("[done]\n");
    FREE_CMATRIX(data);
    FREE(one);
    p_set = set;
    return OK;

bad : {
        FREE_CMATRIX(data);
        FREE(one);
        return FAIL;
    }
}

//=============================================================================================================

int DipoleFit::fit_dipoles_raw(const QString& dataname, MneRawData* raw, mneChSelection sel, DipoleFitData* fit, GuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose)
{
    ECDSet set;
    return fit_dipoles_raw(dataname, raw, sel, fit, guess, tmin, tmax, tstep, integ, verbose, set);
}
