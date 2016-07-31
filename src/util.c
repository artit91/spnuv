#include "spnuv.h"

int spnuv_parse_addr(char *host, int port, struct sockaddr_storage *ss) {
        struct in_addr addr4;
        struct in6_addr addr6;
        struct sockaddr_in *sa4;
        struct sockaddr_in6 *sa6;
        unsigned int scope_id;
        unsigned int flowinfo;

        flowinfo = scope_id = 0;

        memset(ss, 0, sizeof(struct sockaddr_storage));

        if (host[0] == '\0') {
                sa4 = (struct sockaddr_in *) ss;
                sa4->sin_family = AF_INET;
                sa4->sin_port = htons((short)port);
                sa4->sin_addr.s_addr = INADDR_ANY;
                return 0;
        } else if (host[0] == '<' && strcmp(host, "<broadcast>") == 0) {
                sa4 = (struct sockaddr_in *)ss;
                sa4->sin_family = AF_INET;
                sa4->sin_port = htons((short)port);
                sa4->sin_addr.s_addr = INADDR_BROADCAST;
                return 0;
        } else if (uv_inet_pton(AF_INET, host, &addr4) == 0) {
                /* it's an IPv4 address */
                sa4 = (struct sockaddr_in *)ss;
                sa4->sin_family = AF_INET;
                sa4->sin_port = htons((short)port);
                sa4->sin_addr = addr4;
                return 0;
        } else if (uv_inet_pton(AF_INET6, host, &addr6) == 0) {
                /* it's an IPv6 address */
                sa6 = (struct sockaddr_in6 *)ss;
                sa6->sin6_family = AF_INET6;
                sa6->sin6_port = htons((short)port);
                sa6->sin6_addr = addr6;
                sa6->sin6_flowinfo = flowinfo;
                sa6->sin6_scope_id = scope_id;
                return 0;
        } else {
                return -1;
        }

        return 0;
}

SpnValue spnuv_get_error(int code, const char *name, const char *message) {
        SpnHashMap *error = spn_hashmap_new();
        SpnValue ret;
        SpnValue value;

        value = spn_makeint(code);
        spn_hashmap_set_strkey(error, "code", &value);
        spn_value_release(&value);

        value = spn_makestring(name);
        spn_hashmap_set_strkey(error, "name", &value);
        spn_value_release(&value);

        value = spn_makestring(message);
        spn_hashmap_set_strkey(error, "message", &value);
        spn_value_release(&value);

        ret.type = SPN_TYPE_OBJECT;
        ret.v.o = error;

        return ret;
}

void spnuv_close_cb(uv_handle_t *handle) {
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

int spnuv_close(SpnValue *ret, int argc, SpnValue argv[], void *ctx) {
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

        uv_close(handle, spnuv_close_cb);

        return 0;
}
