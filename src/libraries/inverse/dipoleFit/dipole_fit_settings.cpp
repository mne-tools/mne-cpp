

#include "dipole_fit_settings.h"

using namespace Eigen;
using namespace INVERSELIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

/*
 * Basics...
 */
#define MALLOC(x,t) (t *)malloc((x)*sizeof(t))
#define REALLOC(x,y,t) (t *)((x == NULL) ? malloc((y)*sizeof(t)) : realloc((x),(y)*sizeof(t)))

#define X 0
#define Y 1
#define Z 2

#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION     "1.00"
#endif

//=============================================================================================================
// STATIC DEFINITIONS ToDo make members
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DipoleFitSettings::DipoleFitSettings()
{
    initMembers();
}

//=============================================================================================================

DipoleFitSettings::DipoleFitSettings(int *argc,char **argv)
{
    initMembers();

    if (!check_args(argc,argv))
        return;

//    mne_print_version_info(stderr,argv[0],PROGRAM_VERSION,__DATE__,__TIME__);
    printf("%s version %s compiled at %s %s\n",argv[0],PROGRAM_VERSION,__DATE__,__TIME__);

    checkIntegrity();
}

//=============================================================================================================

DipoleFitSettings::~DipoleFitSettings()
{
    //ToDo Garbage collection
}

//=============================================================================================================

void DipoleFitSettings::initMembers()
{
    // Init origin
    r0 << 0.0f,0.0f,0.04f;

    filter.filter_on = true;
    filter.size = 4096;
    filter.taper_size = 2048;
    filter.highpass = 0.0;
    filter.highpass_width = 0.0;
    filter.lowpass = 40.0;
    filter.lowpass_width = 5.0;
    filter.eog_highpass = 0.0;
    filter.eog_highpass_width = 0.0;
    filter.eog_lowpass = 40.0;
    filter.eog_lowpass_width = 5.0;

    accurate    = false;         /**< Use accurate coil definitions?. */

    guess_rad     = 0.080f;       
    guess_mindist = 0.010f;       
    guess_exclude = 0.020f;       
    guess_grid    = 0.010f;      

    grad_std     = 5e-13f;        
    mag_std      = 20e-15f;
    eeg_std      = 0.2e-6f;
    diagnoise    = false;         

    is_raw       = false;         
    badname     = NULL;          
    include_meg  = false;         
    include_eeg  = false;        
    tmin         = -2*BIG_TIME;   
    tmax         = 2*BIG_TIME;
    tstep        = -1.0;          
    integ        = 0.0;
    bmin         = BIG_TIME;      
    bmax         = BIG_TIME;
    do_baseline  = false;         
    setno        = 1;             
    verbose      = false;
    omit_data_proj = false;

         
    eeg_sphere_rad = 0.09f;      
    scale_eeg_pos  = false;     
    mag_reg      = 0.1f;         
    fit_mag_dipoles = false;

    grad_reg     = 0.1f;         
    eeg_reg      = 0.1f;                  

    bool gui    = false;               
}

//=============================================================================================================

