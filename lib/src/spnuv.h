#ifndef __spnuv__
#define __spnuv__

/* TODO: error objects, check params, implement all loop and tcp calls */

#define USE_DYNAMIC_LOADING 1
#define GETUINFO(type, val) (type*)(spn_objvalue(&val))
#define COUNT(x)  (sizeof(x) / sizeof((x)[0]))

#include "uv.h"

#include "ctx.h"
#include "str.h"
#include "hashmap.h"

int not_implemented(SpnValue *, int, SpnValue[], void *);

SpnHashMap *spnuv_loop_api(void);
void spnuv_loop_api_destroy(void);

SpnHashMap *spnuv_tcp_api(void);
void spnuv_tcp_api_destroy(void);

typedef struct SpnUVApi {
        const char *name;
        SpnHashMap *(*fn)(void);
} SpnUVApi;

#endif
