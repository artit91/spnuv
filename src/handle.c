#include "spnuv.h"

void spnuv_handle_close_cb(uv_handle_t *handle) {
        SpnHashMap *self = handle->data;

        SpnValue context_value = spn_hashmap_get_strkey(self, "closeContext");
        void *ctx = spn_ptrvalue(&context_value);
        SpnValue fn_value = spn_hashmap_get_strkey(self, "closeCallback");
        SpnFunction *func = spn_funcvalue(&fn_value);
        int err;

        free(handle);

        if (ctx) {
                err = spn_ctx_callfunc(ctx, func, NULL, 0, NULL);
                if (err) {
                        fprintf(stderr, "%s\n", spn_ctx_geterrmsg(ctx));
                }
        }
}

int spnuv_handle_close(SpnValue *ret, int argc, SpnValue argv[], void *ctx) {
        SpnHashMap *self;
        SpnValue value;
        uv_handle_t *handle;

        spn_value_retain(&argv[0]);
        self = spn_hashmapvalue(&argv[0]);

        if (argc > 1) {
                value = spn_makerawptr(ctx);
                spn_hashmap_set_strkey(self, "closeCallback", &argv[1]);
                spn_hashmap_set_strkey(self, "closeContext", &value);
                spn_value_release(&value);
        }

        value = spn_hashmap_get_strkey(self, "handle");
        handle = spn_ptrvalue(&value);

        uv_close(handle, spnuv_handle_close_cb);

        return 0;
}
