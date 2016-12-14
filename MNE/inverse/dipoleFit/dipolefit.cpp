

#include "dipolefit.h"

#include "dipolefit_helpers.cpp"


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

ECDSet DipoleFit::calculateFit() const
{
    GuessData*           guess    = NULL;
    ECDSet              set;
    FwdEegSphereModel*  eeg_model = NULL;
    dipoleFitData       fit_data = NULL;
    mneMeasData         data     = NULL;
    mneRawData          raw      = NULL;
    mneChSelection      sel      = NULL;

    printf("---- Setting up...\n\n");
    if (settings->include_eeg) {
        if ((eeg_model = setup_eeg_sphere_model(settings->eeg_model_file,settings->eeg_model_name,settings->eeg_sphere_rad)) == NULL)
            goto out;
    }

    if ((fit_data = setup_dipole_fit_data(settings->mriname.isEmpty() ? NULL : settings->mriname.toLatin1().data(),
                                          settings->measname.isEmpty() ? NULL : settings->measname.toLatin1().data(),
                                          settings->bemname.isEmpty() ? NULL : settings->bemname.toLatin1().data(),
                                          &settings->r0,eeg_model,settings->accurate,
                                          settings->badname,
                                          settings->noisename.isEmpty() ? NULL : settings->noisename.toLatin1().data(),
                                          settings->grad_std,settings->mag_std,settings->eeg_std,
                                          settings->mag_reg,settings->grad_reg,settings->eeg_reg,
                                          settings->diagnoise,settings->projnames,settings->nproj,settings->include_meg,settings->include_eeg)) == NULL)
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
        if ((data = mne_read_meas_data(settings->measname.isEmpty() ? NULL : settings->measname.toLatin1().data(),settings->setno,NULL,NULL,
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
    if ((guess = make_guess_data(settings->guessname.toLatin1().data() ? NULL : settings->guessname.toLatin1().data(),
                                 settings->guess_surfname.toLatin1().data() ? NULL : settings->guess_surfname.toLatin1().data(),
                                 settings->guess_mindist, settings->guess_exclude, settings->guess_grid, fit_data)) == NULL)
        goto out;

    fprintf (stderr,"\n---- Fitting : %7.1f ... %7.1f ms (step: %6.1f ms integ: %6.1f ms)\n\n",
             1000*settings->tmin,1000*settings->tmax,1000*settings->tstep,1000*settings->integ);


    if (raw) {
        if (fit_dipoles_raw(settings->measname.isEmpty() ? NULL : settings->measname.toLatin1().data(),raw,sel,fit_data,guess,settings->tmin,settings->tmax,settings->tstep,settings->integ,settings->verbose) == FAIL)
            goto out;
    }
    else {
        if (fit_dipoles(settings->measname.isEmpty() ? NULL : settings->measname.toLatin1().data(),data,fit_data,guess,settings->tmin,settings->tmax,settings->tstep,settings->integ,settings->verbose,set) == FAIL)
            goto out;
    }
    printf("%d dipoles fitted\n",set.size());


out : {
        return set;
    }
}
