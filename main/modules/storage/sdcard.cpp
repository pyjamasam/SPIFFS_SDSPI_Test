#include "modules/storage/sdcard.h"

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include <esp_idf_version.h>

#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <driver/sdspi_host.h>
#include <driver/spi_common.h>
#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>
#include <vfs_fat_internal.h>

#include <diskio_impl.h>
#include <diskio_sdmmc.h>


#define LOGAREA "sdcard"

SPIFFS_SPI_Test::Storage::SDCard* SPIFFS_SPI_Test::Storage::SDCard::_instance = NULL;

SPIFFS_SPI_Test::Storage::SDCard::SDCard()
{
    _online = false;
    _cardpresent = false;
}

void SPIFFS_SPI_Test::Storage::SDCard::setup()
{
    ESP_LOGI(LOGAREA, "Initializing SD card (spi)...");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = VSPI_HOST;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = VSPI_IOMUX_PIN_NUM_MOSI,
        .miso_io_num = VSPI_IOMUX_PIN_NUM_MISO,
        .sclk_io_num = VSPI_IOMUX_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4094,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_IOMUX_PINS,
        .intr_flags = ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL3
    };

    esp_err_t err = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, 2);
    if (err != ESP_OK)
    {
        ESP_LOGE(LOGAREA, "Failed to initialize bus.");
        return;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.host_id   = (spi_host_device_t)host.slot;
    slot_config.gpio_cs   = GPIO_NUM_5;
    slot_config.gpio_cd   = GPIO_NUM_NC;
    slot_config.gpio_wp   = GPIO_NUM_NC;
    slot_config.gpio_int  = GPIO_NUM_NC;
    

    // Use FatFS directly
    bool perform_deinit = false;
    err = (*host.init)();
    if (err == ESP_OK)
    {
        int card_handle = -1;   //uninitialized
        err = sdspi_host_init_device(&slot_config, &card_handle);
        if (err == ESP_OK)
        {
            perform_deinit = true;
            // The 'slot' argument inside host_config should be replaced by the SD SPI handled returned
            host.slot = card_handle;

            // probe and initialize card
            err = sdmmc_card_init(&host, &_card);
            if (err == ESP_OK) 
            {
                _cardpresent = true;

                // Output some info about our card
                sdmmc_card_print_info(stdout, &_card);

                BYTE pdrv = FF_DRV_NOT_USED;
                err = ff_diskio_get_drive(&pdrv);
                if (err == ESP_OK && pdrv != FF_DRV_NOT_USED)
                {
                    ff_diskio_register_sdmmc(pdrv, &_card);

                    //ESP_LOGE(LOGAREA, "fatfs using pdrv=%i", pdrv);
                    char drv[3] = {(char)('0' + pdrv), ':', 0};

                    // Try to mount partition
                    FRESULT res = f_mount(&_fs, drv, 1);
                    if (res == FR_OK) 
                    {
                        _online = true;
                        ESP_LOGI(LOGAREA, "SD card online");

                        // SD is online and mounted
                        return;

                    }
                    else
                    {
                        ESP_LOGW(LOGAREA, "failed to mount card (%d)", res);

                        // SD is online but we were unable to mount it
                        return;
                    }                
                }
                else
                {
                    // Unable to get a unused ff disk drive
                    ESP_LOGE(LOGAREA, "ff_diskio_get_drive failed");
                }                                         
            }
            else
            {
                ESP_LOGE(LOGAREA, "sdmmc_card_init failed");                    
            }
        }
        else 
        {
            ESP_LOGE(LOGAREA, "sdspi slot init failed");
        }
    }
    else 
    {
        ESP_LOGE(LOGAREA, "spi host init failed");
    }

    // If we have fallen this far through we had an error init'ing the sd hardware
    // and we need to clean up
    if (perform_deinit)
    {
        if (host.flags & SDMMC_HOST_FLAG_DEINIT_ARG) 
        {
            host.deinit_p(host.slot);
        }
        else
        {
            host.deinit();
        }
    }
}


void SPIFFS_SPI_Test::Storage::SDCard::unlink(const char* filename)
{
    f_unlink(filename);
}

void SPIFFS_SPI_Test::Storage::SDCard::space(uint32_t *totalsize, uint32_t *freespace)
{
    FATFS *fs;
    DWORD fre_clust = 0, fre_sect = 0, tot_sect = 0;

    /* Get volume information and free clusters of drive 0 */
    FRESULT res = f_getfree("0:", &fre_clust, &fs);
    if (res == FR_OK)
    {
        /* Get total sectors and free sectors */
        tot_sect = (fs->n_fatent - 2) * fs->csize;
        fre_sect = fre_clust * fs->csize;
    }
    else
    {
        ESP_LOGE(LOGAREA, "f_getfree failed (%d)", res);
    }

    *totalsize = tot_sect / 2;
    *freespace = fre_sect / 2;
}

// partition_card has been lifted directly from esp-idf v4.2 vfs_fat_sdmmc.c
static esp_err_t partition_card(BYTE pdrv, const char *drv, sdmmc_card_t *card)
{
    FRESULT res = FR_OK;
    esp_err_t err = ESP_OK;
    const size_t workbuf_size = 4096;
    void* workbuf = NULL;
    ESP_LOGW(LOGAREA, "partitioning card");

    workbuf = ff_memalloc(workbuf_size);
    if (workbuf == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    DWORD plist[] = {100, 0, 0, 0};
    res = f_fdisk(pdrv, plist, workbuf);
    if (res != FR_OK)
    {
        err = ESP_FAIL;
        ESP_LOGD(LOGAREA, "f_fdisk failed (%d)", res);
    }
    else
    {
        size_t alloc_unit_size = esp_vfs_fat_get_allocation_unit_size(card->csd.sector_size, 16*1024);
        ESP_LOGW(LOGAREA, "formatting card, allocation unit size=%d", alloc_unit_size);
        
        res = f_mkfs(drv, FM_FAT32, alloc_unit_size, workbuf, workbuf_size);
        if (res != FR_OK)
        {
            err = ESP_FAIL;
            ESP_LOGD(LOGAREA, "f_mkfs failed (%d)", res);
        }
    }

    ff_memfree(workbuf);
    return err;
}

esp_err_t SPIFFS_SPI_Test::Storage::SDCard::repartition()
{
    esp_err_t err = ESP_OK;

    uint8_t pdrv = 0;
    char drv[3] = {(char)('0' + pdrv), ':', 0};

    // unmount the drive if its mounted
    FRESULT res = f_mount(NULL, drv, 0);
    if (res == FR_OK)
    {
        err = partition_card(pdrv, drv, &_card);
        if (err !=ESP_OK)
        {
            ESP_LOGE(LOGAREA, "partition_card failed (0x%x).", err);
        }
        else
        {
            // Attempt to re-mount the drive
            FRESULT res = f_mount(&_fs, drv, 1);
            if (res == FR_OK)
            {
                ESP_LOGE(LOGAREA, "Card repartitioned");
                err = ESP_OK;
            }
            else
            {
                ESP_LOGE(LOGAREA, "f_mount failed (0x%x).", res);
                err = ESP_FAIL;
            }
        }
    }
    else
    {
        ESP_LOGE(LOGAREA, "f_mount (unmount) failed (0x%x).", res);
        err = ESP_FAIL;
    }

    return err;
}
