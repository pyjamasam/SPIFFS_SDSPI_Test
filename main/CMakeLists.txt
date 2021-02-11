idf_component_register(
    SRCS "main.cpp"
        "modules/storage/base.cpp"
        "modules/storage/sdcard.cpp"
        "modules/storage/spiffs.cpp"
    INCLUDE_DIRS "."
)

target_compile_options(${COMPONENT_LIB} PRIVATE 
    -Wextra 
    -Wall     
    -Winit-self 
    -Wuninitialized 
    -Wmissing-declarations
)

spiffs_create_partition_image(spiffs-ui spiffs/ui FLASH_IN_PROJECT)
spiffs_create_partition_image(spiffs-data spiffs/data FLASH_IN_PROJECT)
 