

#include "compute_fwd_settings.h"

#include <stdio.h>

using namespace Eigen;
using namespace FWDLIB;

#define X 0
#define Y 1
#define Z 2

#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION     "2.10"
#endif

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ComputeFwdSettings::ComputeFwdSettings()
{
    initMembers();
}

//=============================================================================================================

ComputeFwdSettings::ComputeFwdSettings(int *argc,char **argv)
{
    initMembers();

    if (!check_args(argc,argv))
        return;

//    mne_print_version_info(stderr,argv[0],PROGRAM_VERSION,__DATE__,__TIME__);
    printf("%s version %s compiled at %s %s\n",argv[0],PROGRAM_VERSION,__DATE__,__TIME__);

    checkIntegrity();
}

//=============================================================================================================

ComputeFwdSettings::~ComputeFwdSettings()
{
    //ToDo Garbage collection
}

//=============================================================================================================

void ComputeFwdSettings::checkIntegrity()
{
    if (srcname.isEmpty()) {
        qCritical("Source space name is missing. Use the --src option to specify it.");
        return;
    }
    if (!mri_head_ident) {
        if (mriname.isEmpty() && transname.isEmpty()) {
            qCritical("MRI <-> head coordinate transformation is missing. Use the --mri or --trans option to specify it.");
            return;
        }
    }
    if (measname.isEmpty()) {
        qCritical("Source of coil and electrode locations is missing. Use the --meas option to specify it.");
        return;
    }
    if (solname.isEmpty()) {
        qCritical("Solution name is missing. Use the --fwd option to specify it.");
        return;
    }
    if (! (include_meg || include_eeg)) {
        qCritical("Employ the --meg and --eeg options to select MEG and/or EEG");
        return;
    }
}

//=============================================================================================================

void ComputeFwdSettings::initMembers()
{
    // Init origin
    r0 << 0.0f,0.0f,0.04f;

    filter_spaces = true;  
    accurate = false;      
    fixed_ori = false;     
    include_meg = false;
    include_eeg = false;
    compute_grad = false;
    mindist = 0.0f;        
    coord_frame = FIFFV_COORD_HEAD;
    do_all = false;
    nlabel = 0;

    eeg_sphere_rad = 0.09f;   
    scale_eeg_pos = false;    
    use_equiv_eeg = true;     
    use_threads = true;

    pFiffInfo = Q_NULLPTR;
    meg_head_t = Q_NULLPTR;
}

//=============================================================================================================

void ComputeFwdSettings::usage(char *name)
{
    printf("usage : %s [options]\n",name);
    printf("\t--meg             to compute the MEG forward solution\n");
    printf("\t--eeg             to compute the EEG forward solution\n");
    printf("\t--grad            compute the gradient of the field with respect to the dipole coordinates as well\n");
    printf("\t--fixed           to calculate only for the source orientation given by the surface normals\n");
    printf("\t--mricoord        do calculations in MRI coordinates instead of head coordinates\n");
    printf("\t--accurate        use more accurate coil definitions in MEG forward computation\n");
    printf("\t--src name        specify the source space\n");
    printf("\t--label name      label file to select the sources (can have multiple of these)\n");
    printf("\t--mri name        take head/MRI coordinate transform from here (Neuromag MRI description file)\n");
    printf("\t--trans name      take head/MRI coordinate transform from here (text file)\n");
    printf("\t--notrans         head and MRI coordinate systems are identical.\n");
    printf("\t--meas name       take MEG sensor and EEG electrode locations from here\n");
    printf("\t--bem  name       BEM model name\n");
    printf("\t--origin x:y:z/mm use a sphere model with this origin (head coordinates/mm)\n");
    printf("\t--eegscalp        scale the electrode locations to the surface of the scalp when using a sphere model\n");
    printf("\t--eegmodels name  read EEG sphere model specifications from here.\n");
    printf("\t--eegmodel  name  name of the EEG sphere model to use (default : Default)\n");
    printf("\t--eegrad rad/mm   radius of the scalp surface to use in EEG sphere model (default : %7.1f mm)\n",1000*eeg_sphere_rad);
    printf("\t--mindist dist/mm minimum allowable distance of the sources from the inner skull surface.\n");
    printf("\t--mindistout name Output the omitted source space points here.\n");
    printf("\t--includeall      Omit all source space checks\n");
    printf("\t--all             calculate forward solution in all nodes instead the selected ones only.\n");
    printf("\t--fwd  name       save the solution here\n");
    printf("\t--help            print this info.\n");
    printf("\t--version         print version info.\n\n");
    exit(1);
}

//=============================================================================================================

QString ComputeFwdSettings::build_command_line(QString old, QString new_item)
{
    if (!new_item.isEmpty() && new_item.size() > 0) {
        if (!old.isEmpty()) {
            old += " ";
        }
        old += new_item;
    }
    return old;
}

//=============================================================================================================

