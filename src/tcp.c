#include "spnuv.h"

static size_t spnuv_tcp_seq = 0;

int spnuv_tcp_bind(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue value;
        SpnString *host;
        long port;
        long flags = 0;
        uv_loop_t *uv_loop;

        uv_tcp_t *handle = malloc(sizeof(uv_tcp_t));
        struct sockaddr_storage addr;

        spn_value_retain(&argv[0]);
        self = spn_hashmapvalue(&argv[0]);
        spn_value_retain(&argv[1]);
        host = spn_stringvalue(&argv[1]);
        spn_value_retain(&argv[2]);
        port = spn_intvalue(&argv[2]);

        if (argc > 3) {
                spn_value_retain(&argv[3]);
                flags = spn_intvalue(&argv[3]);
        }

        value = spn_hashmap_get_strkey(self, "uv_loop");
        uv_loop = spn_ptrvalue(&value);

        if (spnuv_parse_addr(host->cstr, port, &addr)) {
                fprintf(stderr, "Host parse error\n");
                return -1;
        }

        uv_tcp_init(uv_loop, handle);
        handle->data = self;

        value = spn_makerawptr(handle);
        spn_hashmap_set_strkey(self, "handle", &value);
        spn_value_release(&value);

        return uv_tcp_bind(handle, (struct sockaddr *)&addr, flags);
}

void spnuv_tcp_listen_cb(uv_stream_t *handle, int status)
{
        SpnHashMap *self = handle->data;
        SpnValue context_value = spn_hashmap_get_strkey(self, "listenContext");
        SpnValue fn_value = spn_hashmap_get_strkey(self, "listenCallback");
        void *ctx = spn_ptrvalue(&context_value);
        SpnFunction *func = spn_funcvalue(&fn_value);
        SpnValue err_value;
        SpnValue argv[1];
        int err;

        if (status != 0) {
                err_value = spnuv_get_error(status, uv_err_name(status),
                                            uv_strerror(status));
        } else {
                err_value = spn_nilval;
        }

        argv[0] = err_value;

        err = spn_ctx_callfunc(ctx, func, NULL, 1, argv);
        if (err) {
                fprintf(stderr, "%s\n", spn_ctx_geterrmsg(ctx));
        }
}

int spnuv_tcp_listen(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        uv_tcp_t *handle;
        SpnValue value;
        int backlog = 511;

        spn_value_retain(&argv[0]);
        self = spn_hashmapvalue(&argv[0]);
        spn_value_retain(&argv[1]);

        if (argc > 2) {
                backlog = spn_intvalue(&argv[1]);
                spn_value_retain(&argv[2]);
                spn_hashmap_set_strkey(self, "listenCallback", &argv[2]);
        } else {
                spn_hashmap_set_strkey(self, "listenCallback", &argv[1]);
        }

        value = spn_hashmap_get_strkey(self, "handle");
        handle = spn_ptrvalue(&value);
        
        value = spn_makerawptr(ctx);
        spn_hashmap_set_strkey(self, "listenContext", &value);
        spn_value_release(&value);

        return uv_listen((uv_stream_t *)handle, backlog, spnuv_tcp_listen_cb);
}

int spnuv_tcp_accept(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        uv_tcp_t *handle;

        SpnHashMap *client;
        uv_loop_t *client_uv_loop;

        uv_tcp_t *client_handle = malloc(sizeof(uv_tcp_t));

        SpnValue value;

        spn_value_retain(&argv[0]);
        spn_value_retain(&argv[1]);

        self = spn_hashmapvalue(&argv[0]);
        value = spn_hashmap_get_strkey(self, "handle");
        handle = spn_ptrvalue(&value);

        client = spn_hashmapvalue(&argv[1]);
        value = spn_hashmap_get_strkey(self, "uv_loop");
        client_uv_loop = spn_ptrvalue(&value);

        uv_tcp_init(client_uv_loop, client_handle);
        client_handle->data = client;

        value = spn_makerawptr(client_handle);
        spn_hashmap_set_strkey(client, "handle", &value);
        spn_value_release(&value);

        return uv_accept((uv_stream_t *)handle, (uv_stream_t *)client_handle);
}

void spnuv_tcp_read_alloc_cb(uv_handle_t* handle, size_t suggested_size,
                             uv_buf_t *buf)
{
        SpnHashMap *loop = handle->loop->data;
        SpnValue value = spn_hashmap_get_strkey(loop, "buffer");
        SpnUVLoopBuffer *buffer = spn_ptrvalue(&value);

        if (buffer->in_use) {
                buf->base = NULL;
                buf->len = 0;
        } else {
                buf->base = buffer->slab;
                buf->len = sizeof(buffer->slab);
                buffer->in_use = 1;
        }
}

