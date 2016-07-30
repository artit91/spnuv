#ifndef __spnuv__
#define __spnuv__

/* TODO: error objects */
/* TODO: check params */
/* TODO: test context/callbacks */

#define USE_DYNAMIC_LOADING 1
#define SPNUV_SLAB_SIZE 65536
#define COUNT(x) (sizeof(x) / sizeof((x)[0]))

#include "uv.h"

#include "ctx.h"
#include "str.h"
#include "hashmap.h"

int spnuv_not_implemented(SpnValue *, int, SpnValue[], void *);

SpnHashMap *spnuv_loop_api(void);
void spnuv_loop_api_destroy(void);

SpnHashMap *spnuv_tcp_api(void);
void spnuv_tcp_api_destroy(void);

typedef struct SpnUVApi {
        const char *name;
        SpnHashMap *(*fn)(void);
} SpnUVApi;

typedef struct SpnUVLoopBuffer{
        char slab[SPNUV_SLAB_SIZE];
        int in_use;
} SpnUVLoopBuffer;

int spnuv_util_parse_addr(char *, int, struct sockaddr_storage *);

#endif
