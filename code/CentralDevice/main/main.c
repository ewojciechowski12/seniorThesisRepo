/*
	Central Device Code
*/
#include "esp_crc.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "espnow_example.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ESPNOW_MAXDELAY 512
#define CONFIG_ESPNOW_CHANNEL 6
#define CONFIG_ESPNOW_LMK "lmk1234567890123"
#define CONFIG_ESPNOW_PMK "pmk1234567890123"
#define ESPNOW_WIFI_MODE WIFI_MODE_APSTA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#define CONFIG_ESPNOW_ENABLE_LONG_RANGE 1

#define AP_SSID     "TipUpMonitor"
#define AP_PASSWORD ""

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[]   asm("_binary_index_html_end");

static const char *TAG = "central";
static QueueHandle_t s_example_espnow_queue = NULL;
static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* WiFi should start before using ESPNOW */
static void example_wifi_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));

    // Start WiFi first
    ESP_ERROR_CHECK(esp_wifi_start());

    // Then set AP config
    wifi_config_t ap_config = {
        .ap = {
            .ssid           = AP_SSID,
            .ssid_len       = strlen(AP_SSID),
            .password       = AP_PASSWORD,
            .max_connection = 4,
            .authmode       = WIFI_AUTH_OPEN,
            .channel        = CONFIG_ESPNOW_CHANNEL
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "WiFi APSTA started");
    ESP_LOGI(TAG, "AP SSID: %s", AP_SSID);
    ESP_LOGI(TAG, "Connect -> http://192.168.4.1");

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    // Set LR protocol on STA interface for ESP-NOW
    ESP_ERROR_CHECK(esp_wifi_set_protocol(
        ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G |
                        WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR));

    // Keep AP interface on standard protocols only
    ESP_ERROR_CHECK(esp_wifi_set_protocol(
        ESP_IF_WIFI_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G |
                        WIFI_PROTOCOL_11N));
#endif
}

// --- HTTP Server ---

esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

void start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = root_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root);
        ESP_LOGI(TAG, "Web server started");
    }
}

// --- ESP-NOW ---

static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info,
                                   const uint8_t *data, int len) {
    ESP_LOGI(TAG, "recv_cb fired!");
    example_espnow_event_t evt;
    example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
    uint8_t *mac_addr = recv_info->src_addr;
    uint8_t *des_addr = recv_info->des_addr;

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    if (IS_BROADCAST_ADDR(des_addr)) {
        ESP_LOGD(TAG, "Receive broadcast ESPNOW data");
    } else {
        ESP_LOGD(TAG, "Receive unicast ESPNOW data");
    }

    evt.id = EXAMPLE_ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv_cb->data = malloc(len);
    if (recv_cb->data == NULL) {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    if (xQueueSend(s_example_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb->data);
    }
}

int example_espnow_data_parse(uint8_t *data, uint16_t data_len, uint8_t *state,
                              uint16_t *seq) {
    example_espnow_data_t *buf = (example_espnow_data_t *)data;
    uint16_t crc, crc_cal = 0;

    if (data_len < sizeof(example_espnow_data_t)) {
        ESP_LOGE(TAG, "Receive ESPNOW data too short, len:%d", data_len);
        return -1;
    }

    *state = buf->state;
    *seq = buf->seq_num;
    crc = buf->crc;
    buf->crc = 0;
    crc_cal = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, data_len);

    ESP_LOGI(TAG, "Parsing data, len:%d, crc_received:%d, crc_calc:%d",
             data_len, crc, crc_cal);

    if (crc_cal == crc) {
        return buf->type;
    }
    return -1;
}

static void example_espnow_task(void *pvParameter) {
    example_espnow_event_t evt;
    uint8_t recv_state = 0;
    uint16_t recv_seq = 0;
    int ret;

    while (xQueueReceive(s_example_espnow_queue, &evt, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "Event received from queue, id: %d", evt.id);
        if (evt.id == EXAMPLE_ESPNOW_RECV_CB) {
            example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

            ret = example_espnow_data_parse(recv_cb->data, recv_cb->data_len,
                                            &recv_state, &recv_seq);
            if (ret != -1) {
                ESP_LOGI(TAG, "Flag Tripped! Notification from: " MACSTR,
                         MAC2STR(recv_cb->mac_addr));
            } else {
                ESP_LOGI(TAG, "Received corrupted packet, ignoring");
            }
            free(recv_cb->data);
        }
    }
}

static esp_err_t example_espnow_init(void) {
    s_example_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
    if (s_example_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create queue fail");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(example_espnow_recv_cb));

    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vQueueDelete(s_example_espnow_queue);
        s_example_espnow_queue = NULL;
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESP_IF_WIFI_AP;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);

    xTaskCreate(example_espnow_task, "example_espnow_task", 2048, NULL, 4, NULL);

    return ESP_OK;
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    example_wifi_init();
    example_espnow_init();

    uint8_t mac[6];
    esp_read_mac(mac, WIFI_IF_STA);
    ESP_LOGI("MAC", "Central Mac: " MACSTR, MAC2STR(mac));

    start_webserver();
}