/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: 0
 *
 *
 * Copyright (c) 2020 Thesis projects
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Modified by: Laukik Hase
 *
 * Added Compatibility with ESP-IDF
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "esp_partition.h"
#include "esp_ota_ops.h"

#include "detools.h"
#include "delta.h"

static const char *TAG = "delta";

typedef struct flash_mem {
    const esp_partition_t *src;
    const esp_partition_t *dest;
    const esp_partition_t *patch;
    size_t src_offset;
    size_t patch_offset;
    esp_ota_handle_t ota_handle;
} flash_mem_t;

static int delta_flash_write_dest(void *arg_p, const uint8_t *buf_p, size_t size)
{
    flash_mem_t *flash;
    flash = (flash_mem_t *)arg_p;

    if (!flash) {
        return -DELTA_CASTING_ERROR;
    }
    if (size <= 0) {
        return -DELTA_INVALID_BUF_SIZE;
    }

    if (esp_ota_write(flash->ota_handle, buf_p, size) != ESP_OK) {
        return -DELTA_WRITING_ERROR;
    }

    return DELTA_OK;
}

static int delta_flash_read_src(void *arg_p, uint8_t *buf_p, size_t size)
{
    flash_mem_t *flash;
    flash = (flash_mem_t *)arg_p;

    if (!flash) {
        return -DELTA_CASTING_ERROR;
    }
    if (size <= 0) {
        return -DELTA_INVALID_BUF_SIZE;
    }

    if (esp_partition_read(flash->src, flash->src_offset, buf_p, size) != ESP_OK) {
        return -DELTA_READING_SOURCE_ERROR;
    }

    flash->src_offset += size;
    if (flash->src_offset >= flash->src->size) {
        return -DELTA_OUT_OF_MEMORY;
    }

    return DELTA_OK;
}

static int delta_flash_read_patch(void *arg_p, uint8_t *buf_p, size_t size)
{
    flash_mem_t *flash;
    flash = (flash_mem_t *)arg_p;

    if (!flash) {
        return -DELTA_CASTING_ERROR;
    }
    if (size <= 0) {
        return -DELTA_INVALID_BUF_SIZE;
    }

    if (esp_partition_read(flash->patch, flash->patch_offset, buf_p, size) != ESP_OK) {
        return -DELTA_READING_PATCH_ERROR;
    }

    flash->patch_offset += size;
    if (flash->patch_offset >= flash->patch->size) {
        return -DELTA_READING_PATCH_ERROR;
    }

    return DELTA_OK;
}

static int delta_flash_seek_src(void *arg_p, int offset)
{
    flash_mem_t *flash;
    flash = (flash_mem_t *)arg_p;

    if (!flash) {
        return -DELTA_CASTING_ERROR;
    }

    flash->src_offset += offset;
    if (flash->src_offset >= flash->src->size) {
        return -DELTA_SEEKING_ERROR;
    }

    return DELTA_OK;
}

static int delta_init_flash_mem(flash_mem_t *flash)
{
    if (!flash) {
        return -DELTA_PARTITION_ERROR;
    }

    flash->src = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, PARTITION_LABEL_SRC);
    flash->dest = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, PARTITION_LABEL_DEST);
    flash->patch = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, PARTITION_LABEL_PATCH);

    if (flash->src == NULL || flash->dest == NULL || flash->patch == NULL) {
        return -DELTA_PARTITION_ERROR;
    }

    if (esp_ota_begin(flash->dest, OTA_SIZE_UNKNOWN, &(flash->ota_handle)) != ESP_OK) {
        return -DELTA_PARTITION_ERROR;
    }
    esp_log_level_set("esp_image", ESP_LOG_ERROR);

    flash->src_offset = 0;
    flash->patch_offset = 0;

    return DELTA_OK;
}

static int delta_set_boot_partition(flash_mem_t *flash)
{
    if (esp_ota_set_boot_partition(flash->dest) != ESP_OK) {
        return -DELTA_TARGET_IMAGE_ERROR;
    }
    free(flash);

    const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
    ESP_LOGI(TAG, "Next Boot Partition: Subtype %d at Offset 0x%x", boot_partition->subtype, boot_partition->address);
    ESP_LOGI(TAG, "Ready to reboot!!!");

    return DELTA_OK;
}

int delta_check_and_apply(int patch_size)
{
    ESP_LOGI(TAG, "Initializing delta update...");

    flash_mem_t *flash = calloc(1, sizeof(flash_mem_t));
    int ret = 0;

    if (patch_size < 0) {
        return patch_size;
    } else if (patch_size > 0) {
        ret = delta_init_flash_mem(flash);
        if (ret) {
            return ret;
        }

        ret = detools_apply_patch_callbacks(delta_flash_read_src,
                                            delta_flash_seek_src,
                                            delta_flash_read_patch,
                                            (size_t) patch_size,
                                            delta_flash_write_dest,
                                            flash);

        if (ret <= 0) {
            return ret;
        }

        ESP_LOGI(TAG, "Patch Successful!!!");
        return delta_set_boot_partition(flash);
    }

    return 0;
}

const char *delta_error_as_string(int error)
{
    if (error < 28) {
        return detools_error_as_string(error);
    }

    if (error < 0) {
        error *= -1;
    }

    switch (error) {
    case DELTA_OUT_OF_MEMORY:
        return "Target partition out of memory.";
    case DELTA_READING_PATCH_ERROR:
        return "Error reading patch binary.";
    case DELTA_READING_SOURCE_ERROR:
        return "Error reading source image.";
    case DELTA_WRITING_ERROR:
        return "Error writing to target image.";
    case DELTA_SEEKING_ERROR:
        return "Seek error: source image.";
    case DELTA_CASTING_ERROR:
        return "Error casting to flash_mem_t.";
    case DELTA_INVALID_BUF_SIZE:
        return "Read/write buffer less or equal to 0.";
    case DELTA_CLEARING_ERROR:
        return "Could not erase target region.";
    case DELTA_PARTITION_ERROR:
        return "Flash partition not found.";
    case DELTA_TARGET_IMAGE_ERROR:
        return "Invalid target image to boot from.";
    default:
        return "Unknown error.";
    }
}