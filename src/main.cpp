#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "human_face_detect.hpp"

static const char *TAG = "face";

// init_camera() stays exactly as in your working camera code
// (RGB565, FRAMESIZE_QVGA, fb_count = 1, PSRAM)

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
    config.fb_count     = 1;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

    return esp_camera_init(&config);
}


extern "C" void app_main(void)
{
    if (init_camera() != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed");
        return;
    }

    HumanFaceDetect *detector = new HumanFaceDetect();
    ESP_LOGI(TAG, "Model loaded");

    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        dl::image::img_t img;
        img.data     = fb->buf;
        img.width    = fb->width;
        img.height   = fb->height;
        img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565BE;

        auto &results = detector->run(img);
        if (results.empty()) {
            ESP_LOGI(TAG, "No face");
        }
        for (const auto &r : results) {
            ESP_LOGI(TAG, "face score=%.2f box=[%d,%d,%d,%d]",
                     r.score, r.box[0], r.box[1], r.box[2], r.box[3]);
        }

        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}