bool ComputeFwdSettings::check_unrecognized_args(int argc, char **argv)
{
    int k;

    if (argc > 1) {
        printf("Unrecognized arguments : ");
        for (k = 1; k < argc; k++)
            printf("%s ",argv[k]);
        printf("\n");
        qCritical("Check the command line.");
        return false;
    }
    return true;
}

//=============================================================================================================

bool ComputeFwdSettings::check_args (int *argc,char **argv)
{
    int found;
    char *last;

    if ((last = strrchr(argv[0],'/')) == NULL)
        last = argv[0];
    else
        last++;
    command = build_command_line(command,last);
    for (int k = 0; k < *argc; k++) {
        found = 0;
        if (strcmp(argv[k],"--version") == 0) {
            printf("%s version %s compiled at %s %s\n", argv[0],PROGRAM_VERSION,__DATE__,__TIME__);
            exit(0);
        }
        else if (strcmp(argv[k],"--help") == 0) {
            usage(argv[0]);
            exit(1);
        }
        else if (strcmp(argv[k],"--meg") == 0) {
            found = 1;
            include_meg = true;
        }
        else if (strcmp(argv[k],"--eeg") == 0) {
            found = 1;
            include_eeg = true;
        }
        else if (strcmp(argv[k],"--grad") == 0) {
            found = 1;
            compute_grad = true;
        }
        else if (strcmp(argv[k],"--all") == 0) {
            found = 1;
            do_all = true;
        }
        else if (strcmp(argv[k],"--accurate") == 0) {
            found = 1;
            accurate = true;
        }
        else if (strcmp(argv[k],"--fixed") == 0) {
            found = 1;
            fixed_ori = true;
        }
        else if (strcmp(argv[k],"--src") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--src: argument required.");
                return false;
            }
            srcname = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--mri") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--mri: argument required.");
                return false;
            }
            mri_head_ident = false;
            mriname = QString(argv[k+1]);
            transname.clear();
        }
        else if (strcmp(argv[k],"--trans") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--trans: argument required.");
                return false;
            }
            mri_head_ident = false;
            transname = QString(argv[k+1]);
            mriname.clear();
        }
        else if (strcmp(argv[k],"--notrans") == 0) {
            found = 1;
            mri_head_ident = true;
            mriname.clear();
            transname.clear();
        }
        else if (strcmp(argv[k],"--meas") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--meas: argument required.");
                return false;
            }
            measname = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--bem") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--bem: argument required.");
                return false;
            }
            bemname = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--origin") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--origin: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%f:%f:%f",r0[X],r0[Y],r0[Z]) != 3) {
                qCritical("Could not interpret the origin.");
                return false;
            }
            r0[X] = r0[X]/1000.0f;
            r0[Y] = r0[Y]/1000.0f;
            r0[Z] = r0[Z]/1000.0f;
        }
        else if (strcmp(argv[k],"--eegrad") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--eegrad: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%g",&eeg_sphere_rad) != 1) {
                qCritical("Incomprehensible radius : %s",argv[k+1]);
                return false;
            }
            if (eeg_sphere_rad <= 0) {
                qCritical("Radius must be positive");
                return false;
            }
            eeg_sphere_rad = eeg_sphere_rad/1000.0f;
        }
        else if (strcmp(argv[k],"--eegmodels") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--eegmodels: argument required.");
                return false;
            }
            eeg_model_file = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--eegmodel") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--eegmodel: argument required.");
                return false;
            }
            eeg_model_name = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--eegscalp") == 0) {
            found         = 1;
            scale_eeg_pos = true;
        }
        else if (strcmp(argv[k],"--mindist") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--mindist: argument required.");
                return false;
            }
            if (sscanf(argv[k+1],"%f",&mindist) != 1) {
                qCritical("Could not interpret the distance.");
                return false;
            }
            if (mindist <= 0.0)
                mindist = 0.0f;
            mindist = mindist/1000.0f;
        }
        else if (strcmp(argv[k],"--includeall") == 0) {
            found = 1;
            filter_spaces = false;
        }
        else if (strcmp(argv[k],"--mindistout") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--mindistout: argument required.");
                return false;
            }
            mindistoutname = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--mricoord") == 0) {
            found = 1;
            coord_frame = FIFFV_COORD_MRI;
        }
        else if (strcmp(argv[k],"--fwd") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--fwd: argument required.");
                return false;
            }
            solname = QString(argv[k+1]);
        }
        else if (strcmp(argv[k],"--label") == 0) {
            found = 2;
            if (k == *argc - 1) {
                qCritical("--label: argument required.");
                return false;
            }
            labels.append(QString(argv[k+1]));
            nlabel++;
        }
        if (found) {
            for (int p = k; p < k + found; p++)
                command = build_command_line(command,argv[p]);
            for (int p = k; p < *argc-found; p++)
                argv[p] = argv[p+found];
            *argc = *argc - found;
            k = k - found;
        }
    }
    return check_unrecognized_args(*argc,argv);
}
