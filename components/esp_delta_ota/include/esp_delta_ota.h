/*
 * SPDX-FileCopyrightText: 2016 Intel Corporation
 *                         2020 Thesis projects
 *
 * SPDX-License-Identifier: Apache 2.0 License
 *
 * SPDX-FileContributor: 2021 Laukik Hase
 */

#pragma once

#include "detools_config.h"
#include "detools.h"

/* PARTITION LABELS */
#define DEFAULT_PARTITION_LABEL_SRC "factory"
#define DEFAULT_PARTITION_LABEL_DEST "ota_0"

/* PAGE SIZE */
#define PARTITION_PAGE_SIZE (0x1000)

/* Error codes. */
#define DELTA_OK                                          0
#define DELTA_OUT_OF_MEMORY                              28
#define DELTA_READING_PATCH_ERROR                        29
#define DELTA_READING_SOURCE_ERROR                       30
#define DELTA_WRITING_ERROR			                     31
#define DELTA_SEEKING_ERROR                              32
#define DELTA_CASTING_ERROR                              33
#define DELTA_INVALID_BUF_SIZE                           34
#define DELTA_CLEARING_ERROR                             35
#define DELTA_PARTITION_ERROR                            36
#define DELTA_TARGET_IMAGE_ERROR                         37
#define DELTA_INVALID_ARGUMENT_ERROR                     38
#define DELTA_OUT_OF_BOUNDS_ERROR                        39

typedef void *esp_delta_ota_handle_t;

typedef int (*read_cb_t)(void *arg_p, uint8_t *buf_p, size_t size);
typedef int (*seek_cb_t)(void *arg_p, int offset);
typedef int (*write_cb_t)(void *arg_p, const uint8_t *buf_p, size_t size);

typedef struct {
    const char *src;
    const char *dest;
} delta_opts_t;

typedef struct esp_delta_ota_cfg {
    read_cb_t read_cb;
    seek_cb_t seek_cb;
    write_cb_t write_cb;
    size_t src_offset;
} esp_delta_ota_cfg_t;

#define INIT_DEFAULT_DELTA_OPTS() { \
    .src = DEFAULT_PARTITION_LABEL_SRC, \
    .dest = DEFAULT_PARTITION_LABEL_DEST, \
}

/**
 * Get the error string for given error code.
 *
 * @param[in] Error code.
 *
 * @return Error string.
 */
const char *delta_error_as_string(int error);

// int esp_delta_ota_init(flash_mem_t *flash, delta_opts_t *opts);

esp_delta_ota_handle_t delta_ota_set_cfg(esp_delta_ota_cfg_t *cfg);

int esp_delta_ota_feed_patch(esp_delta_ota_handle_t handle, const uint8_t *buf, int size);

int esp_delta_ota_finish(esp_delta_ota_handle_t handle);

void esp_delta_ota_deinit(esp_delta_ota_handle_t handle);

