// Save/Restore state functionality. We use a simplification of the Universal Binary JSON (UBJSON) format that can only store binary strings.
// All numbers are stored in little endian format.

#include "state.h"
#include "platform.h"
#include "util.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#if defined(_WIN32) && !defined(EMSCRIPTEN)
#include <windows.h>
#define PATHSEP '\\'
#define PATHSEP_STR "\\"
#else
#define PATHSEP '/'
#define PATHSEP_STR "/"
#endif

// Binary reader/writer helpers
struct rstream {
    uint8_t* buf;
    uint32_t pos;
};
static void rstream_init(struct rstream* r, uint8_t* buf)
{
    r->buf = buf;
    r->pos = 0;
}
static uint8_t read8(struct rstream* r)
{
    return r->buf[r->pos++];
}
#if 0
static uint16_t read16(struct rstream* r)
{
    uint16_t res = r->buf[r->pos] | r->buf[r->pos + 1] << 8;
    r->pos += 2;
    return res;
}
static uint32_t read64(struct rstream* r)
{
    uint64_t low = read32(r), high;
    high = read32(r);
    return low | high << 32;
}
#endif
static uint32_t read32(struct rstream* r)
{
    uint32_t res = r->buf[r->pos] | r->buf[r->pos + 1] << 8 | r->buf[r->pos + 2] << 16 | r->buf[r->pos + 3] << 24;
    r->pos += 4;
    return res;
}
static char* readstr(struct rstream* r)
{
    int length = strhlen((void*)&r->buf[r->pos]) + 1;
    char* dest = halloc(length);
    hcpy(dest, &r->buf[r->pos], length);
    r->pos += length;
    return dest;
}
static uint8_t peek8(struct rstream* r)
{
    return r->buf[r->pos];
}
#if 0
static uint16_t peek16(struct rstream* r)
{
    return r->buf[r->pos] | r->buf[r->pos + 1] << 8;
}
static uint32_t peek32(struct rstream* r)
{
    return r->buf[r->pos] | r->buf[r->pos + 1] << 8 | r->buf[r->pos + 1] << 16 | r->buf[r->pos + 1] << 24;
}
static uint32_t peek64(struct rstream* r)
{
    uint64_t low = peek32(r), high;
    r->pos += 4;
    high = peek32(r);
    r->pos -= 4;
    return low | high << 32;
}
#endif
static void* skipptr(struct rstream* r, int skip)
{
    void* a = &r->buf[r->pos];
    r->pos += skip;
    return a;
}

struct wstream {
    uint8_t* buf;
    uint32_t bufsize;
    uint32_t pos;
};
static void wstream_init(struct wstream* w, int initial_size)
{
    w->buf = halloc(initial_size);
    w->pos = 0;
    w->bufsize = initial_size;
}
static void wstream_destroy(struct wstream* w)
{
    hfree(w->buf);
}
static void wstream_grow(struct wstream* w)
{
    w->bufsize <<= 1;
    w->buf = realloc(w->buf, w->bufsize);
}
static void write8(struct wstream* w, uint8_t a)
{
    if (w->pos >= w->bufsize)
        wstream_grow(w);
    w->buf[w->pos++] = a;
}
#if 0
static void write16(struct wstream* w, uint16_t a)
{
    if ((w->pos + 1) >= w->bufsize)
        wstream_grow(w);
    w->buf[w->pos++] = a;
    w->buf[w->pos++] = a >> 8;
}
#endif
static void write32(struct wstream* w, uint32_t a)
{
    if ((w->pos + 3) >= w->bufsize)
        wstream_grow(w);
    w->buf[w->pos++] = a;
    w->buf[w->pos++] = a >> 8;
    w->buf[w->pos++] = a >> 16;
    w->buf[w->pos++] = a >> 24;
}
static void writestr(struct wstream* w, char* str)
{
    int len = strhlen(str) + 1;
    if ((w->pos + len) >= w->bufsize)
        wstream_grow(w);
    hcpy(&w->buf[w->pos], str, len);
    w->pos += len;
}
static void writemem(struct wstream* w, void* mem, int len)
{
    if ((w->pos + len) >= w->bufsize)
        wstream_grow(w);
    hcpy(&w->buf[w->pos], mem, len);
    w->pos += len;
}

