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

static esp_err_t ota_post_handler(httpd_req_t *req)
{
    char *recv_buf = calloc(HTTP_CHUNK_SIZE, sizeof(char));
    if (!recv_buf) {
        return ESP_FAIL;
    }

    int ret, content_length = req->content_len;
    ESP_LOGI(TAG, "Content length: %d B", content_length);

    int remaining = content_length;
    int64_t start = esp_timer_get_time();

    delta_opts_t opts = INIT_DEFAULT_DELTA_OPTS();

    delta_partition_writer_t writer;

    size_t count = 0;
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, recv_buf, MIN(remaining, HTTP_CHUNK_SIZE))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            goto ERROR;
        }

        if (count == 0 && delta_partition_init(&writer, opts.patch, content_length) != ESP_OK) {
            goto ERROR;
        }

        if (delta_partition_write(&writer, recv_buf, ret) != ESP_OK) {
            goto ERROR;
        };

        count += ret;
        remaining -= ret;
        memset(recv_buf, 0x00, HTTP_CHUNK_SIZE);
        ESP_LOGI(TAG, "Download Progress: %0.2f %%", ((float)(content_length - remaining) / content_length) * 100);
    }

    free(recv_buf);

    ESP_LOGI(TAG, "Time taken to download patch: %0.3f s", (float)(esp_timer_get_time() - start) / 1000000L);
    ESP_LOGI(TAG, "Ready to apply patch...");
    ESP_LOGI(TAG, "Patch size: %uKB", count/1024);

    ESP_LOGI(TAG, "---------------- detools ----------------");
    int err = delta_check_and_apply(content_length, &opts);
    if (err) {
        ESP_LOGE(TAG, "Error: %s", delta_error_as_string(err));
        goto ERROR;
    }

    httpd_resp_send(req, NULL, 0);
    ESP_ERROR_CHECK(example_disconnect());
    reboot();

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
