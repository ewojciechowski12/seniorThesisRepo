/*
	TipUp Node Code
*/

#include "driver/i2c_master.h"
#include "esp_crc.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "espnow_example.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "esp_sleep.h"
#include "driver/gpio.h"

#define ESPNOW_MAXDELAY 512
#define CONFIG_ESPNOW_CHANNEL 6
#define CONFIG_ESPNOW_LMK "lmk1234567890123"
#define CONFIG_ESPNOW_PMK "pmk1234567890123"
#define CONFIG_ESPNOW_SEND_COUNT 100
#define CONFIG_ESPNOW_SEND_DELAY 1000
#define CONFIG_ESPNOW_SEND_LEN 10

#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#define CONFIG_ESPNOW_ENABLE_LONG_RANGE 1

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_FREQ_HZ 400000
#define MPU6050_ADDR 0x68
#define MPU6050_WHO_AM_I 0x75

static const char *TAG = "node";

static QueueHandle_t s_example_espnow_queue = NULL;

static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF,
															0xFF, 0xFF, 0xFF};
															
//static uint8_t central_mac[ESP_NOW_ETH_ALEN] = {0x44, 0x1d, 0x64, 0xf4, 0xac, 0xb4};
															
static uint16_t s_example_espnow_seq[EXAMPLE_ESPNOW_DATA_MAX] = {0, 0};

typedef struct {
	float ax;
	float ay;
	float az;
} accel_payload_t;

static void example_espnow_deinit(example_espnow_send_param_t *send_param);

