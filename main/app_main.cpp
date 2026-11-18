#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_spiffs.h"
#include "driver/gpio.h"
#include "human_face_detect.hpp"
#include "human_face_recognition.hpp"
#include "esp_heap_caps.h"

static const char *TAG = "face";
#define ENROLL_BUTTON GPIO_NUM_0      // XIAO ESP32-S3 BOOT button

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
    config.pin_d7 = 48;
    config.pin_d6 = 11;
    config.pin_d5 = 12;
    config.pin_d4 = 14;
    config.pin_d3 = 16;
    config.pin_d2 = 18;
    config.pin_d1 = 17;
    config.pin_d0 = 15;
    config.pin_vsync = 38;
    config.pin_href  = 47;
    config.pin_pclk  = 13;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_RGB565;
    config.frame_size   = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count     = 2;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_LATEST;
    return esp_camera_init(&config);
}

static esp_err_t init_storage()
{
    esp_vfs_spiffs_conf_t conf = {};
    conf.base_path = "/spiffs";
    conf.partition_label = NULL;
    conf.max_files = 5;
    conf.format_if_mount_failed = true;
    return esp_vfs_spiffs_register(&conf);
}

extern "C" void app_main(void)
{
    if (init_camera() != ESP_OK) { ESP_LOGE(TAG, "Camera init failed"); return; }
    if (init_storage() != ESP_OK) { ESP_LOGE(TAG, "Storage init failed"); return; }

    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << ENROLL_BUTTON;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io);

    HumanFaceDetect *detector = new HumanFaceDetect();
    HumanFaceRecognizer *recognizer = new HumanFaceRecognizer("/spiffs/face.db");
    ESP_LOGI(TAG, "Ready. Press BOOT with a face in view to enroll.");

    #define CLEAR_DB_ON_BOOT 1
#if CLEAR_DB_ON_BOOT
    recognizer->clear_all_feats();
    ESP_LOGI(TAG, "Database cleared");
#endif
    ESP_LOGI(TAG, "Free PSRAM: %u KB, free internal RAM: %u KB",
             (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024),
             (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024));
    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) { vTaskDelay(pdMS_TO_TICKS(50)); continue; }

        dl::image::img_t img;
        img.data     = fb->buf;
        img.width    = fb->width;
        img.height   = fb->height;
        img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565BE;

        int64_t t0 = esp_timer_get_time();
        auto &det = detector->run(img);
        int64_t t1 = esp_timer_get_time();

        if (det.empty()) {
            ESP_LOGI(TAG, "No face (detect %lld ms)", (t1 - t0) / 1000);
        } else if (gpio_get_level(ENROLL_BUTTON) == 0) {
            recognizer->enroll(img, det);
            ESP_LOGI(TAG, "Enrolled a face");
            while (gpio_get_level(ENROLL_BUTTON) == 0) vTaskDelay(pdMS_TO_TICKS(20));            // avoid duplicate enrolls
        } else {
            auto res = recognizer->recognize(img, det);
            int64_t t2 = esp_timer_get_time();
            if (res.empty()) {
                ESP_LOGI(TAG, "Unknown | detect %lld ms, recognize %lld ms",
                         (t1 - t0) / 1000, (t2 - t1) / 1000);
            }
            for (const auto &r : res) {
                ESP_LOGI(TAG, "id=%d similarity=%.3f | detect %lld ms, recognize %lld ms",
                         (int)r.id, (double)r.similarity,
                         (t1 - t0) / 1000, (t2 - t1) / 1000);
            }
        }
        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}