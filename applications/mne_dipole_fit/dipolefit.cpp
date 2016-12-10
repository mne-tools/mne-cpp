

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

static char  *bemname     = NULL;		 /* Boundary-element model */
static float r0[]         = { 0.0,0.0,0.04 };    /* Sphere model origin  */
static int   accurate     = FALSE;		 /* Use accurate coil definitions? */
static char  *mriname     = NULL;		 /* Gives the MRI <-> head transform */

static char  *guessname   = NULL;		 /* Initial guess grid (if not present, the values below
                          * will be employed to generate the grid) */
static char  *guess_surfname = NULL;		 /* Load the inner skull surface from this BEM file */
static float guess_rad     = 0.080;              /* Radius of spherical guess surface */
static float guess_mindist = 0.010;		 /* Minimum allowed distance to the surface */
static float guess_exclude = 0.020;		 /* Exclude points closer than this to the origin */
static float guess_grid    = 0.010;		 /* Grid spacing */

static char  *noisename   = NULL;		 /* Noise-covariance matrix */
static float grad_std     = 5e-13;               /* Standard deviations to be used if noise covariance is not specified */
static float mag_std      = 20e-15;
static float eeg_std      = 0.2e-6;
static int   diagnoise    = FALSE;		 /* Use only the diagonals of the noise-covariance matrix */

static char  *measname    = NULL;		 /* Data file */
static int   is_raw       = FALSE;		 /* Is this a raw data file */
static char  *badname     = NULL;		 /* Bad channels */
static int   include_meg  = FALSE;		 /* Use MEG? */
static int   include_eeg  = FALSE;		 /* Use EEG? */
static float tmin         = -2*BIG_TIME;	 /* Possibility to set these from the command line */
static float tmax         = 2*BIG_TIME;
static float tstep        = -1.0;		 /* Step between fits */
static float integ        = 0.0;
static float bmin         = BIG_TIME;	         /* Possibility to set these from the command line */
static float bmax         = BIG_TIME;
static int   do_baseline  = FALSE;	         /* Are both baseline limits set? */
static int   setno        = 1;		         /* Which data set */
static int   verbose      = FALSE;
static mneFilterDefRec filter = { TRUE,		 /* Filter on? */
                  4096,		 /* size */
                  2048,		 /* taper_size */
                  0.0, 0.0,	 /* highpass corner and width */
                  40.0, 5.0,	 /* lowpass corner and width */
                  0.0, 0.0,	 /* EOG highpass corner and width */
                  40.0, 5.0 };	 /* EOG Lowpass corner and width */
static char **projnames   = NULL;                /* Projection file names */
static int  nproj         = 0;
static int  omit_data_proj = FALSE;

static char   *eeg_model_file = NULL;            /* File of EEG sphere model specifications */
static char   *eeg_model_name = NULL;		 /* Name of the EEG model to use */
static float  eeg_sphere_rad = 0.09;		 /* Scalp radius to use in EEG sphere model */
static int    scale_eeg_pos  = FALSE;	         /* Scale the electrode locations to scalp in the sphere model */
static float  mag_reg      = 0.1;                /* Noise-covariance matrix regularization for MEG (magnetometers and axial gradiometers)  */
static int   fit_mag_dipoles = FALSE;

static float  grad_reg     = 0.1;               /* Noise-covariance matrix regularization for EEG (planar gradiometers) */
static float  eeg_reg      = 0.1;               /* Noise-covariance matrix regularization for EEG  */
static char   *dipname     = NULL;		/* Output file in dip format */
static char   *bdipname    = NULL;		/* Output file in bdip format */










