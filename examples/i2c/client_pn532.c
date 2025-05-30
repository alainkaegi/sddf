/*
 * Copyright 2024, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <libco.h>
#include <sddf/util/printf.h>
#include <sddf/serial/queue.h>
#include <sddf/serial/config.h>
#include <sddf/timer/client.h>
#include <sddf/timer/config.h>
#include <sddf/i2c/queue.h>
#include <sddf/i2c/client.h>
#include <sddf/i2c/config.h>
#include <sddf/i2c/devices/pn532/pn532.h>

bool delay_ms(size_t milliseconds);

// #define DEBUG_CLIENT

#ifdef DEBUG_CLIENT
#define LOG_CLIENT(...) do{ sddf_printf("PN532|INFO: "); sddf_printf(__VA_ARGS__); }while(0)
#else
#define LOG_CLIENT(...) do{}while(0)
#endif
#define LOG_CLIENT_ERR(...) do{ sddf_printf("PN532|ERROR: "); sddf_printf(__VA_ARGS__); }while(0)

#define PN_532_ON

#ifdef PN_532_ON
#define USING_HALT(...) do{}while(0)
#else
#define USING_HALT(...) do{ while(1); }while(0)
#endif

__attribute__((__section__(".i2c_client_config"))) i2c_client_config_t i2c_config;
__attribute__((__section__(".timer_client_config"))) timer_client_config_t timer_config;
__attribute__((__section__(".serial_client_config"))) serial_client_config_t serial_config;

static serial_queue_handle_t serial_tx_queue_handle;

i2c_queue_handle_t queue;
uintptr_t data_region;

cothread_t t_event;
cothread_t t_main;

#define DEFAULT_READ_RESPONSE_RETRIES (256)
#define DEFAULT_READ_ACK_FRAME_RETRIES (20)

#define STACK_SIZE (4096)
static char t_client_main_stack[STACK_SIZE];

bool read_passive_target_id(uint8_t card_baud_rate, uint8_t *uid_buf, uint8_t *uid_buf_length, uint8_t timeout)
{
    uint8_t cmd_header[3] = { PN532_CMD_INLISTPASSIVETARGET, 1, card_baud_rate };

    uint8_t write_fail = pn532_write_command(cmd_header, 3, NULL, 0, DEFAULT_READ_ACK_FRAME_RETRIES);
    if (write_fail) {
        LOG_CLIENT_ERR("failed to write PN532_CMD_INLISTPASSIVETARGET!\n");
        return false;
    };

    uint8_t response[64];
    uint8_t read_fail = pn532_read_response(response, 64, DEFAULT_READ_RESPONSE_RETRIES);
    if (read_fail) {
        // LOG_CLIENT_ERR("failed to read response for PN532_CMD_INLISTPASSIVETARGET!\n");
        // this is what fails when card times out
        return false;
    };

    if (response[0] != 1) {
        LOG_CLIENT_ERR("tags found has wrong response value (0x%x)!\n", response[0]);
        return false;
    }

#ifdef DEBUG_CLIENT
    uint16_t sens_res = response[2];
    sens_res <<= 8;
    sens_res |= response[3];
#endif

    LOG_CLIENT("ATQA: 0x%lx\n", sens_res);
    LOG_CLIENT("SAK: 0x%lx\n", response[4]);

    /* Card appears to be Mifare Classic */
    *uid_buf_length = response[5];

    for (uint8_t i = 0; i < response[5]; i++) {
        uid_buf[i] = response[6 + i];
    }

    return true;
}

uint8_t big_buf[64];

