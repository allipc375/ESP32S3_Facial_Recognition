#include <Arduino.h>
#include "esp_camera.h"

#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39
#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    14
#define Y5_GPIO_NUM    16
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    17
#define Y2_GPIO_NUM    15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  13

bool cameraOK = false;
esp_err_t initError = ESP_OK;

void setup() {
  Serial.begin(115200);
  delay(4000);
  Serial.println("\n=== RGB565 capture test ===");
  Serial.printf("PSRAM: %u bytes, free: %u\n", ESP.getPsramSize(), ESP.getFreePsram());

  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer   = LEDC_TIMER_0;
  c.pin_d0 = Y2_GPIO_NUM; c.pin_d1 = Y3_GPIO_NUM;
  c.pin_d2 = Y4_GPIO_NUM; c.pin_d3 = Y5_GPIO_NUM;
  c.pin_d4 = Y6_GPIO_NUM; c.pin_d5 = Y7_GPIO_NUM;
  c.pin_d6 = Y8_GPIO_NUM; c.pin_d7 = Y9_GPIO_NUM;
  c.pin_xclk = XCLK_GPIO_NUM;   c.pin_pclk = PCLK_GPIO_NUM;
  c.pin_vsync = VSYNC_GPIO_NUM; c.pin_href = HREF_GPIO_NUM;
  c.pin_sccb_sda = SIOD_GPIO_NUM; c.pin_sccb_scl = SIOC_GPIO_NUM;
  c.pin_pwdn = PWDN_GPIO_NUM;   c.pin_reset = RESET_GPIO_NUM;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_RGB565;
  c.frame_size   = FRAMESIZE_240X240;
  c.fb_count     = 2;
  c.fb_location  = CAMERA_FB_IN_PSRAM;
  c.grab_mode    = CAMERA_GRAB_LATEST;

  initError = esp_camera_init(&c);
  if (initError != ESP_OK) {
    Serial.printf("Camera init FAILED: 0x%x\n", initError);
    return;
  }
  sensor_t *s = esp_camera_sensor_get();
  if (!s) { Serial.println("Sensor NULL"); return; }
  Serial.printf("Camera OK. Sensor PID: 0x%x\n", s->id.PID);
  cameraOK = true;
}

void loop() {
  if (!cameraOK) {
    Serial.printf("Camera not working (0x%x)\n", initError);
    delay(2000);
    return;
  }
  uint32_t t = millis();
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { Serial.println("Capture failed"); delay(1000); return; }
  Serial.printf("Format %d, %ux%u, %u bytes (expect 115200), %lu ms, free PSRAM %u\n",
                fb->format, fb->width, fb->height, fb->len, millis() - t, ESP.getFreePsram());
  esp_camera_fb_return(fb);
  delay(500);
}