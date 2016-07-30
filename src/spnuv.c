#include "spnuv.h"

static unsigned init_refcount = 0;
static SpnHashMap *library = NULL;

SPN_LIB_OPEN_FUNC(ctx)
{
        SpnValue library_value;

        if (init_refcount == 0) {
                static const SpnUVApi apis[] = {
                        { "Loop", spnuv_loop_api },
                        { "TCP", spnuv_tcp_api },
                };

                SpnHashMap *api;

                size_t i;

                library = spn_hashmap_new();


                for (i = 0; i < COUNT(apis); i += 1) {
                        SpnValue value;
                        api = apis[i].fn();
                        value.type = SPN_TYPE_OBJECT;
                        value.v.o = api;
                        spn_hashmap_set_strkey(library, apis[i].name, &value);
                        spn_value_release(&value);
                }
        }

        init_refcount += 1;

        library_value.type = SPN_TYPE_OBJECT;
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
