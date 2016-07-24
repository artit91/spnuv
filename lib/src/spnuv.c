#include "spnuv.h"

static unsigned init_refcount = 0;
static SpnHashMap *library = NULL;

int not_implemented(SpnValue *ret, int argc, SpnValue argv[], void *ctx) {
    return 1;
}

SPN_LIB_OPEN_FUNC(ctx) {

    if (init_refcount == 0) {
        library = spn_hashmap_new();

        SpnHashMap *loop_api = spnuv_loop_api();
        SpnValue loop = (SpnValue){ .type = SPN_TYPE_HASHMAP, .v.o = loop_api };
        spn_hashmap_set_strkey(library, "Loop", &loop);
        spn_value_release(&loop);

        SpnHashMap *tcp_api = spnuv_tcp_api();
        SpnValue tcp = (SpnValue){ .type = SPN_TYPE_HASHMAP, .v.o = tcp_api };
        spn_hashmap_set_strkey(library, "TCP", &tcp);
        spn_value_release(&tcp);
    }

    init_refcount += 1;
    spn_object_retain(library);

    return (SpnValue){ .type = SPN_TYPE_HASHMAP, .v.o = library };
}

SPN_LIB_CLOSE_FUNC(ctx) {
    init_refcount -= 1;
    if (init_refcount == 0) {
        spnuv_loop_api_destroy();
        spnuv_tcp_api_destroy();

        spn_object_release(library);
        library = NULL;
    }
}