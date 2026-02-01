#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

#define UART_PORT UART_NUM_0
#define BUF_SIZE  128

static const char *TAG = "UART_NVS";

/* Unique device identifier used by PC application
 * for reliable identification when multiple MCUs are connected
 */
static void uart_send_stored_nvs_data(void);

/* Forward declaration */
static const char *DEVICE_ID = "ESP32_UART_TARGET:UNIT_01\n";

/* ===================== UART CONFIGURATION =====================
 * UART is configured strictly as per assessment requirements:
 * - Baud rate: 2400
 * - 8 data bits, no parity, 1 stop bit
 * - Byte-by-byte transmission using low-level ESP-IDF APIs
 */
void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 2400,   
        .data_bits = UART_DATA_8_BITS,     //8bits = 1 Byte (Byte by Byte transmission)
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_PORT, &uart_config);
    uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
}

/* ===================== NVS INITIALIZATION =====================
 * Initializes non-volatile storage for persistent data storage.
 */
void nvs_init(void)  //Initialised Non-Volatile Storge to store the string
{
    nvs_flash_init();
}

/* ===================== UART RECEIVE / STORE / ECHO TASK =====================
 * - Receives UART data byte-by-byte
 * - Stores received data exactly as received into NVS
 * - Reads back stored data and retransmits byte-by-byte
 * - Supports command-based retrieval and device identification
 */
void uart_nvs_echo_task(void)
{
    uint8_t rx;
    uint8_t buffer[512];

    ESP_LOGI(TAG, "UART–NVS echo task started");

    /* ---------- BYTE-BY-BYTE UART RECEPTION ---------- */
    while (1) {
        uint32_t len = 0;

        ESP_LOGI(TAG, "Waiting for UART data...");

        while (1) {
            if (uart_read_bytes(UART_PORT, &rx, 1, portMAX_DELAY) == 1) { 
                
                /* Device identification request from PC */
				if (rx == '?') {
				    uart_write_bytes(UART_PORT, DEVICE_ID, strlen(DEVICE_ID));
				    ESP_LOGI(TAG, "Device ID sent on request");
				    goto next_iteration;
				}
				
				/* Command to retrieve previously stored NVS data */
                if (rx == '1') {
                    ESP_LOGI(TAG, "Command '1' received: sending stored NVS data");
                    uart_send_stored_nvs_data();
                    goto next_iteration;
                }

                buffer[len++] = rx;
				
				/* End of transmission detected */
                if (rx == '\n') {
                    break;
                }
            }
        }
        
        if (len == 0) {
            continue;
        }

        ESP_LOGI(TAG, "UART reception complete, %lu bytes received", len);

        /* ---------- STORE DATA TO NON-VOLATILE MEMORY ---------- */
        nvs_handle_t handle;
        nvs_open("uart", NVS_READWRITE, &handle);
        nvs_set_blob(handle, "rx_data", buffer, len);
        nvs_commit(handle);
        nvs_close(handle);

        ESP_LOGI(TAG, "Data stored to NVS");

        /* ---------- READ BACK STORED DATA ---------- */
        uint32_t read_len = sizeof(buffer);
        nvs_open("uart", NVS_READONLY, &handle);
        nvs_get_blob(handle, "rx_data", buffer, &read_len);
        nvs_close(handle);

        ESP_LOGI(TAG, "Read %lu bytes back from NVS", read_len);

        /* ---------- RETRANSMIT DATA BYTE-BY-BYTE ---------- */
        ESP_LOGI(TAG, "Retransmitting data over UART");
        for (uint32_t i = 0; i < read_len; i++) {
            uart_write_bytes(UART_PORT, (char *)&buffer[i], 1);
        }
        ESP_LOGI(TAG, "UART retransmission complete");
        
    next_iteration:
        continue;
    }
}

/* ===================== NVS DATA RETRIEVAL FUNCTION =====================
 * Reads previously stored data from NVS and transmits it over UART.
 */
static void uart_send_stored_nvs_data(void)
{
    uint8_t buffer[512];
    uint32_t len = sizeof(buffer);
    nvs_handle_t handle;

    esp_err_t err = nvs_open("uart", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No NVS namespace found");
        return;
    }
    err = nvs_get_blob(handle, "rx_data", buffer, &len);
    nvs_close(handle);

    if (err == ESP_OK && len > 0) {
        ESP_LOGI(TAG, "Sending %lu bytes from NVS", len);

        for (uint32_t i = 0; i < len; i++) {
            uart_write_bytes(UART_PORT, (char *)&buffer[i], 1);
        }
    } else {
        ESP_LOGI(TAG, "No stored data in NVS");
    }
}

/* ===================== APPLICATION ENTRY POINT ===================== */
void app_main(void)
{
    uart_init();
    nvs_init();

    xTaskCreate(
        uart_nvs_echo_task,
        "uart_nvs_echo",
        4096,
        NULL,
        5,
        NULL
    );
}