//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     mne_fiff_contents.cpp
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     2006
 * @version  dev
 * @brief    Definition of the MNEFiffContents class.
 */


//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_fiff_contents.h"
#include "mne_show_fiff_settings.h"


#include <mne/fiff/fiff_explain.h>
#include <mne/fiff/fiff_tag.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/base/string_utils.h>


//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <format>
#include <limits>
#include <print>
#include <string>
#include <vector>


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SHOWFIFF;
using namespace mne;


//=============================================================================================================
// Static Definitions
//=============================================================================================================

constexpr auto LONG_LINE = 80;

static std::string explained(const std::string& name)
{
    return name.empty() ? std::string("Not explained") : name;
}

// Qt's asprintf rounds exact decimal ties away from zero where std::format rounds them to even. Moves a tie at
// decimals to the neighbouring double away from zero, so that std::format writes the digits Qt wrote.
static double qtTie(double value, int decimals)
{
    if (!std::isfinite(value)) {
        return value;
    }
    bool tie = false;
    if (decimals >= 0) {
        // A tie at decimals is an odd multiple of 2^-(decimals + 1)
        const double scaled = std::ldexp(value, decimals + 1);
        tie = std::isfinite(scaled) && scaled == std::trunc(scaled) && std::fmod(scaled, 2.0) != 0.0;
    } else if (std::abs(value) < 0x1p53) {
        const double unit = std::pow(10.0, -decimals);
        tie = std::fmod(std::abs(value), unit) == unit / 2;
    }
    return tie ? std::nextafter(value, value < 0 ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity()) : value;
}

// qtTie() for %g, which keeps six significant digits
static double qtTieG(double value)
{
    if (value == 0.0 || !std::isfinite(value)) {
        return value;
    }
    return qtTie(value, 5 - static_cast<int>(std::floor(std::log10(std::abs(value)))));
}

// The local time like QDateTime::toString() (Qt::TextDate), e.g. "Tue Dec 3 14:36:19 2002"
static std::string textDate(std::time_t timeVal)
{
    static constexpr std::array<const char*, 7> days = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static constexpr std::array<const char*, 12> months = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    struct tm timeBuf;
#ifdef _WIN32
    const struct tm* ltime = (localtime_s(&timeBuf, &timeVal) == 0) ? &timeBuf : nullptr;
#else
    const struct tm* ltime = localtime_r(&timeVal, &timeBuf);
#endif
    if (!ltime) {
        return {};
    }
    return std::format("{} {} {} {:02}:{:02}:{:02} {}", days[static_cast<std::size_t>(ltime->tm_wday)], months[static_cast<std::size_t>(ltime->tm_mon)], ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec, ltime->tm_year + 1900);
}

// Drops the newline and the character after it, like QString::replace(pos, 2, ""), or cuts strings longer than
// longLine UTF-16 code units, like QString::truncate()
static std::string shortened(std::string s, int longLine)
{
    if (const std::size_t pos = s.find('\n'); pos != std::string::npos) {
        // The character after the newline may span several bytes
        std::size_t end = pos + 1;
        if (end < s.size()) {
            ++end;
            while (end < s.size() && (static_cast<unsigned char>(s[end]) & 0xC0) == 0x80) {
                ++end;
            }
        }
        s.erase(pos, end - pos);
        return s;
    }
    int units = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const auto c = static_cast<unsigned char>(s[i]);
        if ((c & 0xC0) == 0x80) {
            continue;
        }
        units += c >= 0xF0 ? 2 : 1;
        if (units > longLine) {
            return s.substr(0, i) + "...";
        }
    }
    return s;
}


//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void MNEFiffContents::print_file_id(std::FILE* out, const FiffTag& tag)
{
    FiffId id = tag.to_fiff_id();
    struct tm* ltime;
    time_t time_val;
    char buf[100];

    std::print(out, "\t{}.{} ", id.version >> 16, id.version & 0xFFFF);
    std::print(out, "0x{:x}{:x} ", static_cast<unsigned int>(id.machid[0]), static_cast<unsigned int>(id.machid[1]));

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

    std::print(out, "{}", buf);
}


//=============================================================================================================

