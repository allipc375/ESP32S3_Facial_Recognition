#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "camera";

static esp_err_t init_camera()
{
    camera_config_t config = {};

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;

    config.pin_pwdn  = -1;
    config.pin_reset = -1;
    config.pin_xclk  = 10;
    config.pin_sccb_sda = 40;
    config.pin_sccb_scl = 39;

    config.pin_d7 = 48;   // Y9
    config.pin_d6 = 11;   // Y8
    config.pin_d5 = 12;   // Y7
    config.pin_d4 = 14;   // Y6
    config.pin_d3 = 16;   // Y5
    config.pin_d2 = 18;   // Y4
    config.pin_d1 = 17;   // Y3
    config.pin_d0 = 15;   // Y2

    config.pin_vsync = 38;
    config.pin_href  = 47;
    config.pin_pclk  = 13;

    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_RGB565;
    config.frame_size   = FRAMESIZE_QVGA;   // 320x240
    config.jpeg_quality = 12;
    config.fb_count     = 1;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

    return esp_camera_init(&config);
}

extern "C" void app_main(void)
{
    esp_err_t err = init_camera();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: 0x%x (%s)", err, esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Camera initialized");

    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Frame capture failed");
        } else {
            ESP_LOGI(TAG, "Frame: %dx%d, %u bytes",
                     (int)fb->width, (int)fb->height, (unsigned)fb->len);
            esp_camera_fb_return(fb);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}