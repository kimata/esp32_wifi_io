#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_ota_ops.h"

#include "app.h"

#include "http_task.h"
#include "wifi_task.h"
#include "part_info.h"

SemaphoreHandle_t wifi_start = NULL;

void app_main()
{
    part_info_show("Running", esp_ota_get_running_partition());

    vSemaphoreCreateBinary(wifi_start);
    xSemaphoreTake(wifi_start, portMAX_DELAY);

    wifi_task_start(wifi_start);

    xSemaphoreTake(wifi_start, portMAX_DELAY);
    http_task_start();
}
