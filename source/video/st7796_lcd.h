/*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ST7796_LCD_H
#define __ST7796_LCD_H

#include <stdint.h>

typedef enum {
    ST7796_OK,
    ST7796_ERROR,
} st7796_ret_t;

typedef enum {
    ST7796_DIR_0 = 0x08U,
    ST7796_DIR_90 = 0x68U,
    ST7796_DIR_180 = 0xC8U,
    ST7796_DIR_270 = 0xA8U,
} st7796_direction_t;

typedef enum {
    ST7796_RGB444 = 3,
    ST7796_RGB565 = 5,
    ST7796_RGB666 = 6,
    ST7796_RGB888 = 7
} st7796_pixfmt_t;

typedef struct {
    st7796_ret_t (*reset_cb)(void *handle);
    st7796_ret_t (*backlight_cb)(void *handle, uint8_t on);
    st7796_ret_t (*write_cmd_cb)(void *handle, uint8_t *cmd, uint8_t len);
    st7796_ret_t (*write_data_cb)(void *handle, uint8_t *data, uint32_t len);
} st7796_cb_t;

typedef struct {
    st7796_direction_t direction;
    st7796_pixfmt_t pix_fmt;
    uint8_t inversion;
    uint8_t bgr_mode;
    uint8_t mirrored;
    uint8_t vflip;
} st7796_config_t;

typedef struct {
    void *user_data;
    st7796_cb_t cb;
    st7796_config_t config;
} st7796_lcd_t;

st7796_ret_t st7796_lcd_init(st7796_lcd_t *lcd);
st7796_ret_t st7796_lcd_load(st7796_lcd_t *lcd, uint8_t *data, uint16_t x_start,
                             uint16_t x_end, uint16_t y_start, uint16_t y_end);
st7796_ret_t st7796_lcd_sleep(st7796_lcd_t *lcd, uint8_t sleep_mode);
st7796_ret_t st7796_lcd_display(st7796_lcd_t *lcd, uint8_t display_on);
st7796_ret_t st7796_lcd_config(st7796_lcd_t *lcd, st7796_config_t *config);



#endif
