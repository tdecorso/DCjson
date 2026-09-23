#include "DCjson.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void format_error(DCjson_error_t* err, DCjson_error_type_t t, const char* fmt, ...) {
    if (!err) return;
    err->type = t;
    va_list args;
    va_start(args, fmt);
    vsnprintf(err->msg, DCJSON_ERROR_MSG_LEN, fmt, args);
    va_end(args);
}

typedef struct {
    const char* cur;
    const char* end;
    const char* start;
    size_t line;
    size_t col;
    size_t start_line;
    size_t start_col;
} parser;

static bool at_end(parser* p) {
    return p->cur >= p->end;
}

static char peek(parser* p) {
    return at_end(p) ? '\0' : *p->cur;
}

static void advance(parser* p) {
    char c = peek(p);
    if (c == '\0') return;
    if (c == '\n') {
        p->line++;
        p->col = 0;
    } else p->col++;
    p->cur++;
}

static void skip_ws(parser* p) {
    while (1) {
        char c = peek(p);
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') break;
        advance(p);
    }
}

static void parser_init(parser* p, const char* buffer, size_t len) {
    p->cur = buffer;
    p->end = buffer + len;
    p->start = buffer;
    p->line = 1;
    p->col = 0;
    p->start_line = 1;
    p->start_col = 0;
}

static bool is_alphanum(char c) {
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c == '_');
}

static DCjson_value_t* parse_literal(parser* p, const char* literal, size_t len, DCjson_error_t* err) {
    if (p->cur + len > p->end) {
        format_error(err, DCJSON_ERROR, "Parsing error (%zu:%zu): interrupted literal.", 
            p->start_line, p->start_col);
        return NULL;
    }
    DCjson_error_t internal_error;
    DCjson_value_t* v = NULL;
    bool is_null = 0;
    if (literal[0] == 'n') {
        is_null = 1;
        v = DCjson_create(DCJSON_VALUE_NULL, &internal_error);
        if (!v) {
            format_error(err, DCJSON_ERROR, "Parsing error (%zu:%zu): %s.", 
                p->start_line, p->start_col, internal_error.msg);
            return NULL;
        }
    }
    else {
        v = DCjson_create(DCJSON_VALUE_BOOLEAN, &internal_error);
        if (!v) {
            format_error(err, DCJSON_ERROR, "Parsing error (%zu:%zu): %s.", 
                p->start_line, p->start_col, internal_error.msg);
            return NULL;
        }
    }
    for (size_t i = 0; i < len; i++) {
        char c = peek(p);
        if (c != literal[i]) {
            format_error(err, DCJSON_ERROR, "Parsing error (%zu:%zu): invalid literal sequence.",
                         p->start_line, p->start_col);
            DCjson_destroy(v);
            return NULL;
        }
        advance(p);
    }
    char c = peek(p);
    if (is_alphanum(c)) {
        format_error(err, DCJSON_ERROR, "Parsing error (%zu:%zu): invalid literal sequence.",
            p->start_line, p->start_col);
        DCjson_destroy(v);
        return NULL;
    }

    if (!is_null) {
        v->as.boolean = len == 4;
    }

    return v;
}

static void error_reset(DCjson_error_t* err) {
    if (!err) return;
    err->type = DCJSON_OK;
    err->msg[0] = '\0';
}

DCjson_value_t* DCjson_parse(const char* buffer, size_t len, DCjson_error_t* err) {
    if (!buffer || len == 0 || (!buffer && len)) return NULL;
    error_reset(err);

    parser p;
    parser_init(&p, buffer, len);

    skip_ws(&p);
    char c = peek(&p);
    switch (c) {
    case 'f': return parse_literal(&p, "false", 5, err);
    case 't': return parse_literal(&p, "true", 4, err);
    case 'n': return parse_literal(&p, "null", 4, err);
    default: return NULL;
    }
}