/* WiFi should start before using ESPNOW */
static void example_wifi_init(void) {
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(
		esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
	ESP_ERROR_CHECK(esp_wifi_set_protocol(
		ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G |
							WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR));
#endif

	esp_wifi_set_max_tx_power(84); // max power, 84 = 21dBm
}

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void example_espnow_send_cb(const uint8_t *mac_addr,
                                   esp_now_send_status_t status) {
    example_espnow_event_t evt;
    example_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    evt.id = EXAMPLE_ESPNOW_SEND_CB;
    memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send_cb->status = status;
    if (xQueueSend(s_example_espnow_queue, &evt, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Send send queue fail");
    }

    ESP_LOGI(TAG, "Alert sent to central, status: %s",
             status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAILED");
}

static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info,
								   const uint8_t *data, int len) {
	example_espnow_event_t evt;
	example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
	uint8_t *mac_addr = recv_info->src_addr;
	uint8_t *des_addr = recv_info->des_addr;

	if (mac_addr == NULL || data == NULL || len <= 0) {
		ESP_LOGE(TAG, "Receive cb arg error");
		return;
	}

	if (IS_BROADCAST_ADDR(des_addr)) {
		/* If added a peer with encryption before, the receive packets may be
		 * encrypted as peer-to-peer message or unencrypted over the broadcast
		 * channel. Users can check the destination address to distinguish it.
		 */
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

/* Parse received ESPNOW data. */
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

	if (crc_cal == crc) {
		return buf->type;
	}

	return -1;
}

/* Prepare ESPNOW data to be sent. */
void example_espnow_data_prepare(example_espnow_send_param_t *send_param) {
	example_espnow_data_t *buf = (example_espnow_data_t *)send_param->buffer;

	assert(send_param->len >= sizeof(example_espnow_data_t));

	buf->type = IS_BROADCAST_ADDR(send_param->dest_mac) ?
	EXAMPLE_ESPNOW_DATA_BROADCAST : EXAMPLE_ESPNOW_DATA_UNICAST;	
	//buf->state = send_param->state;
	buf->seq_num = s_example_espnow_seq[buf->type]++;
	buf->crc = 0;
	buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
}

static void example_espnow_task(void *pvParameter) {
	example_espnow_event_t evt;
	uint8_t recv_state = 0;
	uint16_t recv_seq = 0;
	bool is_broadcast = false;
	int ret;

	vTaskDelay(1000 / portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "Start sending broadcast data");

	/* Start sending broadcast ESPNOW data. */
	example_espnow_send_param_t *send_param =
		(example_espnow_send_param_t *)pvParameter;
	if (esp_now_send(send_param->dest_mac, send_param->buffer,
					 send_param->len) != ESP_OK) {
		ESP_LOGE(TAG, "Send error");
		example_espnow_deinit(send_param);
		vTaskDelete(NULL);
	}

	while (xQueueReceive(s_example_espnow_queue, &evt, portMAX_DELAY) ==
		   pdTRUE) {
		switch (evt.id) {
		case EXAMPLE_ESPNOW_SEND_CB: {
			example_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;
			is_broadcast = IS_BROADCAST_ADDR(send_cb->mac_addr);

			ESP_LOGD(TAG, "Send data to " MACSTR ", status1: %d",
					 MAC2STR(send_cb->mac_addr), send_cb->status);

			if (is_broadcast && (send_param->broadcast == false)) {
				break;
			}

			if (!is_broadcast) {
				send_param->count--;
				if (send_param->count == 0) {
					ESP_LOGI(TAG, "Send done");
					example_espnow_deinit(send_param);
					vTaskDelete(NULL);
				}
			}

			/* Delay a while before sending the next data. */
			if (send_param->delay > 0) {
				vTaskDelay(send_param->delay / portTICK_PERIOD_MS);
			}

			ESP_LOGI(TAG, "send data to " MACSTR "",
					 MAC2STR(send_cb->mac_addr));

			memcpy(send_param->dest_mac, send_cb->mac_addr, ESP_NOW_ETH_ALEN);
			example_espnow_data_prepare(send_param);

			/* Send the next data after the previous data is sent. */
			if (esp_now_send(send_param->dest_mac, send_param->buffer,
							 send_param->len) != ESP_OK) {
				ESP_LOGE(TAG, "Send error");
				example_espnow_deinit(send_param);
				vTaskDelete(NULL);
			}
			break;
		}
		case EXAMPLE_ESPNOW_RECV_CB: {
			example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

			ret = example_espnow_data_parse(recv_cb->data, recv_cb->data_len,
											&recv_state, &recv_seq);

			free(recv_cb->data);
			if (ret == EXAMPLE_ESPNOW_DATA_BROADCAST) {
				ESP_LOGI(
					TAG,
					"Receive %dth broadcast data from: " MACSTR ", len: %d",
					recv_seq, MAC2STR(recv_cb->mac_addr), recv_cb->data_len);

				/* If MAC address does not exist in peer list, add it to peer
				 * list. */
				if (esp_now_is_peer_exist(recv_cb->mac_addr) == false) {
					esp_now_peer_info_t *peer =
						malloc(sizeof(esp_now_peer_info_t));
					if (peer == NULL) {
						ESP_LOGE(TAG, "Malloc peer information fail");
						example_espnow_deinit(send_param);
						vTaskDelete(NULL);
					}
					memset(peer, 0, sizeof(esp_now_peer_info_t));
					peer->channel = CONFIG_ESPNOW_CHANNEL;
					peer->ifidx = ESPNOW_WIFI_IF;
					peer->encrypt = true;
					memcpy(peer->lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
					memcpy(peer->peer_addr, recv_cb->mac_addr,
						   ESP_NOW_ETH_ALEN);
					ESP_ERROR_CHECK(esp_now_add_peer(peer));
					free(peer);
				}

				/* Indicates that the device has received broadcast ESPNOW data.
				 */
				if (send_param->state == 0) {
					send_param->state = 1;
				}
			}else if (ret == EXAMPLE_ESPNOW_DATA_UNICAST) {
				ESP_LOGI(
					TAG, "Receive %dth unicast data from: " MACSTR ", len: %d",
					recv_seq, MAC2STR(recv_cb->mac_addr), recv_cb->data_len);

				/* If receive unicast ESPNOW data, also stop sending broadcast
				 * ESPNOW data. */
				send_param->broadcast = false;
			} else {
				ESP_LOGI(TAG, "Receive error data from: " MACSTR "",
						 MAC2STR(recv_cb->mac_addr));
			}
			break;
		}
		default:
			ESP_LOGE(TAG, "Callback type error: %d", evt.id);
			break;
		}
	}
}

static esp_err_t example_espnow_init(void) {
	example_espnow_send_param_t *send_param;

	s_example_espnow_queue =
		xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
	if (s_example_espnow_queue == NULL) {
		ESP_LOGE(TAG, "Create queue fail");
		return ESP_FAIL;
	}

	/* Initialize ESPNOW and register sending and receiving callback function.
	 */
	ESP_ERROR_CHECK(esp_now_init());
	ESP_ERROR_CHECK(esp_now_register_send_cb(example_espnow_send_cb));
	ESP_ERROR_CHECK(esp_now_register_recv_cb(example_espnow_recv_cb));
#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
	ESP_ERROR_CHECK(esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW));
	ESP_ERROR_CHECK(esp_wifi_connectionless_module_set_wake_interval(
		CONFIG_ESPNOW_WAKE_INTERVAL));
#endif
	/* Set primary master key. */
	//ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK));

	/* Add broadcast peer information to peer list. */
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
	peer->ifidx = ESPNOW_WIFI_IF;
	peer->encrypt = false;
	memcpy(peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
	ESP_ERROR_CHECK(esp_now_add_peer(peer));
	free(peer);

	/* Initialize sending parameters. */
	send_param = malloc(sizeof(example_espnow_send_param_t));
	if (send_param == NULL) {
		ESP_LOGE(TAG, "Malloc send parameter fail");
		vQueueDelete(s_example_espnow_queue);
		s_example_espnow_queue = NULL;
		esp_now_deinit();
		return ESP_FAIL;
	}
	memset(send_param, 0, sizeof(example_espnow_send_param_t));
	send_param->unicast = false;
	send_param->broadcast = true;
	send_param->state = 0;
	send_param->count = 1;
	send_param->delay = CONFIG_ESPNOW_SEND_DELAY;
	send_param->len = CONFIG_ESPNOW_SEND_LEN;

	const char *msg = "Flag Tripped!";
	int payload_len = strlen(msg) + 1; // include null terminator
	int total_len = sizeof(example_espnow_data_t) + payload_len;

	example_espnow_data_t *data = malloc(total_len);
	assert(data);

	data->type = EXAMPLE_ESPNOW_DATA_UNICAST;
	data->state = 0;
	data->seq_num = 1;

	// copy payload safely
	memcpy(data->payload, msg, payload_len);

	// set send parameters
	send_param->buffer = (uint8_t *)data;
	send_param->len = total_len;

	if (send_param->buffer == NULL) {
		ESP_LOGE(TAG, "Malloc send buffer fail");
		free(send_param);
		vQueueDelete(s_example_espnow_queue);
		s_example_espnow_queue = NULL;
		esp_now_deinit();
		return ESP_FAIL;
	}
	//memcpy(send_param->dest_mac, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
	
	memcpy(send_param->dest_mac, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
	example_espnow_data_prepare(send_param);

	xTaskCreate(example_espnow_task, "example_espnow_task", 2048, send_param, 4,
				NULL);

	return ESP_OK;
}

static void example_espnow_deinit(example_espnow_send_param_t *send_param) {
	free(send_param->buffer);
	free(send_param);
	vQueueDelete(s_example_espnow_queue);
	s_example_espnow_queue = NULL;
	esp_now_deinit();
}

static void configureMPU_6050(){
	
	ESP_LOGI(TAG, "Configuring MPU-6050...");
	
	// Configure the I2C bus
	i2c_master_bus_config_t bus_config = {
		.clk_source = I2C_CLK_SRC_DEFAULT,
		.i2c_port = I2C_NUM_0,
		.scl_io_num = I2C_MASTER_SCL_IO,
		.sda_io_num = I2C_MASTER_SDA_IO,
		.glitch_ignore_cnt = 7,
		.flags.enable_internal_pullup = true,
	};
	i2c_master_bus_handle_t bus_handle;
	ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

	// Configure the MPU-6050 as a device on the bus
	i2c_device_config_t dev_config = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = MPU6050_ADDR,
		.scl_speed_hz = 400000,
	};
	i2c_master_dev_handle_t dev_handle;
	ESP_ERROR_CHECK(
		i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));

	// Wake up the MPU-6050 first
	uint8_t wake_cmd[2] = {0x6B, 0x00};
	ESP_ERROR_CHECK(
		i2c_master_transmit(dev_handle, wake_cmd, 2, pdMS_TO_TICKS(1000)));

	// Small delay to let it wake up
	vTaskDelay(pdMS_TO_TICKS(10));

	// set threshold - 0x1F
	uint8_t mot_thr_cmd[2] = {0x1F, 0x02}; // 1F = motion threshold register, 32 = threshold (100mg force)
	ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, mot_thr_cmd, 2, pdMS_TO_TICKS(1000)));
	
	// set duration - 0x20
	uint8_t set_dur_cmd[2] = {0x20, 0x14};// 0x14 = 20 , as in 20 milliseconds
	ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, set_dur_cmd, 2, pdMS_TO_TICKS(1000)));
	
	// configure INT pin behavior - INT_PIN_CFG = 0x37, INT_ENABLE = 0x38
	uint8_t pin_cfg_cmd[2] = {0x37, 0x20};
	ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, pin_cfg_cmd, 2, pdMS_TO_TICKS(1000)));
	
	// enable motion interrupt
	uint8_t ena_mot_int[2] = {0x38, 0x40};
	ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, ena_mot_int, 2, pdMS_TO_TICKS(1000)));	
	
	ESP_LOGI(TAG, "MPU-6050 configuration complete");

}

