#include "spnuv.h"

int tcp_bind(SpnValue *ret, int argc, SpnValue argv[], void *ctx) {
    spn_value_retain(&argv[0]); //self
    spn_value_retain(&argv[1]); //address
    spn_value_retain(&argv[2]); //port
    //spn_value_retain(&argv[3]); //flags
    SpnHashMap *self = spn_hashmapvalue(&argv[0]);
    SpnValue val = spn_hashmap_get_strkey(self, "uv_loop");
    uv_loop_t *uv_loop = getuinfo(uv_loop_t, val);

    uv_tcp_t *tcp_h = malloc(sizeof(uv_tcp_t));
    struct sockaddr_in addr;
    uv_ip4_addr("127.0.0.1", 8080, &addr);

    uv_tcp_init(uv_loop, tcp_h);
    tcp_h->data = self;

    SpnValue u_info = spn_makeweakuserinfo(tcp_h);
    spn_hashmap_set_strkey(self, "tcp_h", &u_info);
    spn_value_release(&u_info);

    return uv_tcp_bind(tcp_h, (struct sockaddr *)&addr, 0);
}

void tcp_listen_cb(uv_stream_t *handle, int status) {
    uv_tcp_t *tcp_h = (uv_tcp_t *)handle;
    SpnHashMap *self = tcp_h->data;
    SpnValue context_val = spn_hashmap_get_strkey(self, "listenContext");
    void *ctx = getuinfo(void *, context_val);
    SpnValue fn_value = spn_hashmap_get_strkey(self, "listenCallback");

    SpnFunction *func = spn_funcvalue(&fn_value);
    SpnValue err_value;
    if (status != 0) {
        err_value = spn_makeint(status);
    } else {
        err_value = spn_nilval;
    }
    SpnValue argv[] = { err_value };
    spn_ctx_callfunc(ctx, func, NULL, 1, argv);
}

int tcp_listen(SpnValue *ret, int argc, SpnValue argv[], void *ctx) {
    spn_value_retain(&argv[0]); //self
    spn_value_retain(&argv[1]); //callback

    SpnHashMap *self = spn_hashmapvalue(&argv[0]);
    SpnValue val = spn_hashmap_get_strkey(self, "tcp_h");
    uv_tcp_t *tcp_h = getuinfo(uv_tcp_t, val);

    spn_hashmap_set_strkey(self, "listenCallback", &argv[1]);
    SpnValue u_info = spn_makeweakuserinfo(ctx);
    spn_hashmap_set_strkey(self, "listenContext", &u_info);
    spn_value_release(&u_info);

    return uv_listen((uv_stream_t *)tcp_h, 511, tcp_listen_cb);
}

int tcp_accept(SpnValue *ret, int argc, SpnValue argv[], void *ctx) {
    spn_value_retain(&argv[0]); //self
    spn_value_retain(&argv[1]); //client

    SpnHashMap *self = spn_hashmapvalue(&argv[0]);
    SpnValue val = spn_hashmap_get_strkey(self, "tcp_h");
    uv_tcp_t *tcp_h = getuinfo(uv_tcp_t, val);

    SpnHashMap *client = spn_hashmapvalue(&argv[1]);
    SpnValue client_loop = spn_hashmap_get_strkey(self, "uv_loop");
    uv_loop_t *client_uv_loop = getuinfo(uv_loop_t, client_loop);

    uv_tcp_t *client_tcp_h = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(client_uv_loop, client_tcp_h);
    client_tcp_h->data = client;

    SpnValue u_info = spn_makeweakuserinfo(client_tcp_h);
    spn_hashmap_set_strkey(client, "tcp_h", &u_info);
    spn_value_release(&u_info);

    return uv_accept((uv_stream_t *)tcp_h, (uv_stream_t *)client_tcp_h);
}

void tcp_read_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t *buf) {
    uv_buf_t suggested_buffer = uv_buf_init(malloc(suggested_size), suggested_size);
    *buf = suggested_buffer;
}

