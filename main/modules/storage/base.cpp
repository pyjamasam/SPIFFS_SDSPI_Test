#include "modules/storage/base.h"

#include <esp_log.h>

#include <stdio.h>
#include <unistd.h>

#define LOGAREA "basestorage"

FILE* SPIFFS_SPI_Test::Storage::Base::fopen(const char* __filename, const char* __mode)
{
    char *fullFilenameBuffer = this->assembleFileName(__filename);

    FILE *fp = ::fopen(fullFilenameBuffer, __mode);

    if (!fp)
    {
        ESP_LOGE(LOGAREA, "Failed to open %s for mode %s", fullFilenameBuffer, __mode);
    }

    free(fullFilenameBuffer); fullFilenameBuffer = NULL;

    return fp;
}


void SPIFFS_SPI_Test::Storage::Base::unlink(const char* filename)
{
    char *fullFilenameBuffer = this->assembleFileName(filename);

    ::unlink(fullFilenameBuffer);

    free(fullFilenameBuffer); fullFilenameBuffer = NULL;
}


char *SPIFFS_SPI_Test::Storage::Base::assembleFileName(const char* filename)
{
    const char* mountPoint = this->mountPoint();
    const char* seperator = "";
    if (filename[0] != '/')
    {
        seperator = "/";
    }

    size_t fullFilenameBufferSize = snprintf(NULL, 0, "%s%s%s", mountPoint, seperator, filename) + 1;
    char *fullFilenameBuffer = reinterpret_cast<char*>(malloc(fullFilenameBufferSize));
    snprintf(fullFilenameBuffer, fullFilenameBufferSize, "%s%s%s", mountPoint, seperator, filename);

    return fullFilenameBuffer;
}