void client_main(void)
{
    USING_HALT();

    LOG_CLIENT("client_main: started\n");
    uint8_t header[1];
    header[0] = PN532_CMD_GETFIRMWAREVERSION;
    uint8_t write_fail = pn532_write_command(header, 1, NULL, 0, DEFAULT_READ_ACK_FRAME_RETRIES);
    if (write_fail) {
        LOG_CLIENT_ERR("failed to write PN532_CMD_GETFIRMWAREVERSION\n");
        assert(false);
    }

    uint8_t response_buffer[6];
    uint8_t read_fail = pn532_read_response(response_buffer, 6, DEFAULT_READ_RESPONSE_RETRIES);
    if (read_fail) {
        LOG_CLIENT_ERR("failed to read response for PN532_CMD_GETFIRMWAREVERSION\n");
        assert(false);
    }

    LOG_CLIENT("read response!\n");
    for (int i = 0; i < 6; i++) {
        LOG_CLIENT("firmware_version[%d]: 0x%lx\n", i, response_buffer[i]);
    }

    LOG_CLIENT("set passive activation retries\n");
    uint8_t passive_header[5];
    passive_header[0] = PN532_CMD_RFCONFIGURATION;
    passive_header[1] = 5; // Config item 5 (MaxRetries)
    passive_header[2] = 0xFF; // MxRtyATR (default = 0xFF)
    passive_header[3] = 0x1; // MxRtyPSL (default = 0x01)
    passive_header[4] = 0xff; // max retries according to the Arduino code
    write_fail = pn532_write_command(passive_header, 5, NULL, 0, DEFAULT_READ_ACK_FRAME_RETRIES);
    if (write_fail) {
        LOG_CLIENT_ERR("failed to write PN532_CMD_RFCONFIGURATION\n");
        assert(false);
    }
    read_fail = pn532_read_response(big_buf, 64, DEFAULT_READ_RESPONSE_RETRIES);
    if (read_fail) {
        LOG_CLIENT_ERR("failed to read response for PN532_CMD_RFCONFIGURATION\n");
        assert(false);
    }

    LOG_CLIENT("SAM configure\n");
    uint8_t sam_config_header[4];
    sam_config_header[0] = PN532_CMD_SAMCONFIGURATION;
    sam_config_header[1] = 0x1;
    sam_config_header[2] = 0x14;
    sam_config_header[3] = 0x01;
    write_fail = pn532_write_command(sam_config_header, 4, NULL, 0, DEFAULT_READ_ACK_FRAME_RETRIES);
    if (write_fail) {
        LOG_CLIENT_ERR("failed to write PN532_CMD_SAMCONFIGURATION\n");
        assert(false);
    }
    read_fail = pn532_read_response(big_buf, 64, DEFAULT_READ_RESPONSE_RETRIES);
    if (read_fail) {
        LOG_CLIENT_ERR("failed to read response for PN532_CMD_RFCONFIGURATION\n");
        assert(false);
    }

    LOG_CLIENT("waiting for card!\n");
    while (true) {
        /* Buffer to store the returned UID */
        uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
        /* Length of the UID (4 or 7 bytes depending on ISO14443A card type) */
        uint8_t uid_length;

        // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
        // 'uid' will be populated with the UID, and uid_length will indicate
        // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
        bool success = read_passive_target_id(PN532_MIFARE_ISO14443A, &uid[0], &uid_length, 0);

        if (success) {
            LOG_CLIENT("Found a card!\n");
            sddf_printf("UID Length: %d bytes\n", uid_length);
            sddf_printf("UID Value: ");
            for (int i = 0; i < uid_length; i++) {
                sddf_printf(" 0x%x", uid[i]);
            }
            sddf_printf("\n");
        } else {
            // PN532 probably timed out waiting for a card
            LOG_CLIENT_ERR("Timed out waiting for a card\n");
        }

        delay_ms(500);
    }
}

bool delay_ms(size_t milliseconds)
{
    size_t time_ns = milliseconds * NS_IN_MS;

    /* Detect potential overflow */
    if (milliseconds != 0 && time_ns / milliseconds != NS_IN_MS) {
        LOG_CLIENT_ERR("overflow detected in delay_ms");
        return false;
    }

    sddf_timer_set_timeout(timer_config.driver_id, time_ns);
    co_switch(t_event);

    return true;
}

void init(void)
{
    assert(serial_config_check_magic(&serial_config));
    serial_queue_init(&serial_tx_queue_handle, serial_config.tx.queue.vaddr, serial_config.tx.data.size,
                      serial_config.tx.data.vaddr);
    serial_putchar_init(serial_config.tx.id, &serial_tx_queue_handle);

    sddf_printf("PN532|INFO: init\n");

    assert(i2c_config_check_magic((void *)&i2c_config));

    data_region = (uintptr_t)i2c_config.virt.data.vaddr;
    queue = i2c_queue_init(i2c_config.virt.req_queue.vaddr, i2c_config.virt.resp_queue.vaddr);

    bool claimed = i2c_bus_claim(i2c_config.virt.id, PN532_I2C_BUS_ADDRESS);
    if (!claimed) {
        LOG_CLIENT_ERR("failed to claim PN532 bus\n");
        return;
    }

    /* Define the event loop/notified thread as the active co-routine */
    t_event = co_active();

    /* derive main entry point */
    t_main = co_derive((void *)t_client_main_stack, STACK_SIZE, client_main);

    co_switch(t_main);
}

void notified(microkit_channel ch)
{
    if (ch == i2c_config.virt.id || ch == timer_config.driver_id) {
        co_switch(t_main);
    } else if (ch == serial_config.tx.id) {
        // Nothing to do
    } else {
        LOG_CLIENT_ERR("Unknown channel 0x%x!\n", ch);
    }
}
