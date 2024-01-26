/*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "st7796_lcd.h"

#include <stddef.h>

#ifndef ST7796_LCD_IPS_PANEL
#define ST7796_LCD_IPS_PANEL 1
#else
#define ST7796_LCD_IPS_PANEL 0
#endif

#define ST7796_INIT_SEQ_NAME(x) st7796_init_seq_##x##_480_320
#if ST7796_LCD_IPS_PANEL
#define ST7796_LCD_INIT_SEQ ST7796_INIT_SEQ_NAME(ips)
#else
#define ST7796_LCD_INIT_SEQ ST7796_INIT_SEQ_NAME(tft)
#endif

uint8_t st7796_init_seq_tft_480_320[] = {
    0x01, 0xF0, 0xC3,                                            // Enable command part 1
    0x01, 0xF0, 0x96,                                            // Enable command part 2
    0x08, 0xE8, 0x40, 0x82, 0x07, 0x18, 0x27, 0x0A, 0xB6, 0x33,  // DOCA
    0x01, 0xC5, 0x27,                                            // VCOM control
    0x01, 0xC2, 0xA7,                                            // Power control 3
    0x0E, 0xE0, 0xF0, 0x01, 0x06, 0x0F, 0x12, 0x1D, 0x36, 0x54, 0x44, 0x0C, 0x18, 0x16, 0x13, 0x15,  // PGC
    0x0E, 0xE1, 0xF0, 0x01, 0x05, 0x0A, 0x0B, 0x07, 0x32, 0x44, 0x44, 0x0C, 0x18, 0x17, 0x13, 0x16,  // NGC
    0x01, 0xF0, 0x3C,  // Disable command part 1
    0x01, 0xF0, 0x69,  // Disable command part 2
};

uint8_t st7796_init_seq_ips_480_320[] = {
    0x01, 0xF0, 0xC3,                                            // Enable command part 1
    0x01, 0xF0, 0x96,                                            // Enable command part 2
    0x01, 0xB4, 0x01,                                            // Display inversion
    0x02, 0xB1, 0x80, 0x10,                                      // Frame rate control 1
    0x04, 0xB5, 0x1F, 0x50, 0x00, 0x20,                          // Blanking porch control
    0x03, 0xB6, 0x8A, 0x07, 0x3B,                                // Display function control
    0x02, 0xC0, 0x80, 0x64,                                      // Power control 1
    0x01, 0xC1, 0x13,                                            // Power control 2
    0x01, 0xC2, 0xA7,                                            // Power control 3
    0x01, 0xC5, 0x09,                                            // VCOM control
    0x08, 0xE8, 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33,  // DOCA
    0x0E, 0xE0, 0xF0, 0x06, 0x0B, 0x07, 0x06, 0x05, 0x2E, 0x33, 0x47, 0x3A, 0x17, 0x16, 0x2E, 0x31,  // PGC
    0x0E, 0xE1, 0xF0, 0x09, 0x0D, 0x09, 0x08, 0x23, 0x2E, 0x33, 0x46, 0x38, 0x13, 0x13, 0x2C, 0x32,  // NGC
    0x01, 0xF0, 0x3C,  // Disable command part 1
    0x01, 0xF0, 0x69,  // Disable command part 2
};

st7796_ret_t _st7796_init_seq(st7796_lcd_t *lcd) {
    uint16_t i = 0;

    while (i < sizeof(ST7796_LCD_INIT_SEQ)) {
        if (lcd->cb.write_cmd_cb(lcd->user_data, &ST7796_LCD_INIT_SEQ[i + 1], ST7796_LCD_INIT_SEQ[i] + 1) !=
            ST7796_OK) {
            return ST7796_ERROR;
        };
        i += ST7796_LCD_INIT_SEQ[i] + 2;
    }

    return ST7796_OK;
}

st7796_ret_t _st7796_window(st7796_lcd_t *lcd, uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end) {
    uint16_t real_x_start, real_x_end, real_y_start, real_y_end;

    uint16_t x_offset, y_offset;
    switch (lcd->config.direction) {
        case ST7796_DIR_0:
            x_offset = 0;
            y_offset = 0;
            break;
        case ST7796_DIR_90:
            x_offset = 0;
            y_offset = 0;
            break;
        case ST7796_DIR_180:
            x_offset = 320;
            y_offset = 480;
            break;
        case ST7796_DIR_270:
            x_offset = 480;
            y_offset = 320;
            break;
        default:
            x_offset = 0;
            y_offset = 0;
    }

    real_x_start = x_start + x_offset;
    real_x_end   = x_end + x_offset;
    real_y_start = y_start + y_offset;
    real_y_end   = y_end + y_offset;

    uint8_t tx_buf[5] = {0x2A, ((uint8_t)(real_x_start >> 0x08U) & 0xFFU), (real_x_start & 0xFFU),
                         ((uint8_t)(real_x_end >> 0x08U) & 0xFFU), (real_x_end & 0xFFU)};

    if (lcd->cb.write_cmd_cb(lcd->user_data, tx_buf, 0x05) != ST7796_OK) {
        return ST7796_ERROR;
    }

    tx_buf[0] = 0x2B;
    tx_buf[1] = ((uint8_t)(real_y_start >> 0x08U) & 0xFFU);
    tx_buf[2] = (real_y_start & 0xFFU);
    tx_buf[3] = ((uint8_t)(real_y_end >> 0x08U) & 0xFFU);
    tx_buf[4] = (real_y_end & 0xFFU);

    if (lcd->cb.write_cmd_cb(lcd->user_data, tx_buf, 0x05) != ST7796_OK) {
        return ST7796_ERROR;
    }

    return ST7796_OK;
}

st7796_ret_t _st7796_reset(st7796_lcd_t *lcd) {
    return lcd->cb.reset_cb(lcd->user_data);
}

st7796_ret_t st7796_lcd_init(st7796_lcd_t *lcd) {
    if (_st7796_reset(lcd) != ST7796_OK) return ST7796_ERROR;
    if (_st7796_init_seq(lcd) != ST7796_OK) return ST7796_ERROR;
    if (st7796_lcd_config(lcd, &lcd->config) != ST7796_OK) return ST7796_ERROR;
    if (st7796_lcd_sleep(lcd, 0) != ST7796_OK) return ST7796_ERROR;
    if (st7796_lcd_display(lcd, 1) != ST7796_OK) return ST7796_ERROR;

    return ST7796_OK;
}

st7796_ret_t st7796_lcd_load(st7796_lcd_t *lcd, uint8_t *data, uint16_t x_start, uint16_t x_end, uint16_t y_start,
                             uint16_t y_end) {
    uint32_t pixel_count = (y_end - y_start + 1) * (x_end - x_start + 1);

    uint32_t data_len = 0;

    switch (lcd->config.pix_fmt) {
        case ST7796_RGB444:
            data_len = pixel_count * 3 / 2;
            break;
        case ST7796_RGB565:
            data_len = pixel_count * 2;
            break;
        case ST7796_RGB666:
        case ST7796_RGB888:
            data_len = pixel_count * 3;
            break;
        default:
            data_len = pixel_count;
            break;
    }

    // Set cursor
    if (_st7796_window(lcd, x_start, x_end, y_start, y_end) != ST7796_OK) {
        return ST7796_ERROR;
    }

    uint8_t command = 0x2C;  // Memory Write
    if (lcd->cb.write_cmd_cb(lcd->user_data, &command, 0x01) != ST7796_OK) {
        return ST7796_ERROR;
    }

    // Write pixel data
    if (lcd->cb.write_data_cb(lcd->user_data, data, data_len) != ST7796_OK) {
        return ST7796_ERROR;
    }

    return ST7796_OK;
}

st7796_ret_t st7796_lcd_sleep(st7796_lcd_t *lcd, uint8_t sleep_mode) {
    // Write SLPIN or SLPOUT command.
    uint8_t command = sleep_mode ? 0x10 : 0x11;
    return lcd->cb.write_cmd_cb(lcd->user_data, &command, 0x01);
}

st7796_ret_t st7796_lcd_display(st7796_lcd_t *lcd, uint8_t display_on) {
    // write display_on command;
    uint8_t command = display_on ? 0x29 : 0x28;
    if (lcd->cb.write_cmd_cb(lcd->user_data, &command, 0x01) != ST7796_OK) {
        return ST7796_ERROR;
    }

    if ((lcd->cb.backlight_cb != NULL) && (lcd->cb.backlight_cb(lcd->user_data, display_on) != ST7796_OK)) {
        return ST7796_ERROR;
    }

    return ST7796_OK;
}

st7796_ret_t st7796_lcd_config(st7796_lcd_t *lcd, st7796_config_t *config) {
    lcd->config.direction = config->direction;

    // Write inversion command.
    uint8_t command[2] = {config->inversion ? 0x20 : 0x21, 0x00};
    if (lcd->cb.write_cmd_cb(lcd->user_data, command, 0x01) != ST7796_OK) {
        return ST7796_ERROR;
    }
    lcd->config.inversion = config->inversion;

    command[0] = 0x3A;
    command[1] = config->pix_fmt;
    if (lcd->cb.write_cmd_cb(lcd->user_data, command, 0x02) != ST7796_OK) {
        return ST7796_ERROR;
    }
    lcd->config.pix_fmt = config->pix_fmt;

    command[0] = 0x36;
    command[1] = config->direction;
    if (!config->bgr_mode) {
        command[1] &= ~0x08U;
    }

    if (config->mirrored) {
        /* Invert X or Y bit */
        if (config->direction == ST7796_DIR_90 || config->direction == ST7796_DIR_270) {
            command[1] ^= 0x80U;
        } else {
            command[1] ^= 0x40U;
        }
    }

    if(config->vflip)
    {
    	if (config->direction == ST7796_DIR_0) {
			command[1] ^= 0x80U;
		} else {
			command[1] ^= 0x40U;
		}
    }

    return lcd->cb.write_cmd_cb(lcd->user_data, command, 0x02);
}
