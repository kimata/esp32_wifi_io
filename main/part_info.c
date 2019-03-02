#include "app.h"
#include "part_info.h"

static const char *part_type(const esp_partition_t *part)
{
    switch (part->type) {
    case ESP_PARTITION_TYPE_APP:
        return "app";
    case ESP_PARTITION_TYPE_DATA:
        return "data";
    default:
        return "?";
    }
}

static const char *part_subtype(const esp_partition_t *part)
{
    if (part->type == ESP_PARTITION_TYPE_APP) {
        switch (part->subtype) {
        case ESP_PARTITION_SUBTYPE_APP_FACTORY:
            return "Factory application";
        case ESP_PARTITION_SUBTYPE_APP_OTA_0:
            return "OTA 0";
        case ESP_PARTITION_SUBTYPE_APP_OTA_1:
            return "OTA 1";
        case ESP_PARTITION_SUBTYPE_APP_OTA_2:
            return "OTA 2";
        case ESP_PARTITION_SUBTYPE_APP_OTA_3:
            return "OTA 3";
        case ESP_PARTITION_SUBTYPE_APP_OTA_4:
            return "OTA 4";
        case ESP_PARTITION_SUBTYPE_APP_OTA_5:
            return "OTA 5";
        case ESP_PARTITION_SUBTYPE_APP_OTA_6:
            return "OTA 6";
        case ESP_PARTITION_SUBTYPE_APP_OTA_7:
            return "OTA 7";
        case ESP_PARTITION_SUBTYPE_APP_OTA_8:
            return "OTA 8";
        case ESP_PARTITION_SUBTYPE_APP_OTA_9:
            return "OTA 9";
        case ESP_PARTITION_SUBTYPE_APP_OTA_10:
            return "OTA 10";
        case ESP_PARTITION_SUBTYPE_APP_OTA_11:
            return "OTA 11";
        case ESP_PARTITION_SUBTYPE_APP_OTA_12:
            return "OTA 12";
        case ESP_PARTITION_SUBTYPE_APP_OTA_13:
            return "OTA 13";
        case ESP_PARTITION_SUBTYPE_APP_OTA_14:
            return "OTA 14";
        case ESP_PARTITION_SUBTYPE_APP_OTA_15:
            return "OTA 15";
        default:
            return "?";
        }
    } else if (part->type == ESP_PARTITION_TYPE_DATA) {
        switch (part->subtype) {
        case ESP_PARTITION_SUBTYPE_DATA_OTA:
            return "OTA selection";
        case ESP_PARTITION_SUBTYPE_DATA_PHY:
            return "PHY init data";
        case ESP_PARTITION_SUBTYPE_DATA_NVS:
            return "NVS";
        case ESP_PARTITION_SUBTYPE_DATA_COREDUMP:
            return "COREDUMP";
        case ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS:
            return "NVS keys";
        case ESP_PARTITION_SUBTYPE_DATA_EFUSE_EM:
            return "Emulate eFuse bits";
        case ESP_PARTITION_SUBTYPE_DATA_ESPHTTPD:
            return "ESPHTTPD";
        case ESP_PARTITION_SUBTYPE_DATA_FAT:
            return "FAT";
        case ESP_PARTITION_SUBTYPE_DATA_SPIFFS:
            return "SPIFFS";
        default:
            return "?";
        }
    } else {
        return "?";
    }
}

void part_info_show(const char *label, const esp_partition_t *part)
{
    ESP_LOGI(TAG, "%s partition: type=%s, subtyp=%s, addr=0x%04X, size=%dKB, label=%s",
             label,
             part_type(part), part_subtype(part),
             part->address, part->size / 1024, part->label);
}
