#include "spnuv.h"

static SpnHashMap *default_loop = NULL;

int spnuv_loop_run(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue value;
        uv_loop_t *uv_loop;

        spn_value_retain(&argv[0]);

        self = spn_hashmapvalue(&argv[0]);
        value = spn_hashmap_get_strkey(self, "uv_loop");
        uv_loop = spn_ptrvalue(&value);

        return uv_run(uv_loop, UV_RUN_DEFAULT);
}

int spnuv_loop_stop(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue value;
        uv_loop_t *uv_loop;

        spn_value_retain(&argv[0]);

        self = spn_hashmapvalue(&argv[0]);
        value = spn_hashmap_get_strkey(self, "uv_loop");
        uv_loop = spn_ptrvalue(&value);

        uv_stop(uv_loop);

        return  0;
}

SpnHashMap *spnuv_loop_new(int is_default)
{
        SpnHashMap *self = spn_hashmap_new();
        size_t i;
        SpnValue value;
        uv_loop_t *uv_loop;

        static const SpnExtFunc fns[] = {
                { "run", spnuv_loop_run },
                { "stop", spnuv_loop_stop }
        };

        /* TODO: free SpnUVLoopBuffer */
        SpnUVLoopBuffer *buffer = malloc(sizeof(SpnUVLoopBuffer));
        buffer->in_use = 0;

        for (i = 0; i < COUNT(fns); i += 1) {
                SpnValue fnval = spn_makenativefunc(fns[i].name, fns[i].fn);
                spn_hashmap_set_strkey(self, fns[i].name, &fnval);
                spn_value_release(&fnval);
        }

        value = spn_makebool(is_default);
        spn_hashmap_set_strkey(self, "isDefault", &value);
        spn_value_release(&value);

        uv_loop = uv_default_loop();

        value = spn_makerawptr(uv_loop);
        spn_hashmap_set_strkey(self, "uv_loop", &value);
        spn_value_release(&value);

        value = spn_makerawptr(buffer);
        spn_hashmap_set_strkey(self, "buffer", &value);
        spn_value_release(&value);

        uv_loop->data = self;

        return self;
}

int spnuv_loop_default(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnValue res;

        res.type = SPN_TYPE_OBJECT;

        if (!default_loop) {
                SpnHashMap *val = spnuv_loop_new(1);
                if (!val) {
                        return 1;
                }
                default_loop = val;
        }

        res.v.o = default_loop;

        *ret = res;
        return 0;
}

SpnHashMap *spnuv_loop_api(void)
{
        SpnHashMap *api = spn_hashmap_new();
        size_t i;

        static const SpnExtFunc fns[] = {
                { "defaultLoop", spnuv_loop_default }
        };

        for (i = 0; i < COUNT(fns); i += 1) {
                SpnValue fnval = spn_makenativefunc(fns[i].name, fns[i].fn);
                spn_hashmap_set_strkey(api, fns[i].name, &fnval);
                spn_value_release(&fnval);
        }

        return api;
}

void spnuv_loop_api_destroy(void)
{
        spn_object_release(default_loop);
}
