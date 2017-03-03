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

#include <fiff/fiff_tag.h>
#include <fiff/fiff_stream.h>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDateTime>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SHOWFIFF;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// Static Definitions
//=============================================================================================================

#define DEFAULT_INDENT 3
#define LONG_LINE  80

#define CLASS_TAG     1
#define CLASS_BLOCK   2
#define CLASS_UNIT    3
#define CLASS_UNITM   4
#define CLASS_CH_KIND 5




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

    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    FiffDirEntry::SPtr this_ent;
    FiffTag::SPtr   tag;
    int             day,month,year;
    int             block;
    int             count = 0;
    int             prev_kind;
    int             indent = 0;
    bool            show_it = false;
    QString         c,s;
    bool            output_taginfo = false;
//    char            buf[MAXBUF];
    MneFiffExp      exp;

    if (!stream->open())
        return false;
    prev_kind = -1;

    if (blocks_only) {
//        for (auto this_ent : stream->dir()) {//C++11
        for (int i = 0; i < stream->dir().size(); ++i) {
            this_ent = stream->dir()[i];
            if (this_ent->kind == FIFF_BLOCK_START || this_ent->kind == FIFF_BLOCK_END) {
                if (this_ent->kind == FIFF_BLOCK_END)
                    indent = indent - indent_step;
                if (this_ent->kind == FIFF_BLOCK_START) {
                    for (int k = 0; k < indent; k++)
                        fprintf(out," ");
                    if ( stream->read_tag(tag, this_ent->pos) ) {
                        block = *tag->toInt();
                        exp = *set.mne_find_fiff_explanation(CLASS_BLOCK,block);
                        if (!exp.text.isEmpty())
                            fprintf(out,"%-d = %-s\n",exp.kind,exp.text.toUtf8().constData());
                        else
                            fprintf(out,"%-d = %-s\n",block,"Not explained");
                    }
                }
                if (this_ent->kind == FIFF_BLOCK_START)
                    indent = indent + indent_step;
            }
        }
    }
    else {
//        for (auto this_ent : stream->dir()) {//C++11
        for (int i = 0; i < stream->dir().size(); ++i) {
            this_ent = stream->dir()[i];
            if (ntag == 0)
                show_it = true;
            else {
                show_it = false;
                for (int k = 0; k < ntag; k++) {
                    if (this_ent->kind == tags[k]) {
                        show_it = true;
                        break;
                    }
                }
            }
            if (show_it) {
                if (this_ent->kind == FIFF_BLOCK_START || this_ent->kind == FIFF_BLOCK_END) {
                    if (!verbose) {
                        if (count > 1)
                            fprintf(out," [%d]\n",count);
                        else if (this_ent != stream->dir()[0])
                            fprintf(out,"\n");
                    }
                    if (this_ent->kind == FIFF_BLOCK_END)
                        indent = indent - indent_step;
                    for (int k = 0; k < indent; k++)
                        fprintf(out," ");
                    exp = *set.mne_find_fiff_explanation(CLASS_TAG,this_ent->kind);
                    if (!exp.text.isEmpty())
                        fprintf(out,"%4d = %-s",exp.kind,exp.text.toUtf8().constData());
                    else
                        fprintf(out,"%4d = %-s",this_ent->kind,"Not explained");
                    if ( stream->read_tag(tag,this_ent->pos)) {
                        block = *tag->toInt();
                        exp = *set.mne_find_fiff_explanation(CLASS_BLOCK,block);
                        if (!exp.text.isEmpty())
                            fprintf(out,"\t%-d = %-s",exp.kind,exp.text.toUtf8().constData());
                        else
                            fprintf(out,"\t%-d = %-s",block,"Not explained");
                    }
                    if ( this_ent->kind == FIFF_BLOCK_START)
                        indent = indent + indent_step;
                    count = 1;
                    if (verbose)
                        fprintf(out,"\n");
                }
                else if (verbose) {
                    for (int k = 0; k < indent; k++)
                        fprintf(out," ");
                    if (output_taginfo) {
                        fprintf(out,"%d %d ",this_ent->size,this_ent->type);
                    }
                    exp = *set.mne_find_fiff_explanation(CLASS_TAG,this_ent->kind);
                    if (!exp.text.isEmpty())
                        fprintf(out,"%4d = %-18s",exp.kind,exp.text.toUtf8().constData());
                    else
                        fprintf(out,"%4d = %-18s",this_ent->kind,"Not explained");
                    if (FiffTag::fiff_type_fundamental(this_ent->type) == FIFFTS_FS_MATRIX) {
                        fprintf(out,"TODO print_matrix");
//                        print_matrix(out,in,this_ent);
                    }
                    else {
                        switch (this_ent->type) {
                        case FIFFT_INT :
                            if (stream->read_tag(tag,this_ent->pos)) {
                                if (this_ent->kind == FIFF_BLOCK_START ||
                                        this_ent->kind == FIFF_BLOCK_END) {
                                    block = *tag->toInt();
                                    exp = *set.mne_find_fiff_explanation(CLASS_BLOCK,block);
                                    if (!exp.text.isEmpty())
                                        fprintf(out,"\t%-d = %s",exp.kind,exp.text.toUtf8().constData());
                                    else
                                        fprintf(out,"\t%-d = %-s",block,"Not explained");
                                }
                                else if (this_ent->kind == FIFF_MEAS_DATE) {
                                    QDateTime ltime = QDateTime::fromSecsSinceEpoch(tag->toInt()[0]);
                                    fprintf(out,"\t%s",ltime.toString().toUtf8().constData());
                                }
                                else if (tag->size() == sizeof(fiff_int_t))
                                    fprintf(out,"\t%d",*tag->toInt());
                                else
                                    fprintf(out,"\t%d ints",(int)(tag->size()/sizeof(fiff_int_t)));
                            }
                            break;
                        case FIFFT_UINT :
                            if (stream->read_tag(tag,this_ent->pos)) {
                                if (tag->size() == sizeof(fiff_int_t))
                                    fprintf(out,"\t%d",*tag->toUnsignedInt());
                                else
                                    fprintf(out,"\t%d u_ints",(int)(tag->size()/sizeof(fiff_int_t)));
                            }
                            break;
                        case FIFFT_JULIAN :
                            if (stream->read_tag(tag,this_ent->pos)) {
                                fprintf(out,"TODO fiff_caldate");
//                                fiff_caldate (*(fiff_julian_t *)tag.data,&day,&month,&year);
                                fprintf(out,"\t%d.%d.%d",day,month,year);
                            }
                            break;
                        case FIFFT_STRING :
                            if (stream->read_tag(tag,this_ent->pos)) {
                                s = tag->toString();
                                if (long_strings)
                                    fprintf(out,"\t%s",tag->toString().toUtf8().constData());
                                else {
                                    if ((s.indexOf("\n")) != -1)
                                        s.replace(s.indexOf("\n"), 2, "\0");
                                    else if (s.size() > LONG_LINE) {
                                        s.truncate(LONG_LINE);
                                        s += "...";
                                    }
                                    fprintf(out,"\t%s",s.toUtf8().constData());
                                }
                            }
                            break;
                        case FIFFT_FLOAT :
                            if (stream->read_tag(tag,this_ent->pos)) {
                                if (tag->size() == sizeof(fiff_float_t))
                                    fprintf(out,"\t%g",*tag->toFloat());
                                else
                                    fprintf(out,"\t%d floats",(int)(tag->size()/sizeof(fiff_float_t)));
                            }
                            break;
                        case FIFFT_DOUBLE :
                            if (stream->read_tag(tag,this_ent->pos)) {
                                if (tag->size() == sizeof(fiff_double_t))
                                    fprintf(out,"\t%g",*tag->toDouble());
                                else
                                    fprintf(out,"\t%d doubles",(int)(tag->size()/sizeof(fiff_double_t)));
                            }
                            break;
                        case FIFFT_COMPLEX_FLOAT :
                            if (stream->read_tag(tag,this_ent->pos)) {
                                float *fdata = tag->toFloat();
                                if (tag->size() == 2*sizeof(fiff_float_t))
                                    fprintf(out,"\t(%g %g)",fdata[0],fdata[1]);
                                else
                                    fprintf(out,"\t%d complex numbers",
                                            (int)(tag->size()/(2*sizeof(fiff_float_t))));
                            }
                            break;
                        case FIFFT_COMPLEX_DOUBLE :
                            if (stream->read_tag(tag,this_ent->pos)) {
                                double *ddata = tag->toDouble();
                                if (tag->size() == 2*sizeof(fiff_double_t))
                                    fprintf(out,"\t(%g %g)",ddata[0],ddata[1]);
                                else
                                    fprintf(out,"\t%d double complex numbers",
                                            (int)(tag->size()/(2*sizeof(fiff_double_t))));
                            }
                            break;
                        case FIFFT_CH_INFO_STRUCT :
                            if (stream->read_tag(tag,this_ent->pos))
                                fprintf(out,"TODO print_ch_info");
//                                print_ch_info (out,set,(fiff_ch_info_t *)tag.data);
                            break;
                        case FIFFT_ID_STRUCT :
                            if (stream->read_tag(tag,this_ent->pos))
                                fprintf(out,"TODO print_file_id");
//                                if (tag.size == sizeof(fiff_id_t))
//                                    print_file_id (out,(fiff_id_t *)tag.data);
                            break;
                        case FIFFT_DIG_POINT_STRUCT :
                            if (stream->read_tag(tag,this_ent->pos))
                                fprintf(out,"TODO print_dig_point");
//                                if (tag.size == sizeof(fiff_dig_point_t))
//                                    print_dig_point (out,(fiff_dig_point_t *)tag.data);
                            break;
                        case FIFFT_DIG_STRING_STRUCT :
                            if (stream->read_tag(tag,this_ent->pos)) {
#ifdef FOO
                                if ((ds = decode_fiff_dig_string(&tag)) != NULL)
                                    print_dig_string (ds);
                                free_fiff_dig_string(ds);
#endif
                            }
                            break;
                        case FIFFT_COORD_TRANS_STRUCT :
                            if (stream->read_tag(tag,this_ent->pos))
                                fprintf(out,"TODO print_transform");
//                                if (tag.size == sizeof(fiff_coord_trans_t))
//                                    print_transform   (out,(fiff_coord_trans_t *)tag.data);
                            break;
                        default :
                            if (this_ent->kind == FIFF_DIG_STRING)
                                fprintf(out,"type = %d\n",this_ent->type);
                            if (this_ent->size > 0)
                                fprintf(out,"\t%d bytes",this_ent->size);
                            break;
                        }
                    }
                    fprintf(out,"\n");
                    prev_kind = this_ent->kind;
                }
                else {
                    if (this_ent->kind != prev_kind) {
                        if (count > 1)
                            fprintf(out," [%d]\n",count);
                        else if (this_ent != stream->dir()[0])
                            fprintf(out,"\n");
                        for (int k = 0; k < indent; k++)
                            fprintf(out," ");
                        exp = *set.mne_find_fiff_explanation(CLASS_TAG,this_ent->kind);
                        if (!exp.text.isEmpty())
                            fprintf(out,"%4d = %-s",exp.kind,exp.text.toUtf8().constData());
                        else
                            fprintf(out,"%4d = %-s",this_ent->kind,"Not explained");
                        count = 1;
                    }
                    else
                        count++;
                }
            }
            prev_kind = this_ent->kind;
        }
        if (!verbose) {
            if (count > 1)
                fprintf(out," [%d]\n",count);
            else
                fprintf(out,"\n");
        }
    }

    stream->close();

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