//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DipoleFit::DipoleFit(int *argc,char **argv)
{
    if (check_args(argc,argv) == FAIL)
        return;

    do_baseline = (bmin < BIG_TIME && bmax < BIG_TIME);

    if (!measname) {
        qCritical ("Data file name missing. Please specify one using the --meas option.");
        return;
    }
    if (!dipname && !bdipname) {
        qCritical ("Output file name missing. Please use the --dip or --bdip options to do this.");
        return;
    }
    if (!guessname) {
        if (!bemname && guess_surfname && !mriname) {
            qCritical ("Please specify the MRI/head coordinate transformation with the --mri option");
            return;
        }
    }
    if (!include_meg && !include_eeg) {
        qCritical ("Specify one or both of the --eeg and --meg options");
        return;
    }
    if (!omit_data_proj) {
        projnames = REALLOC(projnames,nproj+1,char *);
        nproj++;
        for (int k = 1; k < nproj; k++)
            projnames[k] = projnames[k-1];
        projnames[0] = mne_strdup(measname);
    }

    printf("\n");
//    mne_print_version_info(stderr,argv[0],PROGRAM_VERSION,__DATE__,__TIME__);
    printf("%s version %s\n",argv[0],PROGRAM_VERSION);//,__DATE__,__TIME__);

    if (bemname)
        printf("BEM              : %s\n",bemname);
    else {
        printf("Sphere model     : origin at (% 7.2f % 7.2f % 7.2f) mm\n",
        1000*r0[X],1000*r0[Y],1000*r0[Z]);
    }
    printf("Using %s MEG coil definitions.\n",accurate ? "accurate" : "standard");
    if (mriname)
        printf("MRI transform    : %s\n",mriname);
    if (guessname)
        printf("Guesses          : %s\n",guessname);
    else {
        if (guess_surfname)
          fprintf(stderr,"Guess space bounded by %s\n",guess_surfname);
        else
          fprintf(stderr,"Spherical guess space, rad = %.1f mm\n",1000*guess_rad);
        printf("Guess grid       : %6.1f mm\n",1000*guess_grid);
        if (guess_mindist > 0.0)
            printf("Guess mindist    : %6.1f mm\n",1000*guess_mindist);
        if (guess_exclude > 0)
            printf("Guess exclude    : %6.1f mm\n",1000*guess_exclude);
    }
    printf("Data             : %s\n",measname);
    if (nproj > 0) {
        printf("SSP sources      :\n");
    for (int k = 0; k < nproj; k++)
        printf("\t%s\n",projnames[k]);
    }
    if (badname)
        printf("Bad channels     : %s\n",badname);
    if (do_baseline)
        printf("Baseline         : %10.2f ... %10.2f ms\n",
    1000*bmin,1000*bmax);
    if (noisename) {
        printf("Noise covariance : %s\n",noisename);
        if (include_meg) {
            if (mag_reg > 0.0)
                printf("\tNoise-covariange regularization (mag)     : %-5.2f\n",mag_reg);
            if (grad_reg > 0.0)
                printf("\tNoise-covariange regularization (grad)    : %-5.2f\n",grad_reg);
        }
        if (include_eeg && eeg_reg > 0.0)
            printf("\tNoise-covariange regularization (EEG)     : %-5.2f\n",eeg_reg);
    }
    if (fit_mag_dipoles)
        printf("Fit data with magnetic dipoles\n");
    if (dipname)
        printf("dip output      : %s\n",dipname);
    if (bdipname)
        printf("bdip output     : %s\n",bdipname);
    printf("\n");
}


//*************************************************************************************************************

