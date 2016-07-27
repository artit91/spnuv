#include "spnuv.h"

static SpnHashMap *default_loop = NULL;

int loop_run(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue val;
        uv_loop_t *uv_loop;

        spn_value_retain(&argv[0]);

        self = spn_hashmapvalue(&argv[0]);
        val = spn_hashmap_get_strkey(self, "uv_loop");
        uv_loop = GETUINFO(uv_loop_t, val);

        return uv_run(uv_loop, UV_RUN_DEFAULT);
}

int loop_stop(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue val;
        uv_loop_t *uv_loop;

        spn_value_retain(&argv[0]);

        self = spn_hashmapvalue(&argv[0]);
        val = spn_hashmap_get_strkey(self, "uv_loop");
        uv_loop = GETUINFO(uv_loop_t, val);

        uv_stop(uv_loop);

        return  0;
}

int loop_now(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue val; 
        uv_loop_t *uv_loop;

        spn_value_retain(&argv[0]);

        self = spn_hashmapvalue(&argv[0]);
        val = spn_hashmap_get_strkey(self, "uv_loop");
        uv_loop = GETUINFO(uv_loop_t, val);

        *ret = spn_makeint(uv_now(uv_loop));
        return 0;
}

int loop_update_time(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue val;
        uv_loop_t *uv_loop;

        spn_value_retain(&argv[0]);

        self = spn_hashmapvalue(&argv[0]);
        val = spn_hashmap_get_strkey(self, "uv_loop");
        uv_loop = GETUINFO(uv_loop_t, val);

        uv_update_time(uv_loop);

        return 0;
}

int loop_fileno(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue val;
        uv_loop_t *uv_loop;

        spn_value_retain(&argv[0]);

        self = spn_hashmapvalue(&argv[0]);
        val = spn_hashmap_get_strkey(self, "uv_loop");
        uv_loop = GETUINFO(uv_loop_t, val);

        *ret = spn_makeint(uv_backend_fd(uv_loop));

        return 0;
}

int loop_get_timeout(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue val;
        uv_loop_t *uv_loop;

        spn_value_retain(&argv[0]);

        self = spn_hashmapvalue(&argv[0]);
        val = spn_hashmap_get_strkey(self, "uv_loop");
        uv_loop = GETUINFO(uv_loop_t, val);

        *ret = spn_makefloat(uv_backend_timeout(uv_loop) / 1000.0);

        return 0;
}

int loop_isalive(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue val;
        uv_loop_t *uv_loop;

        spn_value_retain(&argv[0]);

        self = spn_hashmapvalue(&argv[0]);
        val = spn_hashmap_get_strkey(self, "uv_loop");
        uv_loop = GETUINFO(uv_loop_t, val);

        *ret = spn_makebool(uv_loop_alive(uv_loop));

        return 0;
}

SpnHashMap *new_loop(int is_default)
{
        SpnHashMap *members = spn_hashmap_new();
        size_t i;
        SpnValue value;
        uv_loop_t *uv_loop;

        static const SpnExtFunc fns[] = {
                { "run", loop_run },
                { "stop", loop_stop },
                { "now", loop_now },
                { "updateTime", loop_update_time },
                { "fileno", loop_fileno },
                { "getTimeout", loop_get_timeout },
                { "queueWork", not_implemented },
                { "excepthook", not_implemented },
                { "isAlive", loop_isalive },
                { "getHandles", not_implemented }
        };

        for (i = 0; i < COUNT(fns); i += 1) {
                SpnValue fnval = spn_makenativefunc(fns[i].name, fns[i].fn);
                spn_hashmap_set_strkey(members, fns[i].name, &fnval);
                spn_value_release(&fnval);
        }

        value = spn_makebool(is_default);
        spn_hashmap_set_strkey(members, "isDefault", &value);
        spn_value_release(&value);

        uv_loop = uv_default_loop();

        value = spn_makeweakuserinfo(uv_loop);
        spn_hashmap_set_strkey(members, "uv_loop", &value);
        spn_value_release(&value);

        return members;
}

int loop_default(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnValue res;

        res.type = SPN_TYPE_HASHMAP;

        if (!default_loop) {
                SpnHashMap *val = new_loop(1);
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
                { "defaultLoop", loop_default },
                { "new", not_implemented }
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