void tcp_read_cb(uv_stream_t* handle, int nread, const uv_buf_t* buf) {
    uv_tcp_t *tcp_h = (uv_tcp_t *)handle;
    SpnHashMap *self = tcp_h->data;
    SpnValue context_val = spn_hashmap_get_strkey(self, "readContext");
    void *ctx = getuinfo(void *, context_val);
    SpnValue fn_value = spn_hashmap_get_strkey(self, "readCallback");
    SpnFunction *func = spn_funcvalue(&fn_value);
    char *data = NULL;
    int err = 0;
    if (nread < 0) {
        err = nread;
        if (buf->base) {
            free(buf->base);
        }
        uv_read_stop(handle);
    } else if (nread == 0) {
        free(buf->base);
    } else {
        data = malloc((nread + 1) * sizeof(char));
        strncpy(data, buf->base, nread);
        free(buf->base);
        data[nread] = '\0';
    }
    SpnValue err_value;
    if (err != 0) {
        err_value = spn_makeint(err);
    } else {
        err_value = spn_nilval;
    }
    SpnValue data_value;
    if (data) {
        data_value = spn_makestring(data);
        free(data);
    } else {
        data_value = spn_nilval;
    }
    SpnValue argv[] = { err_value, data_value };
    spn_ctx_callfunc(ctx, func, NULL, 2, argv);
}

int tcp_read(SpnValue *ret, int argc, SpnValue argv[], void *ctx) {
    spn_value_retain(&argv[0]); //self
    spn_value_retain(&argv[1]); //callback

    SpnHashMap *self = spn_hashmapvalue(&argv[0]);
    SpnValue val = spn_hashmap_get_strkey(self, "tcp_h");
    uv_tcp_t *tcp_h = getuinfo(uv_tcp_t, val);

    spn_hashmap_set_strkey(self, "readCallback", &argv[1]);
    SpnValue u_info = spn_makeweakuserinfo(ctx);
    spn_hashmap_set_strkey(self, "readContext", &u_info);
    spn_value_release(&u_info);

    return uv_read_start((uv_stream_t *)tcp_h, (uv_alloc_cb)tcp_read_alloc_cb, (uv_read_cb)tcp_read_cb);
}

void tcp_write_cb(uv_write_t* req, int status) {}

int tcp_write(SpnValue *ret, int argc, SpnValue argv[], void *ctx) {
    spn_value_retain(&argv[0]); //self
    spn_value_retain(&argv[1]); //data

    SpnHashMap *self = spn_hashmapvalue(&argv[0]);
    SpnValue val = spn_hashmap_get_strkey(self, "tcp_h");
    uv_tcp_t *tcp_h = getuinfo(uv_tcp_t, val);

    SpnString *str = spn_stringvalue(&argv[1]);

    char buffer[4096];
    uv_buf_t buf = uv_buf_init(buffer, str->len);

    buf.len = str->len;
    buf.base = str->cstr;

    uv_write_t req;

    return uv_write(&req, (uv_stream_t *)tcp_h, &buf, 1, tcp_write_cb);
}

int tcp_new(SpnValue *ret, int argc, SpnValue argv[], void *ctx) {
    spn_value_retain(&argv[1]);
    SpnHashMap *loop = spn_hashmapvalue(&argv[1]);
    SpnValue val = spn_hashmap_get_strkey(loop, "uv_loop");
    uv_loop_t *uv_loop = getuinfo(uv_loop_t, val);

    SpnHashMap *members = spn_hashmap_new();
    
    static const SpnExtFunc fns[] = {
        { "bind", tcp_bind },
        { "listen", tcp_listen },
        { "accept", tcp_accept },
        { "read", tcp_read },
        { "write", tcp_write },
    };

    for (size_t i = 0; i < 5; i += 1) {
        SpnValue fnval = spn_makenativefunc(fns[i].name, fns[i].fn);
        spn_hashmap_set_strkey(members, fns[i].name, &fnval);
        spn_value_release(&fnval);
    }

    SpnValue u_info = spn_makeweakuserinfo(uv_loop);
    spn_hashmap_set_strkey(members, "uv_loop", &u_info);
    spn_value_release(&u_info);

    *ret = (SpnValue){ .type = SPN_TYPE_HASHMAP, .v.o = members };
    return 0;
}

SpnHashMap *spnuv_tcp_api(void) {
    SpnHashMap *api = spn_hashmap_new();

    static const SpnExtFunc fns[] = {
        { "new", tcp_new }
    };

    for (size_t i = 0; i < 1; i += 1) {
        SpnValue fnval = spn_makenativefunc(fns[i].name, fns[i].fn);
        spn_hashmap_set_strkey(api, fns[i].name, &fnval);
        spn_value_release(&fnval);
    }

    return api;
}

void spnuv_tcp_api_destroy(void) {}