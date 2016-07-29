#include "spnuv.h"

int tcp_bind(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        /* TODO: address, port, flags, free tcp_h */
        SpnHashMap *self;
        SpnValue value;
        uv_loop_t *uv_loop;

        uv_tcp_t *tcp_h = malloc(sizeof(uv_tcp_t));
        struct sockaddr_in addr;

        spn_value_retain(&argv[0]);
        spn_value_retain(&argv[1]);
        spn_value_retain(&argv[2]);

        self = spn_hashmapvalue(&argv[0]);
        value = spn_hashmap_get_strkey(self, "uv_loop");
        uv_loop = spn_ptrvalue(&value);

        uv_ip4_addr("127.0.0.1", 8080, &addr);
        uv_tcp_init(uv_loop, tcp_h);
        tcp_h->data = self;

        value = spn_makerawptr(tcp_h);
        spn_hashmap_set_strkey(self, "tcp_h", &value);
        spn_value_release(&value);

        return uv_tcp_bind(tcp_h, (struct sockaddr *)&addr, 0);
}

void tcp_listen_cb(uv_stream_t *handle, int status)
{
        uv_tcp_t *tcp_h = (uv_tcp_t *)handle;
        SpnHashMap *self = tcp_h->data;
        SpnValue context_value = spn_hashmap_get_strkey(self, "listenContext");
        SpnValue fn_value = spn_hashmap_get_strkey(self, "listenCallback");
        void *ctx = spn_ptrvalue(&context_value);
        SpnFunction *func = spn_funcvalue(&fn_value);
        SpnValue err_value;
        SpnValue argv[1];

        if (status != 0) {
                err_value = spn_makeint(status);
        } else {
                err_value = spn_nilval;
        }

        argv[0] = err_value;

        spn_ctx_callfunc(ctx, func, NULL, 1, argv);
}

int tcp_listen(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        /* TODO: backlog */
        SpnHashMap *self;
        uv_tcp_t *tcp_h;
        SpnValue value;

        spn_value_retain(&argv[0]);
        spn_value_retain(&argv[1]);

        self = spn_hashmapvalue(&argv[0]);
        value = spn_hashmap_get_strkey(self, "tcp_h");
        tcp_h = spn_ptrvalue(&value);

        spn_hashmap_set_strkey(self, "listenCallback", &argv[1]);
        value = spn_makerawptr(ctx);
        spn_hashmap_set_strkey(self, "listenContext", &value);
        spn_value_release(&value);

        return uv_listen((uv_stream_t *)tcp_h, 511, tcp_listen_cb);
}

int tcp_accept(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        /* TODO: free client_tcp_h */
        SpnHashMap *self;
        uv_tcp_t *tcp_h;

        SpnHashMap *client;
        uv_loop_t *client_uv_loop;

        uv_tcp_t *client_tcp_h = malloc(sizeof(uv_tcp_t));

        SpnValue value;

        spn_value_retain(&argv[0]);
        spn_value_retain(&argv[1]);

        self = spn_hashmapvalue(&argv[0]);
        value = spn_hashmap_get_strkey(self, "tcp_h");
        tcp_h = spn_ptrvalue(&value);

        client = spn_hashmapvalue(&argv[1]);
        value = spn_hashmap_get_strkey(self, "uv_loop");
        client_uv_loop = spn_ptrvalue(&value);

        uv_tcp_init(client_uv_loop, client_tcp_h);
        client_tcp_h->data = client;

        value = spn_makerawptr(client_tcp_h);
        spn_hashmap_set_strkey(client, "tcp_h", &value);
        spn_value_release(&value);

        return uv_accept((uv_stream_t *)tcp_h, (uv_stream_t *)client_tcp_h);
}

void tcp_read_alloc_cb(uv_handle_t* handle, size_t suggested_size,
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

void tcp_read_cb(uv_stream_t* handle, int nread, const uv_buf_t* buf)
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
                data = malloc((nread + 1) * sizeof(char));
                strncpy(data, buf->base, nread);
                data[nread] = '\0';
        }

        if (err != 0) {
                err_value = spn_makeint(err);
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

        spn_ctx_callfunc(ctx, func, NULL, 2, argv);

        buffer->in_use = 0;
}

int tcp_read(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        /* TODO: test context/callbacks */
        SpnHashMap *self;
        SpnValue value;
        uv_tcp_t *tcp_h;

        spn_value_retain(&argv[0]);
        spn_value_retain(&argv[1]);

        self = spn_hashmapvalue(&argv[0]);
        value = spn_hashmap_get_strkey(self, "tcp_h");
        tcp_h = spn_ptrvalue(&value);

        value = spn_makerawptr(ctx);
        spn_hashmap_set_strkey(self, "readCallback", &argv[1]);
        spn_hashmap_set_strkey(self, "readContext", &value);
        spn_value_release(&value);

        return uv_read_start((uv_stream_t *)tcp_h,
                             (uv_alloc_cb)tcp_read_alloc_cb,
                             (uv_read_cb)tcp_read_cb);
}

void tcp_write_cb(uv_write_t* req, int status)
{
        /* TODO: write */
}

int tcp_write(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        /* TODO: rewrite */
        SpnHashMap *self;
        SpnValue value;
        uv_tcp_t *tcp_h;
        SpnString *str;
        char buffer[4096];
        uv_buf_t buf;
        uv_write_t req;

        spn_value_retain(&argv[0]);
        spn_value_retain(&argv[1]);

        self = spn_hashmapvalue(&argv[0]);
        value = spn_hashmap_get_strkey(self, "tcp_h");
        tcp_h = spn_ptrvalue(&value);
        str = spn_stringvalue(&argv[1]);

        buf = uv_buf_init(buffer, str->len);

        buf.len = str->len;
        buf.base = str->cstr;

        return uv_write(&req, (uv_stream_t *)tcp_h, &buf, 1, tcp_write_cb);
}

int tcp_new(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *members = spn_hashmap_new();

        SpnHashMap *loop;
        SpnValue value;
        uv_loop_t *uv_loop;

        size_t i;

        SpnValue res;

        static const SpnExtFunc fns[] = {
                { "bind", tcp_bind },
                { "listen", tcp_listen },
                { "accept", tcp_accept },
                { "read", tcp_read },
                { "write", tcp_write },
        };

        spn_value_retain(&argv[1]);

        loop = spn_hashmapvalue(&argv[1]);
        value = spn_hashmap_get_strkey(loop, "uv_loop");
        uv_loop = spn_ptrvalue(&value);

        for (i = 0; i < COUNT(fns); i += 1) {
                SpnValue fnval = spn_makenativefunc(fns[i].name, fns[i].fn);
                spn_hashmap_set_strkey(members, fns[i].name, &fnval);
                spn_value_release(&fnval);
        }

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
                { "new", tcp_new }
        };

        for (i = 0; i < COUNT(fns); i += 1) {
                SpnValue fnval = spn_makenativefunc(fns[i].name, fns[i].fn);
                spn_hashmap_set_strkey(api, fns[i].name, &fnval);
                spn_value_release(&fnval);
        }

        return api;
}

void spnuv_tcp_api_destroy(void)
{
        /* empty function yet */
}
