#include "spnuv.h"

void spnuv_stream_read_alloc_cb(uv_handle_t* handle, size_t suggested_size,
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

void spnuv_stream_read_cb(uv_stream_t* handle, int nread, const uv_buf_t* buf)
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

int spnuv_stream_read(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue value;
        uv_stream_t *handle;

        spn_value_retain(&argv[0]);
        spn_value_retain(&argv[1]);

        self = spn_hashmapvalue(&argv[0]);
        value = spn_hashmap_get_strkey(self, "handle");
        handle = spn_ptrvalue(&value);

        value = spn_makerawptr(ctx);
        spn_hashmap_set_strkey(self, "readCallback", &argv[1]);
        spn_hashmap_set_strkey(self, "readContext", &value);
        spn_value_release(&value);

        return uv_read_start(handle, (uv_alloc_cb)spnuv_stream_read_alloc_cb,
                            (uv_read_cb)spnuv_stream_read_cb);
}

void spnuv_stream_write_cb(uv_write_t* req, int status)
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

int spnuv_stream_write(SpnValue *ret, int argc, SpnValue argv[], void *ctx)
{
        SpnHashMap *self;
        SpnValue value;
        uv_stream_t *handle;
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

        return uv_write(req, handle, &buf, 1, spnuv_stream_write_cb);
}
