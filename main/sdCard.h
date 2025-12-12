#ifndef SD_CARD_H
#define SD_CARD_H

#include "esp_err.h"
#include "sdmmc_cmd.h"

#define MOUNT_POINT "/sdcard"

extern sdmmc_card_t *sdcard;

esp_err_t initialize_sd_card();
void verify_filesystem(void);

#endif