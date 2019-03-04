#include <stdlib.h>
#include <string.h>

#include "esp_ota_ops.h"
#include "cJSON.h"

#include "app.h"
#include "http_task.h"
#include "http_ota_handler.h"

#define ARRAY_SIZE_OF(a) (sizeof(a) / sizeof(a[0]))

#define APP_PATH "/app"
#define DRIVE_PERIOD_MS 300

extern const unsigned char index_html_start[] asm("_binary_index_html_start");
extern const unsigned char index_html_end[]   asm("_binary_index_html_end");
extern const unsigned char runtime_js_start[] asm("_binary_runtime_js_gz_start");
extern const unsigned char runtime_js_end[]   asm("_binary_runtime_js_gz_end");
extern const unsigned char main_js_start[] asm("_binary_main_js_gz_start");
extern const unsigned char main_js_end[]   asm("_binary_main_js_gz_end");
extern const unsigned char polyfills_js_start[] asm("_binary_polyfills_js_gz_start");
extern const unsigned char polyfills_js_end[]   asm("_binary_polyfills_js_gz_end");
extern const unsigned char scripts_js_start[] asm("_binary_scripts_js_gz_start");
extern const unsigned char scripts_js_end[]   asm("_binary_scripts_js_gz_end");
extern const unsigned char styles_css_start[] asm("_binary_styles_css_gz_start");
extern const unsigned char styles_css_end[]   asm("_binary_styles_css_gz_end");
extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_gz_start");
extern const unsigned char favicon_ico_end[]   asm("_binary_favicon_ico_gz_end");

typedef struct static_content {
    const char *path;
    const unsigned char *data_start;
    const unsigned char *data_end;
    const char *content_type;
    bool is_gzip;
} static_content_t;

static static_content_t content_list[] = {
    { "index.htm", index_html_start, index_html_end, "text/html", false, },
    { "runtime.js", runtime_js_start, runtime_js_end, "text/javascript", true, },
    { "main.js", main_js_start, main_js_end, "text/javascript", true, },
    { "polyfills.js", polyfills_js_start, polyfills_js_end, "text/javascript", true, },
    { "scripts.js", scripts_js_start, scripts_js_end, "text/javascript", true, },
    { "styles.css", styles_css_start, styles_css_end, "text/css", true, },
    { "favicon.ico", favicon_ico_start, favicon_ico_end, "image/x-icon", true, },
};

static esp_err_t http_handle_app(httpd_req_t *req)
{
    static_content_t *content = NULL;

    for (uint32_t i = 0; i < ARRAY_SIZE_OF(content_list); i++) {
        if (strstr(req->uri, content_list[i].path) != NULL) {
            content = &(content_list[i]);
        }
    }
    if (content == NULL) {
        content = &(content_list[0]);
    }

    ESP_ERROR_CHECK(httpd_resp_set_type(req, content->content_type));
    if (content->is_gzip) {
        ESP_ERROR_CHECK(httpd_resp_set_hdr(req, "Content-Encoding", "gzip"));
    }
    httpd_resp_send(req,
                    (const char *)content->data_start,
                    content->data_end - content->data_start);

    return ESP_OK;
}

static esp_err_t http_handle_app_redirect(httpd_req_t *req) {
    ESP_ERROR_CHECK(httpd_resp_set_status(req, "301 Moved Permanently"));
    ESP_ERROR_CHECK(httpd_resp_set_hdr(req, "Location", APP_PATH "/"));
    httpd_resp_sendstr(req, "");

    return ESP_OK;
}

static void gpio_ctrl_task(void *param) {
    gpio_config_t io_conf;
    uint8_t gpio_num = (uint32_t)param;

    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << gpio_num;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;

    gpio_config(&io_conf);

    vTaskDelay(DRIVE_PERIOD_MS / portTICK_PERIOD_MS);

    io_conf.mode = GPIO_MODE_INPUT;
    gpio_config(&io_conf);

    vTaskDelete(NULL); // NOTE: 自分自身を削除
}

static esp_err_t process_api(const char *uri) {
    const char *gpio_str;
    uint32_t gpio_num;

    gpio_str = strrchr(uri, '/');
    if (gpio_str == NULL) {
        return ESP_FAIL;
    }
    gpio_num = atoi(gpio_str + 1);

    // NOTE: 実際の GPIO は別タスクで行い，HTTP の応答は即返せるようにする
    xTaskCreate(gpio_ctrl_task, "gpio_ctrl_task", 2048, (void *)gpio_num, 10, NULL);

    return ESP_OK;
}

static esp_err_t http_handle_api(httpd_req_t *req)
{
    esp_err_t api_result = process_api(req->uri);
    ESP_ERROR_CHECK(httpd_resp_set_type(req, "text/json"));
    if (api_result == ESP_OK) {
        httpd_resp_sendstr(req, "{ \"status\": \"OK\" }");
    } else {
        httpd_resp_sendstr(req, "{ \"status\": \"NG\" }");
    }

    return ESP_OK;
}

static esp_err_t http_handle_status(httpd_req_t *req)
{
    const esp_partition_t *part_info;
    esp_app_desc_t app_info;
    char elapsed_str[32];
    uint32_t elapsed_sec, day, hour, min, sec;

    part_info = esp_ota_get_running_partition();
    ESP_ERROR_CHECK(esp_ota_get_partition_description(part_info, &app_info));

    elapsed_sec = (uint32_t)(esp_timer_get_time() / 1000000);
    day = elapsed_sec / 86400;
    hour = (elapsed_sec % 86400) / 3600;
    min = (elapsed_sec % 3600) / 60;
    sec = elapsed_sec % 60;

    sprintf(elapsed_str, "%d day(s) %02d:%02d:%02d", day, hour, min, sec);

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "name", app_info.project_name);
    cJSON_AddStringToObject(json, "version", app_info.version);
    cJSON_AddStringToObject(json, "esp_idf", app_info.idf_ver);
    cJSON_AddStringToObject(json, "compile_date", app_info.date);
    cJSON_AddStringToObject(json, "compile_time", app_info.time);
    cJSON_AddStringToObject(json, "elapsed", elapsed_str);

    httpd_resp_sendstr(req, cJSON_PrintUnformatted(json));

    cJSON_Delete(json);

    return ESP_OK;
}

static httpd_uri_t http_uri_app = {
    .uri       = APP_PATH "*",
    .method    = HTTP_GET,
    .handler   = http_handle_app,
    .user_ctx  = NULL
};


static httpd_uri_t http_uri_app_redirect = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = http_handle_app_redirect,
    .user_ctx  = NULL
};

static httpd_uri_t http_uri_api = {
    .uri       = "/api*",
    .method    = HTTP_GET,
    .handler   = http_handle_api,
    .user_ctx  = NULL
};

static httpd_uri_t http_uri_status = {
    .uri       = "/status*",
    .method    = HTTP_GET,
    .handler   = http_handle_status,
    .user_ctx  = NULL
};


httpd_handle_t http_task_start(void)
{
    ESP_LOGI(TAG, "Start HTTP server.");

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_ERROR_CHECK(httpd_start(&server, &config));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_uri_app));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_uri_app_redirect));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_uri_api));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_uri_status));

    http_ota_handler_install(server);

    return server;
}

void http_task_stop(httpd_handle_t server)
{
    ESP_ERROR_CHECK(httpd_stop(server));
}