#define STATE_LOG(x, ...) LOG("STATE", x, ##__VA_ARGS__)
#define STATE_FATAL(x, ...) \
    FATAL("STATE", x, ##__VA_ARGS__);

#define MAX_STATE_HANDLERS 16
static state_handler state_handlers[MAX_STATE_HANDLERS];
static int state_handler_count = 0;
static int is_reading = 0;
void state_register(state_handler s)
{
    if (state_handler_count == MAX_STATE_HANDLERS)
        ABORT();
    state_handlers[state_handler_count++] = s;
}

#define MAGIC 0xC8C70FF0
#define VERSION 0

static struct bjson_key_value* get_value(struct bjson_object* o, char* key)
{
    for (unsigned int i = 0; i < o->length; i++) {
        if (!o->keys[i].key)
            break; // Is NULL pointer, invalid
        if (!strhcmp(key, o->keys[i].key))
            return &o->keys[i];
    }
    return NULL;
}

// Getters
struct bjson_data* state_get_mem(struct bjson_object* o, char* key)
{
    struct bjson_key_value* keyval = get_value(o, key);
    if (!keyval || keyval->datatype != TYPE_DATA) {
        STATE_LOG("%s is invalid key -- setting to zero\n", key);
        return NULL;
    }
    return &keyval->mem_data;
}
struct bjson_object* state_get_object(struct bjson_object* o, char* key)
{
    struct bjson_key_value* keyval = get_value(o, key);
    if (!keyval || keyval->datatype != TYPE_OBJECT) {
        STATE_LOG("%s is invalid key\n", key);
        return NULL;
    }
    return keyval->ptr_value;
}

static char* dupstr(char* v)
{
    int len = strhlen(v) + 1;
    char* res = halloc(len + 1);
    hcpy(res, v, len);
    return res;
}

// Setters
static struct bjson_key_value* set_value(struct bjson_object* o, char* key)
{
    unsigned int i = 0;
    for (; i < o->length; i++) {
        if (!o->keys[i].key)
            break;
        if (!strhcmp(key, o->keys[i].key)) {
            STATE_FATAL("Key %s already in object\n", key);
        }
    }
    if (i == o->length)
        STATE_FATAL("Not enough room in existing object\n");
    // Add a new element for us too look at.
    o->keys[i].key = dupstr(key);
    return &o->keys[i];
}
void state_add_mem(struct bjson_object* o, char* key, struct bjson_data* value)
{
    struct bjson_key_value* kv = set_value(o, key);
    kv->datatype = TYPE_DATA;
    kv->mem_data.data = value->data;
    kv->mem_data.length = value->length;
}
void state_add_object(struct bjson_object* o, char* key, struct bjson_object* value)
{
    struct bjson_key_value* kv = set_value(o, key);
    kv->datatype = TYPE_OBJECT;
    kv->ptr_value = value;
}

// Creates a BJSON object and initializes it.
struct bjson_object* state_create_bjson_object(int keyvalues)
{
    struct bjson_object* obj = halloc(sizeof(struct bjson_object));
    obj->length = keyvalues;
    obj->keys = challoc(keyvalues, sizeof(struct bjson_key_value));
    return obj;
}
void state_init_bjson_mem(struct bjson_data* arr, int length)
{
    arr->data = halloc(length);
    arr->length = length;
}

// Public API
static struct bjson_object* global_obj;
struct bjson_object* state_obj(char* name, int keyvalues)
{
    if (is_reading)
        return state_get_object(global_obj, name);
    else {
        struct bjson_object* b = state_create_bjson_object(keyvalues);
        state_add_object(global_obj, name, b);
        return b;
    }
}
void state_field(struct bjson_object* cur, int length, char* name, void* data)
{
    if (is_reading) {
        struct bjson_data* arr = state_get_mem(cur, name);
        if (arr == NULL) {
            hset(data, 0, length);
        } else {
            if ((unsigned int)length > arr->length)
                length = arr->length;
            hcpy(data, arr->data, length);
        }
    } else {
        struct bjson_data arr;
        state_init_bjson_mem(&arr, length);
        hcpy(arr.data, data, length);
        state_add_mem(cur, name, &arr);
    }
}
void state_string(struct bjson_object* cur, char* name, char** val)
{
    if (is_reading) {
        struct bjson_data* arr = state_get_mem(cur, name);
        int length = arr->length, ok = 0;
        for (int i = 0; i < length; i++) {
            if (((uint8_t*)arr->data)[i] == 0) {
                // Check if it's at the end of the string
                if (i == (length - 1))
                    ok = 1;
                else
                    break;
            }
        }
        if (!ok) // Either we hit an unexpected \00 or we didn't hit one at all!
            STATE_FATAL("Invalid string literal");
        char* dest = halloc(length);
        hcpy(dest, arr->data, length);
        *val = dest;
    } else {
        char* src = *val;
        int length = strhlen(src) + 1;

        struct bjson_data arr;
        state_init_bjson_mem(&arr, length);
        hcpy(arr.data, src, length);
        state_add_mem(cur, name, &arr);
    }
}

static void bjson_serialize(struct wstream* w, struct bjson_object* obj)
{
    write8(w, TYPE_OBJECT);
    // Prune out all NULL elements
    int true_len = 0;
    for (unsigned int i = 0; i < obj->length; i++) {
        if (obj->keys[i].key)
            true_len++;
    }
    write8(w, true_len);
    for (int i = 0; i < true_len; i++) {
        struct bjson_key_value* kv = &obj->keys[i];
        writestr(w, kv->key);
        if (kv->datatype == TYPE_OBJECT)
            bjson_serialize(w, kv->ptr_value);
        else { // Must be TYPE_DATA
            write8(w, kv->datatype);
            write32(w, kv->mem_data.length);
            writemem(w, kv->mem_data.data, kv->mem_data.length);
        }
    }
}

static struct bjson_object* bjson_parse(struct rstream* r)
{
    if (read8(r) != TYPE_OBJECT)
        STATE_FATAL("Expected object type\n");
    int length = read8(r);
    struct bjson_object* obj = state_create_bjson_object(length);
    for (unsigned int i = 0; i < obj->length; i++) {
        struct bjson_key_value* kv = &obj->keys[i];
        kv->key = readstr(r);
        kv->datatype = peek8(r);
        if (kv->datatype == TYPE_OBJECT)
            kv->ptr_value = bjson_parse(r);
        else { // Must be TYPE_DATA
            kv->datatype = read8(r);
            kv->mem_data.length = read32(r);
            kv->mem_data.data = skipptr(r, kv->mem_data.length);
        }
    }
    return obj;
}

static struct bjson_object* parse_bjson(struct rstream* r)
{
    if (read32(r) != MAGIC) // F0 0F C7 C8
        STATE_FATAL("Invalid magic number\n");
    if (read32(r) != VERSION)
        STATE_FATAL("Unsupported version\n");

    struct bjson_object* obj = bjson_parse(r);
    return obj;
}

static void bjson_destroy_object(struct bjson_object* obj)
{
    for (unsigned int i = 0; i < obj->length; i++) {
        struct bjson_key_value* keyvalue = &obj->keys[i];
        hfree(keyvalue->key);
        if (keyvalue->datatype == TYPE_OBJECT)
            bjson_destroy_object(keyvalue->ptr_value);
        // DATA pointers point to the file itself.
    }
    hfree(obj->keys);
    hfree(obj);
}

static char* global_file_base;
void state_file(int size, char* name, void* ptr)
{
    char temp[1000];
    sprintf(temp, "%s" PATHSEP_STR "%s", global_file_base, name);
    if (is_reading) {
#ifndef EMSCRIPTEN
#ifdef USE_F_API
        FILE* fh = fopen(temp, "rb");
        if (!fh)
            STATE_FATAL("Unable to open file %s\n", temp);
        if (fread(ptr, 1, size, fh) != size)
            STATE_FATAL("Could not read\n");
        fclose(fh);
#else
        int fh = open(temp, O_RDONLY | O_BINARY);
        if (fh == -1)
            STATE_FATAL("Unable to open file %s\n", temp);
        if (read(fh, ptr, size) != size)
            STATE_FATAL("Could not read\n");
        close(fh);
#endif
#else
        EM_ASM_({
            window["loadFile"]($0, $1, $2);
        }, temp, ptr, size);
#endif
    } else {
#ifndef EMSCRIPTEN
#ifdef USE_F_API
        FILE* fh = fopen(temp, "wb"); // TODO: is this right?
        if (!fh)
            STATE_FATAL("Unable to create file %s\n", temp);
        if (fwrite(ptr, 1, size, fh) != size)
            STATE_FATAL("Could not write\n");
        fclose(fh);
#else
        int fh = open(temp, O_WRONLY | O_CREAT | O_BINARY | O_TRUNC, 0666);
        if (fh == -1)
            STATE_FATAL("Unable to create file %s\n", temp);
        if (write(fh, ptr, size) != size)
            STATE_FATAL("Could not write\n");
        close(fh);
#endif
#else
        EM_ASM_({
            window["saveFile"]($0, $1, $2);
        },
            temp, ptr, size);
#endif
    }
}
static char* normalize(char* a)
{
    int len = strhlen(a);
    char* res;
    if (a[len - 1] == PATHSEP) {
        res = halloc(len);
        hcpy(res, a, len);
        res[len - 1] = 0;
    } else {
        res = halloc(len + 1);
        hcpy(res, a, len + 1);
    }
    return res;
}

void state_read_from_file(char* fn)
{
    char path[1000];
    global_file_base = normalize(fn);
    sprintf(path, "%s" PATHSEP_STR "state.bin", fn);

#ifndef EMSCRIPTEN
#ifdef USE_F_API
    FILE* fh = fopen(path, "rb");
    if (!fh)
        STATE_FATAL("Cannot open file %s\n", fn);
    int size = fseek(fh, 0, SEEK_END);
    fseek(fh, 0, SEEK_SET);
    void* buf = halloc(size);
    if (fread(buf, 1, size, fh) != size)
        STATE_FATAL("Cannot read from file %s\n", fn);
    fclose(fh);
#else
    int fh = open(path, O_RDONLY | O_BINARY);
    if (fh == -1)
        STATE_FATAL("Cannot open file %s\n", fn);
    int size = lseek(fh, 0, SEEK_END);
    lseek(fh, 0, SEEK_SET);
    void* buf = halloc(size);
    if (read(fh, buf, size) != size)
        STATE_FATAL("Cannot read from file %s\n", fn);
    close(fh);
#endif
#else
    void* buf = (void*)(EM_ASM_INT({
        return window["loadFile2"]($0, $1, $2);
    },
        path, global_file_base));
#endif

    struct rstream r;
    rstream_init(&r, buf);
    global_obj = parse_bjson(&r);
    is_reading = 1;
    for (int i = 0; i < state_handler_count; i++)
        state_handlers[i]();
    bjson_destroy_object(global_obj);
    hfree(global_file_base);
    hfree(buf);
}

void state_store_to_file(char* fn)
{
    char path[1000];
    struct wstream w;
    wstream_init(&w, 65536);
    write32(&w, MAGIC);
    write32(&w, VERSION);

    is_reading = 0;
    global_file_base = normalize(fn);
    global_obj = state_create_bjson_object(64);
    for (int i = 0; i < state_handler_count; i++)
        state_handlers[i]();
    bjson_serialize(&w, global_obj);

    sprintf(path, "%s" PATHSEP_STR "state.bin", fn);

#ifdef USE_F_API
    FILE* fh = fopen(path, "wb"); // TODO: right?
    if (!fh)
        STATE_FATAL("Cannot open file %s\n", fn);
    if (fwrite(w.buf, 1, w.pos, fh) != (ssize_t)w.pos) // Clang complains that w.pos and write have different signs.
        STATE_FATAL("Cannot write to file %s\n", fn);
    fclose(fh);
#else
    int fh = open(path, O_WRONLY | O_CREAT | O_BINARY, 0666);
    if (fh == -1)
        STATE_FATAL("Cannot open file %s\n", fn);
    if (write(fh, w.buf, w.pos) != (ssize_t)w.pos) // Clang complains that w.pos and write have different signs.
        STATE_FATAL("Cannot write to file %s\n", fn);
    close(fh);
#endif
    wstream_destroy(&w);
    bjson_destroy_object(global_obj);
    hfree(global_file_base);
}

#ifdef EMSCRIPTEN
// Store the state to a buffer
void state_get_buffer(void)
{
    char* fn = "/";
    char path[1000];
    struct wstream w;
    wstream_init(&w, 65536);
    write32(&w, MAGIC);
    write32(&w, VERSION);

    is_reading = 0;
    global_file_base = normalize(fn);
    global_obj = state_create_bjson_object(64);
    for (int i = 0; i < state_handler_count; i++)
        state_handlers[i]();
    bjson_serialize(&w, global_obj);

    sprintf(path, "%s/state.bin", fn);
    EM_ASM_({
        window["saveFile"]($0, $1, $2);
    },
        path, w.buf, w.pos);
    wstream_destroy(&w);
    //ABORT();
    bjson_destroy_object(global_obj);
    hfree(global_file_base);
}
#endif

char* state_get_path_base(void)
{
    return global_file_base;
}

void state_mkdir(char* path)
{
#ifndef EMSCRIPTEN
#ifdef _WIN32
    // TODO: Unicode
    if (!CreateDirectoryA(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        STATE_FATAL("Unable to make new directory %s\n", path);
    }
#else
    if (mkdir(path, 0777) == -1) {
        if (errno != EEXIST)
            STATE_FATAL("Unable to make new directory %s\n", path);
    }
#endif
#else
    UNUSED(path);
#endif
}
int state_is_reading(void)
{
    return is_reading;
}