/*
 * SPDX-FileCopyrightText: 2016 Intel Corporation
 *                         2020 Thesis projects
 *
 * SPDX-License-Identifier: Apache 2.0 License
 *
 * SPDX-FileContributor: 2021 Laukik Hase
 */

#pragma once

/* PARTITION LABELS */
#define DEFAULT_PARTITION_LABEL_SRC "factory"
#define DEFAULT_PARTITION_LABEL_DEST "ota_0"
#define DEFAULT_PARTITION_LABEL_PATCH "patch"

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

typedef struct {
    const char *src;
    const char *dest;
    const char *patch;
} delta_opts_t;

#define INIT_DEFAULT_DELTA_OPTS() { \
    .src = DEFAULT_PARTITION_LABEL_SRC, \
    .dest = DEFAULT_PARTITION_LABEL_DEST, \
    .patch = DEFAULT_PARTITION_LABEL_PATCH \
}

typedef struct {
    const char *name;
    const void *patch;
    int offset;
    int size;
} delta_partition_writer_t;

int delta_partition_init(delta_partition_writer_t *writer, const char *partition, int patch_size);

int delta_partition_write(delta_partition_writer_t *writer, const char *buf, int size);

/**
 * Checks if there is patch in the patch partition
 * and applies that patch if it exists. Then restarts
 * the device and boots from the new image.
 *
 * @param[in] patch_size size of the patch.
 * @param[in] opts options for applying the patch.
 *
 * @return zero(0) if no patch or a negative error
 * code.
 */
int delta_check_and_apply(int patch_size, const delta_opts_t *opts);

/**
 * Get the error string for given error code.
 *
 * @param[in] Error code.
 *
 * @return Error string.
 */
const char *delta_error_as_string(int error);