void DipoleFitSettings::checkIntegrity()
{
    do_baseline = (bmin < BIG_TIME && bmax < BIG_TIME);

    if (measname.isEmpty()) {
        qCritical ("Data file name missing. Please specify one using the --meas option.");
        return;
    }
    if (dipname.isEmpty() && bdipname.isEmpty()) {
        qCritical ("Output file name missing. Please use the --dip or --bdip options to do this.");
        return;
    }
    if (guessname.isEmpty()) {
        if (bemname.isEmpty() && !guess_surfname.isEmpty() && mriname.isEmpty()) {
            qCritical ("Please specify the MRI/head coordinate transformation with the --mri option");
            return;
        }
    }
    if (!include_meg && !include_eeg) {
        qCritical ("Specify one or both of the --eeg and --meg options");
        return;
    }
    if (!omit_data_proj)
        projnames.prepend(measname);
    printf("\n");

    if (!bemname.isEmpty())
        printf("BEM              : %s\n",bemname.toUtf8().data());
    else {
        printf("Sphere model     : origin at (% 7.2f % 7.2f % 7.2f) mm\n",
               1000*r0[X],1000*r0[Y],1000*r0[Z]);
    }
    printf("Using %s MEG coil definitions.\n",accurate ? "accurate" : "standard");
    if (!mriname.isEmpty())
        printf("MRI transform    : %s\n",mriname.toUtf8().data());
    if (!guessname.isEmpty())
        printf("Guesses          : %s\n",guessname.toUtf8().data());
    else {
        if (!guess_surfname.isEmpty())
            printf("Guess space bounded by %s\n",guess_surfname.toUtf8().data());
        else
            printf("Spherical guess space, rad = %.1f mm\n",1000*guess_rad);
        printf("Guess grid       : %6.1f mm\n",1000*guess_grid);
        if (guess_mindist > 0.0)
            printf("Guess mindist    : %6.1f mm\n",1000*guess_mindist);
        if (guess_exclude > 0)
            printf("Guess exclude    : %6.1f mm\n",1000*guess_exclude);
    }
    printf("Data             : %s\n",measname.toUtf8().data());
    if (projnames.size() > 0) {
        printf("SSP sources      :\n");
        for (int k = 0; k < projnames.size(); k++)
            printf("\t%s\n",projnames[k].toUtf8().data());
    }
    if (badname)
        printf("Bad channels     : %s\n",badname);
    if (do_baseline)
        printf("Baseline         : %10.2f ... %10.2f ms\n", 1000*bmin,1000*bmax);
    if (!noisename.isEmpty()) {
        printf("Noise covariance : %s\n",noisename.toUtf8().data());
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
    if (!dipname.isEmpty())
        printf("dip output      : %s\n",dipname.toUtf8().data());
    if (!bdipname.isEmpty())
        printf("bdip output     : %s\n",bdipname.toUtf8().data());
    printf("\n");
}

//=============================================================================================================

void DipoleFitSettings::usage(char *name)
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
    printf("\t--guesssurf name  Read the inner skull surface from this fif file to generate the guesses.\n");
    printf("\t--guessrad value  Radius of a spherical guess volume if neither of the above is present (default : %.1f mm)\n",1000*guess_rad);
    printf("\t--exclude dist/mm Exclude points which are closer than this distance from the CM of the inner skull surface (default =  %6.1f mm).\n",1000*guess_exclude);
    printf("\t--mindist dist/mm Exclude points which are closer than this distance from the inner skull surface  (default = %6.1f mm).\n",1000*guess_mindist);
    printf("\t--grid    dist/mm Source space grid size (default = %6.1f mm).\n",1000*guess_grid);
    printf("\t--magdip          Fit magnetic dipoles instead of current dipoles.\n");
    printf("\nOutput:\n\n");
    printf("\t--dip     name    xfit dip format output file name\n");
    printf("\t--bdip    name    xfit bdip format output file name\n");
    printf("\nGeneral:\n\n");
    printf("\t--gui             Enables the gui.\n");
    printf("\t--help            print this info.\n");
    printf("\t--version         print version info.\n\n");
    return;
}

//=============================================================================================================

bool DipoleFitSettings::check_unrecognized_args(int argc, char **argv)
{
    if ( argc > 1 ) {
        printf("Unrecognized arguments : ");
        for (int k = 1; k < argc; k++)
            printf("%s ",argv[k]);
        printf("\n");
        qCritical ("Check the command line.");
        return false;
    }
    return true;
}

//=============================================================================================================

