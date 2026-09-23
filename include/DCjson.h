#ifndef H_DCJSON
#define H_DCJSON

#include <stdbool.h>
#include <stddef.h>

#define DCJSON_ERROR_MSG_LEN 128

typedef enum DCjson_error_type {
    DCJSON_OK,
    DCJSON_ERROR,
} DCjson_error_type_t;

typedef struct DCjson_error {
    DCjson_error_type_t type;
    char msg[DCJSON_ERROR_MSG_LEN];
} DCjson_error_t;

typedef enum DCjson_type {
    DCJSON_VALUE_ARRAY,
    DCJSON_VALUE_OBJECT,
    DCJSON_VALUE_BOOLEAN,
    DCJSON_VALUE_NULL,
    DCJSON_VALUE_NUMBER,
    DCJSON_VALUE_STRING,
} DCjson_type_t;

typedef struct DCjson_string{
    char* text;
    size_t len;
} DCjson_string_t;

typedef struct DCjson_value DCjson_value_t;

typedef struct DCjson_member {
    DCjson_string_t* key;
    DCjson_value_t* value;
} DCjson_member_t;

typedef struct DCjson_object {
    DCjson_member_t* data;
    size_t count;
    size_t cap;
} DCjson_object_t;

typedef struct DCjson_array {
    DCjson_value_t* data;
    size_t count;
    size_t cap;
} DCjson_array_t;

struct DCjson_value {
    DCjson_type_t type;
    union {
        bool boolean;
        double number;
        DCjson_string_t* string;
        DCjson_object_t* object;
        DCjson_array_t* array;
    } as;
};

DCjson_value_t* DCjson_create(DCjson_type_t type, DCjson_error_t* err);

void DCjson_destroy(DCjson_value_t* v);

DCjson_value_t* DCjson_parse(const char* buffer, size_t len, DCjson_error_t* err); 

#endif // H_DCJSON