bool DipoleFit::calculateFit()
{
    int                 res      = FAIL;
    guessData           guess    = NULL;
    ECDSet              set;
    fwdEegSphereModel   eeg_model = NULL;
    dipoleFitData       fit_data = NULL;
    mneMeasData         data     = NULL;
    mneRawData          raw      = NULL;
    mneChSelection      sel      = NULL;

    printf("---- Setting up...\n\n");
    if (include_eeg) {
        if ((eeg_model = setup_eeg_sphere_model(eeg_model_file,eeg_model_name,eeg_sphere_rad)) == NULL)
            goto out;
    }

    if ((fit_data = setup_dipole_fit_data(mriname,measname,bemname,r0,eeg_model,accurate,
        badname,noisename,grad_std,mag_std,eeg_std,
        mag_reg,grad_reg,eeg_reg,
        diagnoise,projnames,nproj,include_meg,include_eeg)) == NULL)
        goto out;

    fit_data->fit_mag_dipoles = fit_mag_dipoles;
    if (is_raw) {
        int c;
        float t1,t2;

        printf("\n---- Opening a raw data file...\n\n");
        if ((raw = mne_raw_open_file(measname,TRUE,FALSE,&filter)) == NULL)
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
            if (tmin < t1 + integ)
            tmin = t1 + integ;
            if (tmax > t2 - integ)
            tmax =  t2 - integ;
            if (tstep < 0)
            tstep = 1.0/raw->info->sfreq;

            printf("\tOpened raw data file %s : %d MEG and %d EEG \n",
            measname,fit_data->nmeg,fit_data->neeg);
        }
    else {
        printf("\n---- Reading data...\n\n");
        if ((data = mne_read_meas_data(measname,setno,NULL,NULL,
            fit_data->ch_names,fit_data->nmeg+fit_data->neeg)) == NULL)
            goto out;
        if (do_baseline)
            mne_adjust_baselines(data,bmin,bmax);
        else
            printf("\tNo baseline setting in effect.\n");
        if (tmin < data->current->tmin + integ/2.0)
            tmin = data->current->tmin + integ/2.0;
        if (tmax > data->current->tmin + (data->current->np-1)*data->current->tstep - integ/2.0)
            tmax =  data->current->tmin + (data->current->np-1)*data->current->tstep - integ/2.0;
        if (tstep < 0)
            tstep = data->current->tstep;

        printf("\tRead data set %d from %s : %d MEG and %d EEG \n",
        setno,measname,fit_data->nmeg,fit_data->neeg);
        if (noisename) {
            printf("\nScaling the noise covariance...\n");
            if (scale_noise_cov(fit_data,data->current->nave) == FAIL)
                goto out;
        }
    }

    /*
    * Proceed to computing the fits
    */
    printf("\n---- Computing the forward solution for the guesses...\n\n");
    if ((guess = make_guess_data(guessname, guess_surfname, guess_mindist, guess_exclude, guess_grid, fit_data)) == NULL)
    goto out;

    fprintf (stderr,"\n---- Fitting : %7.1f ... %7.1f ms (step: %6.1f ms integ: %6.1f ms)\n\n",
    1000*tmin,1000*tmax,1000*tstep,1000*integ);


    if (raw) {
        if (fit_dipoles_raw(measname,raw,sel,fit_data,guess,tmin,tmax,tstep,integ,verbose) == FAIL)
            goto out;
    }
    else {
        if (fit_dipoles(measname,data,fit_data,guess,tmin,tmax,tstep,integ,verbose,set) == FAIL)
            goto out;
    }
    printf("%d dipoles fitted\n",set.size());

    /*
    * Saving...
    */
    if (save_dipoles_dip(dipname,set) == FAIL)
    goto out;
    if (save_dipoles_bdip(bdipname,set) == FAIL)
    goto out;
//    free_ecd_set(set);
    res = OK;

    out : {
        if (res == FAIL) {
            return false;
        }
        else
            return true;
    }
}


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

