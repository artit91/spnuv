#ifndef __spnuv__
#define __spnuv__

#define USE_DYNAMIC_LOADING 1

#include "uv.h"

#include "ctx.h"
#include "str.h"
#include "hashmap.h"

int not_implemented(SpnValue *, int, SpnValue[], void *);

SpnHashMap *spnuv_loop_api(void);
void spnuv_loop_api_destroy(void);

SpnHashMap *spnuv_tcp_api(void);
void spnuv_tcp_api_destroy(void);

#define getuinfo(type,val) (type*)(spn_objvalue(&val))

#endif