void MNEFiffContents::print_ch_info(std::FILE* out, const FiffTag& tag)
{
    FiffChInfo info = tag.to_ch_info();
    // FIFF_UNIT_NONE has an empty symbol; only units the table does not know are "N/A"
    const std::string unit = FiffExplain::unit(info.unit);
    const std::string text1 = info.unit == FIFF_UNIT_NONE || !unit.empty() ? unit : std::string("N/A");
    const std::string text2 = FiffExplain::unitMultiplier(info.unit_mul);
    const std::string kind = FiffExplain::channelKind(info.kind);
    const std::string text3 = kind.empty() ? std::string("N/A") : kind;

    std::print(out, "\t{:3} \"{}\" ({} {:3}) [{}{}]", info.scanNo, info.ch_name.c_str(), text3, info.logNo, text2, text1);
    std::print(out, "\trange {:g} cal {:g}", qtTieG(info.range), qtTieG(info.cal));
}


//=============================================================================================================

void MNEFiffContents::print_transform(std::FILE* out, const FiffTag& tag)
{
    FiffCoordTrans t = tag.to_coord_trans();
    int k, p;
    int frame;
    std::print(out, "\t");
    for (frame = t.from, k = 0; k < 2; k++) {
        std::print(out, "{}", mne::FiffCoordTrans::frame_name(frame));
        if (k == 0) {
            std::print(out, " -> ");
            frame = t.to;
        } else {
            std::print(out, "\n");
            for (p = 0; p < 3; p++)
                std::print(out, "{:>24}\t{:8.5f} {:8.5f} {:8.5f} {:8.2f}\n", " ", qtTie(t.trans(p, 0), 5), qtTie(t.trans(p, 1), 5), qtTie(t.trans(p, 2), 5), qtTie(1000 * t.trans(p, 3), 2));
            std::print(out, "{:>24}\t{:8.5f} {:8.5f} {:8.5f} {:8.2f}", " ", 0.0, 0.0, 0.0, 1.0);
        }
    }
}


//=============================================================================================================

void MNEFiffContents::print_dig_point(std::FILE* out, const FiffTag& tag)
{
    FiffDigPoint point = tag.to_dig_point();
    switch (point.kind) {
        case FIFFV_POINT_CARDINAL:
            std::print(out, "\tcardinal {:3} ", point.ident);
            break;
        case FIFFV_POINT_HPI:
            std::print(out, "\thpi      {:3} ", point.ident);
            break;
        case FIFFV_POINT_EEG:
            std::print(out, "\teeg      {:3} ", point.ident);
            break;
        case FIFFV_POINT_EXTRA:
            std::print(out, "\textra    {:3} ", point.ident);
            break;
        default:
            break;
    }
    std::print(out, "({:6.1f},{:6.1f},{:6.1f})", qtTie(1000 * point.r[0], 1), qtTie(1000 * point.r[1], 1), qtTie(1000 * point.r[2], 1));
}


//=============================================================================================================

void MNEFiffContents::print_matrix(std::FILE* out, [[maybe_unused]] FiffStream& stream, FiffDirEntry this_ent)
{
    // Simplified matrix info since we don't want to read the whole tag yet if it's large,
    // but FiffStream has some matrix reading logic.
    std::print(out, "\tMatrix type {}, size {} bytes", this_ent.type, this_ent.size);
}


//=============================================================================================================

bool MNEFiffContents::show_fiff_contents(std::FILE* out, const MNEShowFiffSettings& settings)
{
    return show_fiff_contents(out, settings.inname, settings.verbose, settings.tags, settings.indent, settings.long_strings, settings.blocks_only);
}


//=============================================================================================================