static void clear_mpu6050_interrupt(void) {
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = 400000,
    };
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));

    // Reading INT_STATUS clears the interrupt latch
    uint8_t reg = 0x3A;
    uint8_t val = 0;
    i2c_master_transmit_receive(dev_handle, &reg, 1, &val, 1, pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Cleared INT_STATUS, was: 0x%02X", val);
}
void app_main(void) {

	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
		ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);	
	
	esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
	ESP_LOGI(TAG, "Wakeup cause: %d", cause);
	
	if(cause == ESP_SLEEP_WAKEUP_EXT0){
	    ESP_LOGI(TAG, "Woke from interrupt, initializing WiFi...");
	    example_wifi_init();
	    ESP_LOGI(TAG, "WiFi init done, initializing ESP-NOW...");

	    esp_err_t err = example_espnow_init();
	    if(err != ESP_OK){
	        ESP_LOGE(TAG, "ESP-NOW init failed: %s", esp_err_to_name(err));
	    } else {
	        ESP_LOGI(TAG, "ESP-NOW init done, waiting for send...");
	        vTaskDelay(pdMS_TO_TICKS(5000));
	        clear_mpu6050_interrupt();
	        ESP_LOGI(TAG, "Alert sent, going back to sleep.");
	    }
	} else {
		// First boot - configure MPU-6050
		configureMPU_6050();
	}
	
	ESP_LOGI(TAG, "GOING TO SLEEP");
	esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1);
	esp_deep_sleep_start();

}


