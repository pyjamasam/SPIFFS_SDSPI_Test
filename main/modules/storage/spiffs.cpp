#include "modules/storage/spiffs.h"

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include <esp_spiffs.h>
#include <esp_partition.h>
#include <esp_log.h>

#define LOGAREA "spiffs"
SPIFFS_SPI_Test::Storage::SPIFFS* SPIFFS_SPI_Test::Storage::SPIFFS::_instance[2] = { NULL, NULL };

#define MOUNT_POINT_UI "/ui"
#define MOUNT_POINT_DATA "/data"

SPIFFS_SPI_Test::Storage::SPIFFS::SPIFFS(const parition_type partition) : _vfs_conf {
    .base_path = NULL,
    .partition_label = NULL,
    .max_files = 10,
    .format_if_mount_failed = true
}
{
    _partition = partition;
}

const char* SPIFFS_SPI_Test::Storage::SPIFFS::mountPoint() const
{
    switch (_partition)
    {
        case partition_ui:
        {
            return MOUNT_POINT_UI;
            break;
        }

        case partition_data:
        {
            return MOUNT_POINT_DATA;
            break;
        }

        default:
        {
            // Unknown spiffs partition type
            return NULL;
        }
    }
}


void SPIFFS_SPI_Test::Storage::SPIFFS::setup()
{
    switch (_partition)
    {
        case partition_ui:
        {
            _vfs_conf.base_path = MOUNT_POINT_UI;
            _vfs_conf.partition_label = "spiffs-ui";

            esp_partition_iterator_t foundPartition = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, _vfs_conf.partition_label);
            if (foundPartition)
            {
                // We found the nicley labeled partition.  Use it.
                esp_partition_iterator_release(foundPartition);
            }
            else
            {
                // This is more then likley an OTA upgraded v1 firmware board. So use the old partition name
                _vfs_conf.partition_label = "spiffs1";
            }
            break;
        }

        case partition_data:
        {
            _vfs_conf.base_path = MOUNT_POINT_DATA;
            _vfs_conf.partition_label = "spiffs-data";

            esp_partition_iterator_t foundPartition = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, _vfs_conf.partition_label);
            if (foundPartition)
            {
                // We found the nicley labeled partition.  Use it.

                esp_partition_iterator_release(foundPartition);
            }
            else
            {
                // This is more then likley an OTA upgraded v1 firmware board. So use the old partition name
                _vfs_conf.partition_label = "spiffs2";
            }
            break;
        }

        default:
        {
            // Unknown spiffs partition type
            return;
        }
    }

    ESP_LOGI(LOGAREA, "Initializing SPIFFS partition (%s)...", _vfs_conf.partition_label);

    esp_err_t ret = esp_vfs_spiffs_register(&_vfs_conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(LOGAREA, "Failed to mount or format filesystem: %s", _vfs_conf.partition_label);
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(LOGAREA, "Failed to find SPIFFS partition: %s", _vfs_conf.partition_label);
        } else {
            ESP_LOGE(LOGAREA, "Failed to initialize SPIFFS partition (%s): %s", _vfs_conf.partition_label, esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(_vfs_conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(LOGAREA, "Failed to get SPIFFS partition (%s) information (%s)", _vfs_conf.partition_label, esp_err_to_name(ret));
    } else {
        ESP_LOGI(LOGAREA, "Partition size (%s): total: %d, used: %d", _vfs_conf.partition_label, total, used);
    }

    _rev = strdup("");
    _version = 1;        
}

const char* SPIFFS_SPI_Test::Storage::SPIFFS::rev() const
{
    return _rev;
}