void DipoleFit::usage(char *name)
{
    printf("usage: %s [options]\n",name);
    printf("This is a program for sequential single dipole fitting.\n");
    printf("\nInput data:\n\n");
    printf("\t--meas name       specify an evoked-response data file\n");
    printf("\t--set   no        evoked data set number to use (default: 1)\n");
    printf("\t--bad name        take bad channel list from here\n");

    printf("\nModality selection:\n\n");
    printf("\t--meg             employ MEG data in fitting\n");
    printf("\t--eeg             employ EEG data in fitting\n");

    printf("\nTime scale selection:\n\n");
    printf("\t--tmin  time/ms   specify the starting analysis time\n");
    printf("\t--tmax  time/ms   specify the ending analysis time\n");
    printf("\t--tstep time/ms   specify the time step between frames (default 1/(sampling frequency))\n");
    printf("\t--integ time/ms   specify the time integration for each frame (default 0)\n");

    printf("\nPreprocessing:\n\n");
    printf("\t--bmin  time/ms   specify the baseline starting time (evoked data only)\n");
    printf("\t--bmax  time/ms   specify the baseline ending time (evoked data only)\n");
    printf("\t--proj name       Load the linear projection from here\n");
    printf("\t                  Multiple projections can be specified.\n");
    printf("\t                  The data file will be automatically included, unless --noproj is present.\n");
    printf("\t--noproj          Do not load the projection from the data file, just those given with the --proj option.\n");
    printf("\n\tFiltering (raw data only):\n\n");
    printf("\t--filtersize size desired filter length (default = %d)\n",filter.size);
    printf("\t--highpass val/Hz highpass corner (default = %6.1f Hz)\n",filter.highpass);
    printf("\t--lowpass  val/Hz lowpass  corner (default = %6.1f Hz)\n",filter.lowpass);
    printf("\t--lowpassw val/Hz lowpass transition width (default = %6.1f Hz)\n",filter.lowpass_width);
    printf("\t--filteroff       do not filter the data\n");

    printf("\nNoise specification:\n\n");
    printf("\t--noise name      take the noise-covariance matrix from here\n");
    printf("\t--gradnoise val   specify a gradiometer noise value in fT/cm\n");
    printf("\t--magnoise val    specify a gradiometer noise value in fT\n");
    printf("\t--eegnoise val    specify an EEG value in uV\n");
    printf("\t                  NOTE: The above will be used only if --noise is missing\n");
    printf("\t--diagnoise       omit off-diagonal terms from the noise-covariance matrix\n");
    printf("\t--reg amount      Apply regularization to the noise-covariance matrix (same fraction for all channels).\n");
    printf("\t--gradreg amount  Apply regularization to the MEG noise-covariance matrix (planar gradiometers, default = %6.2f).\n",grad_reg);
    printf("\t--magreg amount   Apply regularization to the EEG noise-covariance matrix (axial gradiometers and magnetometers, default = %6.2f).\n",mag_reg);
    printf("\t--eegreg amount   Apply regularization to the EEG noise-covariance matrix (default = %6.2f).\n",eeg_reg);


    printf("\nForward model:\n\n");
    printf("\t--mri name        take head/MRI coordinate transform from here (Neuromag MRI description file)\n");
    printf("\t--bem  name       BEM model name\n");
    printf("\t--origin x:y:z/mm use a sphere model with this origin (head coordinates/mm)\n");
    printf("\t--eegscalp        scale the electrode locations to the surface of the scalp when using a sphere model\n");
    printf("\t--eegmodels name  read EEG sphere model specifications from here.\n");
    printf("\t--eegmodel  name  name of the EEG sphere model to use (default : Default)\n");
    printf("\t--eegrad val      radius of the scalp surface to use in EEG sphere model (default : %7.1f mm)\n",1000*eeg_sphere_rad);
    printf("\t--accurate        use accurate coil definitions in MEG forward computation\n");

    printf("\nFitting parameters:\n\n");
    printf("\t--guess name      The source space of initial guesses.\n");
    printf("\t                  If not present, the values below are used to generate the guess grid.\n");
    printf("\t--gsurf   name    Read the inner skull surface from this fif file to generate the guesses.\n");
    printf("\t--exclude dist/mm Exclude points which are closer than this distance from the CM of the inner skull surface (default =  %6.1f mm).\n",1000*guess_exclude);
    printf("\t--mindist dist/mm Exclude points which are closer than this distance from the inner skull surface  (default = %6.1f mm).\n",1000*guess_mindist);
    printf("\t--grid    dist/mm Source space grid size (default = %6.1f mm).\n",1000*guess_grid);
    printf("\t--magdip          Fit magnetic dipoles instead of current dipoles.\n");
    printf("\nOutput:\n\n");
    printf("\t--dip     name    xfit dip format output file name\n");
    printf("\t--bdip    name    xfit bdip format output file name\n");
    printf("\nGeneral:\n\n");
    printf("\t--help            print this info.\n");
    printf("\t--version         print version info.\n\n");
    return;
}


