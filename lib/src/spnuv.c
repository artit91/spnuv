#include "spnuv.h"

static unsigned init_refcount = 0;
static SpnHashMap *library = NULL;

int not_implemented(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
    return 1;
}

SPN_LIB_OPEN_FUNC(ctx)
{
    /* TODO: refactor */
    SpnValue library_value;
    SpnValue loop_value;
    SpnValue tcp_value;
    SpnHashMap *loop_api;
    SpnHashMap *tcp_api;

    if (init_refcount == 0) {
        library = spn_hashmap_new();

        loop_api = spnuv_loop_api();
        loop_value.type = SPN_TYPE_HASHMAP;
        loop_value.v.o = loop_api;
        spn_hashmap_set_strkey(library, "Loop", &loop_value);
        spn_value_release(&loop_value);

        tcp_api = spnuv_tcp_api();
        tcp_value.type = SPN_TYPE_HASHMAP;
        tcp_value.v.o = tcp_api;
        spn_hashmap_set_strkey(library, "TCP", &tcp_value);
        spn_value_release(&tcp_value);
    }

    init_refcount += 1;

    library_value.type = SPN_TYPE_HASHMAP;
    library_value.v.o = library;

    spn_object_retain(library);

    return library_value;
}

SPN_LIB_CLOSE_FUNC(ctx)
{
    init_refcount -= 1;
    if (init_refcount == 0) {
        spnuv_loop_api_destroy();
        spnuv_tcp_api_destroy();

        spn_object_release(library);
        library = NULL;
    }
}
