#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch_gt911.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "lvgl.h"

#include "displayConfig.h"

#define TAG "[DISPLAY]"

const esp_lcd_rgb_panel_config_t panelConfig = {
    .clk_src = LCD_CLK_SRC_PLL160M,
    .timings = {
        .pclk_hz = SUNTON_ESP32_LCD_PIXEL_CLOCK_HZ,
        .h_res = SUNTON_ESP32_LCD_WIDTH,
        .v_res = SUNTON_ESP32_LCD_HEIGHT,
        .hsync_pulse_width = 4,
        .hsync_back_porch = 8,
        .hsync_front_porch = 8,
        .vsync_pulse_width = 4,
        .vsync_back_porch = 8,
        .vsync_front_porch = 8,
        .flags = {
            .hsync_idle_low = false,
            .vsync_idle_low = false,
            .de_idle_high = false,
            .pclk_active_neg = true,
            .pclk_idle_high = false,
        },
    },
    .data_width = 16,
    .bits_per_pixel = 0,
    .num_fbs = 2,
    .bounce_buffer_size_px = 800 * 10,
    .sram_trans_align = 8,
    .psram_trans_align = 64,
    .hsync_gpio_num = SUNTON_ESP32_LCD_PIN_HSYNC,
    .vsync_gpio_num = SUNTON_ESP32_LCD_PIN_VSYNC,
    .de_gpio_num = SUNTON_ESP32_LCD_PIN_DE,
    .pclk_gpio_num = SUNTON_ESP32_LCD_PIN_PCLK,
    .disp_gpio_num = SUNTON_ESP32_LCD_PIN_DISP_EN,
    .data_gpio_nums = {
        SUNTON_ESP32_LCD_PIN_DATA0, SUNTON_ESP32_LCD_PIN_DATA1, SUNTON_ESP32_LCD_PIN_DATA2, SUNTON_ESP32_LCD_PIN_DATA3, SUNTON_ESP32_LCD_PIN_DATA4, // B0 - B4
        SUNTON_ESP32_LCD_PIN_DATA5, SUNTON_ESP32_LCD_PIN_DATA6, SUNTON_ESP32_LCD_PIN_DATA7, SUNTON_ESP32_LCD_PIN_DATA8, SUNTON_ESP32_LCD_PIN_DATA9, SUNTON_ESP32_LCD_PIN_DATA10, // G0 - G5
        SUNTON_ESP32_LCD_PIN_DATA11, SUNTON_ESP32_LCD_PIN_DATA12, SUNTON_ESP32_LCD_PIN_DATA13, SUNTON_ESP32_LCD_PIN_DATA14, SUNTON_ESP32_LCD_PIN_DATA15, // R0 - R4
    },
    .flags = {
        .disp_active_low = false,
        .refresh_on_demand = false,
        .fb_in_psram = true,
        .double_fb = true,
        .no_fb = false,
        .bb_invalidate_cache = false,
    },
};

static TaskHandle_t lvglPortTaskHandle = NULL;
static esp_timer_handle_t lvglTickTimerHandle = NULL;

static void suntonEsp32s3BacklightInit(void)
{
    ledc_timer_config_t ledcTimer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num       = BACKLIGHT_LEDC_TIMER,
        .freq_hz         = 200,
        .clk_cfg         = LEDC_USE_RC_FAST_CLK,
        .deconfigure     = false
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledcTimer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledcChannel = {
        .gpio_num   = SUNTON_ESP32_PIN_BCKL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = BACKLIGHT_CHANNEL,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = BACKLIGHT_LEDC_TIMER,
        .duty       = 0, // Set duty to 0%
        .hpoint     = 0,
        .flags      = {
            .output_invert = 0,
        }
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledcChannel));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL, 255));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL));
}

void suntonEsp32s3SetBrightness(uint8_t brightness)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL, brightness);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL);
}

IRAM_ATTR bool lvglPortTaskNotify(uint32_t value)
{
    BaseType_t need_yield = pdFALSE;

    // Notify LVGL task
    if (xPortInIsrContext() == pdTRUE)
    {
        xTaskNotifyFromISR(lvglPortTaskHandle, value, eNoAction, &need_yield);
    }
    else
    {
        xTaskNotify(lvglPortTaskHandle, value, eNoAction);
    }

    return (need_yield == pdTRUE);
}

static bool lvglPortFlushVsyncReadyCallback(esp_lcd_panel_handle_t panel_io, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    return lvglPortTaskNotify(ULONG_MAX);
}

static void lvglDispFlush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
    if (lv_display_flush_is_last(disp))
    {
        esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);

        /* Waiting for the last frame buffer to complete transmission */
        ulTaskNotifyValueClear(NULL, ULONG_MAX);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
    lv_display_flush_ready(disp);
};

