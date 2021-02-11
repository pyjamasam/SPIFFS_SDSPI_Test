#ifndef MAIN_MODULES_STORAGE_SPIFFS_H_
#define MAIN_MODULES_STORAGE_SPIFFS_H_

#include <stdint.h>
#include <stdio.h>

#include <esp_spiffs.h>

#include <cstddef>

#include "modules/storage/base.h"

namespace SPIFFS_SPI_Test
{
    namespace Storage
    {
        class SPIFFS: public Base
        {
         public:
            typedef enum
            {
                partition_ui = 0,
                partition_data
            } parition_type;

            static inline SPIFFS* instance(const parition_type partition) { if (!_instance[partition]) { _instance[partition] = new SPIFFS(partition); } return _instance[partition]; }

            void setup();

            virtual const char* mountPoint() const;

            const char* partitionLabel() const { return _vfs_conf.partition_label; }

            const char* rev() const;
            uint16_t version() const;

         private:
            explicit SPIFFS(const parition_type partition);

            esp_vfs_spiffs_conf_t _vfs_conf;
            parition_type _partition;

            static SPIFFS *_instance[2];

            char *_rev;
            uint16_t _version;
        };
    }  // namespace Storage
}  // namespace SPIFFS_SPI_Test

#endif  // MAIN_MODULES_STORAGE_SPIFFS_H_
