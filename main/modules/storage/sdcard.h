#ifndef MAIN_MODULES_STORAGE_SDCARD_H_
#define MAIN_MODULES_STORAGE_SDCARD_H_

#include <stdio.h>

#include <sdmmc_cmd.h>
#include <driver/sdmmc_defs.h>

#include <ff.h>

#include <cstddef>

#include "modules/storage/base.h"

namespace SPIFFS_SPI_Test
{
    namespace Storage
    {
        class SDCard
        {
         public:
            static inline SDCard* instance() { if (!_instance) { _instance = new SDCard(); } return _instance; }

            void setup();

            void unlink(const char* filename);

            bool online() const { return _online; }
            bool cardpresent() const { return _cardpresent; }

            void space(uint32_t *totalsize, uint32_t *freespace);

            esp_err_t repartition();

         private:
            SDCard();

            static SDCard *_instance;

            sdmmc_card_t _card;
            FATFS _fs;

            bool _online;
            bool _cardpresent;
        };
    }  // namespace Storage
}  // namespace SPIFFS_SPI_Test

#endif  // MAIN_MODULES_STORAGE_SDCARD_H_