static void lvglPortTask(void *arg)
{
    lv_draw_init();
    uint32_t task_delay_ms = 500;
    while (1)
    {
        task_delay_ms = lv_timer_handler();
        if (task_delay_ms > 500)
        {
            task_delay_ms = 500;
        }
        else if (task_delay_ms < 1)
        {
            task_delay_ms = 1;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

static void lvglTick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void touchpadRead(lv_indev_t *indev, lv_indev_data_t *data)
{
    esp_lcd_touch_handle_t tp = (esp_lcd_touch_handle_t)lv_indev_get_user_data(indev);

    esp_lcd_touch_point_data_t touchData[1];
    uint8_t pointCnt = 0;

    esp_lcd_touch_read_data(tp);

    esp_err_t ret = esp_lcd_touch_get_data(tp, touchData, &pointCnt, 1);
    if (ret == ESP_OK && pointCnt > 0)
    {
        data->point.x = touchData[0].x;
        data->point.y = touchData[0].y;
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

lv_display_t *suntonEsp32s3LcdInit(void)
{
    ESP_LOGI(TAG, "I2C Init");
    const i2c_config_t touchI2cBusConfig = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SUNTON_ESP32_TOUCH_PIN_SDA,
        .scl_io_num = SUNTON_ESP32_TOUCH_PIN_SCL,
        .sda_pullup_en = true,
        .scl_pullup_en = true,
        .master.clk_speed = SUNTON_ESP32_TOUCH_FREQ_HZ
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &touchI2cBusConfig));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, touchI2cBusConfig.mode, 0, 0, 0));

    // create lcd panel
    ESP_LOGI(TAG, "LCD Init");
    esp_lcd_panel_handle_t panelHandle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panelConfig, &panelHandle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panelHandle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panelHandle));

    ESP_LOGI(TAG, "Backlight Init");
    suntonEsp32s3BacklightInit();

    ESP_LOGI(TAG, "Touch Init");
    esp_lcd_panel_io_handle_t gt911TouchIoHandle = NULL;
    const esp_lcd_panel_io_i2c_config_t gt911TouchIoConfig = {
        .dev_addr = SUNTON_ESP32_TOUCH_ADDRESS,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .control_phase_bytes = 1,
        .dc_bit_offset = 0,
        .lcd_cmd_bits = 16,
        .lcd_param_bits = 0,
        .flags = {
            .dc_low_on_data = 0,
            .disable_control_phase = 1,
        }
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &gt911TouchIoConfig, &gt911TouchIoHandle));

    esp_lcd_touch_handle_t touchHandle = NULL;
    esp_lcd_touch_io_gt911_config_t tp_gt911_config = {
        .dev_addr = gt911TouchIoConfig.dev_addr,
    };
    const esp_lcd_touch_config_t gt911_touch_cfg = {
        .x_max = SUNTON_ESP32_LCD_WIDTH,
        .y_max = SUNTON_ESP32_LCD_HEIGHT,
        .rst_gpio_num = SUNTON_ESP32_TOUCH_PIN_RST,
        .int_gpio_num = SUNTON_ESP32_TOUCH_PIN_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .process_coordinates = NULL,
        .interrupt_callback = NULL,
        .user_data = NULL,
        .driver_data = &tp_gt911_config,
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(gt911TouchIoHandle, &gt911_touch_cfg, &touchHandle));

    // assign callback and handle
    ESP_LOGI(TAG, "LVGL Init");
    lv_init();

    void *buf1 = NULL;
    void *buf2 = NULL;
    int buffer_size;

    ESP_LOGI(TAG, "Register display to lvgl");
    lv_display_t * disp = lv_display_create(SUNTON_ESP32_LCD_WIDTH, SUNTON_ESP32_LCD_HEIGHT);
    lv_display_set_user_data(disp, panelHandle);
    lv_display_set_flush_cb(disp, lvglDispFlush);

    buffer_size = SUNTON_ESP32_LCD_WIDTH * SUNTON_ESP32_LCD_HEIGHT * sizeof(lv_color_t); // 2 = 16bit color data

    // register flush callback to avoid tearing effect
    const esp_lcd_rgb_panel_event_callbacks_t vsyncCbs = {
        .on_vsync = lvglPortFlushVsyncReadyCallback,
        .on_bounce_empty = NULL,
        .on_bounce_frame_finish = NULL,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panelHandle, &vsyncCbs, disp));

    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panelHandle, 2, &buf1, &buf2));
    lv_display_set_buffers(disp, buf1, buf2, buffer_size, LV_DISPLAY_RENDER_MODE_DIRECT);

    ESP_LOGI(TAG, "Register touch to lvgl");
    lv_indev_t * indevTouchpad = lv_indev_create();
    lv_indev_set_type(indevTouchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_user_data(indevTouchpad, touchHandle);
    lv_indev_set_read_cb(indevTouchpad, touchpadRead);

    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    ESP_LOGI(TAG, "Creating lvgl tick timer");
    const esp_timer_create_args_t lvglTickTimerArgs = {
        .callback = &lvglTick,
        .name = "lvgl_tick"
    };
    ESP_ERROR_CHECK(esp_timer_create(&lvglTickTimerArgs, &lvglTickTimerHandle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvglTickTimerHandle, LVGL_TICK_PERIOD_MS * 1000));

    xTaskCreate(lvglPortTask, "lvgl_port_task", (8 * 1024), NULL, 3, &lvglPortTaskHandle);

    ESP_LOGI(TAG, "Started");
    return disp;
}
