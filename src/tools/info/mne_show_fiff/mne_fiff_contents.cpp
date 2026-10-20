//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     mne_fiff_contents.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     December, 2016
 * @version  dev
 * @brief    Definition of the MNEFiffContents class.
 */


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_fiff_contents.h"
#include "mne_show_fiff_settings.h"


#include <fiff/fiff_explain.h>
#include <fiff/fiff_tag.h>
#include <fiff/fiff_stream.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDateTime>
#include <QDebug>


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

constexpr auto LONG_LINE = 80;

static QString explained(const QString& name)
{
    return name.isEmpty() ? QStringLiteral("Not explained") : name;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================
//*************************************************************************************************************

void MNEFiffContents::print_file_id(QTextStream& out, const FiffTag::UPtr& tag)
{
    FiffId id = tag->toFiffID();
    struct tm* ltime;
    time_t time_val;
    char buf[100];

    out << QString::asprintf("\t%d.%d ", id.version >> 16, id.version & 0xFFFF);
    out << QString::asprintf("0x%x%x ", id.machid[0], id.machid[1]);

    time_val = id.time.secs;
    //
    // localtime() returns a pointer into a shared static buffer. Use the
    // reentrant variant so a concurrent caller cannot overwrite the result
    // between this call and strftime below.
    //
    struct tm timeBuf;
#ifdef _WIN32
    ltime = (localtime_s(&timeBuf, &time_val) == 0) ? &timeBuf : nullptr;
#else
    ltime = localtime_r(&time_val, &timeBuf);
#endif
    if (ltime) {
        if (strftime(buf, sizeof(buf), "%c", ltime) == 0)
            buf[0] = '\0';
    } else {
        buf[0] = '\0';
    }

    out << buf;
}


//*************************************************************************************************************

void MNEFiffContents::print_ch_info(QTextStream& out, const FiffTag::UPtr& tag)
{
    FiffChInfo info = tag->toChInfo();
    // FIFF_UNIT_NONE has an empty symbol; only units the table does not know are "N/A"
    const QString text1 = info.unit == FIFF_UNIT_NONE || !FiffExplain::unit(info.unit).isEmpty() ? FiffExplain::unit(info.unit) : QStringLiteral("N/A");
    const QString text2 = FiffExplain::unitMultiplier(info.unit_mul);
    const QString text3 = FiffExplain::channelKind(info.kind).isEmpty() ? QStringLiteral("N/A") : FiffExplain::channelKind(info.kind);

    out << QString::asprintf("\t%3d \"%s\" (%s %3d) [%s%s]", info.scanNo, info.ch_name.toUtf8().constData(), text3.toUtf8().constData(), info.logNo, text2.toUtf8().constData(), text1.toUtf8().constData());
    out << QString::asprintf("\trange %g cal %g", info.range, info.cal);
}


//*************************************************************************************************************

void MNEFiffContents::print_transform(QTextStream& out, const FiffTag::UPtr& tag)
{
    FiffCoordTrans t = tag->toCoordTrans();
    int k, p;
    int frame;
    out << "\t";
    for (frame = t.from, k = 0; k < 2; k++) {
        out << FIFFLIB::FiffCoordTrans::frame_name(frame);
        if (k == 0) {
            out << " -> ";
            frame = t.to;
        } else {
            out << "\n";
            for (p = 0; p < 3; p++)
                out << QString::asprintf("%24s\t%8.5f %8.5f %8.5f %8.2f\n", " ", t.trans(p, 0), t.trans(p, 1), t.trans(p, 2), 1000 * t.trans(p, 3));
            out << QString::asprintf("%24s\t%8.5f %8.5f %8.5f %8.2f", " ", 0.0, 0.0, 0.0, 1.0);
        }
    }
}


//*************************************************************************************************************

void MNEFiffContents::print_dig_point(QTextStream& out, const FiffTag::UPtr& tag)
{
    FiffDigPoint point = tag->toDigPoint();
    switch (point.kind) {
        case FIFFV_POINT_CARDINAL:
            out << QString::asprintf("\tcardinal %3d ", point.ident);
            break;
        case FIFFV_POINT_HPI:
            out << QString::asprintf("\thpi      %3d ", point.ident);
            break;
        case FIFFV_POINT_EEG:
            out << QString::asprintf("\teeg      %3d ", point.ident);
            break;
        case FIFFV_POINT_EXTRA:
            out << QString::asprintf("\textra    %3d ", point.ident);
            break;
    }
    out << QString::asprintf("(%6.1f,%6.1f,%6.1f)", 1000 * point.r[0], 1000 * point.r[1], 1000 * point.r[2]);
}


//*************************************************************************************************************

void MNEFiffContents::print_matrix(QTextStream& out, [[maybe_unused]] FiffStream::SPtr stream, FiffDirEntry::SPtr this_ent)
{
    // Simplified matrix info since we don't want to read the whole tag yet if it's large,
    // but FiffStream has some matrix reading logic.
    out << QString::asprintf("\tMatrix type %d, size %d bytes", this_ent->type, this_ent->size);
}


//*************************************************************************************************************

bool MNEFiffContents::show_fiff_contents(QTextStream& out, const MNEShowFiffSettings& settings)
{
    return show_fiff_contents(out, settings.inname, settings.verbose, settings.tags, settings.indent, settings.long_strings, settings.blocks_only);
}


//*************************************************************************************************************

bool MNEFiffContents::show_fiff_contents(QTextStream& out, const QString& name, bool verbose, const QList<int>& tags, int indent_step, bool long_strings, bool blocks_only)
{
    QFile file(name);
    FiffStream::SPtr stream(new FiffStream(&file));

    FiffDirEntry::SPtr this_ent;
    FiffTag::UPtr tag;
    int block;
    int count = 0;
    int prev_kind;
    int indent = 0;
    bool show_it = false;
    QString s;
    bool output_taginfo = false;

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
                        out << " ";
                    if (stream->read_tag(tag, this_ent->pos)) {
                        block = *tag->toInt();
                        out << QString::asprintf("%-d = %-s\n", block, explained(FiffExplain::block(block)).toUtf8().constData());
                    }
                }
                if (this_ent->kind == FIFF_BLOCK_START)
                    indent = indent + indent_step;
            }
        }
    } else {
        //        for (auto this_ent : stream->dir()) {//C++11
        for (int i = 0; i < stream->dir().size(); ++i) {
            this_ent = stream->dir()[i];
            if (this_ent->kind == -1) {
            }
            if (tags.size() == 0)
                show_it = true;
            else {
                show_it = false;
                for (int k = 0; k < tags.size(); k++) {
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
                            out << QString::asprintf(" [%d]\n", count);
                        else if (this_ent != stream->dir()[0])
                            out << "\n";
                    }
                    if (this_ent->kind == FIFF_BLOCK_END)
                        indent = indent - indent_step;
                    for (int k = 0; k < indent; k++)
                        out << " ";
                    out << QString::asprintf("%4d = %-s", this_ent->kind, explained(FiffExplain::tag(this_ent->kind)).toUtf8().constData());
                    if (stream->read_tag(tag, this_ent->pos)) {
                        block = *tag->toInt();
                        out << QString::asprintf("\t%-d = %-s", block, explained(FiffExplain::block(block)).toUtf8().constData());
                    }
                    if (this_ent->kind == FIFF_BLOCK_START)
                        indent = indent + indent_step;
                    count = 1;
                    if (verbose)
                        out << "\n";
                } else if (verbose) {
                    for (int k = 0; k < indent; k++)
                        out << " ";
                    if (output_taginfo) {
                        out << QString::asprintf("%d %d ", this_ent->size, this_ent->type);
                    }
                    out << QString::asprintf("%4d = %-18s", this_ent->kind, explained(FiffExplain::tag(this_ent->kind)).toUtf8().constData());
                    if (FiffTag::fiff_type_fundamental(this_ent->type) == FIFFTS_FS_MATRIX) {
                        print_matrix(out, stream, this_ent);
                    } else {
                        switch (this_ent->type) {
                            case FIFFT_INT:
                                if (stream->read_tag(tag, this_ent->pos)) {
                                    if (this_ent->kind == FIFF_BLOCK_START ||
                                        this_ent->kind == FIFF_BLOCK_END) {
                                        block = *tag->toInt();
                                        out << QString::asprintf("\t%-d = %s", block, explained(FiffExplain::block(block)).toUtf8().constData());
                                    } else if (this_ent->kind == FIFF_MEAS_DATE) {
                                        QDateTime ltime;
                                        ltime.setSecsSinceEpoch(tag->toInt()[0]);
                                        out << QString::asprintf("\t%s", ltime.toString().toUtf8().constData());
                                    } else if (tag->size() == sizeof(fiff_int_t))
                                        out << QString::asprintf("\t%d", *tag->toInt());
                                    else
                                        out << QString::asprintf("\t%d ints", static_cast<int>(tag->size() / sizeof(fiff_int_t)));
                                }
                                break;
                            case FIFFT_UINT:
                                if (stream->read_tag(tag, this_ent->pos)) {
                                    if (tag->size() == sizeof(fiff_int_t))
                                        out << QString::asprintf("\t%d", *tag->toUnsignedInt());
                                    else
                                        out << QString::asprintf("\t%d u_ints", static_cast<int>(tag->size() / sizeof(fiff_int_t)));
                                }
                                break;
                            case FIFFT_JULIAN:
                                if (stream->read_tag(tag, this_ent->pos)) {
                                    // MNE-CPP doesn't seem to have a direct julian to caldate, skipping for now or simplistic print
                                    out << "\tJulian date tag";
                                }
                                break;
                            case FIFFT_STRING:
                                if (stream->read_tag(tag, this_ent->pos)) {
                                    s = tag->toString();
                                    if (long_strings)
                                        out << QString::asprintf("\t%s", tag->toString().toUtf8().constData());
                                    else {
                                        if ((s.indexOf("\n")) != -1)
                                            s.replace(s.indexOf("\n"), 2, "\0");
                                        else if (s.size() > LONG_LINE) {
                                            s.truncate(LONG_LINE);
                                            s += "...";
                                        }
                                        out << QString::asprintf("\t%s", s.toUtf8().constData());
                                    }
                                }
                                break;
                            case FIFFT_FLOAT:
                                if (stream->read_tag(tag, this_ent->pos)) {
                                    if (tag->size() == sizeof(fiff_float_t))
                                        out << QString::asprintf("\t%g", *tag->toFloat());
                                    else
                                        out << QString::asprintf("\t%d floats", static_cast<int>(tag->size() / sizeof(fiff_float_t)));
                                }
                                break;
                            case FIFFT_DOUBLE:
                                if (stream->read_tag(tag, this_ent->pos)) {
                                    if (tag->size() == sizeof(fiff_double_t))
                                        out << QString::asprintf("\t%g", *tag->toDouble());
                                    else
                                        out << QString::asprintf("\t%d doubles", static_cast<int>(tag->size() / sizeof(fiff_double_t)));
                                }
                                break;
                            case FIFFT_COMPLEX_FLOAT:
                                if (stream->read_tag(tag, this_ent->pos)) {
                                    const float* fdata = tag->toFloat();
                                    if (tag->size() == 2 * sizeof(fiff_float_t))
                                        out << QString::asprintf("\t(%g %g)", fdata[0], fdata[1]);
                                    else
                                        out << QString::asprintf("\t%d complex numbers",
                                                                 static_cast<int>(tag->size() / (2 * sizeof(fiff_float_t))));
                                }
                                break;
                            case FIFFT_COMPLEX_DOUBLE:
                                if (stream->read_tag(tag, this_ent->pos)) {
                                    const double* ddata = tag->toDouble();
                                    if (tag->size() == 2 * sizeof(fiff_double_t))
                                        out << QString::asprintf("\t(%g %g)", ddata[0], ddata[1]);
                                    else
                                        out << QString::asprintf("\t%d double complex numbers",
                                                                 static_cast<int>(tag->size() / (2 * sizeof(fiff_double_t))));
                                }
                                break;
                            case FIFFT_CH_INFO_STRUCT:
                                if (stream->read_tag(tag, this_ent->pos))
                                    print_ch_info(out, tag);
                                break;
                            case FIFFT_ID_STRUCT:
                                if (stream->read_tag(tag, this_ent->pos))
                                    print_file_id(out, tag);
                                break;
                            case FIFFT_DIG_POINT_STRUCT:
                                if (stream->read_tag(tag, this_ent->pos))
                                    print_dig_point(out, tag);
                                break;
                            case FIFFT_DIG_STRING_STRUCT:
                                if (stream->read_tag(tag, this_ent->pos)) {
#ifdef FOO
                                    if ((ds = decode_fiff_dig_string(&tag)) != nullptr)
                                        print_dig_string(ds);
                                    free_fiff_dig_string(ds);
#endif
                                }
                                break;
                            case FIFFT_COORD_TRANS_STRUCT:
                                if (stream->read_tag(tag, this_ent->pos))
                                    print_transform(out, tag);
                                break;
                            default:
                                if (this_ent->kind == FIFF_DIG_STRING)
                                    out << QString::asprintf("type = %d\n", this_ent->type);
                                if (this_ent->size > 0)
                                    out << QString::asprintf("\t%d bytes", this_ent->size);
                                break;
                        }
                    }
                    out << "\n";
                    prev_kind = this_ent->kind;
                } else {
                    if (this_ent->kind != prev_kind) {
                        if (count > 1)
                            out << QString::asprintf(" [%d]\n", count);
                        else if (this_ent != stream->dir()[0])
                            out << "\n";
                        for (int k = 0; k < indent; k++)
                            out << " ";
                        out << QString::asprintf("%4d = %-s", this_ent->kind, explained(FiffExplain::tag(this_ent->kind)).toUtf8().constData());
                        count = 1;
                    } else
                        count++;
                }
            }
            prev_kind = this_ent->kind;
        }
        if (!verbose) {
            if (count > 1)
                out << QString::asprintf(" [%d]\n", count);
            else
                out << "\n";
        }
    }

    stream->close();

    return true;
}
