#ifndef __spnuv__
#define __spnuv__

/* TODO:  handle + tcp + stream + util refcount, check self + params,
   error messages */

#define USE_DYNAMIC_LOADING 1
#define SPNUV_SLAB_SIZE 65536
#define COUNT(x) (sizeof(x) / sizeof((x)[0]))

#include "uv.h"

#include "ctx.h"
#include "str.h"
#include "hashmap.h"

SpnHashMap *spnuv_loop_api(void);
SpnHashMap *spnuv_tcp_api(void);
SpnHashMap *spnuv_signal_api(void);

void spnuv_loop_api_destroy(void);

/* loop.c */
typedef struct SpnUVLoopBuffer {
        char slab[SPNUV_SLAB_SIZE];
        int in_use;
} SpnUVLoopBuffer;

/* stream.c */
typedef struct SpnUVWriteData {
        void *ctx;
        SpnFunction *callback;
} SpnUVWriteData;

int spnuv_stream_write(SpnValue *, int, SpnValue [], void *);
int spnuv_stream_read(SpnValue *, int, SpnValue [], void *);

/* handle.c */
int spnuv_handle_close(SpnValue *, int, SpnValue [], void *);

/* util.c */
int spnuv_parse_addr(char *, int, struct sockaddr_storage *);
SpnValue spnuv_get_error(int, const char *, const char *);

#endif
