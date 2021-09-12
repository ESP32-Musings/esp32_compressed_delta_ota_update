idf_component_register(SRCS "delta.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES app_update detools freertos log spi_flash)