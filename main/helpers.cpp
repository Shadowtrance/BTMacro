#include "helpers.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <esp_log.h>
#include <cJSON.h>
#include "sdCard.h"

static const char *TAG = "[HELPERS]";

// Helper function to load and parse JSON from file
cJSON* load_json_from_file(const char* filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s for reading", filename);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = (char*)malloc(size + 1);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for file buffer");
        fclose(f);
        return NULL;
    }
    fread(buffer, 1, size, f);
    fclose(f);
    buffer[size] = '\0';

    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON from %s", filename);
    } else {
        ESP_LOGI(TAG, "Successfully parsed JSON from %s", filename);
    }
    return root;
}

// Helper function to convert a hexadecimal character to its integer value
int hex_to_int(char h)
{
  if (h >= '0' && h <= '9')
    return h - '0';
  if (h >= 'a' && h <= 'f')
    return h - 'a' + 10;
  if (h >= 'A' && h <= 'F')
    return h - 'A' + 10;
  return 0; // Should not happen with valid input
}

// Function to process a string and convert \xHH sequences to raw bytes
char *unescape_utf8_string(const char *src)
{
  // Allocate memory for the destination string.
  // It might be shorter than the source string.
  char *dest = (char *)malloc(strlen(src) + 1);
  if (!dest)
    return NULL;

  int i = 0, j = 0;
  while (src[i] != '\0')
  {
    // Check for an escape sequence start
    if (src[i] == '\\' && src[i + 1] == 'x')
    {
      // Check if there are enough characters for a hex code (e.g., \xEF)
      if (isxdigit(src[i + 2]) && isxdigit(src[i + 3]))
      {
        // Convert the two hex digits into a single byte
        dest[j] = (char)(hex_to_int(src[i + 2]) * 16 + hex_to_int(src[i + 3]));
        i += 4; // Skip the \xHH characters
        j += 1; // Move to the next destination character position
      }
      else
      {
        // Not a valid \xHH sequence, copy literally
        dest[j++] = src[i++];
      }
    }
    else if (src[i] == '\\' && src[i + 1] == 'n')
    {
      // Handle the newline escape sequence for button matrix
      dest[j++] = '\n';
      i += 2;
    }
    else
    {
      // Copy normal characters literally
      dest[j++] = src[i++];
    }
  }
  dest[j] = '\0'; // Null-terminate the new string
  return dest;
}

const char **create_button_map_from_json(const char *filename)
{
  cJSON *root = load_json_from_file(filename);
  if (!root) {
    return NULL;
  }

  cJSON *buttons_array = cJSON_GetObjectItemCaseSensitive(root, "buttons");
  if (!cJSON_IsArray(buttons_array))
  {
    ESP_LOGE(TAG, "Buttons item is not an array");
    cJSON_Delete(root);
    return NULL;
  }

  int n = cJSON_GetArraySize(buttons_array);
  int cols = (n <= 6) ? 3 : (n <= 9) ? 3 : (n <= 12) ? 4 : 5;
  // Allocate for labels + \n + ""
  const char **map_data_arr = (const char **)malloc(sizeof(char *) * (n + (n / cols) + 2));
  if (map_data_arr == NULL)
  {
    ESP_LOGE(TAG, "Failed to allocate memory for map array");
    cJSON_Delete(root);
    return NULL;
  }

  int index = 0;
  int button_index = 0;
  cJSON *item = NULL;
  cJSON_ArrayForEach(item, buttons_array)
  {
    if (cJSON_IsObject(item))
    {
      cJSON *label = cJSON_GetObjectItem(item, "label");
      if (cJSON_IsString(label))
      {
        char *unescaped = unescape_utf8_string(label->valuestring);
        map_data_arr[index++] = unescaped ? unescaped : "ERR";
        button_index++;
        if (button_index % cols == 0 && button_index < n) {
          map_data_arr[index++] = "\n";
        }
      }
    } else if (cJSON_IsString(item)) {
      // Fallback for old string array format
      char *unescaped = unescape_utf8_string(item->valuestring);
      map_data_arr[index++] = unescaped ? unescaped : "ERR";
      button_index++;
      if (button_index % cols == 0 && button_index < n) {
        map_data_arr[index++] = "\n";
      }
    }
  }
  map_data_arr[index++] = "";
  map_data_arr[index] = NULL;

  cJSON_Delete(root);
  ESP_LOGI(TAG, "Successfully loaded button map from %s", filename);
  return map_data_arr;
}
