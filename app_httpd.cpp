#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "driver/ledc.h"

#include "sdkconfig.h"
#include "FS.h"
#include "SD_MMC.h"
#include <DHT.h>

extern DHT dht;

#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define TAG ""
#else
#include "esp_log.h"
static const char *TAG = "camera_httpd";
#endif

typedef struct {
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;

static size_t jpg_encode_stream(void * arg, size_t index, const void *data, size_t len){
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if(!index){
        j->len = 0;
    }
    if(httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK){
        return 0;
    }
    j->len += len;
    return len;
}

static esp_err_t stream_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
        } else {
            if(fb->format != PIXFORMAT_JPEG){
                bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                if(!jpeg_converted){
                    ESP_LOGE(TAG, "JPEG conversion failed");
                    res = ESP_FAIL;
                }
            } else {
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(fb){
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf){
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if(res != ESP_OK){
            break;
        }
    }
    last_frame = 0;
    return res;
}

static esp_err_t file_handler(httpd_req_t *req) {
    const char* path = req->uri;
    char full_path[64];
    snprintf(full_path, sizeof(full_path), "/%s", path + 1);

    if (strcmp(path, "/") == 0) {
        snprintf(full_path, sizeof(full_path), "/index.html");
    }

    if (!SD_MMC.exists(full_path)) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    const char* content_type;
    if (strstr(full_path, ".html")) content_type = "text/html";
    else if (strstr(full_path, ".css")) content_type = "text/css";
    else if (strstr(full_path, ".js")) content_type = "application/javascript";
    else content_type = "text/plain";
    
    httpd_resp_set_type(req, content_type);

    File file = SD_MMC.open(full_path, FILE_READ);
    if (!file) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    char* buf = (char*)malloc(file.size() + 1);
    if (!buf) {
        httpd_resp_send_500(req);
        file.close();
        return ESP_FAIL;
    }
    file.readBytes(buf, file.size());
    buf[file.size()] = '\0';
    httpd_resp_sendstr(req, (const char*)buf);
    free(buf);
    file.close();

    return ESP_OK;
}

static esp_err_t dht_handler(httpd_req_t *req) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char response[128];
    snprintf(response, sizeof(response), "{\"temperature\":%.1f,\"humidity\":%.1f}", t, h);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, response);
    return ESP_OK;
}

void startCameraServer(){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 81;

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = file_handler,
        .user_ctx  = NULL
    };
    
    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t css_uri = {
        .uri       = "/style.css",
        .method    = HTTP_GET,
        .handler   = file_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t js_uri = {
        .uri       = "/script.js",
        .method    = HTTP_GET,
        .handler   = file_handler,
        .user_ctx  = NULL
    };
    
    httpd_uri_t dht_uri = {
        .uri       = "/dht",
        .method    = HTTP_GET,
        .handler   = dht_handler,
        .user_ctx  = NULL
    };

    ESP_LOGI(TAG, "Starting web server on port: '%d'", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &index_uri);
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        httpd_register_uri_handler(stream_httpd, &css_uri);
        httpd_register_uri_handler(stream_httpd, &js_uri);
        httpd_register_uri_handler(stream_httpd, &dht_uri);
    }
}