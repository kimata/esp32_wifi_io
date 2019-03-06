#include <string.h>

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_task_wdt.h"

#include "nvs_flash.h"

#include "app.h"
#include "wifi_task.h"
#include "wifi_config.h"
// wifi_config.h should define followings.
// #define WIFI_SSID "XXXXXXXX"            // WiFi SSID
// #define WIFI_PASS "XXXXXXXX"            // WiFi Password

static const uint32_t FATAL_DISCON_COUNT = 5;

static uint32_t wifi_discon_count = 0;
static SemaphoreHandle_t wifi_start = NULL;
static SemaphoreHandle_t wifi_stop  = NULL;

static void wifi_disconnect()
{
    ESP_LOGI(TAG, "Disconnect WiFi.");
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
}

static const char *wifi_authmode_str(wifi_auth_mode_t mode)
{
    switch (mode) {
    case WIFI_AUTH_OPEN:
        return "open";
    case WIFI_AUTH_WEP:
        return "WEP";
    case WIFI_AUTH_WPA_PSK:
        return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK:
        return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
        return "WPA_WPA2_PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE:
        return "WPA2_ENTERPRISE";
    default:
        return "?";
    }
}

static const char *wifi_cipher_type_str(wifi_cipher_type_t type)
{
    switch (type) {
    case WIFI_CIPHER_TYPE_NONE:
        return "none";
    case WIFI_CIPHER_TYPE_WEP40:
        return "WEP40";
    case WIFI_CIPHER_TYPE_WEP104:
        return "WEP104";
    case WIFI_CIPHER_TYPE_TKIP:
        return "TKIP";
    case WIFI_CIPHER_TYPE_CCMP:
        return "CCMP";
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        return "TKIP and CCMP";
    default:
        return "?";
    }
}

static void wifi_log_rssi()
{
    wifi_ap_record_t ap_info;

    if (esp_wifi_sta_get_ap_info(&ap_info) != OK) {
        ESP_LOGE(TAG, "Failed to get AP info.");
        return;
    }

    ESP_LOGI(TAG, "WiFi status: SSID=%s, CH=%d, AUTH=%s, CIPHER=%s, RSSI=%d",
             ap_info.ssid, ap_info.primary,
             wifi_authmode_str(ap_info.authmode),
             wifi_cipher_type_str(ap_info.pairwise_cipher),
             ap_info.rssi);
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "Event: SYSTEM_EVENT_STA_START.");
        ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, WIFI_HOSTNAME));
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "Event: SYSTEM_EVENT_STA_GOT_IP.");
        xSemaphoreGive(wifi_start);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "Event: SYSTEM_EVENT_STA_DISCONNECTED.");
        xSemaphoreGive(wifi_stop);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void init_wifi()
{
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

#ifdef WIFI_SSID
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    wifi_config_t wifi_config_cur;
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config_cur));

    if (strcmp((const char *)wifi_config_cur.sta.ssid, (const char *)wifi_config.sta.ssid) ||
        strcmp((const char *)wifi_config_cur.sta.password, (const char *)wifi_config.sta.password)) {
        ESP_LOGI(TAG, "Save WIFI config.");
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    }
#endif
}

static esp_err_t wifi_connect()
{
    ESP_LOGI(TAG, "Start to connect to WiFi.");
    xSemaphoreTake(wifi_start, portMAX_DELAY);
    ESP_ERROR_CHECK(esp_wifi_start());
    if (xSemaphoreTake(wifi_start, 10000 / portTICK_RATE_MS) == pdTRUE) {
        ESP_LOGI(TAG, "Succeeded in connecting to WiFi.");
        wifi_log_rssi();
        wifi_discon_count = 0;
        xSemaphoreGive(wifi_start);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to connect to WiFi.");
        wifi_discon_count++;
        xSemaphoreGive(wifi_start);
        xSemaphoreGive(wifi_stop);
        return ESP_FAIL;
    }
}

static void wifi_watch_task(void *param) {
    SemaphoreHandle_t mutex = (SemaphoreHandle_t)param;

    ESP_ERROR_CHECK(esp_task_wdt_init(60, true));
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    vSemaphoreCreateBinary(wifi_start);
    vSemaphoreCreateBinary(wifi_stop);

    init_wifi();
    xSemaphoreTake(wifi_stop, portMAX_DELAY);
    if ((wifi_connect() == ESP_OK) && (mutex != NULL)) {
        xSemaphoreGive(mutex);
    }

    while (1) {
        if (xSemaphoreTake(wifi_stop, 30000 / portTICK_RATE_MS) == pdTRUE) {
            ESP_LOGI(TAG, "WiFi disconnect count: %d",  wifi_discon_count);
            // NOTE: 接続に一定回数連続して失敗したら，何かがおかしいので再起動する．
            if (wifi_discon_count >= FATAL_DISCON_COUNT) {
                ESP_LOGI(TAG, "Too many connect failures, restarting...");
                esp_restart();
            }
            wifi_disconnect();
            if ((wifi_connect() == ESP_OK) && (mutex != NULL)) {
                xSemaphoreGive(mutex);
            }
        }
        ESP_ERROR_CHECK(esp_task_wdt_reset());
    }
}

void wifi_task_start(SemaphoreHandle_t mutex)
{
    xTaskCreate(wifi_watch_task, "wifi_watch_task", 2048, mutex, 10, NULL);
}
