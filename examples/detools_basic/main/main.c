#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "esp_partition.h"
#include "esp_ota_ops.h"

#include "detools.h"

static const char *TAG = "detools_basic";

typedef struct flash_mem {
    const esp_partition_t *src;
    const esp_partition_t *dest;
    size_t src_offset;
    size_t dest_offset;
    size_t patch_offset;
    esp_ota_handle_t ota_handle;
    int step_size;
} flash_mem_t;

static int init_flash_mem(flash_mem_t *flash)
{
    ESP_LOGW(TAG, "In %s", __func__);
    if (!flash) {
        return -1;
    }

    // flash->src = esp_ota_get_running_partition();
    // flash->dest = esp_ota_get_next_update_partition(NULL);

    flash->src = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, "ota_0");
    flash->dest = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, "ota_1");

    if (flash->src == NULL || flash->dest == NULL) {
        return -1;
    }

    if (flash->src->subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_MAX ||
        flash->dest->subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_MAX) {
        return -1;
    }

    if (esp_ota_begin(flash->dest, OTA_SIZE_UNKNOWN, &(flash->ota_handle)) != ESP_OK) {
        return -1;
    }
    esp_log_level_set("esp_image", ESP_LOG_ERROR);

    flash->src_offset = 0;
    flash->dest_offset = 0;
    flash->patch_offset = 0;

    return 0;
}

static int set_boot_partition(flash_mem_t *flash)
{
    if (esp_ota_set_boot_partition(flash->dest) != ESP_OK) {
        return -1;
    }
    free(flash);

    const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
    ESP_LOGI(TAG, "Next Boot Partition: Subtype %d at Offset 0x%" PRIx32, boot_partition->subtype, boot_partition->address);
    ESP_LOGI(TAG, "Ready to reboot!!!");

    return (0);
}

static int flash_read(void *arg_p, void *dst_p, uintptr_t src, size_t size)
{
    ESP_LOGW(TAG, "In %s", __func__);

    flash_mem_t *flash;
    flash = (flash_mem_t *)arg_p;

    if (!flash) {
        return -1;
    }

    if (esp_partition_read(flash->src, flash->src_offset, dst_p, size) != ESP_OK) {
        return -1;
    }

    flash->src_offset += size;
    if (flash->src_offset >= flash->src->size) {
        return -1;
    }

    return (0);
}

static int flash_write(void *arg_p, uintptr_t dst, void *src_p, size_t size)
{
    ESP_LOGW(TAG, "In %s", __func__);

    flash_mem_t *flash;
    flash = (flash_mem_t *)arg_p;

    if (!flash) {
        return -1;
    }

    if (esp_ota_write(flash->ota_handle, src_p, size) != ESP_OK) {
        return -1;
    }

    return (0);
}

static int flash_erase(void *arg_p, uintptr_t addr, size_t size)
{
    ESP_LOGW(TAG, "In %s", __func__);

    return (0);
}

static int step_set(void *arg_p, int step)
{
    ESP_LOGW(TAG, "In %s", __func__);

    flash_mem_t *flash;
    flash = (flash_mem_t *)arg_p;

    if (!flash) {
        return -1;
    }

    flash->step_size = step;
    return (0);
}

static int step_get(void *arg_p, int *step_p)
{
    ESP_LOGW(TAG, "In %s", __func__);

    flash_mem_t *flash;
    flash = (flash_mem_t *)arg_p;

    if (!flash) {
        return -1;
    }

    *step_p = flash->step_size;
    return (0);
}

int init_in_place_process(int patch_size, struct detools_apply_patch_in_place_t *apply_patch, flash_mem_t *flash)
{
    ESP_LOGI(TAG, "Initializing delta update...");
    if (!flash || !apply_patch) {
        return -1;
    }

    int ret = 0;

    if (patch_size < 0) {
        return patch_size;
    } else if (patch_size > 0) {
        ret = init_flash_mem(flash);
        if (ret) {
            return ret;
        }

        ret = detools_apply_patch_in_place_init(apply_patch,
                                                flash_read,
                                                flash_write,
                                                flash_erase,
                                                step_set,
                                                step_get,
                                                (size_t) patch_size,
                                                flash);

        if (ret < 0) {
            return ret;
        }
    }
    ESP_LOGI(TAG, "Done with initializing delta update");
    return 0;
}

static int delta_ota_update(size_t patch_size)
{
    struct detools_apply_patch_in_place_t apply_patch = {0};
    flash_mem_t flash = {0};
    uint8_t buf[256];
    size_t left = 0, patch_offset = 0;
    int res;

    const esp_partition_t *patch = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "patch");
    if (patch == NULL) {
        return -1;
    }

    /* Initialize the in-place apply patch object. */
    res = init_in_place_process(patch_size, &apply_patch, &flash);
    if (res != 0) {
        return (res);
    }

    left = patch_size;

    /* Incrementally process patch data until the whole patch has been
       applied or an error occurrs. */
    while ((left > 0) && (res == 0)) {
        if (esp_partition_read(patch, patch_offset, buf, sizeof(buf)) != ESP_OK) {
            return -1;
        }
        ESP_LOGI(TAG, "----------------");
        ESP_LOGI(TAG, "res: %d", res);
        ESP_LOG_BUFFER_HEXDUMP(TAG, buf, sizeof(buf), ESP_LOG_INFO);
        patch_offset += sizeof(buf);
        if (patch_offset >= patch_size) {
            goto final;
        }

        if (res > 0) {
            left -= res;
            res = detools_apply_patch_in_place_process(&apply_patch,
                                                       buf,
                                                       res);
            // res = detools_apply_patch_in_place_callbacks(detools_mem_read_t mem_read,
            //                                              detools_mem_write_t mem_write,
            //                                              detools_mem_erase_t mem_erase,
            //                                              detools_step_set_t step_set,
            //                                              detools_step_get_t step_get,
            //                                              detools_read_t patch_read,
            //                                              size_t patch_size,
            //                                              void *arg_p);
        }
    }
    // ESP_LOGI(TAG, "Rebooting in 5 seconds...");
    // vTaskDelay(5000 / portTICK_PERIOD_MS);
    // esp_restart();
    /* Finalize patching and verify written data. */
final:
    if (res == 0) {
        res = detools_apply_patch_in_place_finalize(&apply_patch);
    }


    ESP_LOGI(TAG, "Patch Successful!!!");
    res = set_boot_partition(&flash);

    return (res);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Delta OTA in place demo");
    size_t patch_size = 1045;

    ESP_LOGW(TAG, "Delta OTA: %d", delta_ota_update(patch_size));

    // ESP_LOGI(TAG, "Rebooting in 5 seconds...");
    // vTaskDelay(5000 / portTICK_PERIOD_MS);
    // esp_restart();
}
