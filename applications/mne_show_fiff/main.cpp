//=============================================================================================================
/**
* @file     main.cpp
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
* @brief    Implements the mne_show_fiff application.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "show_fiff_settings.h"
#include "mne_fiff_exp_set.h"

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QCoreApplication>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SHOWFIFF;


//*************************************************************************************************************
//=============================================================================================================
// Static Definitions
//=============================================================================================================

#define DEFAULT_INDENT 3














bool show_fiff_contents (FILE *out,                  /* Output file */
                        const QString& name,        /* Input file */
                        const MneFiffExpSet& set,   /* Set of explanations */
                        bool verbose,               /* Verbose output? */
                        QList<int>& tags,           /* Output these specific tags? */
                        int indent_step,            /* Indentation step */
                        bool long_strings,          /* Print long strings in full? */
                        bool blocks_only)
/*
 * Show contents of a fif file
 */
{
    int ntag = tags.size();

//    fiffFile      in = fiff_open(name);
//    fiffDirEntry  this;
//    fiffTagRec    tag;
//    int           day,month,year;
//    int           block;
//    int           count = 0;
//    int           prev_kind;
//    int           indent = 0;
//    int           k;
//    int           show_it;
//    char          *c,*s;
//    int           output_taginfo = FALSE;
//    char          buf[MAXBUF];
//    mneFiffExp    exp;

//    tag.data = NULL;
//    if (!in)
//        return false;
//    prev_kind = -1;

//    if (blocks_only) {
//        for (this = in->dir; this->kind != -1; this++)  {
//            if (this->kind == FIFF_BLOCK_START || this->kind == FIFF_BLOCK_END) {
//                if (this->kind == FIFF_BLOCK_END)
//                    indent = indent - indent_step;
//                if (this->kind == FIFF_BLOCK_START) {
//                    for (k = 0; k < indent; k++)
//                        fprintf(out," ");
//                    if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL) {
//                        block = *(int *)tag.data;
//                        exp = mne_find_fiff_explanation(set,CLASS_BLOCK,block);
//                        if (exp)
//                            fprintf(out,"%-d = %-s\n",exp->kind,exp->text);
//                        else
//                            fprintf(out,"%-d = %-s\n",block,"Not explained");
//                    }
//                }
//                if (this->kind == FIFF_BLOCK_START)
//                    indent = indent + indent_step;
//            }
//        }
//    }
//    else {
//        for (this = in->dir; this->kind != -1; this++)  {
//            if (ntag == 0)
//                show_it = TRUE;
//            else {
//                show_it = FALSE;
//                for (k = 0; k < ntag; k++)
//                    if (this->kind == tags[k]) {
//                        show_it = TRUE;
//                        break;
//                    }
//            }
//            if (show_it) {
//                if (this->kind == FIFF_BLOCK_START || this->kind == FIFF_BLOCK_END) {
//                    if (!verbose) {
//                        if (count > 1)
//                            fprintf(out," [%d]\n",count);
//                        else if (this != in->dir)
//                            fprintf(out,"\n");
//                    }
//                    if (this->kind == FIFF_BLOCK_END)
//                        indent = indent - indent_step;
//                    for (k = 0; k < indent; k++)
//                        fprintf(out," ");
//                    exp = mne_find_fiff_explanation(set,CLASS_TAG,this->kind);
//                    if (exp)
//                        fprintf(out,"%4d = %-s",exp->kind,exp->text);
//                    else
//                        fprintf(out,"%4d = %-s",this->kind,"Not explained");
//                    if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL) {
//                        block = *(int *)tag.data;
//                        exp = mne_find_fiff_explanation(set,CLASS_BLOCK,block);
//                        if (exp)
//                            fprintf(out,"\t%-d = %-s",exp->kind,exp->text);
//                        else
//                            fprintf(out,"\t%-d = %-s",block,"Not explained");
//                    }
//                    if (this->kind == FIFF_BLOCK_START)
//                        indent = indent + indent_step;
//                    count = 1;
//                    if (verbose)
//                        fprintf(out,"\n");
//                }
//                else if (verbose) {
//                    for (k = 0; k < indent; k++)
//                        fprintf(out," ");
//                    if (output_taginfo) {
//                        fprintf(out,"%d %d ",this->size,this->type);
//                    }
//                    exp = mne_find_fiff_explanation(set,CLASS_TAG,this->kind);
//                    if (exp)
//                        fprintf(out,"%4d = %-18s",exp->kind,exp->text);
//                    else
//                        fprintf(out,"%4d = %-18s",this->kind,"Not explained");
//                    if (fiff_type_fundamental(this->type) == FIFFTS_FS_MATRIX)
//                        print_matrix(out,in,this);
//                    else {
//                        switch (this->type) {
//                        case FIFFT_INT :
//                            if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL) {
//                                if (this->kind == FIFF_BLOCK_START ||
//                                        this->kind == FIFF_BLOCK_END) {
//                                    block = *(int *)tag.data;
//                                    exp = mne_find_fiff_explanation(set,CLASS_BLOCK,block);
//                                    if (exp)
//                                        fprintf(out,"\t%-d = %s",exp->kind,exp->text);
//                                    else
//                                        fprintf(out,"\t%-d = %-s",block,"Not explained");
//                                }
//                                else if (this->kind == FIFF_MEAS_DATE) {
//                                    fiffTime meas_date = (fiffTime)tag.data;
//                                    time_t   time = meas_date->secs;
//                                    struct   tm *ltime;

//                                    ltime = localtime(&time);
//                                    (void)strftime(buf,MAXBUF,"%c",ltime);
//                                    fprintf(out,"\t%s",buf);
//                                }
//                                else if (tag.size == sizeof(fiff_int_t))
//                                    fprintf(out,"\t%d",*(int *)tag.data);
//                                else
//                                    fprintf(out,"\t%d ints",(int)(tag.size/sizeof(fiff_int_t)));
//                            }
//                            break;
//                        case FIFFT_UINT :
//                            if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL) {
//                                if (tag.size == sizeof(fiff_int_t))
//                                    fprintf(out,"\t%d",*(unsigned int *)tag.data);
//                                else
//                                    fprintf(out,"\t%d u_ints",(int)(tag.size/sizeof(fiff_int_t)));
//                            }
//                            break;
//                        case FIFFT_JULIAN :
//                            if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL) {
//                                fiff_caldate (*(fiff_julian_t *)tag.data,&day,&month,&year);
//                                fprintf(out,"\t%d.%d.%d",day,month,year);
//                            }
//                            break;
//                        case FIFFT_STRING :
//                            if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL) {
//                                s = (char *)(tag.data);
//                                if (long_strings)
//                                    fprintf(out,"\t%s",(char *)(tag.data));
//                                else {
//                                    if ((c = strchr(s,'\n')) != NULL)
//                                        *c = '\0';
//                                    else if (strlen(s) > LONG_LINE) {
//                                        c  = s+LONG_LINE;
//                                        *c = '\0';
//                                    }
//                                    fprintf(out,"\t%s",(char *)(tag.data));
//                                    if (c != NULL)
//                                        fprintf(out,"...");
//                                }
//                            }
//                            break;
//                        case FIFFT_FLOAT :
//                            if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL) {
//                                if (tag.size == sizeof(fiff_float_t))
//                                    fprintf(out,"\t%g",*(float *)tag.data);
//                                else
//                                    fprintf(out,"\t%d floats",(int)(tag.size/sizeof(fiff_float_t)));
//                            }
//                            break;
//                        case FIFFT_DOUBLE :
//                            if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL) {
//                                if (tag.size == sizeof(fiff_double_t))
//                                    fprintf(out,"\t%g",*(double *)tag.data);
//                                else
//                                    fprintf(out,"\t%d doubles",(int)(tag.size/sizeof(fiff_double_t)));
//                            }
//                            break;
//                        case FIFFT_COMPLEX_FLOAT :
//                            if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL) {
//                                float *fdata = (float *)tag.data;
//                                if (tag.size == 2*sizeof(fiff_float_t))
//                                    fprintf(out,"\t(%g %g)",fdata[0],fdata[1]);
//                                else
//                                    fprintf(out,"\t%d complex numbers",
//                                            (int)(tag.size/(2*sizeof(fiff_float_t))));
//                            }
//                            break;
//                        case FIFFT_COMPLEX_DOUBLE :
//                            if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL) {
//                                double *ddata = (double *)tag.data;
//                                if (tag.size == 2*sizeof(fiff_double_t))
//                                    fprintf(out,"\t(%g %g)",ddata[0],ddata[1]);
//                                else
//                                    fprintf(out,"\t%d double complex numbers",
//                                            (int)(tag.size/(2*sizeof(fiff_double_t))));
//                            }
//                            break;
//                        case FIFFT_CH_INFO_STRUCT :
//                            if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL)
//                                print_ch_info (out,set,(fiff_ch_info_t *)tag.data);
//                            break;
//                        case FIFFT_ID_STRUCT :
//                            if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL)
//                                if (tag.size == sizeof(fiff_id_t))
//                                    print_file_id (out,(fiff_id_t *)tag.data);
//                            break;
//                        case FIFFT_DIG_POINT_STRUCT :
//                            if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL)
//                                if (tag.size == sizeof(fiff_dig_point_t))
//                                    print_dig_point (out,(fiff_dig_point_t *)tag.data);
//                            break;
//                        case FIFFT_DIG_STRING_STRUCT :
//                            if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL) {
//#ifdef FOO
//                                if ((ds = decode_fiff_dig_string(&tag)) != NULL)
//                                    print_dig_string (ds);
//                                free_fiff_dig_string(ds);
//#endif
//                            }
//                            break;
//                        case FIFFT_COORD_TRANS_STRUCT :
//                            if (fiff_read_this_tag (in->fd,this->pos,&tag) != FIFF_FAIL)
//                                if (tag.size == sizeof(fiff_coord_trans_t))
//                                    print_transform   (out,(fiff_coord_trans_t *)tag.data);
//                            break;
//                        default :
//                            if (this->kind == FIFF_DIG_STRING)
//                                fprintf(out,"type = %d\n",this->type);
//                            if (this->size > 0)
//                                fprintf(out,"\t%d bytes",this->size);
//                            break;
//                        }
//                    }
//                    fprintf(out,"\n");
//                    prev_kind = this->kind;
//                }
//                else {
//                    if (this->kind != prev_kind) {
//                        if (count > 1)
//                            fprintf(out," [%d]\n",count);
//                        else if (this != in->dir)
//                            fprintf(out,"\n");
//                        for (k = 0; k < indent; k++)
//                            fprintf(out," ");
//                        exp = mne_find_fiff_explanation(set,CLASS_TAG,this->kind);
//                        if (exp)
//                            fprintf(out,"%4d = %-s",exp->kind,exp->text);
//                        else
//                            fprintf(out,"%4d = %-s",this->kind,"Not explained");
//                        count = 1;
//                    }
//                    else
//                        count++;
//                }
//            }
//            prev_kind = this->kind;
//        }
//        if (!verbose) {
//            if (count > 1)
//                fprintf(out," [%d]\n",count);
//            else
//                fprintf(out,"\n");
//        }
//    }
//    FREE(tag.data);
//    fiff_close (in);
    return true;
}






//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

//=============================================================================================================
/**
* The function main marks the entry point of the mne_dipole_fit application.
* By default, main has the storage class extern.
*
* @param [in] argc  (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
* @param [in] argv  (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
* @return the value that was set to exit() (which is 0 if exit() is called via quit()).
*/
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

//    DipoleFitSettings settings(&argc,argv);
//    DipoleFit dipFit(&settings);
//    ECDSet set = dipFit.calculateFit();

    ShowFiffSettings settings(&argc,argv);
    MneFiffExpSet expSet = MneFiffExpSet::read_fiff_explanations(QCoreApplication::applicationDirPath()+"/resources/explanations/fiff_explanations.txt");

    if (settings.indent < 0)
        settings.indent = settings.verbose ? 0 : DEFAULT_INDENT;

    show_fiff_contents(stdout,settings.inname,
               expSet,settings.verbose,settings.tags,settings.indent,settings.long_strings,settings.blocks_only);



    return app.exec();
}
