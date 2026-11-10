#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "nvs_flash.h"

static const char *TAG = "camera";

#define AP_SSID       "XIAO-CAM"
#define AP_PASS       "12345678"
#define JPEG_QUALITY  10      //10 was found in the Arduino IDE testing

//Fills a camera_config_t and passes it to esp_camera_init()
static esp_err_t init_camera(framesize_t size)
{
    camera_config_t config = {};

    //Generates a clock signal for the sensor
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;

    //Control pins now wired on the board
    config.pin_pwdn  = -1;
    config.pin_reset = -1;

    //Carry the sensor's input clock
    config.pin_xclk  = 10;

    //Control bus is a two-wire link that the
    //driver uses to read and write the sensor's setting
    config.pin_sccb_sda = 40;
    config.pin_sccb_scl = 39;

    //Carry image data one byte at a time
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

    //Sets the sensor's input clock to 20MHz
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size   = size;
    config.jpeg_quality = JPEG_QUALITY;

    //Two frame buffers (so one can be filled while the other is being sent)
    config.fb_count     = 2;

    //Buffers live in 8 MB PSRAM since a large frame
    //wouldn't fit in internal RAM
    config.fb_location  = CAMERA_FB_IN_PSRAM;

    //Always hand the newest frame and drop older ones (reduces lag)
    config.grab_mode    = CAMERA_GRAB_LATEST;

    return esp_camera_init(&config);
}

//Has driver calls that write the sensor registers
static void tune_sensor(sensor_t *s)
{
    //Unsupported settings are ignored by the driver
    s->set_brightness(s, 0);
    s->set_contrast(s, 0);
    s->set_saturation(s, 0);
    s->set_sharpness(s, 1);
    s->set_whitebal(s, 1);        // auto white balance
    s->set_awb_gain(s, 1);
    s->set_exposure_ctrl(s, 1);   // auto exposure
    s->set_aec2(s, 1);            // advanced auto exposure (helps in dim light)
    s->set_gain_ctrl(s, 1);       // auto gain
    s->set_lenc(s, 1);            // lens shading correction
    s->set_bpc(s, 1);             // black pixel correction
    s->set_wpc(s, 1);             // white pixel correction
    s->set_raw_gma(s, 1);         // gamma correction
    
}

//Returns a small HTML containing <img src='/stream'>
static esp_err_t index_handler(httpd_req_t *req)
{
    const char *html =
        "<html><body style='margin:0;background:#000'>"
        "<img src='/stream' style='width:100%;height:auto'></body></html>";
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

//Grabs one frame, sends it as a JPEG, then returns the buffer
static esp_err_t capture_handler(httpd_req_t *req)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    return res;
}

//Endless loop that grabs a frame and sends it as one 
//part of a response (Standard way to do an MJPEG stream)
/*
(1) browser shows each new image as it arrives
(2) sends a boundary marker (small header with the byte length) and JPEG bytes
(3) exits when sending fails (viewer closed page) and occupies the server when running

*/
static esp_err_t stream_handler(httpd_req_t *req)
{
    esp_err_t res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
    if (res != ESP_OK) return res;

    char part[80];
    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        res = httpd_resp_send_chunk(req, "\r\n--frame\r\n", HTTPD_RESP_USE_STRLEN);
        if (res == ESP_OK) {
            int n = snprintf(part, sizeof(part),
                "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", (unsigned)fb->len);
            res = httpd_resp_send_chunk(req, part, n);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        }
        esp_camera_fb_return(fb);
        if (res != ESP_OK) break;
    }
    return res;
}

//Creates the HTTP server with the default config, raises
//the tack stack to 8192 bystes, and registers the three URLs
static void start_server()
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.stack_size = 8192;
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &cfg) == ESP_OK) {
        httpd_uri_t idx = {};
        idx.uri = "/";        idx.method = HTTP_GET; idx.handler = index_handler;
        httpd_uri_t cap = {};
        cap.uri = "/capture"; cap.method = HTTP_GET; cap.handler = capture_handler;
        httpd_uri_t str = {};
        str.uri = "/stream";  str.method = HTTP_GET; str.handler = stream_handler;
        httpd_register_uri_handler(server, &idx);
        httpd_register_uri_handler(server, &cap);
        httpd_register_uri_handler(server, &str);
        ESP_LOGI(TAG, "Server up: http://192.168.4.1  (still: /capture)");
    }
}

//Initializes the NVS, network stack, and event loop,
//also  creayes the access point interface with the address
//and sets the password, channel 1, and max of 2 connections 
static void start_softap()
{
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wcfg);

    wifi_config_t ap = {};
    strcpy((char *)ap.ap.ssid, AP_SSID);
    ap.ap.ssid_len = strlen(AP_SSID);
    strcpy((char *)ap.ap.password, AP_PASS);
    ap.ap.channel = 1;
    ap.ap.max_connection = 2;
    ap.ap.authmode = WIFI_AUTH_WPA2_PSK;

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap);
    esp_wifi_start();
    esp_wifi_set_ps(WIFI_PS_NONE);
}

#define PREFER_QXGA  0    //set to 1 to try 2048x1536 on an OV3660

//Main function
/*
(1) sets the camera at UXGA (1600x1200)
(2) logs the sensor ID (Sensor PID)
(3) if PREFER_QXGA is 1 and the
(4) applies tune_sensor, captures 1 frame, and logs its real size
(5) starts WiFi and the server
*/
extern "C" void app_main(void)
{
    esp_err_t err = init_camera(FRAMESIZE_UXGA);          // 1600x1200
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: 0x%x (%s)", err, esp_err_to_name(err));
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    ESP_LOGI(TAG, "Sensor PID: 0x%x", s->id.PID);

#if PREFER_QXGA
    if (s->id.PID == OV3660_PID) {
        esp_camera_deinit();
        err = init_camera(FRAMESIZE_QXGA);                // 2048x1536
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "QXGA init failed, falling back to UXGA");
            esp_camera_deinit();
            err = init_camera(FRAMESIZE_UXGA);
            if (err != ESP_OK) return;
        }
    }
#endif

    tune_sensor(esp_camera_sensor_get());

    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
        ESP_LOGI(TAG, "Actual frame: %dx%d, %u bytes",
                 (int)fb->width, (int)fb->height, (unsigned)fb->len);
        esp_camera_fb_return(fb);
    }

    start_softap();
    start_server();
}