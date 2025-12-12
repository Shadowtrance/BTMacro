#ifndef HELPERS_H
#define HELPERS_H

#include <cJSON.h>

int hex_to_int(char h);
char *unescape_utf8_string(const char *src);
const char **create_button_map_from_json(const char *filename);
cJSON* load_json_from_file(const char* filename);

#endif