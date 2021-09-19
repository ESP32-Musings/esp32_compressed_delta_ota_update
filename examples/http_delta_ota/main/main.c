#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "esp_partition.h"

#include "esp_http_server.h"
#include "delta.h"

#define HTTP_CHUNK_SIZE (2048)

static const char *TAG = "http_delta_ota";

static void reboot(void)
{
    ESP_LOGI(TAG, "Rebooting in 5 seconds...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    esp_restart();
}

static esp_err_t patch_partition_write(char *recv_buf, size_t size, size_t patch_size)
{
    static size_t offset = 0;
    static const esp_partition_t *patch = NULL;

    if (offset == 0) {
        patch = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, PARTITION_LABEL_PATCH);
        if (patch == NULL){
            ESP_LOGE(TAG, "Partition Error: Could not find 'patch' partition");
            return ESP_FAIL;
        }

        size_t patch_page_size = (patch_size + PARTITION_PAGE_SIZE) - (patch_size % PARTITION_PAGE_SIZE);
        if (esp_partition_erase_range(patch, offset, patch_page_size) != ESP_OK) {
            ESP_LOGE(TAG, "Partition Error: Could not erase 'patch' region!");
            return ESP_FAIL;
        }
    }

    if (esp_partition_write(patch, offset, recv_buf, size) != ESP_OK) {
        ESP_LOGE(TAG, "Partition Error: Could not write to 'patch' region!");
        return ESP_FAIL;
    };

    offset += size;
    return ESP_OK;
}

static esp_err_t ota_post_handler(httpd_req_t *req)
{
    char *recv_buf = calloc(HTTP_CHUNK_SIZE, sizeof(char));
    int ret, content_length = req->content_len;
    ESP_LOGI(TAG, "Content length: %d B", content_length);

    int remaining = content_length;
    int64_t start = esp_timer_get_time();

    while (remaining > 0){
        if ((ret = httpd_req_recv(req, recv_buf, MIN(remaining, HTTP_CHUNK_SIZE))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            goto ERROR;
        }

        if (patch_partition_write(recv_buf, ret, content_length) != ESP_OK) {
            goto ERROR;
        };

        remaining -= ret;
        memset(recv_buf, 0x00, HTTP_CHUNK_SIZE);
        ESP_LOGI(TAG, "Download Progress: %0.2f %%", ((float)(content_length - remaining) / content_length) * 100);
    }

    free(recv_buf);

    ESP_LOGI(TAG, "Time taken to download patch: %0.3f s", (float)(esp_timer_get_time() - start) / 1000000L);
    ESP_LOGI(TAG, "Ready to apply patch...");

    ESP_LOGI(TAG, "---------------- detools ----------------");
    int err = delta_check_and_apply(content_length);
    if (err) {
        ESP_LOGE(TAG, "Error: %s", delta_error_as_string(err));
        goto ERROR;
    }

    httpd_resp_send(req, NULL, 0);
    reboot();
    return ESP_OK;

ERROR:
    httpd_resp_send_500(req);
    return ESP_OK;
}

static const httpd_uri_t ota = {
    .uri = "/ota",
    .method = HTTP_POST,
    .handler = ota_post_handler,
    .user_ctx = NULL
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &ota);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initialising WiFi Connection...");

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());


    /* Start the server for the first time */
    ESP_LOGI(TAG, "Setting up HTTP server...");
    start_webserver();
}
