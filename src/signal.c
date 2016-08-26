#include "spnuv.h"

static size_t spnuv_signal_seq = 0;

void spnuv_signal_start_cb(uv_signal_t *handle, int signum) {
        SpnHashMap *self = handle->data;
        SpnValue context_value = spn_hashmap_get_strkey(self, "signalContext");
        SpnValue fn_value = spn_hashmap_get_strkey(self, "signalCallback");
        void *ctx = spn_ptrvalue(&context_value);
        SpnFunction *func = spn_funcvalue(&fn_value);
        SpnValue argv[1];
        int err;

        argv[0] = spn_makeint(signum);

        err = spn_ctx_callfunc(ctx, func, NULL, 1, argv);
        if (err) {
                fprintf(stderr, "%s\n", spn_ctx_geterrmsg(ctx));
        }

        spn_value_release(&fn_value);
        spn_object_release(self);
}

int spnuv_signal_start(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue value;
        uv_signal_t *handle;
        int signum;

        spn_value_retain(&argv[0]);
        spn_value_retain(&argv[1]);
        spn_value_retain(&argv[2]);

        self = spn_hashmapvalue(&argv[0]);
        value = spn_hashmap_get_strkey(self, "handle");
        handle = spn_ptrvalue(&value);

        signum = spn_intvalue(&argv[1]);

        value = spn_makerawptr(ctx);
        spn_hashmap_set_strkey(self, "signalCallback", &argv[2]);
        spn_hashmap_set_strkey(self, "signalContext", &value);

        handle->data = self;

        spn_value_release(&argv[2]);
        spn_value_release(&argv[1]);

        return uv_signal_start(handle, spnuv_signal_start_cb, signum);
}

int spnuv_signal_new(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *members = spn_hashmap_new();

        SpnHashMap *loop;
        SpnValue value;
        uv_loop_t *uv_loop;
        uv_signal_t *handle;

        size_t i;

        SpnValue res;

        static const SpnExtFunc fns[] = {
                { "start", spnuv_signal_start },
                { "close", spnuv_handle_close }
        };

        spn_value_retain(&argv[1]);

        spnuv_signal_seq += 1;

        loop = spn_hashmapvalue(&argv[1]);
        value = spn_hashmap_get_strkey(loop, "handle");
        uv_loop = spn_ptrvalue(&value);

        for (i = 0; i < COUNT(fns); i += 1) {
                SpnValue fnval = spn_makenativefunc(fns[i].name, fns[i].fn);
                spn_hashmap_set_strkey(members, fns[i].name, &fnval);
                spn_value_release(&fnval);
        }

        value = spn_makeint(spnuv_signal_seq);
        spn_hashmap_set_strkey(members, "id", &value);

        value = spn_makerawptr(uv_loop);
        spn_hashmap_set_strkey(members, "uv_loop", &value);

        handle = malloc(sizeof(uv_signal_t));
        value = spn_makerawptr(handle);
        spn_hashmap_set_strkey(members, "handle", &value);

        res.type = SPN_TYPE_OBJECT;
        res.v.o = members;

        *ret = res;

        spn_value_release(&argv[1]);

        return uv_signal_init(uv_loop, handle);
}

SpnHashMap *spnuv_signal_api(void)
{
        SpnHashMap *api = spn_hashmap_new();
        size_t i;

        static const SpnExtFunc fns[] = {
                { "new", spnuv_signal_new }
        };

        for (i = 0; i < COUNT(fns); i += 1) {
                SpnValue fnval = spn_makenativefunc(fns[i].name, fns[i].fn);
                spn_hashmap_set_strkey(api, fns[i].name, &fnval);
                spn_value_release(&fnval);
        }

        return api;
}