bool DipoleFitSettings::check_args (int *argc,char **argv)
{
    int found;
    float fval;
    int   ival,filter_size;

    for (int k = 0; k < *argc; k++) {
        found = 0;
        if (strcmp(argv[k],"--gui") == 0) {
            found = 1;
            gui = true;
        }
        else if (strcmp(argv[k],"--version") == 0) {
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
                return false;
            }
            guessname = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--gsurf") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--gsurf: argument required.");
                return false;
            }
            guess_surfname = strdup(argv[k+1]);
        }
        else if (strcmp(argv[k],"--guesssurf") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--guesssurf: argument required.");
                return false;
            }
            guess_surfname = strdup(argv[k+1]);
        }
        else if (strcmp(argv[k],"--guessrad") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--guessrad: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%f",&fval) != 1) {
                qCritical ("Could not interpret the radius.");
                return false;
            }
            if (fval <= 0.0) {
                qCritical ("Radius should be positive");
                return false;
            }
            guess_rad = fval/1000.0;
        }
        else if (strcmp(argv[k],"--mindist") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--mindist: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%f",&fval) != 1) {
                qCritical ("Could not interpret the distance.");
                return false;
            }
            guess_mindist = fval/1000.0;
            if (guess_mindist <= 0.0)
                guess_mindist = 0.0;
        }
        else if (strcmp(argv[k],"--exclude") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--exclude: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%f",&fval) != 1) {
                qCritical ("Could not interpret the distance.");
                return false;
            }
            guess_exclude = fval/1000.0;
            if (guess_exclude <= 0.0)
                guess_exclude = 0.0;
        }
        else if (strcmp(argv[k],"--grid") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--grid: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%f",&fval) != 1) {
                qCritical ("Could not interpret the distance.");
                return false;
            }
            if (fval <= 0.0) {
                qCritical ("Grid spacing should be positive");
                return false;
            }
            guess_grid = guess_grid/1000.0;
        }
        else if (strcmp(argv[k],"--mri") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--mri: argument required.");
                return false;
            }
            mriname = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--bem") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--bem: argument required.");
                return false;
            }
            bemname = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--accurate") == 0) {
            found = 1;
            accurate = true;
        }
        else if (strcmp(argv[k],"--meg") == 0) {
            found = 1;
            include_meg = true;
        }
        else if (strcmp(argv[k],"--eeg") == 0) {
            found = 1;
            include_eeg = true;
        }
        else if (strcmp(argv[k],"--origin") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--origin: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%f:%f:%f",r0[X],r0[Y],r0[Z]) != 3) {
                qCritical ("Could not interpret the origin.");
                return false;
            }
            r0[X] = r0[X]/1000.0;
            r0[Y] = r0[Y]/1000.0;
            r0[Z] = r0[Z]/1000.0;
        }
        else if (strcmp(argv[k],"--eegrad") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--eegrad: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&eeg_sphere_rad) != 1) {
                qCritical () << "Incomprehensible radius:" << argv[k+1];
                return false;
            }
            if (eeg_sphere_rad <= 0) {
                qCritical ("Radius must be positive");
                return false;
            }
            eeg_sphere_rad = eeg_sphere_rad/1000.0;
        }
        else if (strcmp(argv[k],"--eegmodels") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--eegmodels: argument required.");
                return false;
            }
            eeg_model_file = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--eegmodel") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--eegmodel: argument required.");
                return false;
            }
            eeg_model_name = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--eegscalp") == 0) {
            found         = 1;
            scale_eeg_pos = true;
        }
        else if (strcmp(argv[k],"--meas") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--meas: argument required.");
                return false;
            }
            measname = QString(argv[k+1]);
            is_raw = false;
        }
        else if (strcmp(argv[k],"--raw") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--raw: argument required.");
                return false;
            }
            measname = QString(argv[k+1]);
            is_raw = true;
        }
        else if (strcmp(argv[k],"--proj") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--proj: argument required.");
                return false;
            }
            projnames.append(QString(argv[k+1]));
        }
        else if (strcmp(argv[k],"--noproj") == 0) {
            found = 1;
            omit_data_proj = true;
        }
        else if (strcmp(argv[k],"--bad") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--bad: argument required.");
                return false;
            }
            badname = strdup(argv[k+1]);
        }
        else if (strcmp(argv[k],"--noise") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--noise: argument required.");
                return false;
            }
            noisename = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--gradnoise") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--gradnoise: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible value:" << argv[k+1];
                return false;
            }
            if (fval < 0.0) {
                qCritical ("Value should be positive");
                return false;
            }
            grad_std = 1e-13*fval;
        }
        else if (strcmp(argv[k],"--magnoise") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--magnoise: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible value:" << argv[k+1];
                return false;
            }
            if (fval < 0.0) {
                qCritical ("Value should be positive");
                return false;
            }
            mag_std = 1e-15*fval;
        }
        else if (strcmp(argv[k],"--eegnoise") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--eegnoise: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical () << "Incomprehensible value:" << argv[k+1];
                return false;
            }
            if (fval < 0.0) {
                qCritical ("Value should be positive");
                return false;
            }
            eeg_std = 1e-6*fval;
        }
        else if (strcmp(argv[k],"--diagnoise") == 0) {
            found = 1;
            diagnoise = true;
        }
        else if (strcmp(argv[k],"--eegreg") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--eegreg: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical () << "Incomprehensible value:" << argv[k+1];
                return false;
            }
            if (fval < 0 || fval > 1) {
                qCritical ("Regularization value should be positive and smaller than one.");
                return false;
            }
            eeg_reg = fval;
        }
        else if (strcmp(argv[k],"--magreg") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--magreg: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical () << "Incomprehensible value:" << argv[k+1];
                return false;
            }
            if (fval < 0 || fval > 1) {
                qCritical ("Regularization value should be positive and smaller than one.");
                return false;
            }
            mag_reg = fval;
        }
        else if (strcmp(argv[k],"--gradreg") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--gradreg: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical () << "Incomprehensible value:" << argv[k+1] ;
                return false;
            }
            if (fval < 0 || fval > 1) {
                qCritical ("Regularization value should be positive and smaller than one.");
                return false;
            }
            grad_reg = fval;
        }
        else if (strcmp(argv[k],"--reg") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--reg: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical () << "Incomprehensible value:" << argv[k+1];
                return false;
            }
            if (fval < 0 || fval > 1) {
                qCritical ("Regularization value should be positive and smaller than one.");
                return false;
            }
            grad_reg = fval;
            mag_reg = fval;
            eeg_reg = fval;
        }
        else if (strcmp(argv[k],"--tstep") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--tstep: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible tstep:" << argv[k+1];
                return false;
            }
            if (fval < 0.0) {
                qCritical ("Time step should be positive");
                return false;
            }
            tstep = fval/1000.0;
        }
        else if (strcmp(argv[k],"--integ") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--integ: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible integration time:" << argv[k+1];
                return false;
            }
            if (fval <= 0.0) {
                qCritical ("Integration time should be positive.");
                return false;
            }
            integ = fval/1000.0f;
        }
        else if (strcmp(argv[k],"--tmin") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--tmin: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible tmin:" << argv[k+1];
                return false;
            }
            tmin = fval/1000.0f;
        }
        else if (strcmp(argv[k],"--tmax") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--tmax: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible tmax:" << argv[k+1];
                return false;
            }
            tmax = fval/1000.0;
        }
        else if (strcmp(argv[k],"--bmin") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--bmin: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible bmin:" << argv[k+1];
                return false;
            }
            bmin = fval/1000.0f;
        }
        else if (strcmp(argv[k],"--bmax") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--bmax: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Incomprehensible bmax:" << argv[k+1];
                return false;
            }
            bmax = fval/1000.0f;
        }
        else if (strcmp(argv[k],"--set") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--set: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%d",&setno) != 1) {
                qCritical() << "Incomprehensible data set number:" << argv[k+1];
                return false;
            }
            if (setno <= 0) {
                qCritical ("Data set number must be > 0");
                return false;
            }
        }
        else if (strcmp(argv[k],"--filteroff") == 0) {
            found = 1;
            filter.filter_on = false;
        }
        else if (strcmp(argv[k],"--lowpass") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--lowpass: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Illegal number:" << argv[k+1];
                return false;
            }
            if (fval <= 0) {
                qCritical ("Lowpass corner must be positive");
                return false;
            }
            filter.lowpass = fval;
        }
        else if (strcmp(argv[k],"--lowpassw") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--lowpassw: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Illegal number:" << argv[k+1];
                return false;
            }
            if (fval <= 0) {
                qCritical ("Lowpass width must be positive");
                return false;
            }
            filter.lowpass_width = fval;
        }
        else if (strcmp(argv[k],"--highpass") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--highpass: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&fval) != 1) {
                qCritical() << "Illegal number:" << argv[k+1];
                return false;
            }
            if (fval <= 0) {
                qCritical ("Highpass corner must be positive");
                return false;
            }
            filter.highpass = fval;
        }
        else if (strcmp(argv[k],"--filtersize") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--filtersize: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%d",&ival) != 1) {
                qCritical() << "Illegal number:" << argv[k+1];
                return false;
            }
            if (ival < 1024) {
                qCritical ("Filtersize should be at least 1024.");
                return false;
            }
            for (filter_size = 1024; filter_size < ival; filter_size = 2*filter_size)
                ;
            filter.size       = filter_size;
            filter.taper_size = filter_size/2;
        }
        else if (strcmp(argv[k],"--magdip") == 0) {
            found = 1;
            fit_mag_dipoles = true;
        }
        else if (strcmp(argv[k],"--dip") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--dip: argument required.");
                return false;
            }
            dipname = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--bdip") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical ("--bdip: argument required.");
                return false;
            }
            bdipname = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--verbose") == 0) {
            found = 1;
            verbose = true;
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
