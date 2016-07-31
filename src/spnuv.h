#ifndef __spnuv__
#define __spnuv__

/* TODO: check params */

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

typedef struct SpnUVApi {
        const char *name;
        SpnHashMap *(*fn)(void);
} SpnUVApi;

typedef struct SpnUVLoopBuffer {
        char slab[SPNUV_SLAB_SIZE];
        int in_use;
} SpnUVLoopBuffer;

typedef struct SpnUVWriteData {
        void *ctx;
        SpnFunction *callback;
} SpnUVWriteData;

int spnuv_parse_addr(char *, int, struct sockaddr_storage *);
SpnValue spnuv_get_error(int, const char *, const char *);
int spnuv_close(SpnValue *, int, SpnValue [], void *);

#endif