bool MNEFiffContents::show_fiff_contents(std::FILE* out, const std::string& name, bool verbose, const std::vector<int>& tags, int indent_step, bool long_strings, bool blocks_only)
{
    FiffStream stream(mne::path_from_utf8(name));

    FiffDirEntry this_ent;
    std::optional<FiffTag> tag;
    int block;
    int count = 0;
    int prev_kind;
    int indent = 0;
    bool show_it = false;
    std::string s;
    bool output_taginfo = false;

    if (!stream.open())
        return false;
    prev_kind = -1;

    if (blocks_only) {
        for (std::size_t i = 0; i < stream.dir().size(); ++i) {
            this_ent = stream.dir()[i];
            if (this_ent.kind == FIFF_BLOCK_START || this_ent.kind == FIFF_BLOCK_END) {
                if (this_ent.kind == FIFF_BLOCK_END)
                    indent = indent - indent_step;
                if (this_ent.kind == FIFF_BLOCK_START) {
                    for (int k = 0; k < indent; k++)
                        std::print(out, " ");
                    if (tag = stream.read_tag(this_ent.pos); tag) {
                        block = *tag->to_int();
                        std::print(out, "{} = {}\n", block, explained(FiffExplain::block(block)));
                    }
                }
                if (this_ent.kind == FIFF_BLOCK_START)
                    indent = indent + indent_step;
            }
        }
    } else {
        for (std::size_t i = 0; i < stream.dir().size(); ++i) {
            this_ent = stream.dir()[i];
            if (this_ent.kind == -1) {
            }
            if (tags.size() == 0)
                show_it = true;
            else {
                show_it = false;
                for (std::size_t k = 0; k < tags.size(); k++) {
                    if (this_ent.kind == tags[k]) {
                        show_it = true;
                        break;
                    }
                }
            }
            if (show_it) {
                if (this_ent.kind == FIFF_BLOCK_START || this_ent.kind == FIFF_BLOCK_END) {
                    if (!verbose) {
                        if (count > 1)
                            std::print(out, " [{}]\n", count);
                        else if (this_ent != stream.dir()[0])
                            std::print(out, "\n");
                    }
                    if (this_ent.kind == FIFF_BLOCK_END)
                        indent = indent - indent_step;
                    for (int k = 0; k < indent; k++)
                        std::print(out, " ");
                    std::print(out, "{:4} = {}", this_ent.kind, explained(FiffExplain::tag(this_ent.kind)));
                    if (tag = stream.read_tag(this_ent.pos); tag) {
                        block = *tag->to_int();
                        std::print(out, "\t{} = {}", block, explained(FiffExplain::block(block)));
                    }
                    if (this_ent.kind == FIFF_BLOCK_START)
                        indent = indent + indent_step;
                    count = 1;
                    if (verbose)
                        std::print(out, "\n");
                } else if (verbose) {
                    for (int k = 0; k < indent; k++)
                        std::print(out, " ");
                    if (output_taginfo) {
                        std::print(out, "{} {} ", this_ent.size, this_ent.type);
                    }
                    std::print(out, "{:4} = {:<18}", this_ent.kind, explained(FiffExplain::tag(this_ent.kind)));
                    if (FiffTag::fiff_type_fundamental(this_ent.type) == FIFFTS_FS_MATRIX) {
                        print_matrix(out, stream, this_ent);
                    } else {
                        switch (this_ent.type) {
                            case FIFFT_INT:
                                if (tag = stream.read_tag(this_ent.pos); tag) {
                                    if (this_ent.kind == FIFF_BLOCK_START ||
                                        this_ent.kind == FIFF_BLOCK_END) {
                                        block = *tag->to_int();
                                        std::print(out, "\t{} = {}", block, explained(FiffExplain::block(block)));
                                    } else if (this_ent.kind == FIFF_MEAS_DATE) {
                                        std::print(out, "\t{}", textDate(static_cast<std::time_t>(tag->to_int()[0])));
                                    } else if (tag->size() == sizeof(fiff_int_t))
                                        std::print(out, "\t{}", *tag->to_int());
                                    else
                                        std::print(out, "\t{} ints", static_cast<int>(tag->size() / sizeof(fiff_int_t)));
                                }
                                break;
                            case FIFFT_UINT:
                                if (tag = stream.read_tag(this_ent.pos); tag) {
                                    if (tag->size() == sizeof(fiff_int_t))
                                        std::print(out, "\t{}", static_cast<int>(*tag->to_unsigned_int()));
                                    else
                                        std::print(out, "\t{} u_ints", static_cast<int>(tag->size() / sizeof(fiff_int_t)));
                                }
                                break;
                            case FIFFT_JULIAN:
                                if (tag = stream.read_tag(this_ent.pos); tag) {
                                    // MNE-CPP doesn't seem to have a direct julian to caldate, skipping for now or simplistic print
                                    std::print(out, "\tJulian date tag");
                                }
                                break;
                            case FIFFT_STRING:
                                if (tag = stream.read_tag(this_ent.pos); tag) {
                                    s = tag->to_string();
                                    if (long_strings)
                                        std::print(out, "\t{}", s.c_str());
                                    else
                                        std::print(out, "\t{}", shortened(s, LONG_LINE).c_str());
                                }
                                break;
                            case FIFFT_FLOAT:
                                if (tag = stream.read_tag(this_ent.pos); tag) {
                                    if (tag->size() == sizeof(fiff_float_t))
                                        std::print(out, "\t{:g}", qtTieG(*tag->to_float()));
                                    else
                                        std::print(out, "\t{} floats", static_cast<int>(tag->size() / sizeof(fiff_float_t)));
                                }
                                break;
                            case FIFFT_DOUBLE:
                                if (tag = stream.read_tag(this_ent.pos); tag) {
                                    if (tag->size() == sizeof(fiff_double_t))
                                        std::print(out, "\t{:g}", qtTieG(*tag->to_double()));
                                    else
                                        std::print(out, "\t{} doubles", static_cast<int>(tag->size() / sizeof(fiff_double_t)));
                                }
                                break;
                            case FIFFT_COMPLEX_FLOAT:
                                if (tag = stream.read_tag(this_ent.pos); tag) {
                                    const float* fdata = tag->to_float();
                                    if (tag->size() == 2 * sizeof(fiff_float_t))
                                        std::print(out, "\t({:g} {:g})", qtTieG(fdata[0]), qtTieG(fdata[1]));
                                    else
                                        std::print(out, "\t{} complex numbers",
                                                   static_cast<int>(tag->size() / (2 * sizeof(fiff_float_t))));
                                }
                                break;
                            case FIFFT_COMPLEX_DOUBLE:
                                if (tag = stream.read_tag(this_ent.pos); tag) {
                                    const double* ddata = tag->to_double();
                                    if (tag->size() == 2 * sizeof(fiff_double_t))
                                        std::print(out, "\t({:g} {:g})", qtTieG(ddata[0]), qtTieG(ddata[1]));
                                    else
                                        std::print(out, "\t{} double complex numbers",
                                                   static_cast<int>(tag->size() / (2 * sizeof(fiff_double_t))));
                                }
                                break;
                            case FIFFT_CH_INFO_STRUCT:
                                if (tag = stream.read_tag(this_ent.pos); tag)
                                    print_ch_info(out, *tag);
                                break;
                            case FIFFT_ID_STRUCT:
                                if (tag = stream.read_tag(this_ent.pos); tag)
                                    print_file_id(out, *tag);
                                break;
                            case FIFFT_DIG_POINT_STRUCT:
                                if (tag = stream.read_tag(this_ent.pos); tag)
                                    print_dig_point(out, *tag);
                                break;
                            case FIFFT_DIG_STRING_STRUCT:
                                if (tag = stream.read_tag(this_ent.pos); tag) {
#ifdef FOO
                                    if ((ds = decode_fiff_dig_string(&tag)) != nullptr)
                                        print_dig_string(ds);
                                    free_fiff_dig_string(ds);
#endif
                                }
                                break;
                            case FIFFT_COORD_TRANS_STRUCT:
                                if (tag = stream.read_tag(this_ent.pos); tag)
                                    print_transform(out, *tag);
                                break;
                            default:
                                if (this_ent.kind == FIFF_DIG_STRING)
                                    std::print(out, "type = {}\n", this_ent.type);
                                if (this_ent.size > 0)
                                    std::print(out, "\t{} bytes", this_ent.size);
                                break;
                        }
                    }
                    std::print(out, "\n");
                } else {
                    if (this_ent.kind != prev_kind) {
                        if (count > 1)
                            std::print(out, " [{}]\n", count);
                        else if (this_ent != stream.dir()[0])
                            std::print(out, "\n");
                        for (int k = 0; k < indent; k++)
                            std::print(out, " ");
                        std::print(out, "{:4} = {}", this_ent.kind, explained(FiffExplain::tag(this_ent.kind)));
                        count = 1;
                    } else
                        count++;
                }
            }
            prev_kind = this_ent.kind;
        }
        if (!verbose) {
            if (count > 1)
                std::print(out, " [{}]\n", count);
            else
                std::print(out, "\n");
        }
    }

    stream.close();

    return true;
}
