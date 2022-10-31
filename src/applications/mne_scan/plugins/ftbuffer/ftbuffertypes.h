#ifndef FTBUFFERTYPES_H
#define FTBUFFERTYPES_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtGlobal>

//=============================================================================================================
// DEFINITION MACROS
//=============================================================================================================

#define VERSION    static_cast<qint16>(0x0001)

#define PUT_HDR    static_cast<qint16>(0x0101) /* decimal 257 */
#define PUT_DAT    static_cast<qint16>(0x0102) /* decimal 258 */
#define PUT_EVT    static_cast<qint16>(0x0103) /* decimal 259 */
#define PUT_OK     static_cast<qint16>(0x0104) /* decimal 260 */
#define PUT_ERR    static_cast<qint16>(0x0105) /* decimal 261 */

#define GET_HDR    static_cast<qint16>(0x0201) /* decimal 513 */
#define GET_DAT    static_cast<qint16>(0x0202) /* decimal 514 */
#define GET_EVT    static_cast<qint16>(0x0203) /* decimal 515 */
#define GET_OK     static_cast<qint16>(0x0204) /* decimal 516 */
#define GET_ERR    static_cast<qint16>(0x0205) /* decimal 517 */

#define WAIT_DAT   static_cast<qint16>(0x0402) /* decimal 1026 */
#define WAIT_OK    static_cast<qint16>(0x0404) /* decimal 1027 */
#define WAIT_ERR   static_cast<qint16>(0x0405) /* decimal 1028 */

#define PUT_HDR_NORESPONSE static_cast<qint16>(0x0501) /* decimal 1281 */
#define PUT_DAT_NORESPONSE static_cast<qint16>(0x0502) /* decimal 1282 */
#define PUT_EVT_NORESPONSE static_cast<qint16>(0x0503) /* decimal 1283 */

//=============================================================================================================
// STRUCT DEFINITIONS
//=============================================================================================================

typedef struct {
    qint32 nchans;
    qint32 nsamples;
    qint32 data_type;
    qint32 bufsize;     /* size of the buffer in bytes */
} datadef_t;

typedef struct {
    qint32  nchans;
    qint32  nsamples;
    qint32  nevents;
    float   fsample;
    qint32  data_type;
    qint32  bufsize;     /* size of the buffer in bytes */
} headerdef_t;

typedef struct {
    qint16 version;   /* see VERSION */
    qint16 command;   /* see PUT_xxx, GET_xxx and FLUSH_xxx */
    qint32 bufsize;   /* size of the buffer in bytes */
} messagedef_t;

typedef struct {
    messagedef_t *def;
    void         *buf;
} message_t;

typedef struct {
    qint32 begsample; /* indexing starts with 0, should be >=0 */
    qint32 endsample; /* indexing starts with 0, should be <header.nsamples */
} datasel_t;

typedef struct {
    qint32 nsamples;
    qint32 nevents;
} samples_events_t;

enum class HeaderChunk : int{
    FT_CHUNK_NEUROMAG_HEADER = 8,
    FT_CHUNK_NEUROMAG_ISOTRAK = 9,
    FT_CHUNK_NEUROMAG_HPIRESULT = 10
};

#endif // FTBUFFERTYPES_H