void spnuv_tcp_read_cb(uv_stream_t* handle, int nread, const uv_buf_t* buf)
{
        SpnHashMap *self = handle->data;
        SpnHashMap *loop = handle->loop->data;

        SpnValue buffer_value = spn_hashmap_get_strkey(loop, "buffer");
        SpnUVLoopBuffer* buffer = spn_ptrvalue(&buffer_value);

        SpnValue context_value = spn_hashmap_get_strkey(self, "readContext");
        void *ctx = spn_ptrvalue(&context_value);
        SpnValue fn_value = spn_hashmap_get_strkey(self, "readCallback");
        SpnFunction *func = spn_funcvalue(&fn_value);

        char *data = NULL;
        int err = 0;
        SpnValue err_value;
        SpnValue data_value;
        SpnValue argv[2];

        if (nread < 0) {
                err = nread;
                uv_read_stop(handle);
        } else if (nread != 0) {
                data = malloc(nread + 1);
                strncpy(data, buf->base, nread);
                data[nread] = '\0';
        }

        if (err != 0) {
                err_value = spnuv_get_error(err, uv_err_name(err),
                                            uv_strerror(err));
        } else {
                err_value = spn_nilval;
        }

        if (data) {
                data_value = spn_makestring(data);
                free(data);
        } else {
                data_value = spn_nilval;
        }

        argv[0] = err_value;
        argv[1] = data_value;

        buffer->in_use = 0;
        err = spn_ctx_callfunc(ctx, func, NULL, 2, argv);
        if (err) {
                fprintf(stderr, "%s\n", spn_ctx_geterrmsg(ctx));
        }
}

int spnuv_tcp_read(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue value;
        uv_tcp_t *handle;

        spn_value_retain(&argv[0]);
        spn_value_retain(&argv[1]);

        self = spn_hashmapvalue(&argv[0]);
        value = spn_hashmap_get_strkey(self, "handle");
        handle = spn_ptrvalue(&value);

        value = spn_makerawptr(ctx);
        spn_hashmap_set_strkey(self, "readCallback", &argv[1]);
        spn_hashmap_set_strkey(self, "readContext", &value);
        spn_value_release(&value);

        return uv_read_start((uv_stream_t *)handle,
                             (uv_alloc_cb)spnuv_tcp_read_alloc_cb,
                             (uv_read_cb)spnuv_tcp_read_cb);
}

void spnuv_tcp_write_cb(uv_write_t* req, int status)
{
        SpnUVWriteData *wrd;
        SpnValue argv[1];
        int err;

        if (req->data) {
                wrd = req->data;
                if (status != 0) {
                        argv[0] = spnuv_get_error(status, uv_err_name(status),
                                                  uv_strerror(status));
                } else {
                        argv[0] = spn_nilval;
                }
                err = spn_ctx_callfunc(wrd->ctx, wrd->callback, NULL, 1, argv);
                if (err) {
                        fprintf(stderr, "%s\n", spn_ctx_geterrmsg(wrd->ctx));
                }
                free(req->data);
        }

        free(req);
}

int spnuv_tcp_write(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue value;
        uv_tcp_t *handle;
        SpnString *str;
        uv_write_t *req;
        uv_buf_t buf;
        SpnUVWriteData *wrd = NULL;

        spn_value_retain(&argv[0]);
        self = spn_hashmapvalue(&argv[0]);
        spn_value_retain(&argv[1]);
        str = spn_stringvalue(&argv[1]);

        if (argc > 2) {
                spn_value_retain(&argv[2]);
                wrd = malloc(sizeof(SpnUVWriteData));
                wrd->ctx = ctx;
                wrd->callback = spn_funcvalue(&argv[2]);
        }
        
        value = spn_hashmap_get_strkey(self, "handle");
        handle = spn_ptrvalue(&value);

        buf.base = str->cstr;
        buf.len = str->len;

        req = malloc(sizeof(uv_write_t));
        memset(req, 0, sizeof(uv_write_t));

        if (wrd) {
                req->data = wrd;
        }

        return uv_write(req, (uv_stream_t *)handle,
                        &buf, 1, spnuv_tcp_write_cb);
}

int spnuv_tcp_new(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *members = spn_hashmap_new();

        SpnHashMap *loop;
        SpnValue value;
        uv_loop_t *uv_loop;

        size_t i;

        SpnValue res;

        static const SpnExtFunc fns[] = {
                { "bind", spnuv_tcp_bind },
                { "listen", spnuv_tcp_listen },
                { "accept", spnuv_tcp_accept },
                { "read", spnuv_tcp_read },
                { "write", spnuv_tcp_write },
                { "close", spnuv_close }
        };

        spn_value_retain(&argv[1]);

        spnuv_tcp_seq += 1;

        loop = spn_hashmapvalue(&argv[1]);
        value = spn_hashmap_get_strkey(loop, "uv_loop");
        uv_loop = spn_ptrvalue(&value);

        for (i = 0; i < COUNT(fns); i += 1) {
                SpnValue fnval = spn_makenativefunc(fns[i].name, fns[i].fn);
                spn_hashmap_set_strkey(members, fns[i].name, &fnval);
                spn_value_release(&fnval);
        }

        value = spn_makeint(spnuv_tcp_seq);
        spn_hashmap_set_strkey(members, "id", &value);
        spn_value_release(&value);

        value = spn_makerawptr(uv_loop);
        spn_hashmap_set_strkey(members, "uv_loop", &value);
        spn_value_release(&value);

        res.type = SPN_TYPE_OBJECT;
        res.v.o = members;

        *ret = res;

        return 0;
}

SpnHashMap *spnuv_tcp_api(void)
{
        SpnHashMap *api = spn_hashmap_new();
        size_t i;

        static const SpnExtFunc fns[] = {
                { "new", spnuv_tcp_new }
        };

        for (i = 0; i < COUNT(fns); i += 1) {
                SpnValue fnval = spn_makenativefunc(fns[i].name, fns[i].fn);
                spn_hashmap_set_strkey(api, fns[i].name, &fnval);
                spn_value_release(&fnval);
        }

        return api;
}