//*************************************************************************************************************

int DipoleFit::check_unrecognized_args(int argc, char **argv)
{
    if ( argc > 1 ) {
        printf("Unrecognized arguments : ");
        for (int k = 1; k < argc; k++)
            printf("%s ",argv[k]);
        printf("\n");
        qCritical ("Check the command line.");
        return FAIL;
    }
    return OK;
}


//*************************************************************************************************************

int DipoleFit::check_args (int *argc,char **argv)
{
    int found;
    float fval;
    int   ival,filter_size;

    for (int k = 0; k < *argc; k++) {
        found = 0;
        if (strcmp(argv[k],"--version") == 0) {
            printf("%s version %s compiled at %s %s\n",
            argv[0],PROGRAM_VERSION,__DATE__,__TIME__);
            exit(0);
        }
        else if (strcmp(argv[k],"--help") == 0) {
            usage(argv[0]);
            exit(1);
        }
        else if (strcmp(argv[k],"--guess") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--guess: argument required.");
                return FAIL;
            }
            guessname = strdup(argv[k+1]);
        }
        else if (strcmp(argv[k],"--gsurf") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--gsurf: argument required.");
                return FAIL;
            }
            guess_surfname = strdup(argv[k+1]);
        }
        else if (strcmp(argv[k],"--mindist") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--mindist: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%f",&fval) != 1) {
                qCritical ("Could not interpret the distance.");
                return FAIL;
            }
            guess_mindist = fval/1000.0;
            if (guess_mindist <= 0.0)
                guess_mindist = 0.0;
        }
        else if (strcmp(argv[k],"--exclude") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--exclude: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%f",&fval) != 1) {
                qCritical ("Could not interpret the distance.");
                return FAIL;
            }
            guess_exclude = fval/1000.0;
            if (guess_exclude <= 0.0)
                guess_exclude = 0.0;
        }
        else if (strcmp(argv[k],"--grid") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--grid: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%f",&fval) != 1) {
                qCritical ("Could not interpret the distance.");
                return FAIL;
            }
            if (fval <= 0.0) {
                qCritical ("Grid spacing should be positive");
                return FAIL;
            }
            guess_grid = guess_grid/1000.0;
        }
        else if (strcmp(argv[k],"--mri") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--mri: argument required.");
                return FAIL;
            }
            mriname = strdup(argv[k+1]);
        }
        else if (strcmp(argv[k],"--bem") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--bem: argument required.");
                return FAIL;
            }
            bemname = strdup(argv[k+1]);
        }
        else if (strcmp(argv[k],"--accurate") == 0) {
            found = 1;
            accurate = TRUE;
        }
        else if (strcmp(argv[k],"--meg") == 0) {
            found = 1;
            include_meg = TRUE;
        }
        else if (strcmp(argv[k],"--eeg") == 0) {
            found = 1;
            include_eeg = TRUE;
        }
        else if (strcmp(argv[k],"--origin") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--origin: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%f:%f:%f",r0+X,r0+Y,r0+Z) != 3) {
                qCritical ("Could not interpret the origin.");
                return FAIL;
            }
            r0[X] = r0[X]/1000.0;
            r0[Y] = r0[Y]/1000.0;
            r0[Z] = r0[Z]/1000.0;
        }
        else if (strcmp(argv[k],"--eegrad") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--eegrad: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&eeg_sphere_rad) != 1) {
                qCritical () << "Incomprehensible radius:" << argv[k+1];
                return FAIL;
            }
            if (eeg_sphere_rad <= 0) {
                qCritical ("Radius must be positive");
                return FAIL;
            }
            eeg_sphere_rad = eeg_sphere_rad/1000.0;
        }
        else if (strcmp(argv[k],"--eegmodels") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--eegmodels: argument required.");
                return FAIL;
            }
            eeg_model_file = strdup(argv[k+1]);
        }
        else if (strcmp(argv[k],"--eegmodel") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--eegmodel: argument required.");
                return FAIL;
            }
            eeg_model_name = strdup(argv[k+1]);
        }
        else if (strcmp(argv[k],"--eegscalp") == 0) {
            found         = 1;
            scale_eeg_pos = TRUE;
        }
        else if (strcmp(argv[k],"--meas") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--meas: argument required.");
                return FAIL;
            }
            measname = strdup(argv[k+1]);
            is_raw = FALSE;
        }
        else if (strcmp(argv[k],"--raw") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--raw: argument required.");
                return FAIL;
            }
            measname = strdup(argv[k+1]);
            is_raw = TRUE;
        }
        else if (strcmp(argv[k],"--proj") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--proj: argument required.");
                return FAIL;
            }
            projnames = REALLOC(projnames,nproj+1,char *);
            projnames[nproj++] = strdup(argv[k+1]);
        }
        else if (strcmp(argv[k],"--noproj") == 0) {
            found = 1;
            omit_data_proj = TRUE;
        }
        else if (strcmp(argv[k],"--bad") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--bad: argument required.");
                return FAIL;
            }
            badname = strdup(argv[k+1]);
        }
        else if (strcmp(argv[k],"--noise") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--noise: argument required.");
                return FAIL;
            }
            noisename = strdup(argv[k+1]);
        }
        else if (strcmp(argv[k],"--gradnoise") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--gradnoise: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible value:" << argv[k+1];
                return FAIL;
            }
            if (fval < 0.0) {
                qCritical ("Value should be positive");
                return FAIL;
            }
            grad_std = 1e-13*fval;
        }
        else if (strcmp(argv[k],"--magnoise") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--magnoise: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible value:" << argv[k+1];
                return FAIL;
            }
            if (fval < 0.0) {
                qCritical ("Value should be positive");
                return FAIL;
            }
            mag_std = 1e-15*fval;
        }
        else if (strcmp(argv[k],"--eegnoise") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--eegnoise: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical () << "Incomprehensible value:" << argv[k+1];
                return FAIL;
            }
            if (fval < 0.0) {
                qCritical ("Value should be positive");
                return FAIL;
            }
            eeg_std = 1e-6*fval;
        }
        else if (strcmp(argv[k],"--diagnoise") == 0) {
            found = 1;
            diagnoise = TRUE;
        }
        else if (strcmp(argv[k],"--eegreg") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--eegreg: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical () << "Incomprehensible value:" << argv[k+1];
                return FAIL;
            }
            if (fval < 0 || fval > 1) {
                qCritical ("Regularization value should be positive and smaller than one.");
                return FAIL;
            }
            eeg_reg = fval;
        }
        else if (strcmp(argv[k],"--magreg") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--magreg: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical () << "Incomprehensible value:" << argv[k+1];
                return FAIL;
            }
            if (fval < 0 || fval > 1) {
                qCritical ("Regularization value should be positive and smaller than one.");
                return FAIL;
            }
            mag_reg = fval;
        }
        else if (strcmp(argv[k],"--gradreg") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--gradreg: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical () << "Incomprehensible value:" << argv[k+1] ;
                return FAIL;
            }
            if (fval < 0 || fval > 1) {
                qCritical ("Regularization value should be positive and smaller than one.");
                return FAIL;
            }
            grad_reg = fval;
        }
        else if (strcmp(argv[k],"--reg") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--reg: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical () << "Incomprehensible value:" << argv[k+1];
                return FAIL;
            }
            if (fval < 0 || fval > 1) {
                qCritical ("Regularization value should be positive and smaller than one.");
                return FAIL;
            }
            grad_reg = fval;
            mag_reg = fval;
            eeg_reg = fval;
        }
        else if (strcmp(argv[k],"--tstep") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--tstep: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible tstep:" << argv[k+1];
                return FAIL;
            }
            if (fval < 0.0) {
                qCritical ("Time step should be positive");
                return FAIL;
            }
            tstep = fval/1000.0;
        }
        else if (strcmp(argv[k],"--integ") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--integ: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible integration time:" << argv[k+1];
                return FAIL;
            }
            if (fval <= 0.0) {
                qCritical ("Integration time should be positive.");
                return FAIL;
            }
            integ = fval/1000.0;
        }
        else if (strcmp(argv[k],"--tmin") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--tmin: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible tmin:" << argv[k+1];
                return FAIL;
            }
            tmin = fval/1000.0;
        }
        else if (strcmp(argv[k],"--tmax") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--tmax: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible tmax:" << argv[k+1];
                return FAIL;
            }
            tmax = fval/1000.0;
        }
        else if (strcmp(argv[k],"--bmin") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--bmin: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible bmin:" << argv[k+1];
                return FAIL;
            }
            bmin = fval/1000.0;
        }
        else if (strcmp(argv[k],"--bmax") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--bmax: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible bmax:" << argv[k+1];
                return FAIL;
            }
            bmax = fval/1000.0;
        }
        else if (strcmp(argv[k],"--set") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--set: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%d",&setno) != 1) {
                qCritical() << "Incomprehensible data set number:" << argv[k+1];
                return FAIL;
            }
            if (setno <= 0) {
                qCritical ("Data set number must be > 0");
                return FAIL;
            }
        }
        else if (strcmp(argv[k],"--filteroff") == 0) {
            found = 1;
            filter.filter_on = FALSE;
        }
        else if (strcmp(argv[k],"--lowpass") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--lowpass: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Illegal number:" << argv[k+1];
                return FAIL;
            }
            if (fval <= 0) {
                qCritical ("Lowpass corner must be positive");
                return FAIL;
            }
            filter.lowpass = fval;
        }
        else if (strcmp(argv[k],"--lowpassw") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--lowpassw: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Illegal number:" << argv[k+1];
                return FAIL;
            }
            if (fval <= 0) {
                qCritical ("Lowpass width must be positive");
                return FAIL;
            }
            filter.lowpass_width = fval;
        }
        else if (strcmp(argv[k],"--highpass") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--highpass: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Illegal number:" << argv[k+1];
                return FAIL;
            }
            if (fval <= 0) {
                qCritical ("Highpass corner must be positive");
                return FAIL;
            }
            filter.highpass = fval;
        }
        else if (strcmp(argv[k],"--filtersize") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--filtersize: argument required.");
                return FAIL;
            }
            if (sscanf(argv[k+1],"%d",&ival) != 1) {
                qCritical() << "Illegal number:" << argv[k+1];
                return FAIL;
            }
            if (ival < 1024) {
                qCritical ("Filtersize should be at least 1024.");
                return FAIL;
            }
            for (filter_size = 1024; filter_size < ival; filter_size = 2*filter_size)
                ;
            filter.size       = filter_size;
            filter.taper_size = filter_size/2;
        }
        else if (strcmp(argv[k],"--magdip") == 0) {
            found = 1;
            fit_mag_dipoles = TRUE;
        }
        else if (strcmp(argv[k],"--dip") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--dip: argument required.");
                return FAIL;
            }
            dipname = strdup(argv[k+1]);
        }
        else if (strcmp(argv[k],"--bdip") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--bdip: argument required.");
                return FAIL;
            }
            bdipname = strdup(argv[k+1]);
        }
        else if (strcmp(argv[k],"--verbose") == 0) {
            found = 1;
            verbose = TRUE;
        }
        if (found) {
            for (int p = k; p < *argc-found; p++)
                argv[p] = argv[p+found];
            *argc = *argc - found;
            k = k - found;
        }
    }
    return check_unrecognized_args(*argc,argv);
}
