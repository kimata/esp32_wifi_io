#include "esp_ota_ops.h"

#include "app.h"
#include "http_ota_handler.h"
#include "part_info.h"

#define BUF_SIZE  1024

static void restart_task(void *param) {
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Restart...");
    esp_restart();
}

static esp_err_t http_handle_ota(httpd_req_t *req)
{
    const esp_partition_t *part;
    esp_ota_handle_t handle;
    char buf[BUF_SIZE];
    int total_size;
    int recv_size;
    int remain;
    uint8_t percent;

    ESP_LOGI(TAG, "Start to update firmware.");

    ESP_ERROR_CHECK(httpd_resp_set_type(req, "text/plain"));
    ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Start to update firmware.\n"));

    part = esp_ota_get_next_update_partition(NULL);
    part_info_show("Target", part);

    total_size = req->content_len;

    ESP_LOGI(TAG, "Sent size: %d KB.", total_size / 1024);

    ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "0        20        40        60        80       100%\n"));
    ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "|---------+---------+---------+---------+---------+\n"));
    ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "*"));

    ESP_ERROR_CHECK(esp_ota_begin(part, total_size, &handle));
    remain = total_size;
    percent = 2;
    while (remain > 0) {
        if (remain < sizeof(buf)) {
            recv_size = remain;
        } else {
            recv_size = sizeof(buf);
        }

        recv_size = httpd_req_recv(req, buf, recv_size);
        if (recv_size <= 0) {
            if (recv_size == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                "Failed to receive firmware.");
            return ESP_FAIL;
        }

        ESP_ERROR_CHECK(esp_ota_write(handle, buf, recv_size));

        remain -= recv_size;
        if (remain < (total_size * (100-percent) / 100)) {
            httpd_resp_sendstr_chunk(req, "*");
            percent += 2;
        }
    }
    ESP_ERROR_CHECK(esp_ota_end(handle));
    ESP_ERROR_CHECK(esp_ota_set_boot_partition(part));
    ESP_LOGI(TAG, "Finished writing firmware.");

    httpd_resp_sendstr_chunk(req, "*\nComplete.\n");
    httpd_resp_sendstr_chunk(req, NULL);

    xTaskCreate(restart_task, "restart_task", 1024, NULL, 10, NULL);

    return ESP_OK;
}

static httpd_uri_t http_uri_ota = {
    .uri       = "/ota*",
    .method    = HTTP_POST,
    .handler   = http_handle_ota,
    .user_ctx  = NULL
};

void http_ota_handler_install(httpd_handle_t server)
{
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_uri_ota));

#ifdef CONFIG_APP_ROLLBACK_ENABLE
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(esp_ota_get_running_partition(),
                                    &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Marking OTA firmware as valid.");
            ESP_ERROR_CHECK(esp_ota_mark_app_valid_cancel_rollback());
        }
    }
#endif
}
