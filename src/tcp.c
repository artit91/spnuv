#include "spnuv.h"

/* TODO: free tcp_h */
int spnuv_tcp_bind(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue value;
        SpnString *host;
        long port;
        long flags = 0;
        uv_loop_t *uv_loop;

        uv_tcp_t *tcp_h = malloc(sizeof(uv_tcp_t));
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

        spnuv_util_parse_addr(host->cstr, port, &addr);
        uv_tcp_init(uv_loop, tcp_h);
        tcp_h->data = self;

        value = spn_makerawptr(tcp_h);
        spn_hashmap_set_strkey(self, "tcp_h", &value);
        spn_value_release(&value);

        return uv_tcp_bind(tcp_h, (struct sockaddr *)&addr, flags);
}

void spnuv_tcp_listen_cb(uv_stream_t *handle, int status)
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

int spnuv_tcp_listen(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        uv_tcp_t *tcp_h;
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

        value = spn_hashmap_get_strkey(self, "tcp_h");
        tcp_h = spn_ptrvalue(&value);
        
        value = spn_makerawptr(ctx);
        spn_hashmap_set_strkey(self, "listenContext", &value);
        spn_value_release(&value);

        return uv_listen((uv_stream_t *)tcp_h, backlog, spnuv_tcp_listen_cb);
}

/* TODO: free client_tcp_h */
int spnuv_tcp_accept(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
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

int spnuv_tcp_read(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
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
                             (uv_alloc_cb)spnuv_tcp_read_alloc_cb,
                             (uv_read_cb)spnuv_tcp_read_cb);
}

/* TODO: write */
void spnuv_tcp_write_cb(uv_write_t* req, int status)
{
        free(req);
}

int spnuv_tcp_write(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue value;
        uv_tcp_t *tcp_h;
        SpnString *str;
        uv_write_t *req;
        uv_buf_t buf;

        spn_value_retain(&argv[0]);
        self = spn_hashmapvalue(&argv[0]);
        spn_value_retain(&argv[1]);
        str = spn_stringvalue(&argv[1]);
        
        value = spn_hashmap_get_strkey(self, "tcp_h");
        tcp_h = spn_ptrvalue(&value);

        buf.base = str->cstr;
        buf.len = str->len;

        req = malloc(sizeof(uv_write_t));

        return uv_write(req, (uv_stream_t *)tcp_h, &buf, 1, spnuv_tcp_write_cb);
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
                { "new", spnuv_tcp_new }
        };

        for (i = 0; i < COUNT(fns); i += 1) {
                SpnValue fnval = spn_makenativefunc(fns[i].name, fns[i].fn);
                spn_hashmap_set_strkey(api, fns[i].name, &fnval);
                spn_value_release(&fnval);
        }

        return api;
}

void spnuv_tcp_api_destroy(void) {}
