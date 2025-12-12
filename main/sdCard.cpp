#include "sdCard.h"
#include "displayConfig.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/spi_master.h"
#include "sys/dirent.h"

static const char *TAG = "[SD CARD]";

sdmmc_card_t *sdcard;

esp_err_t initialize_sd_card()
{
  esp_err_t ret;

  ESP_LOGI(TAG, "Initializing SD card");

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024,
      .disk_status_check_enable = false,
      .use_one_fat = false};

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.max_freq_khz = 20000000U / 1000U;

  spi_bus_config_t bus_cfg = {
      .mosi_io_num = SUNTON_ESP32_SDCARD_PIN_MOSI,
      .miso_io_num = SUNTON_ESP32_SDCARD_PIN_MISO,
      .sclk_io_num = SUNTON_ESP32_SDCARD_PIN_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .data4_io_num = -1,
      .data5_io_num = -1,
      .data6_io_num = -1,
      .data7_io_num = -1,
      .data_io_default_level = 0,
      .max_transfer_sz = 4000,
      .flags = 0,
      .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
      .intr_flags = 0,
  };
  ret = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to initialise bus");
    return ret;
  }

  sdspi_device_config_t device_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  device_config.host_id = (spi_host_device_t)host.slot;
  device_config.gpio_cs = SUNTON_ESP32_SDCARD_PIN_CS;

  ESP_LOGI(TAG, "Mounting Filesystem");
  ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &device_config, &mount_config, &sdcard);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to mount filesystem (%s)", esp_err_to_name(ret));
    return ret;
  }
  ESP_LOGI(TAG, "Filesystem Mounted at %s", MOUNT_POINT);

  sdmmc_card_print_info(stdout, sdcard);
  return ESP_OK;
}

void verify_filesystem(void)
{
  ESP_LOGI(TAG, "Listing files in %s", MOUNT_POINT);
  DIR *dir = opendir(MOUNT_POINT);
  if (dir == NULL)
  {
    ESP_LOGE(TAG, "Failed to open directory: %s", MOUNT_POINT);
    return;
  }
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL)
  {
    ESP_LOGI(TAG, "Found file: %s", entry->d_name);
  }
  closedir(dir);
}
