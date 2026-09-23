#include "DCjson.h"
#include <stdio.h>

int test_literal_true() {
    const char* buffer = "true";
    DCjson_error_t error;
    DCjson_value_t* root = DCjson_parse(buffer, 4, &error);
    if (!root || error.type != DCJSON_OK) return 1;
    if (!root->as.boolean) return 1;
    DCjson_destroy(root);
    return 0;
}

int test_literal_false() {
    const char* buffer = "false";
    DCjson_error_t error;
    DCjson_value_t* root = DCjson_parse(buffer, 5, &error);
    if (!root || error.type != DCJSON_OK) return 1;
    if (root->as.boolean) return 1;
    DCjson_destroy(root);
    return 0;
}

int test_literal_null() {
    const char* buffer = "null";
    DCjson_error_t error;
    DCjson_value_t* root = DCjson_parse(buffer, 5, &error);
    if (!root || error.type != DCJSON_OK) return 1;
    DCjson_destroy(root);
    return 0;
}

int main(void) {

    if (test_literal_true()) return 1;
    if (test_literal_false()) return 1;
    if (test_literal_null()) return 1;

    return 0;
}