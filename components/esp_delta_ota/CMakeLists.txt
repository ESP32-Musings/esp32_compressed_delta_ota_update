idf_component_register(SRCS "esp_delta_ota.c" "detools/c/detools.c" "detools/c/heatshrink/heatshrink_decoder.c"
                       INCLUDE_DIRS "include" "detools/c" "detools/c/heatshrink"
                       PRIV_REQUIRES freertos log)

target_compile_options(${COMPONENT_LIB} PRIVATE "-DDETOOLS_CONFIG_COMPRESSION_LZMA=0")
