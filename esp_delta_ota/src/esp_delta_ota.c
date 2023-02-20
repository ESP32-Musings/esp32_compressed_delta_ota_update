/*
 * SPDX-FileCopyrightText: 2016 Intel Corporation
 *                         2020 Thesis projects
 *
 * SPDX-License-Identifier: Apache 2.0 License
 *
 * SPDX-FileContributor: 2021 Laukik Hase
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_delta_ota.h"
#include "detools.h"

static const char *TAG = "delta";

esp_delta_ota_handle_t delta_ota_set_cfg(esp_delta_ota_cfg_t *cfg)
{
    struct detools_apply_patch_t *apply_patch = calloc(1, sizeof(struct detools_apply_patch_t));
    int ret = detools_apply_patch_init(apply_patch, cfg->read_cb, cfg->seek_cb, 0, cfg->write_cb, &cfg->src_offset);
    if (ret < 0) {
        return NULL;
    }
    esp_delta_ota_handle_t ctx = (esp_delta_ota_handle_t)apply_patch;
    return ctx;
}

int esp_delta_ota_feed_patch(esp_delta_ota_handle_t handle, const uint8_t *buf, int size)
{
    if (handle == NULL) {
        return -1;
    }
    struct detools_apply_patch_t *apply_patch = (struct detools_apply_patch_t *)handle;

    if (!apply_patch) {
        return DELTA_INVALID_ARGUMENT_ERROR;
    }
    return detools_apply_patch_process(apply_patch, (const uint8_t *)buf, size);
}

int esp_delta_ota_finish(esp_delta_ota_handle_t handle)
{
    if (handle == NULL) {
        return -1;
    }
    struct detools_apply_patch_t *apply_patch = (struct detools_apply_patch_t *)handle;
    return detools_apply_patch_finalize(apply_patch);
}

void esp_delta_ota_deinit(esp_delta_ota_handle_t handle)
{
    if (handle == NULL) {
        return;
    }
    struct detools_apply_patch_t *apply_patch = (struct detools_apply_patch_t *)handle;
    free(apply_patch);
    apply_patch = NULL;
    handle = NULL;
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
