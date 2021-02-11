#ifndef MAIN_MODULES_STORAGE_BASE_H_
#define MAIN_MODULES_STORAGE_BASE_H_

#include <stdint.h>
#include <stdio.h>

#include <cstddef>

namespace SPIFFS_SPI_Test
{
    namespace Storage
    {
        class Base
        {
         public:
            virtual const char* mountPoint() const = 0;
            char *assembleFileName(const char* filename);

            FILE* fopen(const char* __filename, const char* __mode);
            void unlink(const char* filename);
        };
    }  // namespace Storage
}  // namespace SPIFFS_SPI_Test

#endif  // MAIN_MODULES_STORAGE_BASE_H_