static DCjson_array_t* array_create(size_t cap, DCjson_error_t* err) {
    DCjson_array_t* arr = malloc(sizeof(DCjson_array_t));
    if (!arr) {
        format_error(err, DCJSON_ERROR, "Memory error.");
        return NULL;
    }
    arr->data = malloc(sizeof(DCjson_value_t*) * cap);
    if (!arr->data) {
        free(arr);
        format_error(err, DCJSON_ERROR, "Memory error.");
        return NULL;
    }
    arr->cap = cap;
    arr->count = 0;
    return arr;
}

static void array_destroy(DCjson_array_t* arr) {
    if (!arr) return;
    for (size_t i = 0; i < arr->count; i++) {
        DCjson_destroy(arr->data + i);
    }
    free(arr);
}

static DCjson_object_t* object_create(size_t cap, DCjson_error_t* err) {
    DCjson_object_t* obj = malloc(sizeof(DCjson_object_t));
    if (!obj) {
        format_error(err, DCJSON_ERROR, "Memory error.");
        return NULL;
    }
    obj->data = malloc(sizeof(DCjson_member_t*) * cap);
    if (!obj->data) {
        free(obj);
        format_error(err, DCJSON_ERROR, "Memory error.");
        return NULL;
    }
    obj->cap = cap;
    obj->count = 0;
    return obj;
}

static DCjson_string_t* string_create(const char* text, size_t len) {
    DCjson_string_t* s = malloc(sizeof(DCjson_string_t));
    if (!s) return NULL;
    s->text = malloc(len+1);
    if (!s->text) {
        free(s);
        return NULL;
    }
    memcpy(s->text, text, len);
    s->text[len] = '\0';
    s->len = len;
    return s;
}

static void string_destroy(DCjson_string_t* str) {
    if (!str) return;
    free(str->text);
    free(str);
}

static DCjson_member_t* member_create(DCjson_string_t* str, DCjson_value_t* v) {
    if (!str || !v) return NULL;
    DCjson_member_t* m = malloc(sizeof(DCjson_member_t));
    if (!m) return NULL;
    m->key = str;
    m->value = v;
    return m;
}

static void member_destroy(DCjson_member_t* m) {
    if (!m) return;
    string_destroy(m->key);
    DCjson_destroy(m->value);
    free(m);
}

static void object_destroy(DCjson_object_t* obj) {
    if (!obj) return;
    for (size_t i = 0; i < obj->count; i++) {
        member_destroy(obj->data + i);
    }
    free(obj);
}

DCjson_value_t* DCjson_create(DCjson_type_t type, DCjson_error_t* err) {
    DCjson_value_t* v = malloc(sizeof(DCjson_value_t));
    if (!v) {
        format_error(err, DCJSON_ERROR, "Memory error.");
        return NULL;
    }
    v->type = type;
    switch (type) {
    case DCJSON_VALUE_ARRAY: {
        v->as.array = array_create(32, err);
        if (!v->as.array) {
            free(v);
            return NULL;
        }
        return v;
    }
    case DCJSON_VALUE_OBJECT: {
        v->as.object = object_create(32, err);
        if (!v->as.object) {
            free(v);
            return NULL;
        }
        return v;
    }
    case DCJSON_VALUE_BOOLEAN: {
        v->as.boolean = false;
        return v;
    }
    case DCJSON_VALUE_NUMBER: {
        v->as.number = 0;
        return v;
    }
    case DCJSON_VALUE_STRING: {
        v->as.string = NULL;
        return v;
    }
    default: return v;
    }
}

void DCjson_destroy(DCjson_value_t* v) {
    if (!v) return;
    switch (v->type) {
    case DCJSON_VALUE_STRING: {
        string_destroy(v->as.string);
        break;
    }
    case DCJSON_VALUE_ARRAY: {
        array_destroy(v->as.array);
        break;
    }
    case DCJSON_VALUE_OBJECT: {
        object_destroy(v->as.object);
        break;
    }
    default: break;
    }

    free(v);
}