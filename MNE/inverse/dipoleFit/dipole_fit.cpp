

#include "dipole_fit.h"

#include "dipolefit_helpers.h"


using namespace INVERSELIB;


//*************************************************************************************************************
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


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS ToDo make members
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DipoleFit::DipoleFit(DipoleFitSettings* p_settings)
: settings(p_settings)
{
}


//*************************************************************************************************************
//todo split in initFit where the settings are handed over and the actual fit
ECDSet DipoleFit::calculateFit() const
{
    GuessData*          guess    = NULL;
    ECDSet              set;
    FwdEegSphereModel*  eeg_model = NULL;
    DipoleFitData*      fit_data = NULL;
    MneMeasData*        data     = NULL;
    mneRawData          raw      = NULL;
    mneChSelection      sel      = NULL;

    printf("---- Setting up...\n\n");
    if (settings->include_eeg) {
        if ((eeg_model = FwdEegSphereModel::setup_eeg_sphere_model(settings->eeg_model_file,settings->eeg_model_name,settings->eeg_sphere_rad)) == NULL)
            goto out;
    }

    if ((fit_data = setup_dipole_fit_data(settings->mriname,
                                          settings->measname,
                                          settings->bemname.isEmpty() ? NULL : settings->bemname.toLatin1().data(),
                                          &settings->r0,eeg_model,settings->accurate,
                                          settings->badname,
                                          settings->noisename,
                                          settings->grad_std,settings->mag_std,settings->eeg_std,
                                          settings->mag_reg,settings->grad_reg,settings->eeg_reg,
                                          settings->diagnoise,settings->projnames,settings->include_meg,settings->include_eeg)) == NULL)
        goto out;

    fit_data->fit_mag_dipoles = settings->fit_mag_dipoles;
    if (settings->is_raw) {
        int c;
        float t1,t2;

        printf("\n---- Opening a raw data file...\n\n");
        if ((raw = mne_raw_open_file(settings->measname.isEmpty() ? NULL : settings->measname.toLatin1().data(),TRUE,FALSE,&(settings->filter))) == NULL)
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
               settings->measname.toLatin1().data(),fit_data->nmeg,fit_data->neeg);
    }
    else {
        printf("\n---- Reading data...\n\n");
        if ((data = mne_read_meas_data(settings->measname,settings->setno,NULL,NULL,
                                       fit_data->ch_names,fit_data->nmeg+fit_data->neeg)) == NULL)
            goto out;
        if (settings->do_baseline)
            mne_adjust_baselines(data,settings->bmin,settings->bmax);
        else
            printf("\tNo baseline setting in effect.\n");
        if (settings->tmin < data->current->tmin + settings->integ/2.0)
            settings->tmin = data->current->tmin + settings->integ/2.0;
        if (settings->tmax > data->current->tmin + (data->current->np-1)*data->current->tstep - settings->integ/2.0)
            settings->tmax =  data->current->tmin + (data->current->np-1)*data->current->tstep - settings->integ/2.0;
        if (settings->tstep < 0)
            settings->tstep = data->current->tstep;

        printf("\tRead data set %d from %s : %d MEG and %d EEG \n",
               settings->setno,settings->measname.toLatin1().data(),fit_data->nmeg,fit_data->neeg);
        if (!settings->noisename.isEmpty()) {
            printf("\nScaling the noise covariance...\n");
            if (scale_noise_cov(fit_data,data->current->nave) == FAIL)
                goto out;
        }
    }

    /*
    * Proceed to computing the fits
    */
    printf("\n---- Computing the forward solution for the guesses...\n\n");
    if ((guess = make_guess_data(settings->guessname,
                                 settings->guess_surfname,
                                 settings->guess_mindist, settings->guess_exclude, settings->guess_grid, fit_data)) == NULL)
        goto out;

    fprintf (stderr,"\n---- Fitting : %7.1f ... %7.1f ms (step: %6.1f ms integ: %6.1f ms)\n\n",
             1000*settings->tmin,1000*settings->tmax,1000*settings->tstep,1000*settings->integ);


    if (raw) {
        if (fit_dipoles_raw(settings->measname,raw,sel,fit_data,guess,settings->tmin,settings->tmax,settings->tstep,settings->integ,settings->verbose) == FAIL)
            goto out;
    }
    else {
        if (fit_dipoles(settings->measname,data,fit_data,guess,settings->tmin,settings->tmax,settings->tstep,settings->integ,settings->verbose,set) == FAIL)
            goto out;
    }
    printf("%d dipoles fitted\n",set.size());


out : {
        return set;
    }
}


//*************************************************************************************************************

int DipoleFit::fit_dipoles( const QString& dataname, MneMeasData* data, DipoleFitData* fit, GuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose, ECDSet& p_set)
{
    float *one = MALLOC(data->nchan,float);
    float time;
    ECDSet set;
    ECD   dip;
    int   s;
    int   report_interval = 10;

    set.dataname = dataname;

    fprintf(stderr,"Fitting...%c",verbose ? '\n' : '\0');
    for (s = 0, time = tmin; time < tmax; s++, time = tmin  + s*tstep) {
        /*
     * Pick the data point
     */
        if (mne_get_values_from_data(time,integ,data->current->data,data->current->np,data->nchan,data->current->tmin,
                                     1.0/data->current->tstep,FALSE,one) == FAIL) {
            fprintf(stderr,"Cannot pick time: %7.1f ms\n",1000*time);
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
                    fprintf(stderr,"%d..",set.size());
            }
        }
    }
    if (!verbose)
        fprintf(stderr,"[done]\n");
    FREE(one);
    p_set = set;
    return OK;
}


//*************************************************************************************************************

int DipoleFit::fit_dipoles_raw(const QString& dataname, mneRawData raw, mneChSelection sel, DipoleFitData* fit, GuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose, ECDSet& p_set)
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
    if (mne_raw_pick_data_filt(raw,sel,start,length,data) == FAIL)
        goto bad;
    fprintf(stderr,"Fitting...%c",verbose ? '\n' : '\0');
    for (s = 0, time = tmin; time < tmax; s++, time = tmin  + s*tstep) {
        picks = time*sfreq - start;
        if (picks > stepo) {		/* Need a new data segment? */
            start = start + step;
            if (mne_raw_pick_data_filt(raw,sel,start,length,data) == FAIL)
                goto bad;
            picks = time*sfreq - start;
            stime = start/sfreq;
        }
        /*
     * Get the values
     */
        if (mne_get_values_from_data_ch (time,integ,data,length,sel->nchan,stime,sfreq,FALSE,one) == FAIL) {
            fprintf(stderr,"Cannot pick time: %8.3f s\n",time);
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
                    fprintf(stderr,"%d..",set.size());
            }
        }
    }
    if (!verbose)
        fprintf(stderr,"[done]\n");
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


//*************************************************************************************************************

int DipoleFit::fit_dipoles_raw(const QString& dataname, mneRawData raw, mneChSelection sel, DipoleFitData* fit, GuessData* guess, float tmin, float tmax, float tstep, float integ, int verbose)
{
    ECDSet set;
    return fit_dipoles_raw(dataname, raw, sel, fit, guess, tmin, tmax, tstep, integ, verbose, set);
}
