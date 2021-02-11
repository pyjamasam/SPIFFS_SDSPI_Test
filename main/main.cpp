#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>

#include <ff.h>
#include <algorithm>

#include <string.h>

#include "modules/storage/sdcard.h"
#include "modules/storage/spiffs.h"

#if (ESP_IDF_VERSION_MAJOR == 4 && (ESP_IDF_VERSION_MINOR == 2))
#else
#pragma GCC error ESP-IDF 4.2 is required to build
#endif

#define CORE_0 0
#define CORE_1 1

volatile static bool core0_setup_complete = false;
volatile static bool core1_setup_complete = false;

static char writeBuffer[1024];
static void processing_core0(void *pvParameters)
{
    /*while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }*/

    ESP_LOGI("core0", "Processing starting...");

    FIL _logFP;

    ESP_LOGE("core0", "about to open file");
    FRESULT fres = f_open(&_logFP, "TestFile", FA_CREATE_ALWAYS | FA_WRITE);

    if (fres == FR_OK)
    {
        while(1)
        {
            size_t writeCount = 0;
            ESP_LOGE("core0", "about write");
            fres = f_write(&_logFP, writeBuffer, 1024, &writeCount); 
            printf("wrote: %d\n", writeCount);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    else
    {
        ESP_LOGE("core0", "Couldn't open the output file");
    }
}
#define SCRATCH_BUFSIZE 8*1024
static char chunk[SCRATCH_BUFSIZE];
static void processing_core0_1(void *pvParameters)
{
    /*while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }*/

    ESP_LOGI("core0_1", "Processing starting...");
    SPIFFS_SPI_Test::Storage::SPIFFS* storageInterface = SPIFFS_SPI_Test::Storage::SPIFFS::instance(SPIFFS_SPI_Test::Storage::SPIFFS::partition_ui);
    char* fullFilename = storageInterface->assembleFileName("html/index.html");

    while(1)
    {
        ESP_LOGE("core0_1", "about to open file"); 
        FILE* fd = fopen(fullFilename, "r");

        ESP_LOGE("core0_1", "file open");
        while (!feof(fd) && !ferror(fd))
        {
            size_t chunksize = 0;
                
            size_t readLoc = 0;
            size_t readCount = 0;
            do
            {
                ESP_LOGE("core0_1", "about to read");
                size_t bytesCanRead = std::min((size_t)512, SCRATCH_BUFSIZE - chunksize);
                readCount = fread(chunk + readLoc, 1, bytesCanRead, fd);
                if (readCount == 0)
                {
                    break;
                }
                readLoc += readCount;
                chunksize += readCount;
                taskYIELD();
            } while (chunksize < SCRATCH_BUFSIZE);
        }   

        fclose(fd);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


static void processing_core1(void *pvParameters)
{
    ESP_LOGI("core1", "Processing starting...");
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


static void setup_core0(void *arg)
{
    ESP_LOGI("core0", "Setup starting...");
    
    // Bring up our storage systems
    SPIFFS_SPI_Test::Storage::SPIFFS::instance(SPIFFS_SPI_Test::Storage::SPIFFS::partition_ui)->setup();
    SPIFFS_SPI_Test::Storage::SDCard::instance()->setup();
    
    ESP_LOGI("core0", "Setup complete");

    core0_setup_complete = true;
    vTaskDelete(NULL);
}

static void setup_core1(void *arg)
{
    ESP_LOGI("core1", "Setup starting...");
    
    // Startup the modules that are running on core 1
    
    ESP_LOGI("core1", "Setup complete");

    core1_setup_complete = true;
    vTaskDelete(NULL);
}


extern "C"
{
    void app_main(void);
    void app_main(void)
    {
        printf("\n\n\n\n\n\n\n\n");
       
        // Setup the individual cores
        // Setup core 0 modules
        xTaskCreatePinnedToCore(&setup_core0, "_setup0", 8*512, NULL, 1, NULL, CORE_0);
        // Wait for core setup to complete
        while(!core0_setup_complete)
        {
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        // Setup core 1 modules
        xTaskCreatePinnedToCore(&setup_core1, "_setup1", 8*512, NULL, 1, NULL, CORE_1);
        // Wait for core setup to complete
        while(!core1_setup_complete)
        {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        
        // Startup our handler process
        xTaskCreatePinnedToCore(&processing_core0, "_processing0", 40*512, NULL, 5, NULL, CORE_0);
        xTaskCreatePinnedToCore(&processing_core0_1, "_processing0_1", 40*512, NULL, 1, NULL, CORE_0);
        xTaskCreatePinnedToCore(&processing_core1, "_processing1", 6*512, NULL, 20, NULL, CORE_1);
    }
}
